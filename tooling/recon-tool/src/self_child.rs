//! Plain exec re-entry for a case's explicitly permitted synchronous self-child.
use crate::{abi::Loaded, assets, fixture::Fixture, native, status, trap};
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use std::{
    ffi::{CString, OsString},
    fs::{self, File},
    os::fd::{AsRawFd, RawFd},
    path::{Path, PathBuf},
};

#[derive(clap::Args)]
pub struct Options {
    pub case: PathBuf,
    #[arg(long)]
    pub workspace: PathBuf,
    /// Prepared fixture cwd; the actual process cwd is inherited across exec.
    #[arg(long)]
    pub cwd: PathBuf,
    #[arg(long)]
    pub trace_fd: RawFd,
    #[arg(long)]
    pub trace_state_fd: RawFd,
    #[arg(last = true, required = true)]
    pub args: Vec<OsString>,
}
#[derive(Serialize, Deserialize)]
struct Paths {
    logical_cwd: Option<String>,
    assets: Option<PathBuf>,
}
pub fn save_paths(f: &Fixture, root: &Path) -> Result<()> {
    if std::env::var_os("RECON_SELF_SPAWN").is_some_and(|v| v == "1") {
        fs::create_dir(root.join("children"))?;
        fs::write(
            root.join("native-child-paths.json"),
            serde_json::to_vec_pretty(&Paths {
                logical_cwd: f.logical_cwd.clone(),
                assets: assets::root(f, root),
            })?,
        )?;
    }
    Ok(())
}
pub fn inheritable(fd: RawFd) -> Result<()> {
    let flags = unsafe { libc::fcntl(fd, libc::F_GETFD) };
    ensure!(
        flags >= 0 && unsafe { libc::fcntl(fd, libc::F_SETFD, flags & !libc::FD_CLOEXEC) } >= 0,
        "inheritable native child descriptor"
    );
    Ok(())
}
pub fn register(
    loaded: &Loaded,
    case: &Path,
    root: &Path,
    cwd: &Path,
    trace: RawFd,
    state: RawFd,
) -> Result<()> {
    type Register = unsafe extern "C" fn(*const libc::c_char, *const *const libc::c_char, usize);
    let register = unsafe {
        loaded
            .library
            .get::<Register>(b"recon_set_native_self_command\0")
    }
    .context("self-child requires shared recon_set_native_self_command")?;
    let exe = CString::new(std::env::current_exe()?.as_os_str().as_encoded_bytes())?;
    let prefix = [
        OsString::from("native-child"),
        case.as_os_str().to_owned(),
        "--workspace".into(),
        root.as_os_str().to_owned(),
        "--cwd".into(),
        cwd.as_os_str().to_owned(),
        "--trace-fd".into(),
        trace.to_string().into(),
        "--trace-state-fd".into(),
        state.to_string().into(),
        "--".into(),
    ];
    let strings = prefix
        .iter()
        .map(|s| CString::new(s.as_encoded_bytes()))
        .collect::<std::result::Result<Vec<_>, _>>()?;
    let pointers = strings.iter().map(|s| s.as_ptr()).collect::<Vec<_>>();
    inheritable(trace)?;
    inheritable(state)?;
    unsafe { register(exe.as_ptr(), pointers.as_ptr(), pointers.len()) };
    Ok(())
}

pub fn configure_trace(
    loaded: &Loaded,
    root: &Path,
    trace: RawFd,
    state: Option<RawFd>,
    initialize_state: bool,
) -> Result<()> {
    #[repr(C)]
    struct TraceEntry {
        address: *const libc::c_void,
        name: *const libc::c_char,
    }
    let symbols = loaded.symbols()?;
    let ranges = symbols
        .iter()
        .map(|(f, address, size)| {
            Ok(trap::Range {
                start: u32::try_from(*address)?,
                end: u32::try_from(
                    address
                        .checked_add(*size)
                        .context("native symbol range overflow")?,
                )?,
                rva: f.rva,
            })
        })
        .collect::<Result<Vec<_>>>()?;
    fs::write(root.join("native-ranges"), serde_json::to_vec(&ranges)?)?;
    let filtered = symbols
        .iter()
        .filter(|(func, _, _)| !loaded.info.first_entry_seen.contains(&func.rva))
        .collect::<Vec<_>>();
    let strings = filtered
        .iter()
        .map(|(f, _, _)| CString::new(f.native_symbol.as_ref().unwrap().as_str()))
        .collect::<std::result::Result<Vec<_>, _>>()?;
    let entries = filtered
        .iter()
        .zip(&strings)
        .map(|((_, address, _), name)| TraceEntry {
            address: *address as *const _,
            name: name.as_ptr(),
        })
        .collect::<Vec<_>>();
    if let Some(state) = state {
        if initialize_state {
            ensure!(
                unsafe { libc::ftruncate(state, (32 + entries.len()) as libc::off_t) } == 0,
                "size shared trace state"
            );
        } else {
            let mut magic = [0u8; 4];
            ensure!(
                unsafe { libc::pread(state, magic.as_mut_ptr().cast(), 4, 0) } == 4
                    && magic == *b"RST1",
                "child requires initialized shared trace state"
            );
        }
        type Initialize = unsafe extern "C" fn(*const TraceEntry, usize, i32, i32);
        let initialize = unsafe {
            loaded
                .library
                .get::<Initialize>(b"recon_trace_configure_shared\0")
        }?;
        unsafe { initialize(entries.as_ptr(), entries.len(), trace, state) };
    } else {
        type Initialize = unsafe extern "C" fn(*const TraceEntry, usize, i32);
        let initialize = unsafe { loaded.library.get::<Initialize>(b"recon_trace_configure\0") }
            .context("libbinary.so must supply shared recon_trace_configure")?;
        unsafe { initialize(entries.as_ptr(), entries.len(), trace) };
    }
    Ok(())
}

pub fn save_environment(root: &Path) -> Result<()> {
    let entries = std::env::vars_os()
        .map(|(key, value)| {
            let mut bytes = key.as_encoded_bytes().to_vec();
            bytes.push(b'=');
            bytes.extend_from_slice(value.as_encoded_bytes());
            bytes
        })
        .collect::<Vec<_>>();
    fs::write(
        root.join(format!("process-environment-{}.json", unsafe {
            libc::getpid()
        })),
        serde_json::to_vec(&entries)?,
    )?;
    Ok(())
}
pub fn environment(root: &Path, pid: u32) -> Result<Vec<CString>> {
    let entries: Vec<Vec<u8>> = serde_json::from_slice(&fs::read(
        root.join(format!("process-environment-{pid}.json")),
    )?)?;
    entries
        .into_iter()
        .map(|v| CString::new(v).map_err(Into::into))
        .collect()
}

#[derive(Serialize, Deserialize)]
pub struct ChildResult {
    pub launcher_pid: u32,
    pub parent_pid: u32,
    pub executor_pid: u32,
    pub kernel_status: i32,
    pub terminal: String,
    pub logical_exit_code: Option<u32>,
    pub trap_rva: Option<u32>,
    pub trap_site: Option<u32>,
    pub errors: Vec<String>,
}
pub fn results(root: &Path, expected: u32) -> Result<Vec<ChildResult>> {
    let mut children = vec![];
    if root.join("children").exists() {
        for entry in fs::read_dir(root.join("children"))? {
            let entry = entry?;
            let result: ChildResult = serde_json::from_slice(
                &fs::read(entry.path().join("result.json")).with_context(|| {
                    format!(
                        "native child {} did not complete",
                        entry.file_name().to_string_lossy()
                    )
                })?,
            )?;
            children.push(result);
        }
    }
    children.sort_by_key(|child| child.launcher_pid);
    ensure!(
        children.len() == expected as usize,
        "native child result count differs: actual {}, permitted {expected}",
        children.len()
    );
    Ok(children)
}
unsafe fn executor(options: &Options, root: &Path) -> Result<()> {
    // The launcher has not loaded the DSO: both CRT and application globals are fresh.
    let trap_file = File::create(root.join("trap"))?;
    trap::install(trap_file.as_raw_fd())?;
    let receipt = status::Receipt::create(root)?;
    crate::errno::set(0);
    let loaded = unsafe { Loaded::open(&options.case) }?;
    let constructor_errno = crate::errno::get();
    ensure!(
        loaded.case.execution == 0,
        "native-child requires an IO adapter"
    );
    let recorder =
        unsafe { loaded.optional::<status::Recorder>(b"recon_record_terminal_status\0") }?;
    receipt.supported(root, recorder.is_some())?;
    let paths: Paths = serde_json::from_slice(&fs::read(
        options.workspace.join("native-child-paths.json"),
    )?)?;
    assets::configure_paths(
        &loaded,
        &options.cwd,
        paths.logical_cwd.as_deref(),
        paths.assets.as_deref(),
    )?;
    let setup_errno = if let Some(setup) = loaded.case.setup {
        let (passed, value) = crate::errno::invoke(constructor_errno, || unsafe { setup() });
        ensure!(passed != 0, "case setup failed");
        value
    } else {
        constructor_errno
    };
    save_environment(&options.workspace)?;
    configure_trace(
        &loaded,
        root,
        options.trace_fd,
        Some(options.trace_state_fd),
        false,
    )?;
    register(
        &loaded,
        &options.case,
        &options.workspace,
        &options.cwd,
        options.trace_fd,
        options.trace_state_fd,
    )?;
    type Enable = unsafe extern "C" fn(i32);
    let enable = unsafe { loaded.library.get::<Enable>(b"recon_trace_enable\0") }?;
    let strings = options
        .args
        .iter()
        .map(|s| CString::new(s.as_encoded_bytes()))
        .collect::<std::result::Result<Vec<_>, _>>()?;
    let mut args = strings
        .iter()
        .map(|s| s.as_ptr() as *mut _)
        .collect::<Vec<_>>();
    args.push(std::ptr::null_mut());
    unsafe { enable(1) };
    let (exit, target_errno) = crate::errno::invoke(setup_errno, || unsafe {
        loaded.case.io.unwrap()(strings.len() as i32, args.as_mut_ptr())
    });
    receipt.returned(recorder.as_ref().map(|record| **record), exit as u32)?;
    crate::errno::set(target_errno);
    unsafe { libc::exit(exit & 255) }
}
pub fn run(options: Options) -> Result<()> {
    ensure!(
        options.workspace.is_absolute() && options.cwd.is_absolute() && options.case.is_absolute(),
        "native-child requires absolute case/workspace/cwd paths"
    );
    let launcher = unsafe { libc::getpid() } as u32;
    let parent = unsafe { libc::getppid() } as u32;
    let root = options
        .workspace
        .join("children")
        .join(launcher.to_string());
    fs::create_dir(&root)?;
    let pid = native::fork()?;
    if pid == 0 {
        let result = unsafe { executor(&options, &root) };
        if let Err(error) = result {
            let _ = fs::write(root.join("executor-error"), format!("{error:#}"));
        }
        unsafe { libc::_exit(125) }
    }
    let terminal = native::wait(pid)?;
    let mut errors = vec![];
    let mut trap_rva = None;
    let mut trap_site = None;
    if libc::WIFSIGNALED(terminal) && libc::WTERMSIG(terminal) == libc::SIGTRAP {
        match trap::owner(&root) {
            Ok((rva, site)) => {
                trap_rva = Some(rva);
                trap_site = Some(site);
            }
            Err(error) => errors.push(format!("{error:#}")),
        }
    }
    let logical = if libc::WIFEXITED(terminal) {
        match status::completed(&root, pid as u32, libc::WEXITSTATUS(terminal)) {
            Ok(code) => Some(code),
            Err(error) => {
                errors.push(format!("{error:#}"));
                None
            }
        }
    } else {
        errors.push(format!(
            "unexpected native child signal {}",
            libc::WTERMSIG(terminal)
        ));
        None
    };
    if let Ok(error) = fs::read_to_string(root.join("executor-error")) {
        errors.push(error);
    }
    let result = ChildResult {
        launcher_pid: launcher,
        parent_pid: parent,
        executor_pid: pid as u32,
        kernel_status: terminal,
        terminal: if libc::WIFEXITED(terminal) {
            format!("exit:{}", libc::WEXITSTATUS(terminal))
        } else {
            format!("signal:{}", libc::WTERMSIG(terminal))
        },
        logical_exit_code: logical,
        trap_rva,
        trap_site,
        errors,
    };
    fs::write(
        root.join("result.json"),
        serde_json::to_vec_pretty(&result)?,
    )?;
    if libc::WIFSIGNALED(terminal) {
        let signal = libc::WTERMSIG(terminal);
        unsafe {
            libc::signal(signal, libc::SIG_DFL);
            libc::raise(signal);
            libc::_exit(128 + signal);
        }
    }
    unsafe { libc::_exit(libc::WEXITSTATUS(terminal)) }
}
