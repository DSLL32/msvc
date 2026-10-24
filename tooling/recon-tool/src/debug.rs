use anyhow::{Context, Result, bail};
use std::{
    fs,
    os::fd::{AsRawFd, FromRawFd, OwnedFd},
    os::unix::process::CommandExt,
    path::{Path, PathBuf},
    process::{Command, ExitStatus},
};
#[derive(Clone, Default)]
pub struct Options {
    pub enabled: bool,
    pub script: Option<PathBuf>,
    pub selected_test: Option<String>,
}
impl Options {
    pub fn prepare(&mut self) -> Result<()> {
        if let Some(script) = &self.script {
            self.script = Some(script.canonicalize()?);
            self.enabled = true;
        }
        Ok(())
    }
}
/// Called in the actual invocation process after library loading and setup.
/// Only the supervisor and its descendants (GDB) gain ptrace permission.
pub fn checkpoint(options: &Options, root: &Path, supervisor: i32) -> Result<()> {
    if !options.enabled {
        return Ok(());
    }
    if unsafe { libc::prctl(libc::PR_SET_PTRACER, supervisor, 0, 0, 0) } < 0 {
        bail!("authorize debugger: {}", std::io::Error::last_os_error())
    }
    fs::write(
        root.join("debug-ready.tmp"),
        unsafe { libc::getpid() }.to_string(),
    )?;
    fs::rename(root.join("debug-ready.tmp"), root.join("debug-ready"))?;
    if unsafe { libc::raise(libc::SIGSTOP) } != 0 {
        bail!("stop for debugger")
    }
    Ok(())
}
pub fn start(options: &Options, root: &Path) -> Result<Session> {
    let pid: i32 = fs::read_to_string(root.join("debug-ready"))?.parse()?;
    // Retain the exact process identity while its parent may reap it during GDB exit.
    let fd = unsafe { libc::syscall(libc::SYS_pidfd_open, pid, 0) };
    if fd < 0 {
        bail!(
            "pidfd_open for debugger: {}",
            std::io::Error::last_os_error()
        )
    }
    let process = unsafe { OwnedFd::from_raw_fd(fd as i32) };
    let mut command = Command::new("gdb");
    command.args([
        "-q",
        "-nx",
        "-p",
        &pid.to_string(),
        "-ex",
        "set pagination off",
        "-ex",
        "set confirm off",
        "-ex",
        "handle SIGSTOP nostop noprint nopass",
    ]);
    if let Some(script) = &options.script {
        command.args(["--batch", "-x"]).arg(script);
    }
    eprintln!(
        "recon-tool: GDB attached to invocation PID {pid}; quitting terminates any remaining invocation"
    );
    command.process_group(0);
    let child = command.spawn().context("start gdb")?;
    let old_foreground = unsafe { libc::tcgetpgrp(0) };
    if old_foreground >= 0 {
        unsafe {
            let old = libc::signal(libc::SIGTTOU, libc::SIG_IGN);
            libc::tcsetpgrp(0, child.id() as i32);
            libc::signal(libc::SIGTTOU, old);
            libc::kill(child.id() as i32, libc::SIGCONT);
        }
    }
    let gdb_fd = unsafe { libc::syscall(libc::SYS_pidfd_open, child.id(), 0) };
    if gdb_fd < 0 {
        bail!("pidfd_open for GDB")
    }
    Ok(Session {
        pid: child.id() as i32,
        gdb: unsafe { OwnedFd::from_raw_fd(gdb_fd as i32) },
        target: process,
        old_foreground,
        finished: false,
    })
}
pub struct Session {
    pub pid: i32,
    gdb: OwnedFd,
    target: OwnedFd,
    old_foreground: i32,
    finished: bool,
}
fn restore_terminal(group: i32) {
    if group >= 0 {
        unsafe {
            let old = libc::signal(libc::SIGTTOU, libc::SIG_IGN);
            libc::tcsetpgrp(0, group);
            libc::signal(libc::SIGTTOU, old);
        }
    }
}
fn terminate(fd: &OwnedFd) {
    unsafe {
        libc::syscall(
            libc::SYS_pidfd_send_signal,
            fd.as_raw_fd(),
            libc::SIGKILL,
            0,
            0,
        );
    }
}
impl Session {
    /// The ordinary supervisor wait loop reaps GDB, keeping the input pump live.
    pub fn finish(mut self, status: ExitStatus) -> Result<()> {
        self.finished = true;
        restore_terminal(self.old_foreground);
        terminate(&self.target);
        if !status.success() {
            bail!("GDB exited with {status}")
        }
        Ok(())
    }
}
impl Drop for Session {
    fn drop(&mut self) {
        if !self.finished {
            restore_terminal(self.old_foreground);
            terminate(&self.gdb);
            terminate(&self.target);
        }
    }
}
