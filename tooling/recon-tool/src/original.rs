//! Explicit original capture only. This module is never called by native replay.
use crate::wine::command;
use anyhow::{Context, Result, bail};
use std::{
    fs,
    path::{Path, PathBuf},
};
fn windows(path: &Path, repo: &Path) -> Result<String> {
    let path = crate::wine::windows(path, repo)?;
    anyhow::ensure!(
        path.is_ascii(),
        "original ANSI transport currently requires ASCII paths"
    );
    Ok(path)
}
/// Target environment controls never become Wine's Unix launch environment.
fn target_environment(env: &std::collections::BTreeMap<String, Option<String>>) -> Result<Vec<u8>> {
    let mut names = std::collections::BTreeSet::new();
    let mut bytes = Vec::new();
    for (key, value) in env {
        anyhow::ensure!(
            !key.is_empty()
                && key.is_ascii()
                && !key.contains(['=', '\0'])
                && value
                    .as_ref()
                    .is_none_or(|v| v.is_ascii() && !v.contains('\0')),
            "original target environment requires valid ASCII names/values"
        );
        anyhow::ensure!(
            names.insert(key.to_ascii_uppercase()),
            "duplicate case-insensitive target environment name: {key}"
        );
        bytes.extend_from_slice(key.as_bytes());
        if let Some(value) = value {
            bytes.push(b'=');
            bytes.extend_from_slice(value.as_bytes());
        }
        bytes.push(0);
    }
    if bytes.is_empty() {
        bytes.push(0);
    }
    bytes.push(0);
    anyhow::ensure!(
        bytes.len() <= 32767,
        "original target environment overlay exceeds 32767 bytes"
    );
    Ok(bytes)
}
pub fn build_helper(repo: &Path) -> Result<PathBuf> {
    let repo = repo.canonicalize()?;
    let root = repo.join("tooling/recon-tool");
    let build = root.join(".build");
    fs::create_dir_all(&build)?;
    let exe = build.join("wine-broker.exe");
    let object = build.join("wine-broker.obj");
    let mut c = crate::toolchain::command(&repo, "1.0-nt", "CL.EXE")?;
    c.args(["/nologo", "/Od", "/G3"])
        .arg(format!("/Fe{}", windows(&exe, &repo)?))
        .arg(format!("/Fo{}", windows(&object, &repo)?))
        .arg(windows(&root.join("wine/broker.c"), &repo)?)
        // Wine reserves the low DOS area; avoid initial-image relocation and start.exe fallback.
        .args(["/link", "/base:0x400000"])
        .current_dir(&build);
    let result = c.output()?;
    fs::write(
        build.join("wine-build.log"),
        [result.stdout.as_slice(), result.stderr.as_slice()].concat(),
    )?;
    if !result.status.success() {
        eprint!(
            "{}{}",
            String::from_utf8_lossy(&result.stdout),
            String::from_utf8_lossy(&result.stderr)
        );
        bail!("build shared Wine debugger helper: {}", result.status)
    }
    Ok(exe)
}

/// Explicit original-only helper compilation, never a native fixture prerequisite.
#[derive(clap::Args)]
pub struct BuildDirect {
    pub source: PathBuf,
    #[arg(long)]
    pub repo: Option<PathBuf>,
    #[arg(long, default_value = "1.0-nt")]
    pub toolchain: String,
    #[arg(short, long)]
    pub output: PathBuf,
    #[arg(long = "include")]
    pub includes: Vec<PathBuf>,
    #[arg(long = "define")]
    pub defines: Vec<String>,
}
pub fn build_direct(options: &BuildDirect) -> Result<PathBuf> {
    let repo = crate::toolchain::repository(options.repo.as_deref())?;
    let source = options.source.canonicalize()?;
    let output = std::path::absolute(&options.output)?;
    let directory = output.parent().context("helper output directory")?;
    fs::create_dir_all(directory)?;
    let stem = output
        .file_stem()
        .context("helper filename")?
        .to_string_lossy();
    let object = directory.join(format!("{stem}-case.obj"));
    let support = directory.join(format!("{stem}-support.obj"));
    let definition = directory.join(format!("{stem}.def"));
    fs::write(
        &definition,
        "EXPORTS\n recon_original_completion_trap=_recon_original_completion_trap@0\n",
    )?;
    let root = repo.join("tooling/recon-tool");
    let mut log = Vec::new();
    let result = (|| -> Result<()> {
        for (input, object) in [
            (&source, &object),
            (&root.join("wine/direct-support.c"), &support),
        ] {
            let mut command = crate::toolchain::command(&repo, &options.toolchain, "CL.EXE")?;
            command
                .args(["/nologo", "/c", "/Od", "/G3", "/DRECON_ORIGINAL_DIRECT"])
                .arg(format!("/I{}", windows(&root.join("include"), &repo)?));
            for include in &options.includes {
                command.arg(format!("/I{}", windows(&include.canonicalize()?, &repo)?));
            }
            for define in &options.defines {
                anyhow::ensure!(
                    !define.is_empty() && !define.contains(['\r', '\n']),
                    "invalid helper define"
                );
                command.arg(format!("/D{define}"));
            }
            let result = command
                .arg(format!("/Fo{}", windows(object, &repo)?))
                .arg(windows(input, &repo)?)
                .current_dir(directory)
                .output()?;
            log.extend_from_slice(&result.stdout);
            log.extend_from_slice(&result.stderr);
            anyhow::ensure!(
                result.status.success(),
                "original typed helper compilation failed: {}",
                result.status
            );
        }
        let result = crate::toolchain::command(&repo, &options.toolchain, "LINK.EXE")?
            .args(["/nologo", "/dll", "/nodefaultlib", "/entry:DllMain@12"])
            .arg(format!("/def:{}", windows(&definition, &repo)?))
            .arg(format!("/out:{}", windows(&output, &repo)?))
            .arg(windows(&object, &repo)?)
            .arg(windows(&support, &repo)?)
            .args(["LIBC.LIB", "KERNEL32.LIB"])
            .current_dir(directory)
            .output()?;
        log.extend_from_slice(&result.stdout);
        log.extend_from_slice(&result.stderr);
        anyhow::ensure!(
            result.status.success(),
            "original typed helper link failed: {}",
            result.status
        );
        Ok(())
    })();
    fs::write(directory.join(format!("{stem}-build.log")), &log)?;
    if result.is_err() {
        eprint!("{}", String::from_utf8_lossy(&log));
    }
    result?;
    Ok(output)
}

use crate::{
    abi::Loaded,
    fixture::{self, Bytes, Fixture, InputFile, OutputFile},
    native,
};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::{BTreeMap, BTreeSet},
    ffi::{CStr, c_char, c_void},
    fs::{File, OpenOptions},
    io::{Read, Write},
    os::fd::{AsRawFd, FromRawFd},
    process::{Child, ChildStdin, ChildStdout, Stdio},
    sync::atomic::Ordering,
    time::{Duration, Instant},
};
#[derive(clap::Args)]
pub struct Capture {
    /// Hash-bound CF format/loader identity and binary-owned code/data layout.
    #[arg(long)]
    pub cf_profile: Option<PathBuf>,
    /// Retired live CF diagnostic switch; identity-only profiles remain static inputs.
    #[arg(long, requires = "cf_profile")]
    pub entry_stop: bool,
    pub case: PathBuf,
    /// Repository root; inferred from cwd or recon-tool's location by default.
    #[arg(long)]
    pub repo: Option<PathBuf>,
    #[arg(long = "capture")]
    pub io: Option<PathBuf>,
    #[arg(long)]
    pub program: Option<PathBuf>,
    #[arg(long)]
    pub helper: Option<PathBuf>,
    /// Win32 invocation host for a DLL; it loads RECON_ORIGINAL_DLL.
    #[arg(long)]
    pub host: Option<PathBuf>,
    /// Select the selected original toolchain's native MSVCRT, independently of target env.
    #[arg(long)]
    pub native_msvcrt: bool,
    /// Export raw generated artifacts to a new external directory before normalization.
    #[arg(long)]
    pub artifacts_dir: Option<PathBuf>,
    /// Typed Win32 probe DLL compiled from case-owned direct.c; explicit original only.
    #[arg(long, requires = "marker_rva")]
    pub direct_helper: Option<PathBuf>,
    /// Genuine App entry at which the typed original callback takes over.
    #[arg(long, requires = "direct_helper")]
    pub marker_rva: Option<u32>,
    /// Typed callback must terminate the target with the fixture's expected status.
    #[arg(long, requires = "direct_helper", conflicts_with = "cf_profile")]
    pub direct_noreturn: bool,
    /// Observe Watcom EAX/EDX main arguments and project actual argv into the fixture.
    #[arg(long, conflicts_with_all = ["cf_profile", "direct_helper"])]
    pub watcom_main_argv: bool,
    #[arg(short, long)]
    pub output: PathBuf,
    #[arg(long)]
    pub report: Option<PathBuf>,
    /// Mask original code, restoring classified boundaries and implemented App extents.
    #[arg(long)]
    pub progress: bool,
    /// At a genuine progress frontier compare only explicitly declared Partial output paths.
    #[arg(long, requires = "progress")]
    pub partial_output_files_only: bool,
    /// Debug one synchronous child loading the exact same staged PE EXE.
    #[arg(long, requires = "progress", conflicts_with_all = ["cf_profile", "direct_helper", "watcom_main_argv"])]
    pub pe_self_child: bool,
    /// Retain the actual target launch OS block separately from the CRT snapshot.
    #[arg(long, requires = "native_msvcrt", conflicts_with_all = ["cf_profile", "direct_helper", "watcom_main_argv"])]
    pub process_os_environment: bool,
    #[arg(long)]
    pub replace: bool,
}
#[derive(Clone, Copy)]
#[repr(C)]
pub struct Registers {
    eax: u32,
    ebx: u32,
    ecx: u32,
    edx: u32,
    esi: u32,
    edi: u32,
    ebp: u32,
    esp: u32,
    eip: u32,
    eflags: u32,
    fp: [u8; 112],
}
#[derive(Clone, Copy)]
#[repr(C)]
pub struct CallResult {
    entry: Registers,
    returned: Registers,
}
#[repr(C)]
pub struct ContextOriginal {
    version: u32,
    size: u32,
    base: u32,
    opaque: *mut c_void,
    read: unsafe extern "C" fn(*mut Self, u32, *mut c_void, u32) -> i32,
    write: unsafe extern "C" fn(*mut Self, u32, *const c_void, u32) -> i32,
    allocate: unsafe extern "C" fn(*mut Self, u32) -> u32,
    release: unsafe extern "C" fn(*mut Self, u32) -> i32,
    until: unsafe extern "C" fn(*mut Self, u32, *mut Registers) -> i32,
    call: unsafe extern "C" fn(*mut Self, u32, u32, u32, *const u32, u32, *mut CallResult) -> i32,
    emit: unsafe extern "C" fn(*mut Self, *const c_char, *const c_void, u32) -> i32,
}
struct Reply {
    a: u32,
    b: u32,
    data: Vec<u8>,
}
struct Broker {
    child: Child,
    input: Option<ChildStdin>,
    output: ChildStdout,
    order: Vec<u32>,
    seen: BTreeSet<u32>,
    app: Vec<u32>,
    emitted: Vec<InputFile>,
    error: Option<String>,
    pe_self: Option<crate::pe_self::Tracker>,
    wait_cleanup: bool,
}
impl Broker {
    fn reply(&mut self) -> Result<Reply> {
        let mut h = [0; 16];
        if let Err(error) = self.output.read_exact(&mut h) {
            let status = self.child.try_wait().ok().flatten();
            return Err(error).with_context(|| {
                format!("read Wine debugger response (helper status {status:?})")
            });
        }
        let word = |i| u32::from_le_bytes(h[i..i + 4].try_into().unwrap());
        let size = word(12) as usize;
        if size > 16 * 1024 * 1024 {
            bail!("Wine debugger response too large")
        }
        let mut data = vec![0; size];
        self.output.read_exact(&mut data)?;
        if word(0) != 1 {
            bail!(
                "Wine debugger rejected operation (a={}, b={})",
                word(4),
                word(8)
            )
        }
        Ok(Reply {
            a: word(4),
            b: word(8),
            data,
        })
    }
    fn rpc(&mut self, op: u32, a: u32, b: u32, c: u32, data: &[u8]) -> Result<Reply> {
        anyhow::ensure!(
            data.len() <= 16 * 1024 * 1024,
            "Wine debugger request too large"
        );
        for v in [op, a, b, c, data.len() as u32] {
            self.input
                .as_mut()
                .context("closed Wine debugger input")?
                .write_all(&v.to_le_bytes())?;
        }
        let input = self.input.as_mut().context("closed Wine debugger input")?;
        input.write_all(data)?;
        input.flush()?;
        self.reply()
            .with_context(|| format!("Wine debugger operation {op}"))
    }
    fn event(&mut self, mut reply: Reply, registers: bool) -> Result<Reply> {
        if let Some(track) = self.pe_self.as_mut() {
            let (owner, data) = crate::pe_self::Owner::decode(
                &reply.data,
                if registers { size_of::<Registers>() } else { 0 },
            )?;
            track.observe(owner, reply.a, reply.b)?;
            reply.data = data;
        }
        Ok(reply)
    }
    fn event_base(&self, root: u32) -> u32 {
        self.pe_self
            .as_ref()
            .and_then(|v| v.current.as_ref())
            .map_or(root, |v| v.base)
    }
    fn record(&mut self, rva: u32) {
        if !self.seen.contains(&rva) && self.app.contains(&rva) {
            self.seen.insert(rva);
            self.order.push(rva);
        }
    }
    fn arm(&mut self) -> Result<()> {
        let words: Vec<_> = self
            .app
            .iter()
            .filter(|rva| !self.seen.contains(rva))
            .flat_map(|rva| rva.to_le_bytes())
            .collect();
        self.rpc(6, 0, 0, 0, &words)?;
        Ok(())
    }
    fn close(&mut self) -> Result<()> {
        self.rpc(9, 0, 0, 0, &[])?;
        let status = self.child.wait()?;
        anyhow::ensure!(status.success(), "Wine debugger exited with {status}");
        Ok(())
    }
}
impl Drop for Broker {
    fn drop(&mut self) {
        if self.wait_cleanup {
            // EOF requests the existing bounded owned-process teardown even on errors.
            self.input.take();
            let _ = self.child.wait();
        }
    }
}
unsafe fn broker<'a>(ctx: *mut ContextOriginal) -> &'a mut Broker {
    unsafe { &mut *((*ctx).opaque as *mut Broker) }
}
fn record_error<T>(b: &mut Broker, r: Result<T>) -> Option<T> {
    match r {
        Ok(v) => Some(v),
        Err(e) => {
            b.error = Some(format!("{e:#}"));
            None
        }
    }
}
unsafe extern "C" fn remote_read(
    ctx: *mut ContextOriginal,
    address: u32,
    bytes: *mut c_void,
    size: u32,
) -> i32 {
    if size > 16 * 1024 * 1024 || size > 0 && bytes.is_null() {
        return 0;
    }
    let b = unsafe { broker(ctx) };
    let r = b.rpc(1, address, size, 0, &[]);
    if let Some(reply) = record_error(b, r) {
        if reply.data.len() != size as usize {
            return 0;
        }
        if size > 0 {
            unsafe {
                std::ptr::copy_nonoverlapping(reply.data.as_ptr(), bytes.cast(), size as usize)
            }
        }
        1
    } else {
        0
    }
}
unsafe extern "C" fn remote_write(
    ctx: *mut ContextOriginal,
    address: u32,
    bytes: *const c_void,
    size: u32,
) -> i32 {
    if size > 16 * 1024 * 1024 || size > 0 && bytes.is_null() {
        return 0;
    }
    let b = unsafe { broker(ctx) };
    let data = if size == 0 {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(bytes.cast::<u8>(), size as usize) }
    };
    let r = b.rpc(2, address, 0, 0, data);
    i32::from(record_error(b, r).is_some())
}
unsafe extern "C" fn remote_allocate(ctx: *mut ContextOriginal, size: u32) -> u32 {
    let b = unsafe { broker(ctx) };
    let r = b.rpc(3, size, 0, 0, &[]);
    record_error(b, r).map_or(0, |r| r.a)
}
unsafe extern "C" fn remote_release(ctx: *mut ContextOriginal, address: u32) -> i32 {
    let b = unsafe { broker(ctx) };
    let r = b.rpc(4, address, 0, 0, &[]);
    i32::from(record_error(b, r).is_some())
}
unsafe extern "C" fn remote_until(ctx: *mut ContextOriginal, rva: u32, out: *mut Registers) -> i32 {
    if out.is_null() {
        return 0;
    }
    let b = unsafe { broker(ctx) };
    let r = b.rpc(5, rva, 0, 0, &[]);
    if let Some(r) = record_error(b, r) {
        if r.data.len() != size_of::<Registers>() {
            return 0;
        }
        unsafe { std::ptr::write_unaligned(out, std::ptr::read_unaligned(r.data.as_ptr().cast())) }
        1
    } else {
        0
    }
}
unsafe extern "C" fn remote_call(
    ctx: *mut ContextOriginal,
    rva: u32,
    ecx: u32,
    edx: u32,
    words: *const u32,
    count: u32,
    out: *mut CallResult,
) -> i32 {
    if out.is_null() || count > 1024 || count > 0 && words.is_null() {
        return 0;
    }
    let b = unsafe { broker(ctx) };
    let r = (|| -> Result<_> {
        b.arm()?;
        let args = if count == 0 {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(words.cast::<u8>(), count as usize * 4) }
        };
        let r = b.rpc(8, rva, ecx, edx, args)?;
        let len = size_of::<CallResult>();
        anyhow::ensure!(r.data.len() >= len + 4, "short original call result");
        let n = u32::from_le_bytes(r.data[len..len + 4].try_into()?) as usize;
        anyhow::ensure!(
            r.data.len() == len + 4 + n * 4,
            "invalid original call trace length"
        );
        for word in r.data[len + 4..].chunks_exact(4) {
            b.record(u32::from_le_bytes(word.try_into()?));
        }
        Ok(unsafe { std::ptr::read_unaligned(r.data.as_ptr().cast::<CallResult>()) })
    })();
    if let Some(result) = record_error(b, r) {
        unsafe { std::ptr::write_unaligned(out, result) }
        1
    } else {
        0
    }
}
unsafe extern "C" fn remote_emit(
    ctx: *mut ContextOriginal,
    name: *const c_char,
    bytes: *const c_void,
    size: u32,
) -> i32 {
    if name.is_null() || size > 16 * 1024 * 1024 || size > 0 && bytes.is_null() {
        return 0;
    }
    let b = unsafe { broker(ctx) };
    let r = (|| -> Result<_> {
        let name = unsafe { CStr::from_ptr(name) }.to_str()?;
        fixture::safe_relative(name)?;
        anyhow::ensure!(
            !b.emitted.iter().any(|f| f.path == name),
            "duplicate emitted original fixture {name}"
        );
        let data = if size == 0 {
            vec![]
        } else {
            unsafe { std::slice::from_raw_parts(bytes.cast::<u8>(), size as usize) }.to_vec()
        };
        Ok(InputFile {
            path: name.to_owned(),
            data: Bytes(data),
            mode: None,
            modified_unix_seconds: None,
        })
    })();
    if let Some(file) = record_error(b, r) {
        b.emitted.push(file);
        1
    } else {
        0
    }
}

pub(crate) fn quote(s: &str) -> String {
    if !s.is_empty() && !s.chars().any(|c| c.is_whitespace() || c == '"') {
        return s.to_owned();
    }
    let mut result = String::from("\"");
    let mut slashes = 0;
    for c in s.chars() {
        if c == '\\' {
            slashes += 1;
            continue;
        }
        if c == '"' {
            result.push_str(&"\\".repeat(slashes * 2 + 1));
        } else {
            result.push_str(&"\\".repeat(slashes));
        }
        slashes = 0;
        result.push(c);
    }
    result.push_str(&"\\".repeat(slashes * 2));
    result.push('"');
    result
}
fn capture_command_line(args: &[String], declared: Option<&str>) -> Result<String> {
    let text = args
        .iter()
        .map(|arg| quote(arg))
        .collect::<Vec<_>>()
        .join(" ");
    anyhow::ensure!(
        text.is_ascii() && !text.contains('\0') && text.len() <= 32766,
        "original command line must be ASCII without NUL and at most 32766 bytes"
    );
    anyhow::ensure!(
        declared.is_none_or(|declared| declared == text),
        "raw_command_line differs from constructed original launch text; use an inputs-only fixture without the captured raw_command_line for a fresh original invocation"
    );
    Ok(text)
}

#[cfg(test)]
mod command_line_tests {
    use super::*;
    #[test]
    fn records_exact_launch_quoting_and_rejects_changed_declared_input() {
        let args = vec!["R:\\requested.exe".into(), "two words".into(), "?".into()];
        let text = capture_command_line(&args, None).unwrap();
        assert_eq!(text, "R:\\requested.exe \"two words\" ?");
        assert_eq!(capture_command_line(&args, Some(&text)).unwrap(), text);
        assert!(capture_command_line(&args, Some("different raw input")).is_err());
        for bad in ["x\0y".into(), "é".into(), "x".repeat(32767)] {
            assert!(capture_command_line(&[bad], None).is_err());
        }
    }
    #[test]
    fn command_line_quotes_only_tokens_that_require_escaping() {
        for (input, expected) in [
            ("?", "?"),
            ("-q", "-q"),
            ("@archive.rsp", "@archive.rsp"),
            (r"R:\bin\tool.exe", r"R:\bin\tool.exe"),
            ("", "\"\""),
            ("two words", "\"two words\""),
            ("a\tb", "\"a\tb\""),
            ("a\"b", "\"a\\\"b\""),
            ("path with\\", "\"path with\\\\\""),
        ] {
            assert_eq!(quote(input), expected, "{input:?}");
        }
    }
}
// Preserve the context already returned for a debugger exception. It is
// diagnostic failure evidence only, never a fixture oracle or extra memory read.
#[derive(Clone, Debug, Serialize, Deserialize)]
struct ExceptionSnapshot {
    code: u32,
    image_base: u32,
    registers: BTreeMap<String, u32>,
    raw_context: Bytes,
    #[serde(skip_serializing_if = "Option::is_none", default)]
    process_id: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none", default)]
    thread_id: Option<u32>,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
struct CaptureFailure {
    error: String,
    #[serde(skip_serializing_if = "Option::is_none", default)]
    exception: Option<ExceptionSnapshot>,
}
impl std::fmt::Display for CaptureFailure {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.error)
    }
}
impl std::error::Error for CaptureFailure {}
impl CaptureFailure {
    fn from_error(error: &anyhow::Error) -> Self {
        Self {
            error: format!("{error:#}"),
            exception: error
                .downcast_ref::<Self>()
                .and_then(|v| v.exception.clone()),
        }
    }
    fn read(text: String) -> Self {
        serde_json::from_str(&text).unwrap_or(Self {
            error: text,
            exception: None,
        })
    }
    fn save(&self, path: &Path) -> Result<()> {
        fs::write(path, serde_json::to_vec(self)?)?;
        Ok(())
    }
}
fn register_values(data: &[u8]) -> Result<BTreeMap<String, u32>> {
    anyhow::ensure!(
        data.len() == size_of::<Registers>(),
        "invalid original register snapshot"
    );
    let names = [
        "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "eip", "eflags",
    ];
    Ok(names
        .iter()
        .zip(data[..40].chunks_exact(4))
        .map(|(name, bytes)| {
            (
                (*name).into(),
                u32::from_le_bytes(bytes.try_into().unwrap()),
            )
        })
        .collect())
}
fn exception_failure(
    label: &str,
    code: u32,
    base: u32,
    data: &[u8],
    owner: Option<&crate::pe_self::Owner>,
) -> Result<CaptureFailure> {
    let registers = register_values(data)?;
    Ok(CaptureFailure {
        error: format!(
            "{label} {code:08x} (EIP {:08x}, target base {base:08x})",
            registers["eip"]
        ),
        exception: Some(ExceptionSnapshot {
            code,
            image_base: base,
            registers,
            raw_context: Bytes(data.to_vec()),
            process_id: owner.map(|v| v.pid),
            thread_id: owner.map(|v| v.tid),
        }),
    })
}
#[cfg(test)]
mod exception_report_tests {
    use super::*;
    #[test]
    fn context_survives_executor_serialization_and_supervisor_error() -> Result<()> {
        let mut data = vec![0xa5; size_of::<Registers>()];
        data[28..32].copy_from_slice(&0x0012ff00u32.to_le_bytes());
        data[32..36].copy_from_slice(&0x00404567u32.to_le_bytes());
        let owner = crate::pe_self::Owner {
            pid: 20,
            tid: 24,
            base: 0x400000,
            child: true,
            entry: true,
        };
        let original = exception_failure(
            "original exception",
            0xc0000005,
            0x400000,
            &data,
            Some(&owner),
        )?;
        let first: anyhow::Error = original.into();
        let encoded = serde_json::to_string(&CaptureFailure::from_error(&first))?;
        let from_executor = CaptureFailure::read(encoded);
        let supervisor: anyhow::Error = from_executor.into();
        let report = serde_json::to_value(CaptureFailure::from_error(
            &supervisor.context("capture executor"),
        ))?;
        assert!(report["error"].as_str().unwrap().contains("EIP 00404567"));
        assert_eq!(report["exception"]["code"], 0xc0000005u32);
        assert_eq!(report["exception"]["image_base"], 0x400000);
        assert_eq!(report["exception"]["registers"]["esp"], 0x0012ff00);
        assert_eq!(report["exception"]["registers"]["eip"], 0x00404567);
        assert_eq!(report["exception"]["process_id"], 20);
        assert_eq!(report["exception"]["thread_id"], 24);
        let decoded: CaptureFailure = serde_json::from_value(report)?;
        assert_eq!(decoded.exception.unwrap().raw_context.0, data);
        Ok(())
    }
    #[test]
    fn generic_failures_and_legacy_error_text_keep_their_shape() -> Result<()> {
        let failure = CaptureFailure::from_error(&anyhow::anyhow!("unchanged failure"));
        assert_eq!(
            serde_json::to_value(&failure)?,
            serde_json::json!({"error":"unchanged failure"})
        );
        assert_eq!(
            serde_json::to_value(CaptureFailure::read("legacy failure".into()))?,
            serde_json::json!({"error":"legacy failure"})
        );
        let data = vec![0; size_of::<Registers>()];
        let typed = exception_failure("typed original exception", 5, 0x400000, &data, None)?;
        let report = serde_json::to_value(typed)?;
        assert!(
            report["error"]
                .as_str()
                .unwrap()
                .starts_with("typed original exception")
        );
        assert!(report["exception"].get("process_id").is_none());
        Ok(())
    }
    #[test]
    fn context_extent_is_exact() {
        for size in [
            0,
            40,
            size_of::<Registers>() - 1,
            size_of::<Registers>() + 1,
        ] {
            assert!(exception_failure("original exception", 5, 0, &vec![0; size], None).is_err());
        }
    }
}
#[derive(Serialize, Deserialize)]
struct FrontierSnapshot {
    image_base: u32,
    rva: u32,
    registers: BTreeMap<String, u32>,
    floating_save: Option<Bytes>,
    stack_address: u32,
    stack_requested: u32,
    stack: Option<Bytes>,
    stack_error: Option<String>,
}
const FRONTIER_STACK_BYTES: u32 = 64;

fn frontier_snapshot(
    broker: &mut Broker,
    base: u32,
    rva: u32,
    data: &[u8],
) -> Result<FrontierSnapshot> {
    let registers = register_values(data)?;
    let esp = registers["esp"];
    // Bounded diagnostic prefix, including return PC and current stack formals.
    let (stack, stack_error) = match broker.rpc(1, esp, FRONTIER_STACK_BYTES, 0, &[]) {
        Ok(reply) if reply.data.len() == FRONTIER_STACK_BYTES as usize => {
            (Some(Bytes(reply.data)), None)
        }
        Ok(reply) => (
            None,
            Some(format!(
                "stack read returned {} of {} bytes",
                reply.data.len(),
                FRONTIER_STACK_BYTES
            )),
        ),
        Err(error) => (None, Some(format!("{error:#}"))),
    };
    Ok(FrontierSnapshot {
        image_base: base,
        rva,
        registers,
        floating_save: Some(Bytes(data[40..].to_vec())),
        stack_address: esp,
        stack_requested: FRONTIER_STACK_BYTES,
        stack,
        stack_error,
    })
}
#[derive(Serialize, Deserialize)]
struct RawArtifact {
    path: String,
    exported_path: PathBuf,
    sha256: String,
    size: u64,
}
fn reserve_artifacts(path: &Path, root: &Path, cwd: &Path) -> Result<PathBuf> {
    let absolute = std::path::absolute(path)?;
    let parent = absolute
        .parent()
        .context("artifact directory parent")?
        .canonicalize()?;
    let destination = parent.join(absolute.file_name().context("artifact directory name")?);
    anyhow::ensure!(
        !destination.starts_with(root.canonicalize()?)
            && !destination.starts_with(cwd.canonicalize()?),
        "artifact directory must be outside the ephemeral workspace and cwd"
    );
    fs::create_dir(&destination)
        .with_context(|| format!("reserve new artifact directory {}", destination.display()))?;
    fs::create_dir(destination.join("files"))?;
    Ok(destination)
}
fn export_artifacts(
    f: &Fixture,
    cwd: &Path,
    rules: &[OutputFile],
    destination: &Path,
) -> Result<Vec<RawArtifact>> {
    let mut paths = BTreeSet::new();
    native::output_paths(cwd, Path::new(""), &mut paths)?;
    let mut artifacts = Vec::new();
    for path in paths {
        fixture::safe_relative(&path)?;
        let bytes = fs::read(cwd.join(&path))?;
        if !rules.iter().any(|v| v.path == path)
            && f.input_files
                .iter()
                .any(|v| v.path == path && v.data.0 == bytes)
        {
            continue;
        }
        let exported_path = destination.join("files").join(&path);
        let parent = exported_path.parent().context("artifact parent")?;
        let files_root = destination.join("files").canonicalize()?;
        let mut existing = parent;
        while !existing.try_exists()? {
            existing = existing.parent().context("artifact parent ancestor")?;
        }
        anyhow::ensure!(
            existing.canonicalize()?.starts_with(&files_root),
            "artifact parent escapes export directory"
        );
        fs::create_dir_all(parent)?;
        anyhow::ensure!(
            parent.canonicalize()?.starts_with(&files_root),
            "artifact parent escapes export directory"
        );
        let mut file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&exported_path)?;
        file.write_all(&bytes)?;
        artifacts.push(RawArtifact {
            path,
            exported_path,
            sha256: fixture::hex(&Sha256::digest(&bytes)),
            size: bytes.len() as u64,
        });
    }
    let mut manifest = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(destination.join("manifest.json"))?;
    manifest.write_all(&serde_json::to_vec_pretty(&artifacts)?)?;
    Ok(artifacts)
}
#[derive(Serialize, Deserialize)]
struct WatcomMainArgv {
    entry_rva: u32,
    argc: u32,
    argv_address: u32,
    requested: Vec<String>,
    actual: Vec<String>,
}
fn read_watcom_main_argv(
    entry_rva: u32,
    pc: u32,
    registers: &[u8],
    requested: &[String],
    mut read: impl FnMut(u32, u32) -> Result<Vec<u8>>,
) -> Result<WatcomMainArgv> {
    anyhow::ensure!(
        registers.len() == size_of::<Registers>(),
        "invalid Watcom main registers"
    );
    let word = |offset| u32::from_le_bytes(registers[offset..offset + 4].try_into().unwrap());
    let argc = word(0);
    let argv = word(12);
    anyhow::ensure!(
        word(32) == pc && argc > 0 && argc <= 256 && argc as usize == requested.len(),
        "Watcom main argc/PC differs from requested invocation"
    );
    let size = (argc + 1) * 4;
    anyhow::ensure!(
        argv != 0 && argv.checked_add(size).is_some(),
        "invalid Watcom argv address"
    );
    let pointers = read(argv, size)?;
    anyhow::ensure!(
        pointers.len() == size as usize && pointers[size as usize - 4..] == [0; 4],
        "Watcom argv vector is short or lacks its terminal NULL"
    );
    let mut actual = Vec::new();
    let mut total = 0;
    for pointer in pointers[..size as usize - 4].chunks_exact(4) {
        let address = u32::from_le_bytes(pointer.try_into().unwrap());
        anyhow::ensure!(address != 0, "NULL Watcom argument");
        let mut bytes = Vec::new();
        loop {
            anyhow::ensure!(
                bytes.len() < 4096 && total < 65536,
                "Watcom argument exceeds capture bounds"
            );
            let at = address
                .checked_add(bytes.len() as u32)
                .context("Watcom argument address overflow")?;
            let byte = read(at, 1)?;
            anyhow::ensure!(byte.len() == 1, "short Watcom argument read");
            total += 1;
            if byte[0] == 0 {
                break;
            }
            anyhow::ensure!(
                byte[0].is_ascii(),
                "Watcom ANSI argument is not supported ASCII"
            );
            bytes.push(byte[0]);
        }
        actual.push(String::from_utf8(bytes)?);
    }
    anyhow::ensure!(
        actual[1..] == requested[1..],
        "actual Watcom main argv[1..] differs from requested arguments"
    );
    Ok(WatcomMainArgv {
        entry_rva,
        argc,
        argv_address: argv,
        requested: requested.to_vec(),
        actual,
    })
}

#[cfg(test)]
mod watcom_argv_tests {
    use super::*;
    fn sample() -> (Vec<u8>, BTreeMap<u32, u8>, Vec<String>) {
        let mut registers = vec![0; size_of::<Registers>()];
        for (offset, value) in [(0, 2u32), (12, 0x1000), (32, 0x412a67)] {
            registers[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
        }
        let mut memory = BTreeMap::new();
        for (address, bytes) in [
            (
                0x1000,
                [0x2000u32, 0x3000, 0]
                    .into_iter()
                    .flat_map(u32::to_le_bytes)
                    .collect::<Vec<_>>(),
            ),
            (0x2000, b"T:\\run-original\\WLIB.EXE\0".to_vec()),
            (0x3000, b"?\0".to_vec()),
        ] {
            for (offset, value) in bytes.into_iter().enumerate() {
                memory.insert(address + offset as u32, value);
            }
        }
        (registers, memory, vec!["WLIB.EXE".into(), "?".into()])
    }
    fn observe(
        registers: &[u8],
        memory: &BTreeMap<u32, u8>,
        requested: &[String],
    ) -> Result<WatcomMainArgv> {
        read_watcom_main_argv(0x12a67, 0x412a67, registers, requested, |address, size| {
            (0..size)
                .map(|offset| {
                    memory
                        .get(&(address + offset))
                        .copied()
                        .context("unreadable target byte")
                })
                .collect()
        })
    }
    #[test]
    fn actual_watcom_argv0_is_observed_without_changing_requested_inputs() {
        let (registers, memory, requested) = sample();
        let result = observe(&registers, &memory, &requested).unwrap();
        assert_eq!(result.requested, requested);
        assert_eq!(result.actual, ["T:\\run-original\\WLIB.EXE", "?"]);
        assert_eq!(result.argc, 2);
        assert_eq!(result.argv_address, 0x1000);
    }
    #[test]
    fn watcom_argument_count_pc_vector_and_tail_are_checked() {
        let (registers, memory, requested) = sample();
        for (offset, value) in [
            (0, 0u32),
            (0, 257),
            (0, 3),
            (12, 0),
            (12, u32::MAX),
            (32, 0x412a68),
        ] {
            let mut bad = registers.clone();
            bad[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
            assert!(observe(&bad, &memory, &requested).is_err());
        }
        for (address, value) in [(0x1008, 1), (0x1001, 0), (0x3000, b'!'), (0x3000, 255)] {
            let mut bad = memory.clone();
            bad.insert(address, value);
            assert!(observe(&registers, &bad, &requested).is_err());
        }
        assert!(observe(&registers[..40], &memory, &requested).is_err());
    }
    #[test]
    fn watcom_strings_must_be_bounded_and_reads_complete() {
        let (registers, mut memory, requested) = sample();
        memory.remove(&0x2000);
        assert!(observe(&registers, &memory, &requested).is_err());
        for offset in 0..4096 {
            memory.insert(0x2000 + offset, b'A');
        }
        assert!(observe(&registers, &memory, &requested).is_err());
        assert!(
            read_watcom_main_argv(0x12a67, 0x412a67, &registers, &requested, |_, _| Ok(vec![]))
                .is_err()
        );
    }
}

#[derive(Serialize, Deserialize)]
struct CaptureReport {
    #[serde(default)]
    identity: Option<crate::original_reference::Identity>,
    #[serde(default)]
    capture_sha256: Option<String>,
    #[serde(default)]
    oracle_written: bool,
    #[serde(default)]
    backend: Option<serde_json::Value>,
    case: String,
    terminal: String,
    first_entry_order: Vec<u32>,
    elapsed_ms: f64,
    workspace: String,
    #[serde(default)]
    staged_files: Vec<crate::original_stage::Provenance>,
    #[serde(default)]
    frontier: Option<FrontierSnapshot>,
    #[serde(default)]
    artifacts_dir: Option<PathBuf>,
    #[serde(default)]
    raw_artifacts: Vec<RawArtifact>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    direct_noreturn: Option<crate::original_reference::DirectExit>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    watcom_main_argv: Option<WatcomMainArgv>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pe_self_child: Option<crate::pe_self::Capture>,
}
fn capture_files(f: &Fixture, cwd: &Path, output_rules: &[OutputFile]) -> Result<Vec<OutputFile>> {
    let mut paths = BTreeSet::new();
    native::output_paths(cwd, Path::new(""), &mut paths)?;
    let mut result = vec![];
    for path in paths {
        let bytes = fs::read(cwd.join(&path))?;
        let explicit = output_rules.iter().find(|v| v.path == path);
        let unchanged = f
            .input_files
            .iter()
            .any(|v| v.path == path && v.data.0 == bytes);
        if unchanged && explicit.is_none() {
            continue;
        }
        let zero_ranges = explicit.map_or(vec![], |v| v.zero_ranges.clone());
        let mut bytes = bytes;
        for &[start, end] in &zero_ranges {
            anyhow::ensure!(
                end <= bytes.len() as u64,
                "normalization exceeds captured output {path}"
            );
            bytes[start as usize..end as usize].fill(0);
        }
        result.push(OutputFile {
            path,
            absent: false,
            data: None,
            sha256: Some(fixture::hex(&Sha256::digest(&bytes))),
            zero_ranges,
        });
    }
    for path in f
        .input_files
        .iter()
        .map(|v| &v.path)
        .chain(output_rules.iter().map(|v| &v.path))
    {
        if !cwd.join(path).exists() && !result.iter().any(|v| v.path == *path) {
            result.push(OutputFile {
                path: path.clone(),
                absent: true,
                data: None,
                sha256: None,
                zero_ranges: vec![],
            });
        }
    }
    Ok(result)
}
fn validate_partial_selection(progress: bool, selected: bool, f: &Fixture) -> Result<()> {
    anyhow::ensure!(
        !selected || (progress && f.partial.is_some()),
        "--partial-output-files-only requires --progress and an explicit Partial input policy"
    );
    Ok(())
}

fn capture_output_files(
    f: &Fixture,
    cwd: &Path,
    partial: bool,
    selected: bool,
) -> Result<(Vec<OutputFile>, bool)> {
    let rules = if partial {
        f.partial
            .as_ref()
            .map_or(&[][..], |p| p.output_files.as_slice())
    } else {
        f.output_files.as_slice()
    };
    let mut files = capture_files(f, cwd, rules)?;
    let reject_unexpected = if partial && selected {
        let policy = f
            .partial
            .as_ref()
            .context("explicit Partial input policy required")?;
        files.retain(|file| rules.iter().any(|rule| rule.path == file.path));
        policy.reject_unexpected_files
    } else {
        f.reject_unexpected_files
    };
    Ok((files, reject_unexpected))
}

fn typed_results(data: &[u8]) -> Result<Vec<InputFile>> {
    let word = |offset: usize| -> Result<u32> {
        Ok(u32::from_le_bytes(
            data.get(offset..offset + 4)
                .context("truncated typed result")?
                .try_into()?,
        ))
    };
    let count = word(0)?;
    anyhow::ensure!(count <= 100000, "too many typed results");
    let mut offset = 4usize;
    let mut result = vec![];
    let mut names = BTreeSet::new();
    for _ in 0..count {
        let name_size = word(offset)? as usize;
        let size = word(offset + 4)? as usize;
        offset += 8;
        anyhow::ensure!(
            name_size > 0 && name_size <= 4096,
            "invalid typed result name length"
        );
        let end = offset
            .checked_add(name_size)
            .context("typed result name overflow")?;
        let name =
            std::str::from_utf8(data.get(offset..end).context("truncated typed name")?)?.to_owned();
        offset = end;
        fixture::safe_relative(&name)?;
        anyhow::ensure!(
            name.bytes().all(|b| b >= 32 && b != b':' && b != b'\\'),
            "invalid typed result path"
        );
        anyhow::ensure!(names.insert(name.clone()), "duplicate typed result {name}");
        let end = offset
            .checked_add(size)
            .context("typed result size overflow")?;
        let bytes = data
            .get(offset..end)
            .context("truncated typed bytes")?
            .to_vec();
        offset = end;
        result.push(InputFile {
            path: name,
            data: Bytes(bytes),
            mode: None,
            modified_unix_seconds: None,
        });
    }
    anyhow::ensure!(offset == data.len(), "trailing typed result data");
    Ok(result)
}

#[cfg(test)]
mod typed_tests {
    use super::*;

    #[test]
    fn nonreturn_exit_keeps_dword_and_rejects_missing_or_wrong_proof() {
        let bytes: Vec<u8> = [1u32, 0x10001000, 123, 124]
            .into_iter()
            .flat_map(u32::to_le_bytes)
            .collect();
        assert_eq!(
            direct_exit(true, true, u32::MAX, u32::MAX, &bytes)
                .unwrap()
                .actual_exit,
            u32::MAX
        );
        assert!(direct_exit(false, true, 4, 4, &bytes).is_err());
        assert!(direct_exit(true, false, 4, 4, &bytes).is_err());
        assert!(direct_exit(true, true, 4, 5, &bytes).is_err());
        assert!(direct_exit(true, true, 4, 4, &[]).is_err());
        let mut missing = bytes;
        missing[..4].fill(0);
        assert!(direct_exit(true, true, 4, 4, &missing).is_err());
    }

    fn packet(names: &[&str]) -> Vec<u8> {
        let mut result = (names.len() as u32).to_le_bytes().to_vec();
        for name in names {
            result.extend_from_slice(&(name.len() as u32).to_le_bytes());
            result.extend_from_slice(&2u32.to_le_bytes());
            result.extend_from_slice(name.as_bytes());
            result.extend_from_slice(&[0, 255]);
        }
        result
    }

    #[test]
    fn typed_receipts_preserve_bytes_and_reject_invalid_records() {
        let valid = packet(&["pending.bin"]);
        let result = typed_results(&valid).unwrap();
        assert_eq!(result[0].path, "pending.bin");
        assert_eq!(result[0].data.0, [0, 255]);
        for name in ["../escape", "/absolute", "C:/drive", "a/../b"] {
            assert!(typed_results(&packet(&[name])).is_err(), "{name}");
        }
        assert!(typed_results(&packet(&["same", "same"])).is_err());
        assert!(typed_results(&valid[..valid.len() - 1]).is_err());
        let mut trailing = valid;
        trailing.push(0);
        assert!(typed_results(&trailing).is_err());
        assert!(typed_results(&100001u32.to_le_bytes()).is_err());
    }
}

fn capture_executor(
    options: &Capture,
    repo: &Path,
    f: &Fixture,
    root: &Path,
    cwd: &Path,
    artifacts_dir: Option<&Path>,
) -> Result<(Option<Fixture>, CaptureReport)> {
    let result = capture_executor_inner(options, repo, f, root, cwd, artifacts_dir);
    if options.pe_self_child || options.process_os_environment {
        result.with_context(|| {
            let log = fs::read(root.join("wine-diagnostics")).unwrap_or_default();
            format!(
                "Original owned-process diagnostics: {}",
                String::from_utf8_lossy(&log[..log.len().min(65536)])
            )
        })
    } else {
        result
    }
}

fn capture_executor_inner(
    options: &Capture,
    repo: &Path,
    f: &Fixture,
    root: &Path,
    cwd: &Path,
    artifacts_dir: Option<&Path>,
) -> Result<(Option<Fixture>, CaptureReport)> {
    let start = Instant::now();
    for key in [
        "RECON_TRACE_FD",
        "RECON_TRACE_FILTER",
        "RECON_TRACE_SEEN",
        "RECON_TRACE_STATE_FD",
    ] {
        unsafe { std::env::remove_var(key) }
    }
    let loaded = unsafe {
        if options.progress {
            Loaded::open_for_progress(&options.case)
        } else {
            Loaded::open(&options.case)
        }
    }?;
    anyhow::ensure!(
        !options.pe_self_child || loaded.info.execution == 0,
        "PE SELF capture requires ordinary IO execution"
    );
    anyhow::ensure!(
        !options.process_os_environment || loaded.info.execution == 0,
        "process OS environment capture requires ordinary IO execution"
    );
    anyhow::ensure!(
        f.process_os_environment.is_none() || options.process_os_environment,
        "declared process_os_environment requires --process-os-environment for original capture"
    );
    let program = options
        .program
        .clone()
        .unwrap_or_else(|| repo.join(&loaded.info.original_path))
        .canonicalize()?;
    let original = fs::read(&program)?;
    anyhow::ensure!(
        fixture::hex(&Sha256::digest(&original)) == loaded.info.original_sha256.to_lowercase(),
        "original binary SHA256 differs from binary_descriptor"
    );
    if options.cf_profile.is_some() {
        let environment = root.join("original-environment");
        fs::write(&environment, target_environment(&f.env)?)?;
        let backend = crate::original_cf::capture(
            options,
            repo,
            &loaded.info,
            f,
            root,
            cwd,
            &program,
            &environment,
        )?;
        let entry_stop = options.entry_stop;
        let terminal = if entry_stop {
            format!("entry-stop:{:x}", loaded.info.original_entry_rva)
        } else {
            backend["terminal"]
                .as_str()
                .context("CF terminal missing")?
                .to_owned()
        };
        let order: Vec<u32> = serde_json::from_value(backend["first_entry_order"].clone())?;
        let mut captured = None;
        let mut raw_artifacts = vec![];
        let mut snapshot = None;
        if !entry_stop {
            let partial = terminal.starts_with("frontier:");
            anyhow::ensure!(
                options.progress && (partial || terminal.starts_with("exit:")),
                "CF backend requires an actual progress frontier or process exit"
            );
            if partial {
                let rva = u32::from_str_radix(terminal.strip_prefix("frontier:").unwrap(), 16)?;
                anyhow::ensure!(
                    loaded
                        .info
                        .functions
                        .iter()
                        .any(|f| f.rva == rva && f.definition == 4 && f.implementation == 0),
                    "CF frontier is not a pending App registry entry"
                );
            }
            let rules = if partial {
                f.partial
                    .as_ref()
                    .map_or(&[][..], |p| p.output_files.as_slice())
            } else {
                f.output_files.as_slice()
            };
            if let Some(directory) = artifacts_dir {
                raw_artifacts = export_artifacts(f, cwd, rules, directory)?;
            }
            let mut fixture = f.clone();
            fixture.raw_command_line = Some(
                backend["raw_command_line"]
                    .as_str()
                    .context("CF actual raw command line missing")?
                    .to_owned(),
            );
            let argv0 = backend["logical_payload_argv0"]
                .as_str()
                .context("CF payload argv0 missing")?;
            if fixture.args.is_empty() {
                fixture.args.push(argv0.to_owned());
            } else {
                fixture.args[0] = argv0.to_owned();
            }
            let stdout = Some(Bytes(fs::read(root.join("original-stdout"))?));
            let stderr = Some(Bytes(fs::read(root.join("original-stderr"))?));
            let (files, reject_unexpected_files) =
                capture_output_files(f, cwd, partial, options.partial_output_files_only)?;
            if partial {
                fixture.partial = Some(fixture::Partial {
                    stdout,
                    stderr,
                    output_files: files,
                    reject_unexpected_files,
                });
                snapshot = Some(serde_json::from_value(
                    backend["frontier_snapshot"].clone(),
                )?);
            } else {
                fixture.partial = None;
                fixture.stdout = stdout;
                fixture.stderr = stderr;
                fixture.output_files = files;
                fixture.exit_code =
                    Some(terminal.strip_prefix("exit:").unwrap().parse::<u32>()? as i32);
            }
            native::dependencies(f)?;
            captured = Some(fixture);
        }
        let report = CaptureReport {
            identity: Some(crate::original_reference::Identity::from_info(&loaded.info)),
            capture_sha256: None,
            case: loaded.info.case,
            terminal,
            first_entry_order: order,
            elapsed_ms: start.elapsed().as_secs_f64() * 1000.,
            workspace: root.display().to_string(),
            staged_files: vec![],
            frontier: snapshot,
            artifacts_dir: artifacts_dir.map(Path::to_owned),
            raw_artifacts,
            oracle_written: !entry_stop,
            backend: Some(backend),
            direct_noreturn: None,
            watcom_main_argv: None,
            pe_self_child: None,
        };
        return Ok((captured, report));
    }
    let image = crate::original_image::Image::parse(&original)?;
    anyhow::ensure!(
        image.entry != 0,
        "original DLL/image without an entry point is not supported"
    );
    anyhow::ensure!(
        image.entry == loaded.info.original_entry_rva,
        "original entry RVA differs from binary_descriptor"
    );
    anyhow::ensure!(
        image.dll == options.host.is_some(),
        "DLL originals require --host; EXE originals do not use --host"
    );
    anyhow::ensure!(
        !options.progress || loaded.info.execution == 0 || options.direct_helper.is_some(),
        "masked DIRECT progress requires the typed original helper"
    );
    let helper = options
        .helper
        .clone()
        .unwrap_or_else(|| repo.join("tooling/recon-tool/.build/wine-broker.exe"));
    anyhow::ensure!(
        helper.is_file(),
        "build explicit Wine helper first: original build-helper"
    );
    // Configure only the private on-disk helper and executable, before launch.
    // The Windows loader loads the import; the DLL owns its local VEH/callback.
    anyhow::ensure!(
        !options.direct_noreturn
            || (options.direct_helper.is_some()
                && loaded.info.execution == 1
                && f.exit_code.is_some()),
        "nonreturn DIRECT requires a typed DIRECT case and expected fixture exit_code"
    );
    anyhow::ensure!(
        f.program_image.is_none() || options.watcom_main_argv,
        "program-image capture input requires --watcom-main-argv to refresh actual argv0"
    );
    let watcom_main = if options.watcom_main_argv {
        anyhow::ensure!(
            !image.dll && loaded.info.execution == 0,
            "Watcom main argv observation requires an EXE IO case"
        );
        let mains: Vec<_> = loaded
            .info
            .functions
            .iter()
            .filter(|f| f.definition == 4 && f.name == "main")
            .map(|f| f.rva)
            .collect();
        anyhow::ensure!(
            mains.len() == 1,
            "Watcom argv observation needs one registry App named main"
        );
        Some(mains[0])
    } else {
        None
    };
    let mut typed = None;
    let private_image =
        options.progress || options.direct_helper.is_some() || options.native_msvcrt;
    let target = if private_image {
        let mut target_bytes = if options.progress {
            image.mask(&original, &loaded.info)?
        } else {
            original.clone()
        };
        if let Some(helper_path) = &options.direct_helper {
            anyhow::ensure!(
                !image.dll && loaded.info.execution == 1,
                "typed original helper requires an EXE DIRECT case"
            );
            let marker = options.marker_rva.context("typed helper marker missing")?;
            anyhow::ensure!(
                loaded
                    .info
                    .functions
                    .iter()
                    .any(|f| f.rva == marker && f.definition == 4 && f.implementation != 0),
                "typed marker must name an implemented App entry"
            );
            let helper_path = helper_path.canonicalize()?;
            let mut helper_bytes = fs::read(&helper_path)?;
            let helper_image = crate::original_image::Image::parse(&helper_bytes)?;
            let exports = helper_image.exports(&helper_bytes)?;
            let lookup = |plain: &str, decorated: &str| -> Result<u32> {
                let values: BTreeSet<_> = [plain, decorated]
                    .iter()
                    .filter_map(|name| exports.get(*name).copied())
                    .collect();
                anyhow::ensure!(
                    values.len() == 1,
                    "typed helper lacks unique export {plain}"
                );
                Ok(*values.first().unwrap())
            };
            let callback = lookup("case_original_at_marker", "_case_original_at_marker@4")?;
            let complete = lookup(
                "recon_original_completion_trap",
                "_recon_original_completion_trap@0",
            )?;
            // The import spelling must be stable and undecorated.
            anyhow::ensure!(
                exports.get("recon_original_completion_trap") == Some(&complete),
                "typed helper requires undecorated completion export"
            );
            let state = lookup("recon_original_probe_state", "_recon_original_probe_state")?;
            let at = helper_image.file_offset(state, 52)?;
            anyhow::ensure!(
                helper_bytes[at..at + 4] == 1u32.to_le_bytes()
                    && helper_bytes[at + 4..at + 8] == 52u32.to_le_bytes()
                    && helper_bytes[at + 8..at + 52].iter().all(|b| *b == 0),
                "invalid initial typed helper state"
            );
            helper_bytes[at + 12..at + 16].copy_from_slice(&marker.to_le_bytes());
            let staged = root.join("recon-direct-probe.dll");
            fs::write(&staged, helper_bytes)?;
            let marker_at = image.file_offset(marker, 1)?;
            target_bytes[marker_at] = 0xcc;
            target_bytes = crate::pe_import::add_helper_import(
                &target_bytes,
                "recon-direct-probe.dll",
                "recon_original_completion_trap",
            )?;
            typed = Some((staged, state, complete, marker, callback));
        }
        let path = root.join(program.file_name().context("original filename")?);
        fs::write(&path, target_bytes)?;
        path
    } else {
        program.clone()
    };
    let host = options.host.as_ref().unwrap_or(&target).canonicalize()?;
    let declared = repo.join(&loaded.info.original_path).canonicalize()?;
    let staged = crate::original_stage::prepare(
        repo,
        &declared,
        &program,
        &target,
        &host,
        root,
        image.dll,
        private_image,
        options.native_msvcrt,
    )?;
    let host = staged.host;
    let self_image_sha = if options.pe_self_child {
        Some(fixture::hex(&Sha256::digest(fs::read(&host)?)))
    } else {
        None
    };
    let stdin = root.join("original-stdin");
    fs::write(&stdin, f.stdin.as_ref().map_or(&[][..], |b| b.0.as_slice()))?;
    let stdout = root.join("original-stdout");
    let stderr = root.join("original-stderr");
    let program_windows = windows(&host, repo)?;
    let args = if f.args.is_empty() {
        vec![loaded.info.binary.clone()]
    } else {
        f.args.clone()
    };
    anyhow::ensure!(
        args.iter().all(|arg| arg.is_ascii()),
        "original ANSI transport currently requires ASCII arguments"
    );
    anyhow::ensure!(
        f.env
            .iter()
            .all(|(key, value)| key.is_ascii() && value.as_ref().is_none_or(|v| v.is_ascii())),
        "original ANSI transport currently requires ASCII environment entries"
    );
    let commandline = capture_command_line(&args, f.raw_command_line.as_deref())?;
    let mut command = command(repo)?;
    if options.native_msvcrt {
        command.env("WINEDLLOVERRIDES", "msvcrt=n");
    }
    command
        .arg(helper.canonicalize()?)
        .arg(&commandline)
        .arg(image.entry.to_string())
        .arg(windows(cwd, repo)?)
        .arg(if f.stdin_kind == fixture::StdinKind::Null {
            "NUL".to_owned()
        } else {
            windows(&stdin, repo)?
        })
        .arg(windows(&stdout, repo)?)
        .arg(windows(&stderr, repo)?)
        .arg(program_windows)
        .arg(f.timeout_ms.to_string())
        .arg(if image.dll {
            windows(&target, repo)?
        } else {
            "-".into()
        });
    if let Some((helper, state, complete, marker, callback)) = &typed {
        command
            .arg(windows(helper, repo)?)
            .arg(state.to_string())
            .arg(complete.to_string())
            .arg(marker.to_string())
            .arg(format!("callback={callback}"));
    }
    command
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::from(File::create(root.join("wine-diagnostics"))?))
        .env(
            "WINEPATH",
            windows(program.parent().context("original directory")?, repo)?,
        )
        .current_dir(cwd);
    let program_image_input = if options.watcom_main_argv && private_image {
        Some(crate::program_image::ProgramImage::capture(
            &program,
            &original,
            &fs::read(&target)?,
            windows(&target, repo)?,
        )?)
    } else {
        None
    };
    let mut target_env = f.env.clone();
    if options.native_msvcrt {
        anyhow::ensure!(
            !target_env
                .keys()
                .any(|name| name.eq_ignore_ascii_case("WINEDLLOVERRIDES")),
            "--native-msvcrt conflicts with fixture WINEDLLOVERRIDES"
        );
    }
    if image.dll
        && !target_env
            .keys()
            .any(|key| key.eq_ignore_ascii_case("RECON_ORIGINAL_DLL"))
    {
        target_env.insert("RECON_ORIGINAL_DLL".into(), Some(windows(&target, repo)?));
    }
    let environment = root.join("original-environment");
    fs::write(&environment, target_environment(&target_env)?)?;
    let process_environment_receipt = root.join("process-os-environment");
    let process_provider_sha256 = if options.process_os_environment {
        anyhow::ensure!(!image.dll, "process OS environment capture requires an EXE");
        let provider = staged
            .provider
            .as_ref()
            .context("process OS environment requires staged native MSVCRT")?;
        let sha = fixture::hex(&Sha256::digest(fs::read(provider)?));
        if let Some(declared) = &f.process_os_environment {
            anyhow::ensure!(
                declared.provider_sha256 == sha,
                "declared process_os_environment provider differs from actual staged MSVCRT"
            );
            fs::write(
                root.join("process-os-environment.expected"),
                &declared.ansi.0,
            )?;
        }
        Some(sha)
    } else {
        None
    };
    command.arg(windows(&environment, repo)?);
    if let Some(provider) = &staged.provider {
        command.arg(windows(provider, repo)?);
    }
    anyhow::ensure!(
        !options.pe_self_child || !image.dll,
        "PE SELF capture requires an EXE"
    );
    if options.pe_self_child {
        command.arg("pe-self-child=1");
    }
    if options.process_os_environment {
        command.arg(format!(
            "process-os-environment={}",
            windows(&process_environment_receipt, repo)?
        ));
    }
    let mut child = command.spawn().context("launch explicit Wine debugger")?;
    let input = child.stdin.take().unwrap();
    let output = child.stdout.take().unwrap();
    let mut broker = Broker {
        child,
        input: Some(input),
        output,
        order: vec![],
        seen: loaded.info.first_entry_seen.iter().copied().collect(),
        app: loaded
            .info
            .functions
            .iter()
            .filter(|f| f.definition == 4)
            .map(|f| f.rva)
            .collect(),
        emitted: vec![],
        error: None,
        pe_self: options.pe_self_child.then(crate::pe_self::Tracker::default),
        wait_cleanup: options.pe_self_child || options.process_os_environment,
    };
    let hello = broker.reply()?;
    // Startup's a/b are base/entry, not kind/status; the owner is still validated.
    let hello = if options.pe_self_child {
        let (owner, data) = crate::pe_self::Owner::decode(&hello.data, size_of::<Registers>())?;
        anyhow::ensure!(
            owner.base == hello.a && !owner.child && owner.entry,
            "PE SELF initial entry owner differs"
        );
        broker.pe_self.as_mut().unwrap().observe(owner, 1, 0)?;
        Reply {
            a: hello.a,
            b: hello.b,
            data,
        }
    } else {
        hello
    };
    let base = hello.a;
    anyhow::ensure!(hello.b == image.entry, "Wine debugger entry mismatch");
    let terminal;
    let mut snapshot = None;
    let mut direct_noreturn = None;
    let mut watcom_main_argv = None;
    if let Some((_, _, _, marker, _)) = typed {
        broker.record(image.entry);
        broker.arm()?;
        let mut invoked = false;
        loop {
            let r = broker.rpc(7, 0, 0, 0, &[])?;
            match r.a {
                0 => {
                    let expected = f.exit_code.unwrap_or(0) as u32;
                    direct_noreturn = Some(direct_exit(
                        options.direct_noreturn,
                        invoked,
                        expected,
                        r.b,
                        &r.data,
                    )?);
                    terminal = format!("exit:{}", r.b);
                    break;
                }
                1 => {
                    broker.record(r.b);
                    if !invoked && r.b == marker {
                        invoked = true;
                    }
                }
                2 => {
                    anyhow::ensure!(
                        options.progress
                            && loaded.info.functions.iter().any(|f| f.rva == r.b
                                && f.definition == 4
                                && f.implementation == 0),
                        "unexpected typed original INT3 at RVA {:x} (PC {:08x}, target base {:08x})",
                        r.b,
                        base.wrapping_add(r.b),
                        base
                    );
                    broker.record(r.b);
                    snapshot = Some(frontier_snapshot(&mut broker, base, r.b, &r.data)?);
                    terminal = format!("frontier:{:x}", r.b);
                    break;
                }
                3 => {
                    return Err(exception_failure(
                        "typed original exception",
                        r.b,
                        base,
                        &r.data,
                        broker.pe_self.as_ref().and_then(|v| v.current.as_ref()),
                    )?
                    .into());
                }
                4 => {
                    anyhow::ensure!(
                        !options.direct_noreturn && invoked && r.b == 1,
                        "typed original callback assertions failed"
                    );
                    broker.emitted.extend(typed_results(&r.data)?);
                    terminal = "direct:pass".to_owned();
                    break;
                }
                _ => bail!("invalid typed original response"),
            }
        }
    } else if loaded.info.execution == 0 {
        broker.record(image.entry);
        broker.arm()?;
        if let Some(main) = watcom_main {
            if main == image.entry {
                watcom_main_argv = Some(read_watcom_main_argv(
                    main,
                    base + main,
                    &hello.data,
                    &args,
                    |address, size| Ok(broker.rpc(1, address, size, 0, &[])?.data),
                )?);
            } else {
                // Observe input even if this entry is already in the seeded trace prefix.
                broker.rpc(6, 0, 0, 0, &main.to_le_bytes())?;
            }
        }
        loop {
            let r = broker.rpc(7, 0, 0, 0, &[])?;
            let registers = matches!(r.a, 1 | 2 | 3);
            let r = broker.event(r, registers)?;
            let base = broker.event_base(base);
            if r.a == 5 {
                continue;
            }
            if matches!(r.a, 1 | 2) && watcom_main == Some(r.b) && watcom_main_argv.is_none() {
                watcom_main_argv = Some(read_watcom_main_argv(
                    r.b,
                    base + r.b,
                    &r.data,
                    &args,
                    |address, size| Ok(broker.rpc(1, address, size, 0, &[])?.data),
                )?);
            }
            match r.a {
                0 => {
                    terminal = format!("exit:{}", r.b);
                    break;
                }
                1 => broker.record(r.b),
                2 => {
                    let frontier = loaded
                        .info
                        .functions
                        .iter()
                        .find(|f| f.rva == r.b && f.definition == 4 && f.implementation == 0);
                    anyhow::ensure!(
                        options.progress && frontier.is_some(),
                        "unexpected original INT3 at RVA {:x} (PC {:08x}, target base {:08x})",
                        r.b,
                        base.wrapping_add(r.b),
                        base
                    );
                    broker.record(r.b);
                    snapshot = Some(frontier_snapshot(&mut broker, base, r.b, &r.data)?);
                    terminal = format!("frontier:{:x}", r.b);
                    break;
                }
                3 => {
                    return Err(exception_failure(
                        "original exception",
                        r.b,
                        base,
                        &r.data,
                        broker.pe_self.as_ref().and_then(|v| v.current.as_ref()),
                    )?
                    .into());
                }
                _ => bail!("invalid original execution response"),
            }
        }
    } else {
        type Direct = unsafe extern "C" fn(*mut ContextOriginal) -> i32;
        let direct = unsafe { loaded.library.get::<Direct>(b"case_original_direct\0") }
            .context("DIRECT capture needs case_original_direct(ReconOriginal*) from direct.c")?;
        let mut ctx = ContextOriginal {
            version: 1,
            size: size_of::<ContextOriginal>() as u32,
            base,
            opaque: &mut broker as *mut _ as *mut _,
            read: remote_read,
            write: remote_write,
            allocate: remote_allocate,
            release: remote_release,
            until: remote_until,
            call: remote_call,
            emit: remote_emit,
        };
        let pass = unsafe { direct(&mut ctx) };
        anyhow::ensure!(
            pass != 0,
            "original direct assertions failed: {}",
            broker.error.as_deref().unwrap_or("callback returned false")
        );
        anyhow::ensure!(
            broker.error.is_none(),
            "original direct operation failed: {:?}",
            broker.error
        );
        terminal = "direct:pass".to_owned();
    }
    broker.close()?;
    let pe_self_child = if let Some(track) = &broker.pe_self {
        let diagnostics = fs::read_to_string(root.join("wine-diagnostics"))?;
        anyhow::ensure!(
            self_image_sha.as_ref() == Some(&fixture::hex(&Sha256::digest(fs::read(&host)?))),
            "PE SELF staged program identity changed"
        );
        Some(track.finish(&diagnostics, terminal.starts_with("frontier:"))?)
    } else {
        None
    };
    let mut fixture = f.clone();
    fixture.raw_command_line = Some(commandline);
    if let Some(provider_sha256) = process_provider_sha256 {
        let actual = fixture::ProcessOsEnvironment {
            ansi: Bytes(
                fs::read(&process_environment_receipt)
                    .context("actual process OS environment receipt missing")?,
            ),
            provider_sha256,
        };
        actual.validate()?;
        anyhow::ensure!(
            f.process_os_environment
                .as_ref()
                .is_none_or(|v| *v == actual),
            "declared process_os_environment differs from actual launch input"
        );
        fixture.process_os_environment = Some(actual);
    }
    fixture.args = if options.watcom_main_argv {
        watcom_main_argv
            .as_ref()
            .context("requested Watcom main argument entry was not reached")?
            .actual
            .clone()
    } else {
        args
    };
    if options.watcom_main_argv {
        fixture.program_image = None;
    }
    if let Some(image) = program_image_input {
        anyhow::ensure!(
            fixture::hex(&Sha256::digest(fs::read(&target)?)) == image.sha256,
            "original staged program input changed during capture"
        );
        anyhow::ensure!(
            fixture.args.first() == Some(&image.logical_path),
            "observed Watcom argv0 differs from actual staged program path"
        );
        if let Some(existing) = fixture
            .readonly_dependencies
            .iter()
            .find(|d| d.path == image.source_path)
        {
            anyhow::ensure!(
                existing.sha256 == image.source_sha256,
                "program source dependency conflicts"
            );
        } else {
            fixture.readonly_dependencies.push(fixture::Dependency {
                path: image.source_path.clone(),
                sha256: image.source_sha256.clone(),
            });
        }
        fixture.program_image = Some(image);
    }
    let stdout = Bytes(fs::read(stdout)?);
    let stderr = Bytes(fs::read(stderr)?);
    let partial = options.progress && terminal.starts_with("frontier:");
    let rules = if partial {
        f.partial
            .as_ref()
            .map_or(&[][..], |p| p.output_files.as_slice())
    } else {
        f.output_files.as_slice()
    };
    let raw_artifacts = match artifacts_dir {
        Some(directory) => export_artifacts(f, cwd, rules, directory)?,
        None => vec![],
    };
    let (files, reject_unexpected_files) =
        capture_output_files(f, cwd, partial, options.partial_output_files_only)?;
    if partial {
        fixture.partial = Some(fixture::Partial {
            stdout: Some(stdout),
            stderr: Some(stderr),
            output_files: files,
            reject_unexpected_files,
        });
    } else {
        fixture.partial = None;
        fixture.stdout = Some(stdout);
        fixture.stderr = Some(stderr);
        fixture.output_files = files;
        fixture.exit_code = Some(if terminal == "direct:pass" {
            0
        } else {
            terminal
                .strip_prefix("exit:")
                .context("expected original exit")?
                .parse::<u32>()? as i32
        });
    }
    for file in std::mem::take(&mut broker.emitted) {
        anyhow::ensure!(
            !fixture.input_files.iter().any(|f| f.path == file.path),
            "emitted fixture collides with input {}",
            file.path
        );
        fixture.input_files.push(file);
    }
    native::dependencies(f)?;
    let report = CaptureReport {
        identity: Some(crate::original_reference::Identity::from_info(&loaded.info)),
        capture_sha256: None,
        oracle_written: true,
        backend: None,
        direct_noreturn,
        watcom_main_argv,
        pe_self_child,
        case: loaded.info.case,
        terminal,
        first_entry_order: std::mem::take(&mut broker.order),
        elapsed_ms: start.elapsed().as_secs_f64() * 1000.,
        workspace: root.display().to_string(),
        staged_files: staged.files,
        frontier: snapshot,
        artifacts_dir: artifacts_dir.map(Path::to_owned),
        raw_artifacts,
    };
    Ok((Some(fixture), report))
}

fn direct_exit(
    enabled: bool,
    marker_seen: bool,
    expected: u32,
    actual: u32,
    bytes: &[u8],
) -> Result<crate::original_reference::DirectExit> {
    anyhow::ensure!(
        enabled,
        "original exited before typed helper completion: {actual}"
    );
    anyhow::ensure!(
        marker_seen && bytes.len() == 16,
        "nonreturn DIRECT lacks actual callback-entry proof"
    );
    let words: Vec<u32> = bytes
        .chunks_exact(4)
        .map(|w| u32::from_le_bytes(w.try_into().unwrap()))
        .collect();
    anyhow::ensure!(
        words[0] == 1 && words[1..].iter().all(|w| *w != 0),
        "nonreturn DIRECT lacks actual callback-entry proof"
    );
    anyhow::ensure!(
        actual == expected,
        "nonreturn DIRECT exit differs: actual {actual}, expected {expected}"
    );
    Ok(crate::original_reference::DirectExit {
        callback_observed: true,
        callback_pc: words[1],
        pid: words[2],
        tid: words[3],
        expected_exit: expected,
        actual_exit: actual,
    })
}

fn canonical_capture_cwd(
    f: &Fixture,
    root: &Path,
    repo: &Path,
    case_name: &str,
) -> Result<PathBuf> {
    if let Some(cwd) = &f.logical_cwd {
        let b = cwd.as_bytes();
        anyhow::ensure!(
            b.len() >= 3 && b[1] == b':' && matches!(b[2], b'\\' | b'/'),
            "logical_cwd must be drive-absolute"
        );
        let suffix = cwd[3..].replace('\\', "/");
        fixture::safe_relative(&suffix)?;
        let base = match b[0].to_ascii_uppercase() {
            b'T' => PathBuf::from("/tmp/recon"),
            b'S' => {
                PathBuf::from(std::env::var_os("HOME").context("HOME unset")?).join(".ai-tmp/msvc2")
            }
            _ => bail!("original capture cwd must be on shared S: or T:"),
        };
        Ok(base.join(suffix))
    } else {
        // Relative-input cases need no stable observable absolute path.
        let _ = (repo, case_name);
        Ok(root.join("work"))
    }
}
fn supervise_capture(options: &Capture, repo: &Path, f: &Fixture) -> Result<CaptureReport> {
    native::signals();
    anyhow::ensure!(
        f.input_files
            .iter()
            .map(|v| &v.path)
            .chain(f.output_files.iter().map(|v| &v.path))
            .all(|p| p.is_ascii()),
        "original ANSI transport requires ASCII fixture paths"
    );
    anyhow::ensure!(
        f.timeout_ms <= 86_400_000,
        "original capture timeout must be at most one day"
    );
    anyhow::ensure!(
        f.stdin_kind != fixture::StdinKind::Pipe,
        "original pipe stdin capture is not implemented; captured stdin transport is never silently changed"
    );
    anyhow::ensure!(
        f.events.is_empty(),
        "original timed/console events are not implemented; native replay may use saved events"
    );
    let root = native::create_workspace()?;
    let mut owned_cwd = None;
    let result = (|| -> Result<CaptureReport> {
        if unsafe { libc::prctl(libc::PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) } < 0 {
            bail!("capture subreaper")
        }
        // Fixed Wine mappings cannot vary between cases: serialize explicit captures.
        let lease = OpenOptions::new()
            .create(true)
            .truncate(false)
            .read(true)
            .write(true)
            .open("/tmp/recon/.capture.lock")?;
        if unsafe { libc::flock(lease.as_raw_fd(), libc::LOCK_EX) } < 0 {
            bail!("lock original capture")
        }
        let cwd = canonical_capture_cwd(f, &root, repo, "")?;
        if let Some(parent) = cwd.parent() {
            fs::create_dir_all(parent)?
        }
        fs::create_dir(&cwd).with_context(|| {
            format!(
                "reserve original cwd {}; existing directories are never overwritten",
                cwd.display()
            )
        })?;
        owned_cwd = Some(cwd.clone());
        anyhow::ensure!(
            !options.output.starts_with(&root) && !options.output.starts_with(&cwd),
            "capture oracle must be saved outside ephemeral workspace"
        );
        let artifacts_dir = options
            .artifacts_dir
            .as_deref()
            .map(|path| reserve_artifacts(path, &root, &cwd))
            .transpose()?;
        native::materialize(f, &cwd)?;
        native::dependencies(f)?;
        let pid = native::fork()?;
        if pid == 0 {
            let result = capture_executor(options, repo, f, &root, &cwd, artifacts_dir.as_deref());
            let status = match result {
                Ok((fixture, mut report)) => {
                    let write = (|| -> Result<()> {
                        if !options.replace && options.output.exists() {
                            bail!("capture output already exists")
                        }
                        if let Some(fixture) = fixture {
                            fixture::write(&options.output, &fixture)?;
                            report.capture_sha256 =
                                Some(fixture::hex(&Sha256::digest(fs::read(&options.output)?)));
                        }
                        fs::write(root.join("capture-report"), serde_json::to_vec(&report)?)?;
                        Ok(())
                    })();
                    match write {
                        Ok(()) => 0,
                        Err(e) => {
                            let _ =
                                CaptureFailure::from_error(&e).save(&root.join("capture-error"));
                            1
                        }
                    }
                }
                Err(e) => {
                    let _ = CaptureFailure::from_error(&e).save(&root.join("capture-error"));
                    1
                }
            };
            unsafe { libc::_exit(status) }
        }
        let start = Instant::now();
        let mut exit = None;
        let mut interrupted = false;
        loop {
            if start.elapsed().as_millis() > f.timeout_ms as u128 + 5000
                || native::SIGNAL.load(Ordering::Relaxed) != 0
            {
                interrupted = true;
                native::kill_descendants();
            }
            let mut status = 0;
            let n = unsafe { libc::waitpid(-1, &mut status, libc::WNOHANG) };
            if n > 0 {
                if n == pid {
                    exit = Some(status)
                }
                continue;
            }
            if n < 0 {
                let e = std::io::Error::last_os_error();
                if e.raw_os_error() == Some(libc::ECHILD) {
                    break;
                }
                if e.raw_os_error() != Some(libc::EINTR) {
                    return Err(e.into());
                }
            }
            std::thread::sleep(Duration::from_millis(2));
        }
        anyhow::ensure!(!interrupted, "original capture interrupted or timed out");
        let status = exit.context("original capture executor not reaped")?;
        if !libc::WIFEXITED(status) || libc::WEXITSTATUS(status) != 0 {
            let mut failure = CaptureFailure::read(
                fs::read_to_string(root.join("capture-error"))
                    .unwrap_or_else(|_| format!("capture process failed status {status}")),
            );
            let diagnostics = fs::read_to_string(root.join("wine-diagnostics")).unwrap_or_default();
            failure.error.push('\n');
            failure.error.extend(diagnostics.chars().take(8192));
            return Err(failure.into());
        }
        Ok(serde_json::from_slice(&fs::read(
            root.join("capture-report"),
        )?)?)
    })();
    if result.is_err() {
        native::kill_descendants();
        loop {
            let mut status = 0;
            let n = unsafe { libc::waitpid(-1, &mut status, 0) };
            if n < 0 && std::io::Error::last_os_error().raw_os_error() != Some(libc::EINTR) {
                break;
            }
        }
    }
    if let Some(cwd) = owned_cwd {
        if !cwd.starts_with(&root) {
            fs::remove_dir_all(&cwd)?;
        }
    }
    let cleanup = fs::remove_dir_all(&root);
    let report = result?;
    cleanup?;
    Ok(report)
}
pub fn capture(mut options: Capture) -> Result<()> {
    anyhow::ensure!(
        !options.entry_stop || options.artifacts_dir.is_none(),
        "CF entry-stop does not export artifacts or write a fixture oracle"
    );
    native::signals();
    let repo = crate::toolchain::repository(options.repo.as_deref())?;
    options.case = if options.case.is_dir() {
        options.case.join(".build/case.so")
    } else {
        options.case
    };
    options.case = options.case.canonicalize()?;
    let inferred = options
        .case
        .parent()
        .and_then(|p| p.parent())
        .map(|p| p.join("capture.bin"));
    let fixture_path = options
        .io
        .as_deref()
        .or_else(|| inferred.as_deref().filter(|p| p.exists()));
    let fixture = match fixture_path {
        Some(path) => fixture::read(path)?,
        None => Fixture::default(),
    };
    validate_partial_selection(
        options.progress,
        options.partial_output_files_only,
        &fixture,
    )?;
    let parent = options
        .output
        .parent()
        .filter(|p| !p.as_os_str().is_empty())
        .unwrap_or(Path::new("."));
    options.output = parent.canonicalize()?.join(
        options
            .output
            .file_name()
            .context("capture output filename")?,
    );
    anyhow::ensure!(
        options.replace || !options.output.exists(),
        "capture output exists; use --replace explicitly"
    );
    let mut pipe = [0; 2];
    if unsafe { libc::pipe2(pipe.as_mut_ptr(), libc::O_CLOEXEC) } < 0 {
        bail!("capture report pipe")
    }
    let reader = unsafe { File::from_raw_fd(pipe[0]) };
    let mut writer = unsafe { File::from_raw_fd(pipe[1]) };
    let pid = native::fork()?;
    if pid == 0 {
        drop(reader);
        let result = supervise_capture(&options, &repo, &fixture);
        let value = match result {
            Ok(report) => serde_json::to_value(report)?,
            Err(e) => serde_json::to_value(CaptureFailure::from_error(&e))?,
        };
        let _ = writer.write_all(&serde_json::to_vec(&value)?);
        unsafe { libc::_exit(0) }
    }
    drop(writer);
    let data = native::receive_child(pid, reader)?;
    let value: serde_json::Value = serde_json::from_slice(&data)?;
    if let Some(path) = &options.report {
        fs::write(path, serde_json::to_vec_pretty(&value)?)?;
    }
    if let Some(error) = value.get("error") {
        bail!("{error}")
    }
    if value["oracle_written"] == false {
        println!(
            "TRANSPORT {} {} ({:.3} ms); no fixture oracle written",
            value["case"].as_str().unwrap_or("?"),
            value["terminal"].as_str().unwrap_or("?"),
            value["elapsed_ms"].as_f64().unwrap_or(0.)
        );
        return Ok(());
    }
    println!(
        "CAPTURE {} {}: {} first entries ({:.3} ms)",
        value["case"].as_str().unwrap_or("?"),
        value["terminal"].as_str().unwrap_or("?"),
        value["first_entry_order"].as_array().map_or(0, Vec::len),
        value["elapsed_ms"].as_f64().unwrap_or(0.)
    );
    Ok(())
}

#[cfg(test)]
mod target_environment_tests {
    use super::target_environment;
    use std::collections::BTreeMap;
    #[test]
    fn target_overlay_distinguishes_empty_removal_and_rejects_ambiguous_inputs() {
        let env = BTreeMap::from([
            ("PATH".into(), Some(String::new())),
            ("WINEPREFIX".into(), None),
            ("VALUE".into(), Some("a=b".into())),
        ]);
        assert_eq!(
            target_environment(&env).unwrap(),
            b"PATH=\0VALUE=a=b\0WINEPREFIX\0\0"
        );
        assert_eq!(target_environment(&BTreeMap::new()).unwrap(), [0, 0]);
        let mut duplicate = env.clone();
        duplicate.insert("path".into(), None);
        assert!(target_environment(&duplicate).is_err());
        for (name, value) in [
            ("", "x"),
            ("=C:", "x"),
            ("A=B", "x"),
            ("A\0B", "x"),
            ("A", "x\0y"),
            ("A", "é"),
        ] {
            assert!(
                target_environment(&BTreeMap::from([(name.into(), Some(value.into()))])).is_err()
            );
        }
        assert!(
            target_environment(&BTreeMap::from([("A".into(), Some("x".repeat(32767)))])).is_err()
        );
    }
}

#[cfg(test)]
mod partial_output_tests {
    use super::*;
    #[test]
    fn selection_requires_progress_and_an_explicit_partial_policy() {
        use clap::Parser;
        assert!(
            crate::Cli::try_parse_from([
                "recon-tool",
                "original",
                "capture",
                "case",
                "--output",
                "capture.bin",
                "--partial-output-files-only",
            ])
            .is_err()
        );
        let mut f = Fixture::default();
        assert!(validate_partial_selection(false, true, &f).is_err());
        assert!(validate_partial_selection(true, true, &f).is_err());
        assert!(validate_partial_selection(false, false, &f).is_ok());
        f.partial = Some(fixture::Partial::default());
        assert!(validate_partial_selection(true, true, &f).is_ok());
    }

    #[test]
    fn selection_records_declared_absence_but_default_and_exit_keep_full_inventory() -> Result<()> {
        let root = native::create_workspace()?;
        let result = (|| -> Result<()> {
            let cwd = root.join("work");
            fs::create_dir(&cwd)?;
            fs::write(cwd.join("selected.bin"), [1u8, 2])?;
            fs::write(cwd.join("internal.tmp"), b"temporary data")?;
            let rule = |path: &str, zero_ranges| OutputFile {
                path: path.into(),
                absent: false,
                data: None,
                sha256: None,
                zero_ranges,
            };
            let mut f = Fixture::default();
            f.reject_unexpected_files = true;
            f.output_files = vec![rule("selected.bin", vec![[1, 2]])];
            f.partial = Some(fixture::Partial {
                output_files: vec![
                    rule("selected.bin", vec![[0, 1]]),
                    rule("missing.obj", vec![]),
                ],
                reject_unexpected_files: false,
                ..Default::default()
            });
            let (selected, reject) = capture_output_files(&f, &cwd, true, true)?;
            assert!(!reject);
            assert_eq!(selected.len(), 2);
            assert!(
                selected
                    .iter()
                    .find(|v| v.path == "missing.obj")
                    .unwrap()
                    .absent
            );
            assert_eq!(
                selected
                    .iter()
                    .find(|v| v.path == "selected.bin")
                    .unwrap()
                    .sha256,
                Some(fixture::hex(&Sha256::digest([0u8, 2])))
            );
            let (default, reject) = capture_output_files(&f, &cwd, true, false)?;
            assert!(reject);
            assert_eq!(default.len(), 3);
            assert!(default.iter().any(|v| v.path == "internal.tmp"));
            let (normal_exit, reject) = capture_output_files(&f, &cwd, false, true)?;
            assert!(reject);
            assert_eq!(normal_exit.len(), 2);
            assert!(normal_exit.iter().any(|v| v.path == "internal.tmp"));
            assert_eq!(
                normal_exit
                    .iter()
                    .find(|v| v.path == "selected.bin")
                    .unwrap()
                    .sha256,
                Some(fixture::hex(&Sha256::digest([1u8, 0])))
            );
            Ok(())
        })();
        fs::remove_dir_all(root)?;
        result
    }

    #[test]
    fn empty_partial_does_not_inherit_final_coff_normalization() {
        let root = crate::native::create_workspace().unwrap();
        let result = (|| -> Result<()> {
            fs::write(root.join("out.obj"), [])?;
            let mut f = Fixture::default();
            let final_rule = OutputFile {
                path: "out.obj".into(),
                absent: false,
                data: None,
                sha256: None,
                zero_ranges: vec![[4, 8]],
            };
            f.output_files.push(final_rule);
            let partial = OutputFile {
                path: "out.obj".into(),
                absent: false,
                data: Some(Bytes(vec![])),
                sha256: None,
                zero_ranges: vec![],
            };
            let observed = capture_files(&f, &root, &[partial])?;
            assert!(observed[0].zero_ranges.is_empty());
            assert!(!observed[0].absent);
            assert!(capture_files(&f, &root, &f.output_files).is_err());
            fs::write(root.join("out.obj"), [1u8; 12])?;
            let observed = capture_files(&f, &root, &f.output_files)?;
            assert_eq!(
                observed
                    .iter()
                    .find(|v| v.path == "out.obj")
                    .unwrap()
                    .zero_ranges,
                vec![[4, 8]]
            );
            fs::remove_file(root.join("out.obj"))?;
            assert!(
                capture_files(&f, &root, &f.output_files)?
                    .iter()
                    .find(|v| v.path == "out.obj")
                    .unwrap()
                    .absent
            );
            Ok(())
        })();
        fs::remove_dir_all(root).unwrap();
        result.unwrap();
    }
}

#[cfg(test)]
mod raw_artifact_tests {
    use super::*;
    #[test]
    fn exported_bytes_are_raw_while_fixture_hashes_remain_normalized() -> Result<()> {
        let root = native::create_workspace()?;
        let result = (|| -> Result<()> {
            let capture = root.join("capture");
            let cwd = capture.join("work");
            fs::create_dir_all(cwd.join("nested"))?;
            let raw = vec![1, 2, 3, 4, 5, 6, 7, 8];
            fs::write(cwd.join("nested/out.obj"), &raw)?;
            fs::write(cwd.join("manifest.json"), b"application output")?;
            fs::write(cwd.join("keep.in"), b"unchanged input")?;
            let mut fixture = Fixture::default();
            fixture.input_files.push(InputFile {
                path: "keep.in".into(),
                data: Bytes(b"unchanged input".to_vec()),
                mode: None,
                modified_unix_seconds: None,
            });
            let rules = vec![
                OutputFile {
                    path: "nested/out.obj".into(),
                    absent: false,
                    data: None,
                    sha256: None,
                    zero_ranges: vec![[4, 8]],
                },
                OutputFile {
                    path: "missing.obj".into(),
                    absent: true,
                    data: None,
                    sha256: None,
                    zero_ranges: vec![],
                },
            ];
            let destination = reserve_artifacts(&root.join("raw"), &capture, &cwd)?;
            let exported = export_artifacts(&fixture, &cwd, &rules, &destination)?;
            assert_eq!(exported.len(), 2);
            fixture.partial = Some(fixture::Partial {
                output_files: vec![rules[0].clone()],
                ..Default::default()
            });
            let (selected, _) = capture_output_files(&fixture, &cwd, true, true)?;
            assert_eq!(selected.len(), 1);
            assert_eq!(selected[0].path, "nested/out.obj");
            assert!(exported.iter().any(|v| v.path == "manifest.json"));
            let object = exported
                .iter()
                .find(|v| v.path == "nested/out.obj")
                .unwrap();
            assert_eq!(fs::read(&object.exported_path)?, raw);
            assert_eq!(object.sha256, fixture::hex(&Sha256::digest(&raw)));
            assert_eq!(object.size, 8);
            let observed = capture_files(&fixture, &cwd, &rules)?;
            let mut normalized = raw.clone();
            normalized[4..8].fill(0);
            assert_eq!(
                observed
                    .iter()
                    .find(|v| v.path == "nested/out.obj")
                    .unwrap()
                    .sha256,
                Some(fixture::hex(&Sha256::digest(&normalized)))
            );
            assert!(observed.iter().all(|v| v.data.is_none()));
            let manifest: Vec<RawArtifact> =
                serde_json::from_slice(&fs::read(destination.join("manifest.json"))?)?;
            assert_eq!(manifest.len(), 2);
            assert!(!destination.join("files/keep.in").exists());
            assert!(!destination.join("files/missing.obj").exists());
            assert_eq!(
                fs::read(destination.join("files/manifest.json"))?,
                b"application output"
            );
            assert!(reserve_artifacts(&destination, &capture, &cwd).is_err());
            assert!(export_artifacts(&fixture, &cwd, &rules, &destination).is_err());
            assert_eq!(fs::read(&object.exported_path)?, raw);
            let mut invalid = rules.clone();
            invalid[0].zero_ranges = vec![[4, 99]];
            assert!(capture_files(&fixture, &cwd, &invalid).is_err());
            assert_eq!(fs::read(&object.exported_path)?, raw);
            Ok(())
        })();
        fs::remove_dir_all(root)?;
        result
    }
    #[test]
    fn export_reservation_and_file_parents_cannot_escape_owned_directory() -> Result<()> {
        let root = native::create_workspace()?;
        let result = (|| -> Result<()> {
            let capture = root.join("capture");
            let cwd = capture.join("work");
            fs::create_dir_all(cwd.join("nested"))?;
            assert!(reserve_artifacts(&capture.join("inside"), &capture, &cwd).is_err());
            assert!(reserve_artifacts(&cwd.join("inside"), &capture, &cwd).is_err());
            std::os::unix::fs::symlink(&capture, root.join("alias"))?;
            assert!(reserve_artifacts(&root.join("alias/inside"), &capture, &cwd).is_err());
            let destination = reserve_artifacts(&root.join("raw"), &capture, &cwd)?;
            let outside = root.join("outside");
            fs::create_dir(&outside)?;
            std::os::unix::fs::symlink(&outside, destination.join("files/nested"))?;
            fs::create_dir(cwd.join("nested/deeper"))?;
            fs::write(cwd.join("nested/deeper/file"), b"data")?;
            assert!(export_artifacts(&Fixture::default(), &cwd, &[], &destination).is_err());
            assert!(!outside.join("deeper").exists());
            assert!(!outside.join("deeper/file").exists());
            Ok(())
        })();
        fs::remove_dir_all(root)?;
        result
    }
}

#[cfg(test)]
mod pe_self_option_tests {
    use clap::Parser;
    #[test]
    fn self_child_requires_progress_and_rejects_incompatible_transport() {
        let base = [
            "recon-tool",
            "original",
            "capture",
            "case",
            "--output",
            "capture.bin",
        ];
        let mut args = base.to_vec();
        args.push("--pe-self-child");
        assert!(crate::Cli::try_parse_from(&args).is_err());
        args.push("--progress");
        assert!(crate::Cli::try_parse_from(&args).is_ok());
        for extra in [
            vec!["--cf-profile", "profile.json"],
            vec!["--watcom-main-argv"],
            vec!["--direct-helper", "probe.dll", "--marker-rva", "4096"],
        ] {
            let mut bad = args.clone();
            bad.extend(extra);
            assert!(crate::Cli::try_parse_from(&bad).is_err());
        }
    }
}

#[cfg(test)]
mod process_os_environment_options {
    use clap::Parser;
    #[test]
    fn process_os_environment_requires_supported_capture_mode() {
        let command = [
            "recon-tool",
            "original",
            "capture",
            "case.so",
            "--output",
            "out.bin",
            "--process-os-environment",
        ];
        assert!(crate::Cli::try_parse_from(command).is_err());
        let mut valid = command.to_vec();
        valid.push("--native-msvcrt");
        assert!(crate::Cli::try_parse_from(&valid).is_ok());
        for extra in [
            vec!["--cf-profile", "cf.json"],
            vec!["--direct-helper", "x.dll"],
            vec!["--watcom-main-argv"],
        ] {
            let mut invalid = valid.clone();
            invalid.extend(extra);
            assert!(crate::Cli::try_parse_from(invalid).is_err());
        }
    }
}
