//! The existing shim's PID-owned full-DWORD terminal receipt.
use anyhow::{Context, Result, ensure};
use std::{
    fs::{self, File, OpenOptions},
    os::{fd::AsRawFd, unix::fs::FileExt},
    path::Path,
};

pub type Recorder = unsafe extern "C" fn(u32);
const SIZE: usize = 24;
fn fields(data: &[u8], owner: u32) -> Result<[u32; 6]> {
    ensure!(data.len() == SIZE, "malformed terminal receipt size");
    let mut words = [0; 6];
    for (word, bytes) in words.iter_mut().zip(data.chunks_exact(4)) {
        *word = u32::from_le_bytes(bytes.try_into()?);
    }
    ensure!(
        words[0] == 0x31535452
            && words[1] == 1
            && words[2] == owner
            && words[3] <= 1
            && words[5] == 0,
        "missing, malformed or wrong-owner terminal receipt"
    );
    Ok(words)
}
pub struct Receipt(File);
impl Receipt {
    pub fn create(root: &Path) -> Result<Self> {
        let file = OpenOptions::new()
            .read(true)
            .write(true)
            .create_new(true)
            .open(root.join("status"))?;
        file.set_len(SIZE as u64)?;
        let fd = file.as_raw_fd();
        let flags = unsafe { libc::fcntl(fd, libc::F_GETFD) };
        ensure!(
            flags >= 0 && unsafe { libc::fcntl(fd, libc::F_SETFD, flags & !libc::FD_CLOEXEC) } >= 0,
            "inheritable terminal receipt descriptor"
        );
        // The CRT constructor maps this before any application code executes.
        unsafe { std::env::set_var("RECON_STATUS_FD", fd.to_string()) };
        Ok(Self(file))
    }
    fn read(&self) -> Result<[u32; 6]> {
        let mut data = [0; SIZE];
        self.0.read_exact_at(&mut data, 0)?;
        fields(&data, unsafe { libc::getpid() } as u32)
    }
    pub fn supported(&self, root: &Path, supported: bool) -> Result<()> {
        if supported {
            self.read()?;
            fs::write(root.join("status-supported"), b"1")?;
        }
        Ok(())
    }
    pub fn returned(&self, recorder: Option<Recorder>, code: u32) -> Result<()> {
        if let Some(record) = recorder {
            let words = self.read()?;
            if words[3] == 1 {
                ensure!(
                    words[4] == code,
                    "returned status differs from existing terminal receipt"
                );
            } else {
                unsafe { record(code) };
            }
        }
        Ok(())
    }
}
pub fn completed(root: &Path, owner: u32, posix_code: i32) -> Result<u32> {
    let data = fs::read(root.join("status")).context("read terminal receipt")?;
    if !root.join("status-supported").exists() {
        ensure!(
            data == [0; SIZE],
            "unexpected receipt from library without terminal API"
        );
        return Ok(posix_code as u32);
    }
    published(&data, owner, posix_code)
}
fn published(data: &[u8], owner: u32, posix_code: i32) -> Result<u32> {
    let words = fields(data, owner)?;
    ensure!(words[3] == 1, "missing terminal status publication");
    ensure!(
        words[4] & 255 == posix_code as u32,
        "logical DWORD/POSIX status mismatch"
    );
    Ok(words[4])
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn receipt_rejects_wrong_owner_state_and_posix_mapping() {
        let bytes = |words: [u32; 6]| {
            words
                .into_iter()
                .flat_map(u32::to_le_bytes)
                .collect::<Vec<_>>()
        };
        assert_eq!(
            fields(&bytes([0x31535452, 1, 42, 1, 0x8000, 0]), 42).unwrap()[4],
            0x8000
        );
        let valid = bytes([0x31535452, 1, 42, 1, 0x8000, 0]);
        assert_eq!(published(&valid, 42, 0).unwrap(), 0x8000);
        assert!(published(&valid, 42, 1).is_err());
        assert!(published(&bytes([0x31535452, 1, 42, 0, 0, 0]), 42, 0).is_err());
        assert_eq!(
            published(&bytes([0x31535452, 1, 42, 1, u32::MAX, 0]), 42, 255).unwrap(),
            u32::MAX
        );
        for words in [
            [0, 1, 42, 1, 0, 0],
            [0x31535452, 1, 43, 1, 0, 0],
            [0x31535452, 1, 42, 2, 0, 0],
            [0x31535452, 1, 42, 1, 0, 1],
        ] {
            assert!(fields(&bytes(words), 42).is_err());
        }
    }
}
