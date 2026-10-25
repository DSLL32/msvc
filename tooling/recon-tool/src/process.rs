//! Native process boundary. Unregistered children are denied, never executed.
use anyhow::{Context, Result, ensure};
use std::{
    ffi::CString,
    io::{Read, Write},
    os::{fd::AsRawFd, unix::net::UnixStream},
};
const MAX_REQUEST: usize = 4 * 1024 * 1024;
const RSP1: u32 = 0x31505352;
const RSP2: u32 = 0x32505352;

struct Request {
    file: CString,
}
fn word(bytes: &[u8], offset: usize) -> Result<u32> {
    Ok(u32::from_le_bytes(
        bytes
            .get(offset..offset + 4)
            .context("truncated process request")?
            .try_into()?,
    ))
}
fn decode(protocol: u32, payload: &[u8]) -> Result<Request> {
    ensure!(matches!(protocol, RSP1 | RSP2), "unknown process protocol");
    let mode = word(payload, 0)? as i32;
    let pid = word(payload, 4)?;
    let argc = word(payload, 8)? as usize;
    let envc = word(payload, 12)? as usize;
    ensure!(
        mode == 0 && pid > 0 && argc <= 4096 && envc <= 16384,
        "invalid process identity/counts"
    );
    let mut offset = 16;
    if protocol == RSP2 {
        let error = word(payload, 16)?;
        let dos = word(payload, 20)?;
        let last = word(payload, 24)?;
        let source = word(payload, 28)?;
        ensure!(
            mode == 0 && error == 0 && dos == 2 && matches!(last, 0 | 2) && source == 2,
            "unreviewed RSP2 request state/namespace"
        );
        offset = 32;
    }
    let mut string = || -> Result<CString> {
        let length = word(payload, offset)? as usize;
        offset += 4;
        let end = offset
            .checked_add(length)
            .context("process string length overflow")?;
        let value = CString::new(
            payload
                .get(offset..end)
                .context("truncated process string")?,
        )?;
        offset = end;
        Ok(value)
    };
    let file = string()?;
    let _cwd = string()?;
    for _ in 0..argc + envc {
        let _ = string()?;
    }
    ensure!(offset == payload.len(), "trailing process request bytes");
    Ok(Request { file })
}

pub struct DenyServer {
    stream: UnixStream,
    buffered: Vec<u8>,
    closed: bool,
}
impl DenyServer {
    pub fn new() -> Result<(Self, UnixStream)> {
        let (server, client) = UnixStream::pair()?;
        server.set_nonblocking(true)?;
        let fd = client.as_raw_fd();
        let flags = unsafe { libc::fcntl(fd, libc::F_GETFD) };
        ensure!(
            flags >= 0 && unsafe { libc::fcntl(fd, libc::F_SETFD, flags & !libc::FD_CLOEXEC) } >= 0,
            "inheritable process boundary descriptor"
        );
        Ok((
            Self {
                stream: server,
                buffered: vec![],
                closed: false,
            },
            client,
        ))
    }
    pub fn poll(&mut self) -> Result<Vec<String>> {
        let result = self.poll_inner();
        if result.is_err() {
            self.closed = true;
            let _ = self.stream.shutdown(std::net::Shutdown::Both);
        }
        result
    }
    fn poll_inner(&mut self) -> Result<Vec<String>> {
        let mut denied = vec![];
        if self.closed {
            return Ok(denied);
        }
        let mut bytes = [0; 8192];
        loop {
            match self.stream.read(&mut bytes) {
                Ok(0) => {
                    self.closed = true;
                    ensure!(
                        self.buffered.is_empty(),
                        "incomplete process request at EOF"
                    );
                    return Ok(denied);
                }
                Ok(count) => self.buffered.extend_from_slice(&bytes[..count]),
                Err(error) if error.kind() == std::io::ErrorKind::Interrupted => continue,
                Err(error) if error.kind() == std::io::ErrorKind::WouldBlock => return Ok(denied),
                Err(error) => return Err(error.into()),
            }
            while self.buffered.len() >= 8 {
                let protocol = word(&self.buffered, 0)?;
                let length = word(&self.buffered, 4)? as usize;
                ensure!(
                    matches!(protocol, RSP1 | RSP2) && length <= MAX_REQUEST,
                    "invalid process frame"
                );
                if self.buffered.len() < 8 + length {
                    break;
                }
                let request = decode(protocol, &self.buffered[8..8 + length])?;
                denied.push(format!(
                    "undeclared child invocation rejected: {}",
                    request.file.to_string_lossy()
                ));
                // Action 2 is intentionally invalid in both existing shim
                // protocols: fail closed, without manufacturing a captured errno
                // or allowing a fallback native process.
                let response: [u32; 3] = [protocol, 2, 0];
                for word in response {
                    self.stream.write_all(&word.to_le_bytes())?;
                }
                self.buffered.drain(..8 + length);
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn payload() -> Vec<u8> {
        let mut bytes = vec![];
        for word in [0u32, 123, 1, 1] {
            bytes.extend_from_slice(&word.to_le_bytes());
        }
        for text in [b"tool.exe".as_slice(), b"case", b"tool", b"PATH="] {
            bytes.extend_from_slice(&(text.len() as u32).to_le_bytes());
            bytes.extend_from_slice(text);
        }
        bytes
    }
    #[test]
    fn complete_process_requests_and_malformed_frames() {
        let p = payload();
        assert_eq!(decode(RSP1, &p).unwrap().file.to_bytes(), b"tool.exe");
        let mut p2 = p[..16].to_vec();
        for state in [0u32, 2, 2, 2] {
            p2.extend_from_slice(&state.to_le_bytes());
        }
        p2.extend_from_slice(&p[16..]);
        assert!(decode(RSP2, &p2).is_ok());
        p2[16] = 1;
        assert!(decode(RSP2, &p2).is_err());
        assert!(decode(RSP1, &p[..p.len() - 1]).is_err());
        let mut trailing = p.clone();
        trailing.push(0);
        assert!(decode(RSP1, &trailing).is_err());
        let mut nul = p.clone();
        nul[20] = 0;
        assert!(decode(RSP1, &nul).is_err());
    }
    #[test]
    fn denial_cannot_be_interpreted_as_a_captured_failure_or_native_allow() {
        let (mut server, mut target) = DenyServer::new().unwrap();
        let payload = payload();
        target.write_all(&RSP1.to_le_bytes()).unwrap();
        target
            .write_all(&(payload.len() as u32).to_le_bytes())
            .unwrap();
        assert!(server.poll().unwrap().is_empty());
        target.write_all(&payload).unwrap();
        assert_eq!(server.poll().unwrap().len(), 1);
        let mut reply = [0; 12];
        target.read_exact(&mut reply).unwrap();
        assert_eq!(word(&reply, 0).unwrap(), RSP1);
        assert_eq!(word(&reply, 4).unwrap(), 2);
    }
}
