//! Rust-owned runner memory uses a separate, prefixed allocator with arena reuse.
//!
//! The application and native shims retain libc malloc/free/realloc unchanged.
//! CString arguments, pointer arrays, trace labels and descriptor views passed
//! across FFI are borrowed: C must not free/realloc Rust-owned memory, and Rust
//! must not adopt libc/target allocations as Vec/Box/CString ownership.
//!
//! jemalloc's normal prefork/parent/child handlers maintain inherited host heap
//! state. Runner forks remain outside signal handlers and single-threaded;
//! background allocator threads are disabled. exec children initialize afresh.
//! Loader/libc allocations still exist; this is host allocation separation,
//! not a guarantee about unwritten application bytes or a target heap model.
use anyhow::{Result, ensure};
use std::{ffi::CStr, mem, ptr};
use tikv_jemallocator::Jemalloc;

#[global_allocator]
static HOST_ALLOCATOR: Jemalloc = Jemalloc;

unsafe fn option<T: Copy>(name: &'static CStr) -> Result<T> {
    let mut value = mem::MaybeUninit::<T>::uninit();
    let mut length = mem::size_of::<T>();
    let result = unsafe {
        tikv_jemalloc_sys::mallctl(
            name.as_ptr(),
            value.as_mut_ptr().cast(),
            &mut length,
            ptr::null_mut(),
            0,
        )
    };
    ensure!(
        result == 0 && length == mem::size_of::<T>(),
        "invalid host allocator option {name:?}: {result}"
    );
    Ok(unsafe { value.assume_init() })
}

/// Fail before CLI dispatch/application load if startup options broke isolation.
/// Build configuration sets these before allocator initialization; checking them
/// here must not attempt to repair a heap that already used the wrong policy.
pub fn verify() -> Result<()> {
    unsafe {
        let dss: *const libc::c_char = option(c"opt.dss")?;
        ensure!(
            !dss.is_null() && CStr::from_ptr(dss) == c"disabled",
            "host allocator DSS must be disabled"
        );
        ensure!(
            !option::<bool>(c"opt.background_thread")?,
            "host allocator background option must be disabled"
        );
        ensure!(
            !option::<bool>(c"background_thread")?,
            "host allocator background thread is active"
        );
        ensure!(
            option::<u32>(c"opt.narenas")? == 1,
            "host allocator requires one arena"
        );
        ensure!(
            !option::<bool>(c"opt.retain")?,
            "host allocator requires 32-bit retention disabled"
        );
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::alloc::{GlobalAlloc, Layout};

    unsafe fn libc_owner(address: *const libc::c_void) {
        let mut info: libc::Dl_info = unsafe { mem::zeroed() };
        assert_ne!(unsafe { libc::dladdr(address, &mut info) }, 0);
        assert!(!info.dli_fname.is_null());
        assert!(
            unsafe { CStr::from_ptr(info.dli_fname) }
                .to_bytes()
                .ends_with(b"/libc.so.6")
        );
    }

    // Keep fork and allocator checks in one test; use --test-threads=1 when run.
    #[test]
    fn host_allocator_contract() {
        verify().unwrap();
        unsafe {
            libc_owner(libc::malloc as *const libc::c_void);
            libc_owner(libc::free as *const libc::c_void);
            for power in 0..=12 {
                let align = 1 << power;
                for size in [1, 7, 20, 33, 4096] {
                    let layout = Layout::from_size_align(size, align).unwrap();
                    let block = HOST_ALLOCATOR.alloc(layout);
                    assert!(!block.is_null() && block as usize % align == 0);
                    ptr::write_bytes(block, 0x5a, size);
                    let grown = HOST_ALLOCATOR.realloc(block, layout, size + 23);
                    assert!(!grown.is_null() && grown as usize % align == 0);
                    assert!(
                        std::slice::from_raw_parts(grown, size)
                            .iter()
                            .all(|x| *x == 0x5a)
                    );
                    HOST_ALLOCATOR
                        .dealloc(grown, Layout::from_size_align(size + 23, align).unwrap());
                }
            }
            let layout = Layout::from_size_align(20, 4).unwrap();
            let mut old = Vec::new();
            for _ in 0..64 {
                let block = HOST_ALLOCATOR.alloc(layout);
                assert!(!block.is_null());
                old.push(block);
            }
            for &block in &old {
                HOST_ALLOCATOR.dealloc(block, layout);
            }
            let mut reused = 0;
            for _ in 0..64 {
                let block = HOST_ALLOCATOR.alloc(layout);
                assert!(!block.is_null());
                if old.contains(&block) {
                    reused += 1;
                }
                HOST_ALLOCATOR.dealloc(block, layout);
            }
            assert!(reused > 0);
            let live = HOST_ALLOCATOR.alloc(layout);
            assert!(!live.is_null());
            ptr::write_bytes(live, 0x33, 20);
            let pid = libc::fork();
            assert!(pid >= 0);
            if pid == 0 {
                let valid = std::slice::from_raw_parts(live, 20)
                    .iter()
                    .all(|x| *x == 0x33);
                let grown = HOST_ALLOCATOR.realloc(live, layout, 100);
                let valid = valid
                    && !grown.is_null()
                    && std::slice::from_raw_parts(grown, 20)
                        .iter()
                        .all(|x| *x == 0x33);
                if !grown.is_null() {
                    ptr::write_bytes(grown, 0x44, 100);
                    HOST_ALLOCATOR.dealloc(grown, Layout::from_size_align(100, 4).unwrap());
                }
                let text = format!("child-{pid}");
                let valid = valid && text == "child-0";
                drop(text);
                libc::_exit(if valid { 0 } else { 99 });
            }
            let mut status = 0;
            assert_eq!(libc::waitpid(pid, &mut status, 0), pid);
            assert!(libc::WIFEXITED(status) && libc::WEXITSTATUS(status) == 0);
            assert!(
                std::slice::from_raw_parts(live, 20)
                    .iter()
                    .all(|x| *x == 0x33)
            );
            HOST_ALLOCATOR.dealloc(live, layout);
        }
    }
}
