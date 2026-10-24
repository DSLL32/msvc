//! Signal-safe terminal observation, separate from application first-entry tracing.
use anyhow::{Context, Result, bail};
use serde::{Deserialize, Serialize};
use std::{
    fs,
    os::fd::RawFd,
    path::Path,
    sync::atomic::{AtomicI32, Ordering},
};
static FD: AtomicI32 = AtomicI32::new(-1);
#[repr(C)]
#[derive(Clone, Copy)]
struct Record {
    magic: u32,
    code: i32,
    pc: u32,
    int3: u32,
}
#[derive(Serialize, Deserialize)]
pub struct Range {
    pub start: u32,
    pub end: u32,
    pub rva: u32,
}
extern "C" fn observed(signal: i32, info: *mut libc::siginfo_t, context: *mut libc::c_void) {
    let pc = unsafe {
        (*(context as *const libc::ucontext_t)).uc_mcontext.gregs[libc::REG_EIP as usize] as u32
    };
    let code = unsafe { (*info).si_code };
    // Linux x86 INT3 reports SI_KERNEL (128), or TRAP_BRKPT. Raised, hardware
    // watchpoint and single-step SIGTRAPs are distinct and cannot prove a stub.
    let int3 = u32::from(
        matches!(code, 128 | 1) && pc != 0 && unsafe { *((pc - 1) as *const u8) } == 0xcc,
    );
    let record = Record {
        magic: 0x31505254,
        code,
        pc,
        int3,
    };
    unsafe {
        libc::write(
            FD.load(Ordering::Relaxed),
            &record as *const _ as *const _,
            size_of::<Record>(),
        );
    }
    // SA_RESETHAND restored SIG_DFL. Queue the same signal; it is delivered
    // after this handler returns, preserving a real WIFSIGNALED(SIGTRAP) exit.
    unsafe {
        libc::raise(signal);
    }
}
pub fn install(fd: RawFd) -> Result<()> {
    FD.store(fd, Ordering::Relaxed);
    let mut action: libc::sigaction = unsafe { std::mem::zeroed() };
    action.sa_sigaction = observed as *const () as usize;
    action.sa_flags = libc::SA_SIGINFO | libc::SA_RESETHAND;
    unsafe {
        libc::sigemptyset(&mut action.sa_mask);
    }
    if unsafe { libc::sigaction(libc::SIGTRAP, &action, std::ptr::null_mut()) } < 0 {
        bail!("install trap observer: {}", std::io::Error::last_os_error())
    }
    Ok(())
}
pub fn owner(root: &Path) -> Result<(u32, u32)> {
    let bytes = fs::read(root.join("trap"))?;
    if bytes.len() != size_of::<Record>() {
        bail!("missing/incomplete native SIGTRAP observation")
    }
    let record = unsafe { std::ptr::read_unaligned(bytes.as_ptr().cast::<Record>()) };
    if record.magic != 0x31505254 || record.int3 != 1 {
        bail!(
            "native SIGTRAP was not an INT3 (si_code={}, PC={:08x})",
            record.code,
            record.pc
        )
    }
    let site = record.pc.checked_sub(1).context("invalid trap PC")?;
    let ranges: Vec<Range> = serde_json::from_slice(&fs::read(root.join("native-ranges"))?)?;
    let matching: Vec<_> = ranges
        .iter()
        .filter(|r| r.start <= site && site < r.end)
        .collect();
    if matching.len() != 1 {
        bail!(
            "native INT3 at {site:08x} has {} App symbol owners",
            matching.len()
        )
    }
    Ok((matching[0].rva, site))
}
