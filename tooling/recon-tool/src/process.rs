//! Native process boundary. Unregistered children are denied, never executed.
use anyhow::{Context, Result, ensure};
use std::{
    ffi::{CString, c_char, c_void},
    fs::{self, File},
    io::{Read, Write},
    os::{fd::AsRawFd, unix::net::UnixStream},
    path::Path,
    time::Duration,
};
const MAX_REQUEST: usize = 4 * 1024 * 1024;
const RSP1: u32 = 0x31505352;
const RSP2: u32 = 0x32505352;

struct Request {
    file: CString,
    cwd: CString,
    mode: i32,
    pid: u32,
    args: Vec<CString>,
    env: Vec<CString>,
    state: [u32; 4],
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
    let mut state = [0; 4];
    if protocol == RSP2 {
        let error = word(payload, 16)?;
        let dos = word(payload, 20)?;
        let last = word(payload, 24)?;
        let source = word(payload, 28)?;
        ensure!(
            mode == 0 && error == 0 && dos == 2 && matches!(last, 0 | 2) && source == 2,
            "unreviewed RSP2 request state/namespace"
        );
        state = [error, dos, last, source];
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
    let cwd = string()?;
    let args = (0..argc).map(|_| string()).collect::<Result<_>>()?;
    let env = (0..envc).map(|_| string()).collect::<Result<_>>()?;
    ensure!(offset == payload.len(), "trailing process request bytes");
    Ok(Request {
        file,
        cwd,
        mode,
        pid,
        args,
        env,
        state,
    })
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

#[derive(serde::Serialize, serde::Deserialize)]
pub struct ProcessReport {
    pub requests: u32,
    pub pids: Vec<u32>,
    pub errors: Vec<String>,
    #[serde(default)]
    pub self_children: Vec<crate::self_child::ChildResult>,
}
#[repr(C)]
struct Policy {
    abi_version: u32,
    struct_size: u32,
    expected_request_count: u32,
    request: Option<unsafe extern "C" fn(*const CallbackRequest, *mut Response) -> i32>,
}
#[repr(C)]
struct CallbackRequest {
    struct_size: u32,
    index: u32,
    mode: i32,
    pid: u32,
    protocol: u32,
    file: *const c_char,
    cwd: *const c_char,
    argc: u32,
    argv: *const *const c_char,
    environment_count: u32,
    environment: *const *const c_char,
    parent_errno: i32,
    parent_doserrno: u32,
    parent_last_error: u32,
    environment_source: u32,
    workspace_cwd: *const c_char,
    logical_cwd: *const c_char,
    assets_root: *const c_char,
    prepared_environment_count: u32,
    prepared_environment: *const *const c_char,
}
#[repr(C)]
struct Response {
    struct_size: u32,
    action: u32,
    status: i32,
    failure_errno: u32,
    stdout_data: *const c_void,
    stdout_size: u32,
    stderr_data: *const c_void,
    stderr_size: u32,
}
fn response_bytes(protocol: u32, state: [u32; 4], response: &Response) -> Result<Vec<u8>> {
    ensure!(
        response.struct_size as usize == size_of::<Response>(),
        "process response size mismatch"
    );
    if response.action == 1 {
        ensure!(
            protocol == RSP1
                && response.status == 0
                && response.failure_errno == 0
                && response.stdout_size == 0
                && response.stderr_size == 0
                && response.stdout_data.is_null()
                && response.stderr_data.is_null(),
            "SELF requires RSP1 and an empty zero-status response"
        );
        return Ok([protocol, 0, 0]
            .into_iter()
            .flat_map(u32::to_le_bytes)
            .collect());
    }
    ensure!(response.action == 0, "unknown process response action");
    ensure!(
        (-1..=255).contains(&response.status),
        "captured process status must be -1 or 0..255"
    );
    ensure!(
        if response.status == -1 {
            (1..=4095).contains(&response.failure_errno)
        } else {
            response.failure_errno == 0
        },
        "captured process errno/status mismatch"
    );
    ensure!(
        protocol != RSP2 || response.status == 0,
        "RSP2 permits only captured success"
    );
    ensure!(
        u64::from(response.stdout_size) + u64::from(response.stderr_size) <= 64 * 1024 * 1024,
        "captured process streams exceed 64MiB"
    );
    let mut bytes = vec![];
    let action = if protocol == RSP2 {
        5
    } else if response.status == -1 {
        4
    } else {
        3
    };
    for value in [protocol, action, response.status as u32] {
        bytes.extend_from_slice(&value.to_le_bytes());
    }
    if protocol == RSP2 {
        for value in [state[0], state[1], state[2], 0, 0, state[2]] {
            bytes.extend_from_slice(&value.to_le_bytes());
        }
    }
    if response.status == -1 {
        bytes.extend_from_slice(&response.failure_errno.to_le_bytes());
    }
    for value in [response.stdout_size, response.stderr_size] {
        bytes.extend_from_slice(&value.to_le_bytes());
    }
    for (pointer, count) in [
        (response.stdout_data, response.stdout_size),
        (response.stderr_data, response.stderr_size),
    ] {
        ensure!(
            count == 0 || !pointer.is_null(),
            "null captured process stream"
        );
        if count != 0 {
            bytes.extend_from_slice(unsafe {
                std::slice::from_raw_parts(pointer as *const u8, count as usize)
            });
        }
    }
    Ok(bytes)
}
fn wait_for_handoff(root: &Path) {
    while !root.join("process-go").exists() {
        std::thread::sleep(Duration::from_millis(1));
    }
}
pub(crate) fn close_broker_descriptors(server: i32, diagnostics: i32) -> Result<()> {
    // Collect first: do not close read_dir's own descriptor during iteration.
    let descriptors: Vec<i32> = fs::read_dir("/proc/self/fd")?
        .filter_map(|entry| entry.ok()?.file_name().to_str()?.parse().ok())
        .collect();
    for descriptor in descriptors {
        if descriptor > 2 && descriptor != server && descriptor != diagnostics {
            unsafe {
                libc::close(descriptor);
            }
        }
    }
    let null = File::open("/dev/null")?;
    for (from, to) in [(null.as_raw_fd(), 0), (diagnostics, 1), (diagnostics, 2)] {
        ensure!(
            unsafe { libc::dup2(from, to) } >= 0,
            "broker stream isolation"
        );
    }
    Ok(())
}
/// Fork the optional untraced case provider after setup. The supervisor hands off
/// its socket before either broker or application may run, so there is one reader.
/// The broker closes inherited client/status/trace handles before waiting on EOF.
pub fn prepare(
    loaded: &crate::abi::Loaded,
    fixture: &crate::fixture::Fixture,
    root: &Path,
    cwd: &Path,
    server: DenyServer,
    tests: bool,
) -> Result<()> {
    if tests {
        return Ok(());
    }
    let policy = unsafe { loaded.optional::<*const Policy>(b"case_process_policy\0") }?;
    let Some(policy) = policy else {
        return Ok(());
    };
    let policy = unsafe { (*policy).as_ref() }.context("NULL case_process_policy")?;
    ensure!(
        policy.abi_version == 1
            && policy.struct_size as usize == size_of::<Policy>()
            && policy.expected_request_count <= 4096
            && policy.request.is_some(),
        "invalid case process policy"
    );
    if let Some(disable) =
        unsafe { loaded.optional::<unsafe extern "C" fn(i32)>(b"recon_trace_enable\0") }?
    {
        unsafe {
            disable(0);
        }
    }
    let prepared: Vec<CString> = std::env::vars_os()
        .map(|(key, value)| {
            let mut bytes = key.as_encoded_bytes().to_vec();
            bytes.push(b'=');
            bytes.extend_from_slice(value.as_encoded_bytes());
            CString::new(bytes).map_err(Into::into)
        })
        .collect::<Result<_>>()?;
    let workspace = CString::new(cwd.as_os_str().as_encoded_bytes())?;
    let logical = fixture
        .logical_cwd
        .as_ref()
        .map(|s| CString::new(s.as_str()))
        .transpose()?;
    let assets = crate::assets::root(fixture, root)
        .map(|p| CString::new(p.as_os_str().as_encoded_bytes()))
        .transpose()?;
    let owner = unsafe { libc::getpid() } as u32;
    let self_enabled = std::env::var_os("RECON_SELF_SPAWN").is_some_and(|v| v == "1");
    let child = crate::native::fork()?;
    if child == 0 {
        let result = (|| -> Result<()> {
            unsafe {
                libc::setpgid(0, 0);
                libc::signal(libc::SIGTRAP, libc::SIG_DFL);
            }
            let diagnostics = File::create(root.join("process-diagnostics"))?;
            close_broker_descriptors(server.stream.as_raw_fd(), diagnostics.as_raw_fd())?;
            wait_for_handoff(root);
            server.stream.set_nonblocking(false)?;
            let mut stream = server.stream;
            let mut count = 0;
            let mut self_count = 0;
            let mut errors = vec![];
            let mut pids = vec![];
            loop {
                let mut header = [0; 8];
                match stream.read(&mut header[..1]) {
                    Ok(0) => break,
                    Ok(_) => stream.read_exact(&mut header[1..])?,
                    Err(error) if error.kind() == std::io::ErrorKind::Interrupted => continue,
                    Err(error) => return Err(error.into()),
                }
                let protocol = word(&header, 0)?;
                let length = word(&header, 4)? as usize;
                ensure!(
                    matches!(protocol, RSP1 | RSP2) && length <= MAX_REQUEST,
                    "invalid process frame"
                );
                let mut payload = vec![0; length];
                stream.read_exact(&mut payload)?;
                let request = decode(protocol, &payload)?;
                // Each fresh executor publishes its own post-setup environment.
                let child_prepared;
                let prepared = if request.pid == owner {
                    &prepared
                } else {
                    child_prepared = crate::self_child::environment(root, request.pid)?;
                    &child_prepared
                };
                let prepared_pointers: Vec<_> = prepared.iter().map(|s| s.as_ptr()).collect();
                let args: Vec<_> = request.args.iter().map(|s| s.as_ptr()).collect();
                let env: Vec<_> = request.env.iter().map(|s| s.as_ptr()).collect();
                let call = CallbackRequest {
                    struct_size: size_of::<CallbackRequest>() as u32,
                    index: count,
                    mode: request.mode,
                    pid: request.pid,
                    protocol: if protocol == RSP1 { 1 } else { 2 },
                    file: request.file.as_ptr(),
                    cwd: request.cwd.as_ptr(),
                    argc: args.len() as u32,
                    argv: args.as_ptr(),
                    environment_count: env.len() as u32,
                    environment: env.as_ptr(),
                    parent_errno: request.state[0] as i32,
                    parent_doserrno: request.state[1],
                    parent_last_error: request.state[2],
                    environment_source: request.state[3],
                    workspace_cwd: workspace.as_ptr(),
                    logical_cwd: logical.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
                    assets_root: assets.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
                    prepared_environment_count: prepared_pointers.len() as u32,
                    prepared_environment: prepared_pointers.as_ptr(),
                };
                let mut response = Response {
                    struct_size: size_of::<Response>() as u32,
                    action: 0,
                    status: 0,
                    failure_errno: 0,
                    stdout_data: std::ptr::null(),
                    stdout_size: 0,
                    stderr_data: std::ptr::null(),
                    stderr_size: 0,
                };
                let accepted = count < policy.expected_request_count
                    && unsafe { policy.request.unwrap()(&call, &mut response) } != 0;
                pids.push(request.pid);
                count += 1;
                let reply = if accepted {
                    if response.action == 1 && !self_enabled {
                        Err(anyhow::anyhow!("SELF requires RECON_SELF_SPAWN=1"))
                    } else {
                        response_bytes(protocol, request.state, &response)
                    }
                } else {
                    Err(anyhow::anyhow!(
                        "case rejected child request {}: {}",
                        count - 1,
                        request.file.to_string_lossy()
                    ))
                };
                match reply {
                    Ok(bytes) => {
                        if response.action == 1 {
                            self_count += 1;
                        }
                        stream.write_all(&bytes)?;
                    }
                    Err(error) => {
                        errors.push(format!("{error:#}"));
                        for value in [protocol, 2, 0] {
                            stream.write_all(&value.to_le_bytes())?;
                        }
                    }
                }
            }
            if count != policy.expected_request_count {
                errors.push(format!(
                    "process request count differs: actual {count}, expected {}",
                    policy.expected_request_count
                ));
            }
            let self_children = crate::self_child::results(root, self_count)?;
            for child in &self_children {
                errors.extend(
                    child
                        .errors
                        .iter()
                        .map(|error| format!("native child {}: {error}", child.launcher_pid)),
                );
            }
            fs::write(
                root.join("process-report"),
                serde_json::to_vec(
                    &serde_json::json!({"requests":count,"pids":pids,"errors":errors,"self_children":self_children}),
                )?,
            )?;
            ensure!(errors.is_empty(), "{}", errors.join("; "));
            Ok(())
        })();
        if let Err(error) = &result {
            let _ = fs::write(root.join("process-error"), format!("{error:#}"));
        }
        unsafe {
            libc::_exit(if result.is_ok() { 0 } else { 125 });
        }
    }
    drop(server);
    fs::write(root.join("process-ready.tmp"), child.to_string())?;
    fs::rename(root.join("process-ready.tmp"), root.join("process-ready"))?;
    wait_for_handoff(root);
    Ok(())
}

#[cfg(test)]
mod response_tests {
    use super::*;
    fn response() -> Response {
        Response {
            struct_size: size_of::<Response>() as u32,
            action: 0,
            status: 0,
            failure_errno: 0,
            stdout_data: std::ptr::null(),
            stdout_size: 0,
            stderr_data: std::ptr::null(),
            stderr_size: 0,
        }
    }
    #[test]
    fn reply_domain_and_rsp2_state_remain_strict() {
        let mut r = response();
        let bytes = response_bytes(RSP2, [0, 2, 2, 2], &r).unwrap();
        assert_eq!(word(&bytes, 4).unwrap(), 5);
        assert_eq!(
            &bytes[12..36],
            &[
                0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0
            ]
        );
        for status in [-2, 256] {
            r.status = status;
            assert!(response_bytes(RSP1, [0; 4], &r).is_err());
        }
        r.status = -1;
        assert!(response_bytes(RSP1, [0; 4], &r).is_err());
        r.failure_errno = 2;
        assert!(response_bytes(RSP1, [0; 4], &r).is_ok());
        assert!(response_bytes(RSP2, [0, 2, 2, 2], &r).is_err());
        r = response();
        r.stdout_size = 1;
        assert!(response_bytes(RSP1, [0; 4], &r).is_err());
        r = response();
        r.struct_size = 0;
        assert!(response_bytes(RSP1, [0; 4], &r).is_err());
        r = response();
        r.action = 1;
        assert_eq!(
            response_bytes(RSP1, [0; 4], &r).unwrap(),
            [RSP1, 0, 0]
                .into_iter()
                .flat_map(u32::to_le_bytes)
                .collect::<Vec<_>>()
        );
        assert!(response_bytes(RSP2, [0; 4], &r).is_err());
        r.status = 7;
        assert!(response_bytes(RSP1, [0; 4], &r).is_err());
        r.status = 0;
        r.stdout_size = 1;
        assert!(response_bytes(RSP1, [0; 4], &r).is_err());
    }
}
