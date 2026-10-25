//! Optional file-only checks run in a pristine post-setup fork after target exit.
use crate::{abi::Loaded, fixture::Fixture, native, status};
use anyhow::{Context, Result, ensure};
use sha2::{Digest, Sha256};
use std::{
    ffi::{CString, c_char},
    fs::{self, File},
    os::{fd::AsRawFd, unix::fs::PermissionsExt},
    path::Path,
    time::Duration,
};
#[repr(C)]
struct ContextAbi {
    struct_size: u32,
    executor_pid: u32,
    terminal_kind: u32,
    terminal_value: u32,
    workspace_cwd: *const c_char,
    assets_root: *const c_char,
    stdout_raw: *const c_char,
    stderr_raw: *const c_char,
    stdout_compare: *const c_char,
    stderr_compare: *const c_char,
}
#[repr(C)]
struct Policy {
    abi_version: u32,
    struct_size: u32,
    check_outputs: Option<unsafe extern "C" fn(*const ContextAbi) -> i32>,
}
#[derive(serde::Serialize, serde::Deserialize)]
struct Terminal {
    executor_pid: u32,
    kind: u32,
    value: u32,
}
/// Called before any application execution/provider fork. This child closes all
/// inherited transport endpoints, so it cannot hold the process broker open.
pub fn prepare(loaded: &Loaded, f: &Fixture, root: &Path, cwd: &Path, tests: bool) -> Result<()> {
    if tests {
        return Ok(());
    }
    let policy = unsafe { loaded.library.get::<*const Policy>(b"case_output_policy\0") };
    let Ok(policy) = policy else {
        return Ok(());
    };
    let policy = unsafe { (*policy).as_ref() }.context("NULL case_output_policy")?;
    ensure!(
        policy.abi_version == 1
            && policy.struct_size as usize == size_of::<Policy>()
            && policy.check_outputs.is_some(),
        "invalid case output policy"
    );
    if let Ok(disable) = unsafe {
        loaded
            .library
            .get::<unsafe extern "C" fn(i32)>(b"recon_trace_enable\0")
    } {
        unsafe {
            disable(0);
        }
    }
    let checker = native::fork()?;
    if checker == 0 {
        let result = (|| -> Result<()> {
            unsafe {
                libc::setpgid(0, 0);
                libc::signal(libc::SIGTRAP, libc::SIG_DFL);
            }
            let diagnostics = File::create(root.join("output-diagnostics"))?;
            crate::process::close_broker_descriptors(-1, diagnostics.as_raw_fd())?;
            loop {
                if root.join("output-cancel").exists() {
                    return Ok(());
                }
                if root.join("output-go").exists() {
                    break;
                }
                std::thread::sleep(Duration::from_millis(1));
            }
            let terminal: Terminal = serde_json::from_slice(&fs::read(root.join("output-go"))?)?;
            let cwd = CString::new(cwd.as_os_str().as_encoded_bytes())?;
            let assets = crate::assets::root(f, root)
                .map(|p| CString::new(p.as_os_str().as_encoded_bytes()))
                .transpose()?;
            let paths = ["stdout", "stderr", "stdout-compare", "stderr-compare"]
                .map(|name| CString::new(root.join(name).as_os_str().as_encoded_bytes()));
            let paths = paths
                .into_iter()
                .collect::<std::result::Result<Vec<_>, _>>()?;
            let context = ContextAbi {
                struct_size: size_of::<ContextAbi>() as u32,
                executor_pid: terminal.executor_pid,
                terminal_kind: terminal.kind,
                terminal_value: terminal.value,
                workspace_cwd: cwd.as_ptr(),
                assets_root: assets.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
                stdout_raw: paths[0].as_ptr(),
                stderr_raw: paths[1].as_ptr(),
                stdout_compare: paths[2].as_ptr(),
                stderr_compare: paths[3].as_ptr(),
            };
            ensure!(
                unsafe { policy.check_outputs.unwrap()(&context) } != 0,
                "case output checker rejected artifacts"
            );
            fs::write(root.join("output-complete"), b"1")?;
            Ok(())
        })();
        if let Err(error) = &result {
            let _ = fs::write(root.join("output-error"), format!("{error:#}"));
        }
        unsafe {
            libc::_exit(if result.is_ok() { 0 } else { 125 });
        }
    }
    fs::write(root.join("output-ready.tmp"), checker.to_string())?;
    fs::rename(root.join("output-ready.tmp"), root.join("output-ready"))?;
    Ok(())
}
pub fn checker(root: &Path) -> Result<Option<i32>> {
    match fs::read_to_string(root.join("output-ready")) {
        Ok(pid) => {
            let pid = pid.parse::<i32>()?;
            ensure!(pid > 0, "invalid output checker PID");
            Ok(Some(pid))
        }
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => Ok(None),
        Err(error) => Err(error.into()),
    }
}
/// Release only after waitpid has reaped every nonchecker descendant.
pub fn release(root: &Path, pid: i32, wait_status: i32) -> Result<()> {
    let terminal = if libc::WIFEXITED(wait_status) {
        Terminal {
            executor_pid: pid as u32,
            kind: 1,
            value: status::completed(root, pid as u32, libc::WEXITSTATUS(wait_status))?,
        }
    } else {
        ensure!(
            libc::WIFSIGNALED(wait_status),
            "unsupported output checker terminal"
        );
        Terminal {
            executor_pid: pid as u32,
            kind: 2,
            value: libc::WTERMSIG(wait_status) as u32,
        }
    };
    let mut hashes = vec![];
    for name in ["stdout", "stderr"] {
        let raw = root.join(name);
        let bytes = fs::read(&raw)?;
        hashes.push(crate::fixture::hex(&Sha256::digest(&bytes)));
        fs::write(root.join(format!("{name}-compare")), bytes)?;
        fs::set_permissions(raw, fs::Permissions::from_mode(0o444))?;
    }
    fs::write(root.join("output-hashes"), serde_json::to_vec(&hashes)?)?;
    fs::write(root.join("output-go.tmp"), serde_json::to_vec(&terminal)?)?;
    fs::rename(root.join("output-go.tmp"), root.join("output-go"))?;
    Ok(())
}
pub fn verify(root: &Path) -> Result<()> {
    if !root.join("output-ready").exists() {
        return Ok(());
    }
    if let Ok(error) = fs::read_to_string(root.join("output-error")) {
        anyhow::bail!("output checker: {error}");
    }
    ensure!(
        root.join("output-complete").exists(),
        "output checker did not complete"
    );
    let hashes: Vec<String> = serde_json::from_slice(&fs::read(root.join("output-hashes"))?)?;
    ensure!(hashes.len() == 2, "output raw stream hashes missing");
    for (name, expected) in ["stdout", "stderr"].iter().zip(hashes) {
        ensure!(
            crate::fixture::hex(&Sha256::digest(fs::read(root.join(name))?)) == expected,
            "output checker modified raw {name}"
        );
    }
    Ok(())
}
