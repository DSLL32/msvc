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
    collections::BTreeSet,
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
    pub case: PathBuf,
    #[arg(long, default_value = ".")]
    pub repo: PathBuf,
    #[arg(long = "capture")]
    pub io: Option<PathBuf>,
    #[arg(long)]
    pub program: Option<PathBuf>,
    #[arg(long)]
    pub helper: Option<PathBuf>,
    /// Win32 invocation host for a DLL; it loads RECON_ORIGINAL_DLL.
    #[arg(long)]
    pub host: Option<PathBuf>,
    /// Typed Win32 probe DLL compiled from case-owned direct.c; explicit original only.
    #[arg(long, requires = "marker_rva")]
    pub direct_helper: Option<PathBuf>,
    /// Genuine App entry at which the typed original callback takes over.
    #[arg(long, requires = "direct_helper")]
    pub marker_rva: Option<u32>,
    #[arg(short, long)]
    pub output: PathBuf,
    #[arg(long)]
    pub report: Option<PathBuf>,
    /// Mask original code, restoring classified boundaries and implemented App extents.
    #[arg(long)]
    pub progress: bool,
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
    input: ChildStdin,
    output: ChildStdout,
    order: Vec<u32>,
    seen: BTreeSet<u32>,
    app: Vec<u32>,
    emitted: Vec<InputFile>,
    error: Option<String>,
}
impl Broker {
    fn reply(&mut self) -> Result<Reply> {
        let mut h = [0; 16];
        self.output
            .read_exact(&mut h)
            .context("read Wine debugger response")?;
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
            self.input.write_all(&v.to_le_bytes())?;
        }
        self.input.write_all(data)?;
        self.input.flush()?;
        self.reply()
            .with_context(|| format!("Wine debugger operation {op}"))
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

fn quote(s: &str) -> String {
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
#[derive(Serialize, Deserialize)]
struct CaptureReport {
    case: String,
    terminal: String,
    first_entry_order: Vec<u32>,
    elapsed_ms: f64,
    workspace: String,
}
fn capture_files(f: &Fixture, cwd: &Path) -> Result<Vec<OutputFile>> {
    let mut paths = BTreeSet::new();
    native::output_paths(cwd, Path::new(""), &mut paths)?;
    let mut result = vec![];
    for path in paths {
        let bytes = fs::read(cwd.join(&path))?;
        let explicit = f.output_files.iter().find(|v| v.path == path);
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
        .chain(f.output_files.iter().map(|v| &v.path))
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
    f: &Fixture,
    root: &Path,
    cwd: &Path,
) -> Result<(Fixture, CaptureReport)> {
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
    let program = options
        .program
        .clone()
        .unwrap_or_else(|| options.repo.join(&loaded.info.original_path))
        .canonicalize()?;
    let original = fs::read(&program)?;
    anyhow::ensure!(
        fixture::hex(&Sha256::digest(&original)) == loaded.info.original_sha256.to_lowercase(),
        "original binary SHA256 differs from binary_descriptor"
    );
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
    let helper = options.helper.clone().unwrap_or_else(|| {
        options
            .repo
            .join("tooling/recon-tool/.build/wine-broker.exe")
    });
    anyhow::ensure!(
        helper.is_file(),
        "build explicit Wine helper first: original build-helper --repo REPO"
    );
    // Configure only the private on-disk helper and executable, before launch.
    // The Windows loader loads the import; the DLL owns its local VEH/callback.
    let mut typed = None;
    let target = if options.progress || options.direct_helper.is_some() {
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
            lookup("case_original_at_marker", "_case_original_at_marker@4")?;
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
            typed = Some((staged, state, complete, marker));
        }
        let path = root.join(program.file_name().context("original filename")?);
        fs::write(&path, target_bytes)?;
        path
    } else {
        program.clone()
    };
    let host = options.host.as_ref().unwrap_or(&target);
    let host = host.canonicalize()?;
    let stdin = root.join("original-stdin");
    fs::write(&stdin, f.stdin.as_ref().map_or(&[][..], |b| b.0.as_slice()))?;
    let stdout = root.join("original-stdout");
    let stderr = root.join("original-stderr");
    let program_windows = windows(&host, &options.repo)?;
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
    let commandline = args.iter().map(|a| quote(a)).collect::<Vec<_>>().join(" ");
    let mut command = command(&options.repo)?;
    command
        .arg(windows(&helper, &options.repo)?)
        .arg(commandline)
        .arg(image.entry.to_string())
        .arg(windows(cwd, &options.repo)?)
        .arg(if f.stdin_kind == fixture::StdinKind::Null {
            "NUL".to_owned()
        } else {
            windows(&stdin, &options.repo)?
        })
        .arg(windows(&stdout, &options.repo)?)
        .arg(windows(&stderr, &options.repo)?)
        .arg(program_windows)
        .arg(f.timeout_ms.to_string())
        .arg(if image.dll {
            windows(&target, &options.repo)?
        } else {
            "-".into()
        });
    if let Some((helper, state, complete, marker)) = &typed {
        command
            .arg(windows(helper, &options.repo)?)
            .arg(state.to_string())
            .arg(complete.to_string())
            .arg(marker.to_string());
    }
    command
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::from(File::create(root.join("wine-diagnostics"))?))
        .env(
            "WINEPATH",
            windows(
                program.parent().context("original directory")?,
                &options.repo,
            )?,
        )
        .current_dir(cwd);
    if image.dll {
        command.env("RECON_ORIGINAL_DLL", windows(&target, &options.repo)?);
    }
    for (k, v) in &f.env {
        match v {
            Some(v) => {
                command.env(k, v);
            }
            None => {
                command.env_remove(k);
            }
        }
    }
    let mut child = command.spawn().context("launch explicit Wine debugger")?;
    let input = child.stdin.take().unwrap();
    let output = child.stdout.take().unwrap();
    let mut broker = Broker {
        child,
        input,
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
    };
    let hello = broker.reply()?;
    let base = hello.a;
    anyhow::ensure!(hello.b == image.entry, "Wine debugger entry mismatch");
    let terminal;
    if let Some((_, _, _, marker)) = typed {
        broker.record(image.entry);
        broker.arm()?;
        let mut invoked = false;
        loop {
            let r = broker.rpc(7, 0, 0, 0, &[])?;
            match r.a {
                0 => bail!("original exited before typed helper completion: {}", r.b),
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
                        "unexpected typed original INT3 at RVA {:x}",
                        r.b
                    );
                    broker.record(r.b);
                    terminal = format!("frontier:{:x}", r.b);
                    break;
                }
                3 => bail!("typed original exception {:08x}", r.b),
                4 => {
                    anyhow::ensure!(
                        invoked && r.b == 1,
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
        loop {
            let r = broker.rpc(7, 0, 0, 0, &[])?;
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
                        "unexpected original INT3 at RVA {:x}",
                        r.b
                    );
                    broker.record(r.b);
                    terminal = format!("frontier:{:x}", r.b);
                    break;
                }
                3 => bail!("original exception {:08x}", r.b),
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
    let mut fixture = f.clone();
    fixture.args = args;
    let stdout = Bytes(fs::read(stdout)?);
    let stderr = Bytes(fs::read(stderr)?);
    let files = capture_files(f, cwd)?;
    if options.progress && terminal.starts_with("frontier:") {
        fixture.partial = Some(fixture::Partial {
            stdout: Some(stdout),
            stderr: Some(stderr),
            output_files: files,
            reject_unexpected_files: f.reject_unexpected_files,
        });
    } else {
        fixture.stdout = Some(stdout);
        fixture.stderr = Some(stderr);
        fixture.output_files = files;
        fixture.exit_code = Some(if loaded.info.execution == 1 {
            0
        } else {
            terminal
                .strip_prefix("exit:")
                .context("expected original exit")?
                .parse()?
        });
    }
    for file in broker.emitted {
        anyhow::ensure!(
            !fixture.input_files.iter().any(|f| f.path == file.path),
            "emitted fixture collides with input {}",
            file.path
        );
        fixture.input_files.push(file);
    }
    native::dependencies(f)?;
    let report = CaptureReport {
        case: loaded.info.case,
        terminal,
        first_entry_order: broker.order,
        elapsed_ms: start.elapsed().as_secs_f64() * 1000.,
        workspace: root.display().to_string(),
    };
    Ok((fixture, report))
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
fn supervise_capture(options: &Capture, f: &Fixture) -> Result<CaptureReport> {
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
        let cwd = canonical_capture_cwd(f, &root, &options.repo, "")?;
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
        native::materialize(f, &cwd)?;
        native::dependencies(f)?;
        let pid = native::fork()?;
        if pid == 0 {
            let result = capture_executor(options, f, &root, &cwd);
            let status = match result {
                Ok((fixture, report)) => {
                    let write = (|| -> Result<()> {
                        if !options.replace && options.output.exists() {
                            bail!("capture output already exists")
                        }
                        fixture::write(&options.output, &fixture)?;
                        fs::write(root.join("capture-report"), serde_json::to_vec(&report)?)?;
                        Ok(())
                    })();
                    match write {
                        Ok(()) => 0,
                        Err(e) => {
                            let _ = fs::write(root.join("capture-error"), format!("{e:#}"));
                            1
                        }
                    }
                }
                Err(e) => {
                    let _ = fs::write(root.join("capture-error"), format!("{e:#}"));
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
            let error = fs::read_to_string(root.join("capture-error"))
                .unwrap_or_else(|_| format!("capture process failed status {status}"));
            let diagnostics = fs::read_to_string(root.join("wine-diagnostics")).unwrap_or_default();
            bail!(
                "{error}\n{}",
                diagnostics.chars().take(8192).collect::<String>()
            );
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
    native::signals();
    options.repo = options.repo.canonicalize()?;
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
        let result = supervise_capture(&options, &fixture);
        let value = match result {
            Ok(report) => serde_json::to_value(report)?,
            Err(e) => serde_json::json!({"error":format!("{e:#}")}),
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
    println!(
        "CAPTURE {} {}: {} first entries ({:.3} ms)",
        value["case"].as_str().unwrap_or("?"),
        value["terminal"].as_str().unwrap_or("?"),
        value["first_entry_order"].as_array().map_or(0, Vec::len),
        value["elapsed_ms"].as_f64().unwrap_or(0.)
    );
    Ok(())
}
