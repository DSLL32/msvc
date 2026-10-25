use crate::{
    abi::{self, Loaded},
    assets, debug,
    fixture::{self, Action, Fixture},
    output, process, status, trap,
};
use anyhow::{Context, Result, bail};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::BTreeSet,
    ffi::CString,
    fs::{self, File, OpenOptions},
    io::{Read, Write},
    os::{
        fd::{AsRawFd, FromRawFd, RawFd},
        unix::fs::{OpenOptionsExt, PermissionsExt},
    },
    path::{Path, PathBuf},
    sync::atomic::{AtomicI32, Ordering},
    time::{Duration, Instant},
};
pub(crate) static SIGNAL: AtomicI32 = AtomicI32::new(0);
extern "C" fn signal(sig: i32) {
    SIGNAL.store(sig, Ordering::Relaxed);
}
pub(crate) fn signals() {
    unsafe {
        libc::signal(libc::SIGINT, signal as *const () as usize);
        libc::signal(libc::SIGTERM, signal as *const () as usize);
        libc::signal(libc::SIGPIPE, libc::SIG_IGN);
    }
}
pub(crate) fn fork() -> Result<i32> {
    let p = unsafe { libc::fork() };
    if p < 0 {
        bail!("fork: {}", std::io::Error::last_os_error())
    }
    Ok(p)
}
pub(crate) fn wait(pid: i32) -> Result<i32> {
    let mut status = 0;
    loop {
        let n = unsafe { libc::waitpid(pid, &mut status, 0) };
        if n == pid {
            return Ok(status);
        }
        if n < 0 && std::io::Error::last_os_error().raw_os_error() != Some(libc::EINTR) {
            bail!("waitpid: {}", std::io::Error::last_os_error())
        }
    }
}
pub(crate) fn receive_child(pid: i32, mut reader: File) -> Result<Vec<u8>> {
    let fd = reader.as_raw_fd();
    let flags = unsafe { libc::fcntl(fd, libc::F_GETFL) };
    if unsafe { libc::fcntl(fd, libc::F_SETFL, flags | libc::O_NONBLOCK) } < 0 {
        bail!("nonblocking report pipe")
    }
    let mut result = vec![];
    let mut buffer = [0u8; 8192];
    let mut forwarded = false;
    loop {
        if !forwarded && SIGNAL.load(Ordering::Relaxed) != 0 {
            unsafe {
                libc::kill(pid, libc::SIGTERM);
            }
            forwarded = true;
        }
        match reader.read(&mut buffer) {
            Ok(0) => break,
            Ok(n) => result.extend_from_slice(&buffer[..n]),
            Err(e) if e.kind() == std::io::ErrorKind::Interrupted => continue,
            Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                std::thread::sleep(Duration::from_millis(2))
            }
            Err(e) => return Err(e.into()),
        }
    }
    wait(pid)?;
    Ok(result)
}
fn code(status: i32) -> String {
    if libc::WIFEXITED(status) {
        format!("exit:{}", libc::WEXITSTATUS(status))
    } else if libc::WIFSIGNALED(status) {
        format!("signal:{}", libc::WTERMSIG(status))
    } else {
        format!("status:{status}")
    }
}
#[derive(Serialize, Deserialize)]
pub struct Report {
    pub case: String,
    pub workspace: String,
    pub pass: bool,
    pub terminal: String,
    pub logical_exit_code: Option<u32>,
    pub first_entry_order: Vec<u32>,
    pub tests: bool,
    pub process_requests: Option<process::ProcessReport>,
    pub trap_rva: Option<u32>,
    pub trap_site: Option<u32>,
    pub elapsed_ms: f64,
    pub setup_ms: f64,
    pub execution_ms: f64,
    pub checks_ms: f64,
    pub errors: Vec<String>,
}
fn descendants(pid: i32, out: &mut Vec<i32>) {
    if let Ok(s) = fs::read_to_string(format!("/proc/{pid}/task/{pid}/children")) {
        for p in s.split_whitespace().filter_map(|p| p.parse().ok()) {
            descendants(p, out);
            out.push(p)
        }
    }
}
pub(crate) fn kill_descendants() {
    let mut pids = vec![];
    descendants(unsafe { libc::getpid() }, &mut pids);
    for pid in pids {
        unsafe {
            libc::kill(pid, libc::SIGKILL);
        }
    }
}
pub(crate) fn dependencies(f: &Fixture) -> Result<()> {
    for d in &f.readonly_dependencies {
        let data = fs::read(&d.path).with_context(|| format!("read dependency {}", d.path))?;
        if fixture::hex(&Sha256::digest(&data)) != d.sha256.to_lowercase() {
            bail!("readonly dependency hash differs: {}", d.path)
        }
    }
    fn files(directory: &Path, result: &mut std::collections::BTreeSet<PathBuf>) -> Result<()> {
        for entry in fs::read_dir(directory)? {
            let entry = entry?;
            let kind = entry.file_type()?;
            if kind.is_dir() {
                files(&entry.path(), result)?;
            } else if kind.is_file() {
                result.insert(entry.path());
            } else {
                bail!(
                    "readonly tree contains a symlink or special file: {}",
                    entry.path().display()
                );
            }
        }
        Ok(())
    }
    for root in &f.readonly_tree_roots {
        let root = Path::new(root);
        anyhow::ensure!(
            fs::symlink_metadata(root)?.file_type().is_dir(),
            "readonly tree root must be an actual directory: {}",
            root.display()
        );
        let expected: std::collections::BTreeSet<_> = f
            .readonly_dependencies
            .iter()
            .map(|d| PathBuf::from(&d.path))
            .filter(|p| p.starts_with(root))
            .collect();
        let mut actual = std::collections::BTreeSet::new();
        files(root, &mut actual)?;
        anyhow::ensure!(
            actual == expected,
            "readonly tree file membership differs in {}: added {:?}; missing {:?}",
            root.display(),
            actual.difference(&expected).next(),
            expected.difference(&actual).next()
        );
    }
    Ok(())
}

#[cfg(test)]
mod readonly_tree_tests {
    use super::*;
    #[test]
    fn exact_file_membership_and_hashes_reject_mutation() {
        use std::os::unix::fs::symlink;
        let root = PathBuf::from(std::env::var_os("HOME").unwrap())
            .join(".ai-tmp")
            .join(format!(
                "recon-readonly-{}-{}",
                std::process::id(),
                std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .unwrap()
                    .as_nanos()
            ));
        fs::create_dir_all(root.join("nested")).unwrap();
        let path = root.join("nested/input.h");
        fs::write(&path, b"declared").unwrap();
        let mut f = Fixture::default();
        f.readonly_dependencies.push(fixture::Dependency {
            path: path.to_str().unwrap().into(),
            sha256: fixture::hex(&Sha256::digest(b"declared")),
        });
        f.readonly_tree_roots.push(root.to_str().unwrap().into());
        f.validate().unwrap();
        dependencies(&f).unwrap();
        fs::create_dir(root.join("empty")).unwrap();
        dependencies(&f).unwrap(); // Contract covers files, not empty directories.
        let added = root.join("new.h");
        fs::write(&added, b"added").unwrap();
        assert!(
            dependencies(&f)
                .unwrap_err()
                .to_string()
                .contains("membership")
        );
        fs::remove_file(&added).unwrap();
        symlink(&path, &added).unwrap();
        assert!(
            dependencies(&f)
                .unwrap_err()
                .to_string()
                .contains("symlink")
        );
        fs::remove_file(&added).unwrap();
        fs::write(&path, b"changed").unwrap();
        assert!(
            dependencies(&f)
                .unwrap_err()
                .to_string()
                .contains("hash differs")
        );
        fs::remove_file(&path).unwrap();
        assert!(dependencies(&f).is_err());
        fs::remove_dir_all(root).unwrap();
        for roots in [
            vec!["relative".into()],
            vec!["/not/covered".into()],
            vec!["/x".into(), "/x".into()],
        ] {
            let mut invalid = Fixture::default();
            invalid.readonly_tree_roots = roots;
            assert!(invalid.validate().is_err());
        }
    }
}
pub(crate) fn workspace_cwd(f: &Fixture, root: &Path) -> Result<PathBuf> {
    if let Some(cwd) = &f.logical_cwd {
        let bytes = cwd.as_bytes();
        if bytes.len() < 3 || bytes[1] != b':' || !matches!(bytes[2], b'\\' | b'/') {
            bail!("logical_cwd must be a drive-absolute Windows path")
        }
        if f.logical_drive
            .as_ref()
            .is_some_and(|d| !d.eq_ignore_ascii_case(&cwd[..1]))
        {
            bail!("logical_drive must match logical_cwd")
        }
        let suffix = cwd[3..].replace('\\', "/");
        if suffix.is_empty() {
            return Ok(root.to_owned());
        }
        fixture::safe_relative(&suffix)?;
        Ok(root.join(suffix))
    } else {
        Ok(root.join("work"))
    }
}
pub(crate) fn create_workspace() -> Result<PathBuf> {
    fs::create_dir_all("/tmp/recon")?;
    let template = CString::new("/tmp/recon/run-XXXXXX")?;
    let mut b = template.into_bytes_with_nul();
    if unsafe { libc::mkdtemp(b.as_mut_ptr().cast()) }.is_null() {
        bail!("mkdtemp: {}", std::io::Error::last_os_error())
    }
    let root = PathBuf::from(unsafe { std::ffi::CStr::from_ptr(b.as_ptr().cast()) }.to_str()?);
    fs::write(root.join(".owner"), unsafe { libc::getpid() }.to_string())?;
    Ok(root)
}
pub(crate) fn materialize(f: &Fixture, cwd: &Path) -> Result<()> {
    fs::create_dir_all(cwd)?;
    for input in &f.input_files {
        let path = cwd.join(&input.path);
        if let Some(p) = path.parent() {
            fs::create_dir_all(p)?
        }
        fs::write(&path, &input.data.0)?;
        if let Some(mode) = input.mode {
            fs::set_permissions(&path, fs::Permissions::from_mode(mode))?
        }
        if let Some(t) = input.modified_unix_seconds {
            let file = File::open(&path)?;
            let ts = libc::timespec {
                tv_sec: t as _,
                tv_nsec: 0,
            };
            if unsafe { libc::futimens(file.as_raw_fd(), [ts, ts].as_ptr()) } < 0 {
                bail!("set input file time: {}", std::io::Error::last_os_error())
            }
        }
    }
    Ok(())
}
pub(crate) fn output_paths(root: &Path, relative: &Path, out: &mut BTreeSet<String>) -> Result<()> {
    for e in fs::read_dir(root.join(relative))? {
        let e = e?;
        let rel = relative.join(e.file_name());
        let ty = e.file_type()?;
        if ty.is_dir() {
            output_paths(root, &rel, out)?
        } else if ty.is_file() {
            out.insert(rel.to_str().context("non-UTF8 output path")?.to_owned());
        } else {
            bail!("unsupported output file type: {}", rel.display())
        }
    }
    Ok(())
}
fn checks(f: &Fixture, cwd: &Path, root: &Path) -> Result<()> {
    for (name, want) in [("stdout", &f.stdout), ("stderr", &f.stderr)] {
        if let Some(want) = want {
            let stream = if root.join("output-go").exists() {
                format!("{name}-compare")
            } else {
                name.to_owned()
            };
            let got = fs::read(root.join(stream))?;
            if got != want.0 {
                bail!(
                    "{name} differs: actual {} bytes, expected {}",
                    got.len(),
                    want.0.len()
                )
            }
        }
    }
    let mut files = BTreeSet::new();
    output_paths(cwd, Path::new(""), &mut files)?;
    for want in &f.output_files {
        if want.absent {
            if files.contains(&want.path) {
                bail!("output should be absent: {}", want.path)
            }
            continue;
        }
        if !files.contains(&want.path) {
            bail!("missing output file {}", want.path)
        }
        let mut got = fs::read(cwd.join(&want.path))?;
        for [a, b] in &want.zero_ranges {
            if *b > got.len() as u64 {
                bail!("normalization beyond output {}", want.path)
            }
            got[*a as usize..*b as usize].fill(0);
        }
        if let Some(data) = &want.data {
            if got != data.0 {
                bail!("output bytes differ: {}", want.path)
            }
        }
        if let Some(hash) = &want.sha256 {
            if fixture::hex(&Sha256::digest(&got)) != hash.to_lowercase() {
                bail!("output hash differs: {}", want.path)
            }
        }
    }
    if f.reject_unexpected_files {
        let allowed: BTreeSet<_> = f
            .input_files
            .iter()
            .map(|v| v.path.clone())
            .chain(
                f.output_files
                    .iter()
                    .filter(|v| !v.absent)
                    .map(|v| v.path.clone()),
            )
            .collect();
        if let Some(extra) = files.difference(&allowed).next() {
            bail!("unexpected output: {extra}")
        }
    }
    Ok(())
}
fn open_stream(path: &Path) -> Result<File> {
    Ok(OpenOptions::new()
        .write(true)
        .create_new(true)
        .mode(0o600)
        .open(path)?)
}
fn dup(fd: RawFd, to: RawFd) -> Result<()> {
    if unsafe { libc::dup2(fd, to) } < 0 {
        bail!("dup2: {}", std::io::Error::last_os_error())
    }
    Ok(())
}
fn clone_tree(src: &Path, dst: &Path) -> Result<()> {
    fs::create_dir_all(dst)?;
    for e in fs::read_dir(src)? {
        let e = e?;
        let to = dst.join(e.file_name());
        if e.file_type()?.is_dir() {
            clone_tree(&e.path(), &to)?
        } else if e.file_type()?.is_file() {
            fs::copy(e.path(), to)?;
        } else {
            bail!("test checkpoint contains unsupported file type")
        }
    }
    Ok(())
}
unsafe fn executor(
    case_path: &Path,
    f: &Fixture,
    root: &Path,
    cwd: &Path,
    stdin: RawFd,
    trace: RawFd,
    spawn: RawFd,
    process_server: process::DenyServer,
    tests: bool,
    debug: &debug::Options,
    supervisor: i32,
) -> Result<()> {
    let setup_start = Instant::now();
    unsafe {
        let core = libc::rlimit {
            rlim_cur: 0,
            rlim_max: 0,
        };
        libc::setrlimit(libc::RLIMIT_CORE, &core);
        libc::setpgid(0, 0);
        libc::signal(libc::SIGINT, libc::SIG_DFL);
        libc::signal(libc::SIGTERM, libc::SIG_DFL);
    }
    let stdout = open_stream(&root.join("stdout"))?;
    let stderr = open_stream(&root.join("stderr"))?;
    let trap_file = open_stream(&root.join("trap"))?;
    trap::install(trap_file.as_raw_fd())?;
    dup(stdin, 0)?;
    dup(stdout.as_raw_fd(), 1)?;
    dup(stderr.as_raw_fd(), 2)?;
    // Fresh descriptor configuration must not accidentally activate legacy tracing.
    for key in [
        "RECON_TRACE_FD",
        "RECON_TRACE_FILTER",
        "RECON_TRACE_SEEN",
        "RECON_TRACE_STATE_FD",
    ] {
        unsafe {
            std::env::remove_var(key);
        }
    }
    // Mirror the shared original prefix's fixed repository drive. An explicit
    // capture value (including removal) takes precedence over this default.
    if !f.env.contains_key("RECON_DRIVE_R") {
        let repository = case_path
            .ancestors()
            .find(|root| {
                root.join("toolchains").is_dir()
                    && root.join("tooling/recon-tool/Cargo.toml").is_file()
            })
            .map(Path::to_owned)
            .or_else(|| crate::toolchain::repository(None).ok());
        if let Some(repository) = repository {
            unsafe {
                std::env::set_var("RECON_DRIVE_R", repository);
            }
        }
    }
    for (k, v) in &f.env {
        unsafe {
            match v {
                Some(v) => std::env::set_var(k, v),
                None => std::env::remove_var(k),
            }
        }
    }
    unsafe {
        std::env::set_var("RECON_DRIVE_T", root);
        if let Some(drive) = &f.logical_drive {
            if !drive.eq_ignore_ascii_case("R") || !f.env.contains_key("RECON_DRIVE_R") {
                std::env::set_var(format!("RECON_DRIVE_{}", drive.to_uppercase()), root);
            }
        }
        if let Some(cwd) = &f.logical_cwd {
            std::env::set_var("RECON_CANONICAL_CWD", cwd);
        }
    }
    unsafe {
        std::env::set_var("RECON_SPAWN_FD", spawn.to_string());
        std::env::remove_var("RECON_CASE_ASSETS");
    }
    std::env::set_current_dir(cwd)?;
    let receipt = status::Receipt::create(root)?;
    let loaded = unsafe { Loaded::open(case_path) }?;
    let recorder =
        unsafe { loaded.optional::<status::Recorder>(b"recon_record_terminal_status\0") }?;
    receipt.supported(root, recorder.is_some())?;
    fs::write(root.join("descriptor"), serde_json::to_vec(&loaded.info)?)?;
    assets::configure(&loaded, f, root, cwd)?;
    if let Some(setup) = loaded.case.setup {
        if unsafe { setup() } == 0 {
            bail!("case setup failed")
        }
    }
    output::prepare(&loaded, f, root, cwd, tests)?;
    process::prepare(&loaded, f, root, cwd, process_server, tests)?;
    if tests {
        fs::write(
            root.join("executor-setup-ms"),
            setup_start.elapsed().as_secs_f64().to_string(),
        )?;
        if let Some(name) = &debug.selected_test {
            if !loaded.info.tests.contains(name) {
                bail!("unknown unit test {name:?}")
            }
        }
        for (index, t) in unsafe { abi::items(loaded.case.tests, loaded.case.test_count) }?
            .iter()
            .enumerate()
        {
            let name = unsafe { abi::string(t.name) }?;
            if debug
                .selected_test
                .as_ref()
                .is_some_and(|selected| selected != &name)
            {
                continue;
            }
            let pass = if t.fork != 0 {
                let test_root = root.join(format!("test-{index}"));
                let dir = workspace_cwd(f, &test_root)?;
                clone_tree(cwd, &dir)?;
                let pid = fork()?;
                if pid == 0 {
                    unsafe {
                        std::env::set_var("RECON_DRIVE_T", &test_root);
                        if let Some(drive) = &f.logical_drive {
                            if !drive.eq_ignore_ascii_case("R")
                                || !f.env.contains_key("RECON_DRIVE_R")
                            {
                                std::env::set_var(
                                    format!("RECON_DRIVE_{}", drive.to_uppercase()),
                                    &test_root,
                                );
                            }
                        }
                    }
                    std::env::set_current_dir(&dir)?;
                    assets::configure(&loaded, f, root, &dir)?;
                    debug::checkpoint(debug, root, supervisor)?;
                    let result = unsafe { t.exec.unwrap()() };
                    unsafe {
                        libc::fflush(std::ptr::null_mut());
                        libc::_exit(if result != 0 { 0 } else { 1 });
                    }
                }
                let status = wait(pid)?;
                libc::WIFEXITED(status) && libc::WEXITSTATUS(status) == 0
            } else {
                debug::checkpoint(debug, root, supervisor)?;
                unsafe { t.exec.unwrap()() != 0 }
            };
            if !pass {
                bail!("unit test failed: {}", unsafe { abi::string(t.name) }?)
            }
        }
    } else {
        // Configure the single shared library tracer before calling application code.
        #[repr(C)]
        struct TraceEntry {
            address: *const libc::c_void,
            name: *const libc::c_char,
        }
        type Initialize = unsafe extern "C" fn(*const TraceEntry, usize, i32);
        let initialize = unsafe { loaded.library.get::<Initialize>(b"recon_trace_configure\0") }
            .context("libbinary.so must supply shared recon_trace_configure")?;
        let symbols = loaded.symbols()?;
        let ranges: Vec<_> = symbols
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
            .collect::<Result<_>>()?;
        fs::write(root.join("native-ranges"), serde_json::to_vec(&ranges)?)?;
        let strings: Vec<CString> = symbols
            .iter()
            .filter(|(func, _, _)| !loaded.info.first_entry_seen.contains(&func.rva))
            .map(|(f, _, _)| CString::new(f.native_symbol.as_ref().unwrap().as_str()).unwrap())
            .collect();
        let pointers: Vec<_> = strings.iter().map(|s| s.as_ptr()).collect();
        let addresses: Vec<_> = symbols
            .iter()
            .filter(|(func, _, _)| !loaded.info.first_entry_seen.contains(&func.rva))
            .map(|(_, address, _)| *address)
            .collect();
        let entries: Vec<_> = addresses
            .iter()
            .zip(&pointers)
            .map(|(address, name)| TraceEntry {
                address: *address as *const _,
                name: *name,
            })
            .collect();
        unsafe { initialize(entries.as_ptr(), entries.len(), trace) };
        type Enable = unsafe extern "C" fn(i32);
        let enable = unsafe { loaded.library.get::<Enable>(b"recon_trace_enable\0") }?;
        fs::write(
            root.join("executor-setup-ms"),
            setup_start.elapsed().as_secs_f64().to_string(),
        )?;
        unsafe { enable(1) };
        debug::checkpoint(debug, root, supervisor)?;
        let exit = if loaded.case.execution == 0 {
            let effective_args = if f.args.is_empty() {
                vec![loaded.info.binary.clone()]
            } else {
                f.args.clone()
            };
            let strings: Vec<_> = effective_args
                .iter()
                .map(|a| CString::new(a.as_str()))
                .collect::<std::result::Result<_, _>>()?;
            let mut args: Vec<_> = strings.iter().map(|s| s.as_ptr() as *mut _).collect();
            args.push(std::ptr::null_mut());
            unsafe { loaded.case.io.unwrap()(strings.len() as i32, args.as_mut_ptr()) }
        } else {
            let passed = unsafe { loaded.case.direct.unwrap()() } != 0;
            fs::write(root.join("direct-return"), if passed { b"1" } else { b"0" })?;
            if passed { 0 } else { 1 }
        };
        receipt.returned(recorder.as_ref().map(|record| **record), exit as u32)?;
        // A returned EXE main / typed DLL adapter must take the normal C exit
        // path. Keep application tracing active through its atexit callbacks.
        unsafe {
            libc::exit(exit & 255);
        }
    }
    unsafe {
        libc::fflush(std::ptr::null_mut());
        libc::_exit(0);
    }
}
fn supervise(case_path: &Path, f: &Fixture, tests: bool, debug: &debug::Options) -> Result<Report> {
    signals();
    let start = Instant::now();
    let root = create_workspace()?;
    // Supervisor owns cleanup even if executor crashes or exits inside target code.
    let result = (|| -> Result<Report> {
        if unsafe { libc::prctl(libc::PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) } < 0 {
            bail!(
                "PR_SET_CHILD_SUBREAPER: {}",
                std::io::Error::last_os_error()
            )
        }
        dependencies(f)?;
        let cwd = workspace_cwd(f, &root)?;
        materialize(f, &cwd)?;
        assets::materialize(f, &root, &cwd)?;
        let (input, mut writer) = if f.stdin_kind == fixture::StdinKind::Pipe {
            let mut pipe = [0; 2];
            if unsafe { libc::pipe2(pipe.as_mut_ptr(), libc::O_CLOEXEC) } < 0 {
                bail!("stdin pipe: {}", std::io::Error::last_os_error())
            }
            let input = unsafe { File::from_raw_fd(pipe[0]) };
            let writer = unsafe { File::from_raw_fd(pipe[1]) };
            let flags = unsafe { libc::fcntl(pipe[1], libc::F_GETFL) };
            if unsafe { libc::fcntl(pipe[1], libc::F_SETFL, flags | libc::O_NONBLOCK) } < 0 {
                bail!("nonblocking stdin pipe")
            }
            (input, Some(writer))
        } else if f.stdin_kind == fixture::StdinKind::Null {
            (File::open("/dev/null")?, None)
        } else {
            fs::write(
                root.join("stdin"),
                f.stdin.as_ref().map_or(&[][..], |data| data.0.as_slice()),
            )?;
            (File::open(root.join("stdin"))?, None)
        };
        let (process_server, process_client) = process::DenyServer::new()?;
        let trace = open_stream(&root.join("trace"))?;
        let setup_ms = start.elapsed().as_secs_f64() * 1000.;
        let run = Instant::now();
        let supervisor = unsafe { libc::getpid() };
        let pid = fork()?;
        if pid == 0 {
            drop(writer.take());
            let result = unsafe {
                executor(
                    case_path,
                    f,
                    &root,
                    &cwd,
                    input.as_raw_fd(),
                    trace.as_raw_fd(),
                    process_client.as_raw_fd(),
                    process_server,
                    tests,
                    debug,
                    supervisor,
                )
            };
            if let Err(e) = result {
                let _ = fs::write(root.join("executor-error"), format!("{e:#}"));
            }
            unsafe {
                libc::_exit(125);
            }
        }
        unsafe {
            libc::setpgid(pid, pid);
        }
        drop(input);
        drop(process_client);
        let mut process_server = Some(process_server);
        let mut close_requested = false;
        let mut pending = if f.stdin_kind == fixture::StdinKind::Pipe {
            f.stdin.as_ref().map_or(vec![], |b| b.0.clone())
        } else {
            vec![]
        };
        let mut offset = 0;
        let mut fired = vec![false; f.events.len()];
        let mut executor_status = None;
        let mut execution_end = None;
        let mut timed_out = false;
        let mut errors = vec![];
        let mut debugger_started = false;
        let mut debugger: Option<debug::Session> = None;
        loop {
            if process_server.is_some() && root.join("process-ready").exists() {
                drop(process_server.take());
                fs::write(root.join("process-go"), b"go")?;
            }
            match process_server
                .as_mut()
                .map_or_else(|| Ok(vec![]), |server| server.poll())
            {
                Ok(denied) => errors.extend(denied),
                Err(error) => {
                    errors.push(format!("process boundary: {error:#}"));
                    kill_descendants();
                }
            }
            if debug.enabled && !debugger_started && root.join("debug-ready").exists() {
                debugger_started = true;
                match debug::start(debug, &root) {
                    Ok(session) => debugger = Some(session),
                    Err(e) => {
                        errors.push(format!("{e:#}"));
                        kill_descendants();
                    }
                }
            }
            if let Some(w) = writer.as_mut() {
                while offset < pending.len() {
                    match w.write(&pending[offset..]) {
                        Ok(n) => offset += n,
                        Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => break,
                        Err(e) if e.kind() == std::io::ErrorKind::BrokenPipe => {
                            offset = pending.len();
                            break;
                        }
                        Err(e) => return Err(e.into()),
                    }
                }
            }
            for (index, event) in f.events.iter().enumerate() {
                if fired[index] {
                    continue;
                }
                let time_ok = event
                    .after_ms
                    .is_none_or(|t| run.elapsed().as_millis() >= t as u128);
                let output_ok = event.stdout_contains.as_ref().is_none_or(|b| {
                    fs::read(root.join("stdout"))
                        .is_ok_and(|out| b.0.is_empty() || out.windows(b.0.len()).any(|s| s == b.0))
                });
                if time_ok && output_ok {
                    match &event.action {
                        Action::Signal { signal } => unsafe {
                            libc::kill(-pid, *signal);
                        },
                        Action::Stdin { data } => pending.extend(&data.0),
                        Action::CloseStdin => {
                            close_requested = true;
                        }
                    }
                    fired[index] = true;
                } else {
                    break;
                }
            }
            if offset == pending.len()
                && (close_requested
                    || !f.hold_stdin_open
                        && !f.events.iter().zip(&fired).any(|(e, fired)| {
                            !fired && matches!(e.action, Action::Stdin { .. } | Action::CloseStdin)
                        }))
            {
                writer.take();
            }
            if (!debug.enabled && run.elapsed().as_millis() > f.timeout_ms as u128)
                || SIGNAL.load(Ordering::Relaxed) != 0
            {
                if !timed_out {
                    errors.push(
                        if SIGNAL.load(Ordering::Relaxed) != 0 {
                            "interrupted"
                        } else {
                            "timeout"
                        }
                        .to_owned(),
                    );
                    timed_out = true;
                }
                unsafe {
                    libc::kill(-pid, libc::SIGKILL);
                }
                kill_descendants();
            }
            let mut status = 0;
            let n = unsafe { libc::waitpid(-1, &mut status, libc::WNOHANG) };
            if n > 0 {
                if debugger.as_ref().is_some_and(|session| session.pid == n) {
                    use std::os::unix::process::ExitStatusExt;
                    if let Err(e) = debugger
                        .take()
                        .unwrap()
                        .finish(std::process::ExitStatus::from_raw(status))
                    {
                        errors.push(format!("{e:#}"));
                    }
                }
                if n == pid {
                    executor_status = Some(status)
                }
                continue;
            }
            if n < 0 {
                let e = std::io::Error::last_os_error();
                if e.raw_os_error() == Some(libc::ECHILD) {
                    execution_end.get_or_insert_with(Instant::now);
                    break;
                }
                if e.raw_os_error() != Some(libc::EINTR) {
                    return Err(e.into());
                }
            }
            if let (Some(status), Some(checker)) = (executor_status, output::checker(&root)?) {
                if !root.join("output-go").exists() && !root.join("output-cancel").exists() {
                    let mut live = vec![];
                    descendants(supervisor, &mut live);
                    if live == [checker] {
                        execution_end.get_or_insert_with(Instant::now);
                        if timed_out
                            || !errors.is_empty()
                            || root.join("executor-error").exists()
                            || root.join("process-error").exists()
                        {
                            fs::write(root.join("output-cancel"), b"cancel")?;
                        } else if let Err(error) = output::release(&root, pid, status) {
                            errors.push(format!("{error:#}"));
                            fs::write(root.join("output-cancel"), b"cancel")?;
                        }
                    }
                }
            }
            std::thread::sleep(Duration::from_millis(2));
        }
        let execution_end = execution_end.context("execution endpoint missing")?;
        let check_start = execution_end;
        if let Err(error) = output::verify(&root) {
            errors.push(format!("{error:#}"));
        }
        if let Ok(error) = fs::read_to_string(root.join("process-error")) {
            errors.push(format!("process policy: {error}"));
        }
        if root.join("process-ready").exists() && !root.join("process-report").exists() {
            errors.push("process broker did not complete".into());
        }
        dependencies(f)?;
        if let Err(error) = assets::verify(f, &root) {
            errors.push(format!("{error:#}"));
        }
        let executor_setup_ms = fs::read_to_string(root.join("executor-setup-ms"))
            .ok()
            .and_then(|s| s.parse::<f64>().ok())
            .unwrap_or(0.)
            * 1000.;
        let setup_ms = setup_ms + executor_setup_ms;
        let execution_ms =
            (execution_end.duration_since(run).as_secs_f64() * 1000. - executor_setup_ms).max(0.);
        let status = executor_status.context("executor status missing")?;
        if let Ok(e) = fs::read_to_string(root.join("executor-error")) {
            errors.push(e)
        }
        // Reuse the descriptor validated by this executor; do not dlopen the
        // entire application a second time just to check its trace.
        let info: abi::Info = serde_json::from_slice(&fs::read(root.join("descriptor"))?)?;
        let names = fs::read_to_string(root.join("trace"))?;
        let mut order = vec![];
        let name_index: std::collections::HashMap<_, _> = info
            .functions
            .iter()
            .filter_map(|f| f.native_symbol.as_deref().map(|name| (name, f.rva)))
            .collect();
        for name in names.lines() {
            if let Some(rva) = name_index.get(name) {
                order.push(*rva)
            } else {
                errors.push(format!("trace contains unknown function {name:?}"))
            }
        }
        let mut trap_rva = None;
        let mut trap_site = None;
        let logical_exit_code = if !tests && libc::WIFEXITED(status) {
            match status::completed(&root, pid as u32, libc::WEXITSTATUS(status)) {
                Ok(code) => Some(code),
                Err(error) => {
                    errors.push(format!("{error:#}"));
                    None
                }
            }
        } else {
            None
        };
        let terminal =
            logical_exit_code.map_or_else(|| code(status), |code| format!("exit:{code}"));
        if !tests {
            if order != info.first_entry_order {
                errors.push(format!(
                    "first-entry order differs: actual {order:x?}, expected {:x?}",
                    info.first_entry_order
                ))
            }
            let terminal_ok = if info.terminal == 0 {
                let returned_direct = info.execution == 1 && root.join("direct-return").exists();
                if returned_direct && fs::read(root.join("direct-return"))? != b"1" {
                    errors.push("direct callback assertions failed".into());
                }
                let expected = if returned_direct {
                    0
                } else {
                    f.exit_code.unwrap_or(0) as u32
                };
                libc::WIFEXITED(status) && logical_exit_code == Some(expected)
            } else {
                if libc::WIFSIGNALED(status) && libc::WTERMSIG(status) == libc::SIGTRAP {
                    match trap::owner(&root) {
                        Ok((rva, site)) => {
                            trap_rva = Some(rva);
                            trap_site = Some(site);
                            if rva != info.frontier_rva {
                                errors.push(format!("native INT3 at {site:08x} belongs to RVA {rva:x}, expected frontier {:x}",info.frontier_rva));
                                false
                            } else {
                                true
                            }
                        }
                        Err(e) => {
                            errors.push(format!("{e:#}"));
                            false
                        }
                    }
                } else {
                    false
                }
            };
            if !terminal_ok {
                errors.push(format!("unexpected terminal {terminal}"))
            }
            if info.terminal == 0 {
                if let Err(e) = checks(f, &cwd, &root) {
                    errors.push(format!("{e:#}"))
                }
            } else if let Some(p) = &f.partial {
                let mut scoped = f.clone();
                scoped.stdout = p.stdout.clone();
                scoped.stderr = p.stderr.clone();
                scoped.output_files = p.output_files.clone();
                scoped.reject_unexpected_files = p.reject_unexpected_files;
                if let Err(e) = checks(&scoped, &cwd, &root) {
                    errors.push(format!("partial oracle: {e:#}"))
                }
            }
            if fired.iter().any(|v| !*v) {
                errors.push("input event was not reached".into())
            }
        } else if !libc::WIFEXITED(status) || libc::WEXITSTATUS(status) != 0 {
            errors.push(format!("unit tests terminated with {}", code(status)))
        }
        Ok(Report {
            case: info.case,
            workspace: root.display().to_string(),
            pass: errors.is_empty(),
            terminal,
            logical_exit_code,
            first_entry_order: order,
            tests,
            process_requests: fs::read(root.join("process-report"))
                .ok()
                .map(|bytes| serde_json::from_slice(&bytes))
                .transpose()?,
            trap_rva,
            trap_site,
            elapsed_ms: start.elapsed().as_secs_f64() * 1000.,
            setup_ms,
            execution_ms,
            checks_ms: check_start.elapsed().as_secs_f64() * 1000.,
            errors,
        })
    })();
    if result.is_err() {
        kill_descendants();
        loop {
            let mut st = 0;
            let n = unsafe { libc::waitpid(-1, &mut st, 0) };
            if n < 0 && std::io::Error::last_os_error().raw_os_error() != Some(libc::EINTR) {
                break;
            }
        }
    }
    let cleanup = fs::remove_dir_all(&root);
    let report = result?;
    cleanup.context("remove case workspace")?;
    Ok(report)
}
pub fn inspect(path: &Path) -> Result<abi::Info> {
    let mut pipe = [0; 2];
    if unsafe { libc::pipe2(pipe.as_mut_ptr(), libc::O_CLOEXEC) } < 0 {
        bail!("pipe")
    }
    let reader = unsafe { File::from_raw_fd(pipe[0]) };
    let writer = unsafe { File::from_raw_fd(pipe[1]) };
    let pid = fork()?;
    if pid == 0 {
        drop(reader);
        let result = (|| -> Result<_> {
            let loaded = unsafe { Loaded::open(path) }?;
            Ok(serde_json::to_vec(&loaded.info)?)
        })();
        let data = match result {
            Ok(data) => data,
            Err(e) => format!(
                "{{\"error\":{}}}",
                serde_json::to_string(&format!("{e:#}")).unwrap()
            )
            .into_bytes(),
        };
        let mut w = writer;
        let _ = w.write_all(&data);
        unsafe { libc::_exit(0) }
    }
    drop(writer);
    let data = receive_child(pid, reader)?;
    let value: serde_json::Value =
        serde_json::from_slice(&data).context("descriptor inspection child failed")?;
    if let Some(e) = value.get("error") {
        bail!("{e}")
    }
    Ok(serde_json::from_value(value)?)
}
pub fn run(
    path: &Path,
    fixture: Option<&Path>,
    tests: bool,
    debug: &debug::Options,
) -> Result<Report> {
    signals();
    let path = path.canonicalize()?;
    let f = match fixture {
        Some(p) => fixture::read(p)?,
        None => {
            if !tests && inspect(&path)?.execution == 0 {
                bail!(
                    "IO case requires capture.bin; pack a minimal fixture or supply --capture PATH"
                )
            }
            Fixture::default()
        }
    };
    let mut pipe = [0; 2];
    if unsafe { libc::pipe2(pipe.as_mut_ptr(), libc::O_CLOEXEC) } < 0 {
        bail!("pipe")
    }
    let reader = unsafe { File::from_raw_fd(pipe[0]) };
    let writer = unsafe { File::from_raw_fd(pipe[1]) };
    let pid = fork()?;
    if pid == 0 {
        drop(reader);
        let result = supervise(&path, &f, tests, debug);
        let value = match result {
            Ok(r) => serde_json::to_value(r)?,
            Err(e) => serde_json::json!({"error":format!("{e:#}")}),
        };
        let mut w = writer;
        let _ = w.write_all(&serde_json::to_vec(&value)?);
        unsafe { libc::_exit(0) }
    }
    drop(writer);
    let data = receive_child(pid, reader)?;
    let value: serde_json::Value =
        serde_json::from_slice(&data).context("case supervisor failed")?;
    if let Some(e) = value.get("error") {
        bail!("{e}")
    };
    Ok(serde_json::from_value(value)?)
}
