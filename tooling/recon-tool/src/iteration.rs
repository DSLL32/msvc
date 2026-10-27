//! One opt-in attempt composed from existing Make, projection and capture engines.
//! The journal is scratch execution history, never a registry or an acceptance ledger.
use crate::{abi::Info, fixture, native, original_reference};
use anyhow::{Context, Result, bail, ensure};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::{
    collections::{BTreeMap, BTreeSet},
    fs::{self, File, OpenOptions},
    io::Write,
    os::fd::AsRawFd,
    path::{Path, PathBuf},
    process::{Command as Process, Stdio},
    time::{Instant, SystemTime, UNIX_EPOCH},
};

#[derive(clap::Args)]
pub struct Common {
    pub case: PathBuf,
    /// Owned journal directory under ~/.ai-tmp; defaults to a project/case hash.
    #[arg(long)]
    pub state: Option<PathBuf>,
}
#[derive(clap::Args)]
pub struct Discover {
    #[command(flatten)]
    pub common: Common,
    #[arg(long)]
    pub owner: Vec<String>,
    #[arg(long)]
    pub accepted_scope: Option<String>,
    /// Explicit first-use static baseline over every currently accepted App owner.
    #[arg(long)]
    pub all_restored: bool,
    /// Run only preparation/static gates; never capture or compare.
    #[arg(long)]
    pub check_only: bool,
    /// Intentionally preserve observed launch inputs from an existing capture.
    #[arg(long)]
    pub preserve_launch_inputs: bool,
    /// Shared repository/prefix identity; off-tree builds may point to canonical repo.
    #[arg(long)]
    pub repo: Option<PathBuf>,
}
#[derive(clap::Subcommand)]
pub enum Command {
    Replay {
        #[command(flatten)]
        common: Common,
        #[arg(long, default_value = "latest")]
        reference: String,
    },
    Latest {
        #[command(flatten)]
        common: Common,
    },
    Status {
        #[command(flatten)]
        common: Common,
    },
    Export {
        #[command(flatten)]
        common: Common,
        #[arg(long, default_value = "latest")]
        reference: String,
        #[arg(short, long)]
        output: Option<PathBuf>,
    },
}
#[derive(Clone, Serialize, Deserialize, Debug, PartialEq, Eq)]
struct Binding {
    path: PathBuf,
    sha256: String,
}
impl Binding {
    fn read(path: impl AsRef<Path>) -> Result<Self> {
        let path = path
            .as_ref()
            .canonicalize()
            .with_context(|| format!("missing consumed input {}", path.as_ref().display()))?;
        Ok(Self {
            sha256: hash(&fs::read(&path)?),
            path,
        })
    }
    fn verify(&self) -> Result<()> {
        ensure!(
            Self::read(&self.path)? == *self,
            "consumed artifact changed: {}",
            self.path.display()
        );
        Ok(())
    }
}
#[derive(Clone, Serialize, Deserialize)]
struct Phase {
    name: String,
    argv: Vec<String>,
    elapsed_ms: f64,
    status: Option<i32>,
}
#[derive(Clone, Serialize, Deserialize)]
struct Record {
    schema: u32,
    step: String,
    kind: String,
    case: PathBuf,
    owners: Vec<String>,
    phases: Vec<Phase>,
    original_launches: Value,
    checked: bool,
    plan: Option<Value>,
    capture: Option<Binding>,
    original_report: Option<Binding>,
    consumed: Vec<Binding>,
    native_pass: Option<bool>,
    native_report: Option<Binding>,
    exercised_owners: Vec<String>,
    #[serde(default)]
    reference_step: Option<String>,
    #[serde(default)]
    matching_prefix: Option<usize>,
    #[serde(default)]
    terminal: Option<String>,
    #[serde(default)]
    native_timing: Option<Value>,
    error: Option<String>,
}
struct Session {
    project: PathBuf,
    case: PathBuf,
    state: PathBuf,
    dir: PathBuf,
    _lock: File,
    record: Record,
    configuration_sources: Vec<Binding>,
}
struct Config(BTreeMap<String, String>);
fn hash(bytes: &[u8]) -> String {
    fixture::hex(&Sha256::digest(bytes))
}
fn json_file(path: &Path) -> Result<Value> {
    Ok(serde_json::from_slice(&fs::read(path)?)?)
}
fn write_new(path: &Path, value: &impl Serialize) -> Result<()> {
    let mut file = OpenOptions::new().write(true).create_new(true).open(path)?;
    file.write_all(&serde_json::to_vec_pretty(value)?)?;
    file.write_all(b"\n")?;
    Ok(())
}
pub fn case_library(case: &Path) -> Result<PathBuf> {
    Ok(if case.is_dir() {
        case.join(".build/case.so")
    } else {
        case.to_owned()
    }
    .canonicalize()?)
}
/// Shell-style argument grouping only: no expansion, commands, interpolation or shell execution.
fn words(text: &str) -> Result<Vec<String>> {
    let mut out = vec![];
    let mut word = String::new();
    let mut quote = None;
    let mut escape = false;
    let mut started = false;
    for c in text.chars() {
        if escape {
            word.push(c);
            escape = false;
            started = true;
            continue;
        }
        if c == '\\' && quote != Some('\'') {
            escape = true;
            started = true;
            continue;
        }
        if let Some(q) = quote {
            if c == q {
                quote = None;
            } else {
                word.push(c);
            }
            started = true;
        } else if c == '\'' || c == '"' {
            quote = Some(c);
            started = true;
        } else if c.is_whitespace() {
            if started {
                out.push(std::mem::take(&mut word));
                started = false;
            }
        } else {
            word.push(c);
            started = true;
        }
    }
    ensure!(
        !escape && quote.is_none(),
        "unterminated Make argument quoting"
    );
    if started {
        out.push(word);
    }
    Ok(out)
}
impl Config {
    fn load(path: &Path) -> Result<Self> {
        let mut values = BTreeMap::new();
        for line in fs::read_to_string(path)?.lines() {
            let (k, v) = line
                .split_once('=')
                .context("malformed Make iteration configuration")?;
            ensure!(
                values.insert(k.into(), v.into()).is_none(),
                "duplicate Make configuration {k}"
            );
        }
        let c = Self(values);
        ensure!(
            c.get("version") == "1",
            "unsupported Make iteration configuration"
        );
        for k in [
            "project",
            "case",
            "input",
            "library",
            "original",
            "map",
            "image_base",
            "graph",
            "profile",
        ] {
            ensure!(
                !c.get(k).is_empty(),
                "iteration Make configuration lacks {k}; declare applicable existing build/audit inputs"
            );
        }
        Ok(c)
    }
    fn get(&self, k: &str) -> &str {
        self.0.get(k).map(String::as_str).unwrap_or("")
    }
    fn args(&self, k: &str) -> Result<Vec<String>> {
        words(self.get(k))
    }
    fn path(&self, k: &str) -> PathBuf {
        PathBuf::from(self.get(k))
    }
}
fn jobserver_available(flags: &str) -> bool {
    use std::os::unix::fs::FileTypeExt;
    let auth = flags.split_whitespace().find_map(|v| {
        v.strip_prefix("--jobserver-auth=")
            .or_else(|| v.strip_prefix("--jobserver-fds="))
    });
    let Some(auth) = auth else {
        return false;
    };
    if let Some(path) = auth.strip_prefix("fifo:") {
        return fs::metadata(path).is_ok_and(|m| m.file_type().is_fifo());
    }
    let Some((a, b)) = auth.split_once(',') else {
        return false;
    };
    [a, b].iter().all(|v| {
        v.parse::<i32>().is_ok_and(|fd| unsafe {
            let flags = libc::fcntl(fd, libc::F_GETFD);
            let mut stat = std::mem::zeroed::<libc::stat>();
            flags >= 0
                && flags & libc::FD_CLOEXEC == 0
                && libc::fstat(fd, &mut stat) == 0
                && stat.st_mode & libc::S_IFMT == libc::S_IFIFO
        })
    })
}
impl Session {
    fn new(common: Common, kind: &str) -> Result<Self> {
        let project = std::env::current_dir()?.canonicalize()?;
        let case = common
            .case
            .canonicalize()
            .context("iteration case directory must exist")?;
        ensure!(case.is_dir(), "iteration takes a case directory, not a DSO");
        let scratch =
            PathBuf::from(std::env::var_os("HOME").context("HOME missing")?).join(".ai-tmp");
        fs::create_dir_all(&scratch)?;
        let scratch = scratch.canonicalize()?;
        let state = common.state.unwrap_or_else(|| {
            scratch
                .join("msvc2/iterations")
                .join(&hash(format!("{}\n{}", project.display(), case.display()).as_bytes())[..24])
        });
        fs::create_dir_all(&state)?;
        let state = state.canonicalize()?;
        ensure!(
            state.starts_with(&scratch) && !state.starts_with(&project),
            "iteration state must be owned scratch outside project"
        );
        let lock = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .truncate(false)
            .open(state.join("lock"))?;
        ensure!(
            unsafe { libc::flock(lock.as_raw_fd(), libc::LOCK_EX | libc::LOCK_NB) } == 0,
            "another iteration owns this case journal"
        );
        let step = format!(
            "{}-{}",
            SystemTime::now().duration_since(UNIX_EPOCH)?.as_nanos(),
            std::process::id()
        );
        let dir = state.join(&step);
        fs::create_dir(&dir)?;
        let record = Record {
            schema: 1,
            step,
            kind: kind.into(),
            case: case.clone(),
            owners: vec![],
            phases: vec![],
            original_launches: json!(0),
            checked: false,
            plan: None,
            capture: None,
            original_report: None,
            consumed: vec![],
            native_pass: None,
            native_report: None,
            exercised_owners: vec![],
            reference_step: None,
            matching_prefix: None,
            terminal: None,
            native_timing: None,
            error: None,
        };
        Ok(Self {
            project,
            case,
            state,
            dir,
            _lock: lock,
            record,
            configuration_sources: vec![],
        })
    }
    fn command(&mut self, name: &str, program: &Path, args: &[String]) -> Result<()> {
        let log = File::create(self.dir.join(format!("{name}.log")))?;
        let start = Instant::now();
        let mut command = Process::new(program);
        command
            .args(args)
            .current_dir(&self.project)
            .stdin(Stdio::null())
            .stdout(log.try_clone()?)
            .stderr(log);
        if program == Path::new("make") {
            let flags = std::env::var("MAKEFLAGS").unwrap_or_default();
            if !jobserver_available(&flags) {
                let auth = regex::Regex::new(r"(^|\s)--jobserver-(?:auth|fds)=[^\s]+").unwrap();
                command.env("MAKEFLAGS", auth.replace_all(&flags, "$1").as_ref());
            }
        }
        let status = command
            .status()
            .with_context(|| format!("starting {name}"))?;
        let elapsed_ms = start.elapsed().as_secs_f64() * 1000.;
        self.record.phases.push(Phase {
            name: name.into(),
            argv: std::iter::once(program.display().to_string())
                .chain(args.iter().cloned())
                .collect(),
            elapsed_ms,
            status: status.code(),
        });
        println!("{name}: {elapsed_ms:.3} ms ({status})");
        ensure!(
            status.success(),
            "{name} failed; see {}",
            self.dir.join(format!("{name}.log")).display()
        );
        Ok(())
    }
    fn tool(&mut self, name: &str, args: &[String]) -> Result<()> {
        self.command(name, &std::env::current_exe()?, args)
    }
    fn make(&mut self, name: &str, targets: &[String]) -> Result<()> {
        let mut args = vec!["--no-print-directory".into()];
        // GNU Make propagates an inherited jobserver; otherwise use the shared default.
        if !jobserver_available(&std::env::var("MAKEFLAGS").unwrap_or_default()) {
            args.push("-j4".into());
        }
        args.push("-o".into());
        args.push(std::env::current_exe()?.display().to_string());
        args.push(format!("RECON_TOOL={}", std::env::current_exe()?.display()));
        args.push(format!(
            "RECON_ITERATION_CASE={}",
            self.case
                .strip_prefix(&self.project)
                .context("case must belong to the current project")?
                .display()
        ));
        args.push(format!(
            "RECON_ITERATION_CONFIG={}",
            self.dir.join("make.conf").display()
        ));
        args.extend_from_slice(targets);
        self.command(name, Path::new("make"), &args)?;
        verify(&self.configuration_sources)
    }
    fn config(&mut self) -> Result<Config> {
        self.make("configuration", &["recon-iteration-config".into()])?;
        let c = Config::load(&self.dir.join("make.conf"))?;
        ensure!(
            c.path("project").canonicalize()? == self.project
                && c.path("case").canonicalize()? == self.case,
            "Make configuration belongs to another project/case"
        );
        let dependency_files = c
            .args("depfiles")?
            .into_iter()
            .map(|p| std::path::absolute(p))
            .collect::<std::io::Result<BTreeSet<_>>>()?;
        self.configuration_sources = c
            .args("makefiles")?
            .into_iter()
            .filter(|p| std::path::absolute(p).is_ok_and(|p| !dependency_files.contains(&p)))
            .map(Binding::read)
            .collect::<Result<_>>()?;
        Ok(c)
    }
    fn finish(mut self, result: Result<()>) -> Result<()> {
        if let Err(e) = &result {
            self.record.error = Some(format!("{e:#}"));
        }
        write_new(&self.dir.join("result.json"), &self.record)?;
        println!(
            "step {}: original launches {}; {}",
            self.record.step,
            self.record.original_launches,
            self.record.error.as_deref().unwrap_or("complete")
        );
        result
    }
}
fn push_pair(args: &mut Vec<String>, flag: &str, value: impl AsRef<Path>) {
    args.push(flag.into());
    args.push(value.as_ref().display().to_string());
}
fn normalized_args(args: Vec<String>) -> Vec<String> {
    args.into_iter()
        .flat_map(|arg| {
            if arg.starts_with("--")
                && let Some((key, value)) = arg.split_once('=')
            {
                return vec![key.to_owned(), value.to_owned()];
            }
            vec![arg]
        })
        .collect()
}
fn option_path(args: &[String], flag: &str) -> Result<Option<PathBuf>> {
    let positions: Vec<_> = args
        .iter()
        .enumerate()
        .filter(|(_, v)| v.as_str() == flag)
        .map(|(i, _)| i)
        .collect();
    ensure!(positions.len() <= 1, "duplicate input option {flag}");
    positions
        .first()
        .map(|&i| {
            args.get(i + 1)
                .map(PathBuf::from)
                .with_context(|| format!("missing input path {flag}"))
        })
        .transpose()
}
fn parse_flags(c: &Config) -> Result<(Vec<String>, Vec<String>)> {
    let capture = normalized_args(c.args("capture_flags")?);
    let audit = normalized_args(c.args("audit_flags")?);
    for forbidden in [
        "--capture",
        "--output",
        "-o",
        "--report",
        "--artifacts-dir",
        "--replace",
        "--repo",
        "--progress",
    ] {
        ensure!(
            !capture
                .iter()
                .any(|v| v == forbidden || v.starts_with(&format!("{forbidden}="))),
            "iteration owns capture flag {forbidden}"
        );
    }
    for forbidden in [
        "--config",
        "--project",
        "--repo",
        "--report",
        "--metadata",
        "--ret-checks",
        "--profile",
    ] {
        // Ret/runtime inputs are projected below. Remaining existing audit flags are unchanged.
        if ["--metadata", "--ret-checks"].contains(&forbidden) {
            continue;
        }
        ensure!(
            !audit
                .iter()
                .any(|v| v == forbidden || v.starts_with(&format!("{forbidden}="))),
            "iteration owns audit flag {forbidden}"
        );
    }
    for required in ["--check-ownership", "--check-extents"] {
        ensure!(
            audit.iter().any(|v| v == required),
            "iteration requires explicit common gate {required}"
        );
    }
    Ok((capture, audit))
}
fn input_recipe(path: &Path, preserve: bool) -> Result<fixture::Fixture> {
    let f = fixture::read(path)?;
    ensure!(
        preserve
            || (f.process_os_environment.is_none()
                && f.raw_command_line.is_none()
                && f.program_image.is_none()),
        "new invocation contains observed launch inputs; use an authored inputs-only fixture or explicitly --preserve-launch-inputs"
    );
    Ok(f)
}
fn input_view(f: &fixture::Fixture) -> Result<Value> {
    let mut v = serde_json::to_value(f)?;
    for key in [
        "stdout",
        "stderr",
        "output_files",
        "reject_unexpected_files",
        "partial",
        "exit_code",
        "assets",
    ] {
        v.as_object_mut().unwrap().remove(key);
    }
    Ok(v)
}
fn plan(
    info: &Info,
    c: &Config,
    f: &fixture::Fixture,
    capture_flags: &[String],
    repo: &Path,
) -> Result<Value> {
    let mut fs = info.functions.clone();
    fs.sort_by_key(|f| f.rva);
    let rows:Vec<_>=fs.iter().map(|f| json!({"name":f.name,"rva":f.rva,"definition":f.definition,
        "implementation":f.implementation,"code":f.code,"text_data":f.text_data,"import_dll":f.import_dll,"import_name":f.import_name,"import_ordinal":f.import_ordinal})).collect();
    let target = Binding::read(repo.join(&info.original_path))?;
    ensure!(
        target.sha256.eq_ignore_ascii_case(&info.original_sha256),
        "original image SHA differs from registry"
    );
    let mut cf = Value::Null;
    if !c.get("cf_profile").is_empty() {
        cf = json_file(&c.path("cf_profile"))?;
        if let Some(layout) = cf.get_mut("code_layout").and_then(Value::as_object_mut) {
            layout.remove("evidence");
        }
    }
    let mut dependencies = vec![];
    if !capture_flags.iter().any(|v| v == "--helper") {
        dependencies.push(Binding::read(repo.join(
            if c.get("cf_profile").is_empty() {
                "tooling/recon-tool/.build/wine-broker.exe"
            } else {
                "tooling/recon-tool/.build/cf-broker.exe"
            },
        ))?);
    }
    if !c.get("cf_profile").is_empty() {
        if let Some(loader) = cf
            .get("loader")
            .and_then(|v| v.get("path"))
            .and_then(Value::as_str)
        {
            dependencies.push(Binding::read(repo.join(loader))?);
        }
        for name in [
            "frontend.py",
            "debugger.py",
            "broker.py",
            "cf_identity.py",
            "ipc.py",
        ] {
            dependencies.push(Binding::read(
                repo.join("tooling/recon-tool/watcom").join(name),
            )?);
        }
    }
    for flag in ["--helper", "--host", "--direct-helper", "--program"] {
        if let Some(i) = capture_flags.iter().position(|v| v == flag) {
            dependencies.push(Binding::read(
                capture_flags
                    .get(i + 1)
                    .context("capture option missing value")?,
            )?);
        }
    }
    if c.get("cf_profile").is_empty() {
        let value = |flag: &str| {
            capture_flags
                .iter()
                .position(|v| v == flag)
                .and_then(|i| capture_flags.get(i + 1))
                .map(PathBuf::from)
        };
        let declared = repo.join(&info.original_path);
        let program = value("--program").unwrap_or_else(|| declared.clone());
        let host = value("--host").unwrap_or_else(|| program.clone());
        let image = crate::original_image::Image::parse(&fs::read(&program)?)?;
        let (provider, files) = crate::original_stage::input_sources(
            repo,
            &declared,
            &program,
            &host,
            image.dll,
            capture_flags.iter().any(|v| v == "--native-msvcrt"),
        )?;
        for path in provider.into_iter().chain(files) {
            let b = Binding::read(path)?;
            if !dependencies.contains(&b) {
                dependencies.push(b);
            }
        }
    }
    Ok(
        json!({"identity":original_reference::Identity::from_info(info),"entry":info.original_entry_rva,
        "functions":rows,"target":target,"cf":cf,"invocation":input_view(f)?,"capture_flags":capture_flags,"dependencies":dependencies,"transport":hash(concat!(include_str!("original.rs"),include_str!("original_cf.rs"),include_str!("original_stage.rs"),include_str!("wine.rs")).as_bytes())}),
    )
}
fn prior_records(state: &Path) -> Result<Vec<(PathBuf, Record)>> {
    let mut records = vec![];
    for e in fs::read_dir(state)? {
        let e = e?;
        let p = e.path().join("result.json");
        if p.is_file() {
            records.push((e.path(), serde_json::from_slice::<Record>(&fs::read(p)?)?));
        }
    }
    records.sort_by(|a, b| b.1.step.cmp(&a.1.step));
    Ok(records)
}
fn selected_coverage(
    current: &Value,
    previous: Option<&Value>,
    owners: &[String],
    all: bool,
) -> Result<()> {
    if all {
        return Ok(());
    }
    let previous=previous.context("no checked static baseline: explicitly request --all-restored --check-only once (or --all-restored for this attempt); narrow selection never triggers a full audit")?;
    ensure!(
        previous.get("target") == current.get("target"),
        "original target identity changed; explicitly establish --all-restored static baseline"
    );
    for row in current["functions"].as_array().context("plan functions")? {
        if row["definition"] != 4 || row["implementation"] != 1 {
            continue;
        }
        let old = previous["functions"]
            .as_array()
            .unwrap()
            .iter()
            .find(|v| v["name"] == row["name"]);
        ensure!(
            old == Some(row) || owners.iter().any(|n| row["name"] == n.as_str()),
            "changed restored App owner {} is outside checked selection",
            row["name"]
        );
    }
    Ok(())
}
// Make-generated dependency lists are actual compiler/scanner inputs, including headers.
fn dependency_paths(path: &Path) -> Result<Vec<PathBuf>> {
    let text = fs::read_to_string(path)?.replace("\\\n", " ");
    let mut out = vec![];
    for line in text.lines() {
        if let Some((_, deps)) = line.split_once(':') {
            for p in words(deps)? {
                if !p.is_empty() {
                    out.push(p.into());
                }
            }
        }
    }
    Ok(out)
}
fn bound_paths(value: &Value, out: &mut Vec<PathBuf>) -> Result<()> {
    match value {
        Value::Object(object) => {
            if let (Some(path), Some(sha)) = (
                object.get("path").and_then(Value::as_str),
                object.get("sha256").and_then(Value::as_str),
            ) {
                let b = Binding::read(path)?;
                ensure!(
                    b.sha256.eq_ignore_ascii_case(sha),
                    "pinned input SHA differs: {path}"
                );
                out.push(b.path);
            }
            for child in object.values() {
                bound_paths(child, out)?;
            }
        }
        Value::Array(a) => {
            for child in a {
                bound_paths(child, out)?;
            }
        }
        _ => {}
    }
    Ok(())
}
fn consumed(
    c: &Config,
    info: &Info,
    case: &Path,
    extra: &[PathBuf],
    original: bool,
    repo: Option<&Path>,
) -> Result<Vec<Binding>> {
    let mut paths: BTreeSet<PathBuf> = c.args("inputs")?.into_iter().map(PathBuf::from).collect();
    paths.insert(case_library(case)?);
    paths.insert(info.module.clone());
    paths.insert(std::env::current_exe()?);
    for p in c.args("depfiles")? {
        let p = PathBuf::from(p);
        if p.is_file() {
            paths.extend(dependency_paths(&p)?);
            paths.insert(p);
        }
    }
    if original {
        paths.extend(c.args("artifacts")?.into_iter().map(PathBuf::from));
        paths.insert(c.path("graph"));
    }
    paths.extend(extra.iter().cloned());
    let mut pinned = vec![];
    for path in extra {
        if path.extension().is_some_and(|v| v == "json") {
            let mut value = json_file(path)?;
            // Target/container identities have repository coordinates and are
            // already bound separately in the execution plan. Project evidence
            // and object witnesses below retain their ordinary project paths.
            if value.get("function_registry").is_some() {
                value.as_object_mut().unwrap().remove("target");
            }
            if !c.get("cf_profile").is_empty()
                && path.canonicalize()? == c.path("cf_profile").canonicalize()?
            {
                value
                    .as_object_mut()
                    .context("CF profile object")?
                    .remove("target");
                value.as_object_mut().unwrap().remove("loader");
                if let Some(binding) = value
                    .get_mut("code_layout")
                    .and_then(Value::as_object_mut)
                    .and_then(|o| o.remove("evidence"))
                {
                    let source = repo
                        .context("CF source repository")?
                        .join(binding["path"].as_str().context("CF evidence path")?);
                    let b = Binding::read(source)?;
                    ensure!(
                        Some(b.sha256.as_str()) == binding["sha256"].as_str(),
                        "CF evidence SHA differs"
                    );
                    pinned.push(b.path);
                }
            }
            bound_paths(&value, &mut pinned)?;
        }
    }
    paths.extend(pinned);
    paths
        .into_iter()
        .filter(|p| !p.as_os_str().is_empty())
        .map(Binding::read)
        .collect()
}
fn verify(bindings: &[Binding]) -> Result<()> {
    for b in bindings {
        b.verify()?;
    }
    Ok(())
}
fn replace_option(args: &mut Vec<String>, flag: &str, value: &Path) -> Result<()> {
    let mut i = 0;
    while i < args.len() {
        if args[i] == flag {
            ensure!(i + 1 < args.len(), "missing {flag} value");
            args.drain(i..i + 2);
        } else {
            i += 1;
        }
    }
    push_pair(args, flag, value);
    Ok(())
}
fn project_args(s: &Session, c: &Config) -> Vec<String> {
    let mut args = vec![
        "registry-export".into(),
        s.case.display().to_string(),
        "--progress".into(),
        "--image-base".into(),
        c.get("image_base").into(),
    ];
    for (key, flag) in [("policy", "--policy"), ("cf_profile", "--cf-profile")] {
        if !c.get(key).is_empty() {
            push_pair(&mut args, flag, c.path(key));
        }
    }
    args
}
fn prepare(s: &mut Session, c: &Config, o: &Discover) -> Result<(Info, Value, Vec<String>)> {
    let (capture_flags, mut audit) = parse_flags(c)?;
    s.make("native-build", &["recon-iteration-native".into()])?;
    s.make("original-build", &["recon-iteration-original".into()])?;
    let info = native::inspect_for_reference(&case_library(&s.case)?, true)?;
    // Metadata hooks are target-owned and finite. Never infer a missing hook.
    s.make("metadata-build", &["recon-iteration-metadata".into()])?;
    let request = input_recipe(&c.path("input"), o.preserve_launch_inputs)?;
    let repo = crate::toolchain::repository(o.repo.as_deref())?;
    let mut current = plan(&info, c, &request, &capture_flags, &repo)?;
    current["repo"] = json!(repo);
    let previous = if s.state.join("checked-plan.json").is_file() {
        Some(json_file(&s.state.join("checked-plan.json"))?)
    } else {
        None
    };
    let mut owners = o.owner.clone();
    if o.all_restored {
        owners = info
            .functions
            .iter()
            .filter(|f| f.definition == 4 && f.implementation == 1)
            .map(|f| f.name.clone())
            .collect();
    }
    // The projector is the single validator for named scopes, duplicates and implementation availability.
    let mut args = project_args(s, c);
    if let Some(scope) = &o.accepted_scope {
        args.extend(["--accepted-scope".into(), scope.clone()]);
    }
    for name in &owners {
        args.extend(["--owner".into(), name.clone()]);
    }
    ensure!(
        o.all_restored || !owners.is_empty() || o.accepted_scope.is_some(),
        "select --owner, --accepted-scope or explicitly --all-restored"
    );
    let config = s.dir.join("registry.json");
    let metadata = s.dir.join("metadata.json");
    let ret = s.dir.join("ret.json");
    let runtime = s.dir.join("runtime.json");
    push_pair(&mut args, "--output", &config);
    push_pair(&mut args, "--metadata-output", &metadata);
    push_pair(&mut args, "--runtime-output", &runtime);
    push_pair(&mut args, "--rebuilt-image", c.path("original"));
    push_pair(&mut args, "--rebuilt-map", c.path("map"));
    let metadata_input = if !c.get("metadata").is_empty() {
        Some(c.path("metadata"))
    } else {
        option_path(&audit, "--metadata")?
    };
    let ret_input = if !c.get("ret_checks").is_empty() {
        Some(c.path("ret_checks"))
    } else {
        option_path(&audit, "--ret-checks")?
    };
    if c.get("registry_metadata") != "1"
        && let Some(path) = &metadata_input
    {
        push_pair(&mut args, "--metadata-input", path);
    }
    let has_ret = ret_input.is_some() || c.get("registry_ret") == "1";
    if has_ret {
        if c.get("registry_ret") != "1" {
            push_pair(
                &mut args,
                "--ret-checks-input",
                ret_input.as_ref().context("missing RET input")?,
            );
        }
        push_pair(&mut args, "--ret-checks-output", &ret);
    }
    // Named-only selection is validated first, then its actual accepted owners are used below.
    s.tool("projection", &args)?;
    let mut projected = json_file(&config)?;
    owners = projected["accepted"]
        .as_array()
        .context("projected accepted owners")?
        .iter()
        .map(|v| {
            v.as_str()
                .context("accepted owner string")
                .map(str::to_owned)
        })
        .collect::<Result<_>>()?;
    selected_coverage(&current, previous.as_ref(), &owners, o.all_restored)?;
    s.record.owners = owners.clone();
    s.record.plan = Some(plan_identity(&current));
    // Scope independent owner checks without discarding generic contracts/tables.
    let mut m = json_file(&metadata)?;
    super::projection::scope_metadata(&mut m, &projected)?;
    super::projection::scope_config(&mut projected)?;
    fs::write(&config, serde_json::to_vec_pretty(&projected)?)?;
    fs::write(&metadata, serde_json::to_vec_pretty(&m)?)?;
    if has_ret {
        let mut r = json_file(&ret)?;
        r["functions"]
            .as_object_mut()
            .context("RET functions")?
            .retain(|n, _| owners.contains(n));
        fs::write(&ret, serde_json::to_vec_pretty(&r)?)?;
        replace_option(&mut audit, "--ret-checks", &ret)?;
        if r["functions"].as_object().unwrap().is_empty() {
            // The graph reader rejects a vacuous RET file. No implemented
            // selected owner has a RET contract here; all global gates remain.
            let i = audit.iter().position(|v| v == "--ret-checks").unwrap();
            audit.drain(i..i + 2);
        }
    }
    replace_option(&mut audit, "--metadata", &metadata)?;
    // Keep an explicit target-owned filtered runtime catalog. The Make hook
    // refreshes it; the raw registry projection is only the default.
    if !audit.iter().any(|v| v == "--runtime") {
        push_pair(&mut audit, "--runtime", &runtime);
    }
    for (flag, expected) in [
        ("--original", repo.join(&info.original_path)),
        ("--rebuilt", c.path("original")),
        ("--map", c.path("map")),
    ] {
        validate_path_option(&mut audit, flag, &expected)?;
    }
    let mut extras = vec![config.clone(), metadata, runtime, c.path("input")];
    if has_ret {
        extras.push(ret);
    }
    if c.get("registry_metadata") != "1" {
        extras.extend(metadata_input);
    }
    if c.get("registry_ret") != "1" {
        extras.extend(ret_input);
    }
    for key in ["policy", "cf_profile"] {
        if !c.get(key).is_empty() && c.path(key).is_file() {
            extras.push(c.path(key));
        }
    }
    for (i, a) in audit.iter().enumerate() {
        if ["--rebuilt-runtime", "--runtime"].contains(&a.as_str()) {
            extras.push(audit.get(i + 1).context("missing audit input")?.into());
        }
    }
    // Include explicit capture helper/host and original target, not only rebuilt image.
    extras.push(repo.join(&info.original_path));
    for b in current["dependencies"].as_array().unwrap() {
        extras.push(serde_json::from_value::<Binding>(b.clone())?.path);
    }
    s.record.consumed = consumed(c, &info, &s.case, &extras, true, Some(&repo))?;
    let mut graph = vec![
        "--profile".into(),
        c.get("profile").into(),
        "--project".into(),
        s.project.display().to_string(),
        "--repo".into(),
        repo.display().to_string(),
        "--config".into(),
        config.display().to_string(),
        "--original".into(),
        repo.join(&info.original_path).display().to_string(),
        "--rebuilt".into(),
        c.path("original").display().to_string(),
        "--map".into(),
        c.path("map").display().to_string(),
    ];
    graph.extend(audit);
    push_pair(&mut graph, "--report", s.dir.join("graph.json"));
    s.command("graph", &c.path("graph"), &graph)?;
    let hooks = c.args("check_targets")?;
    if !hooks.is_empty() {
        s.make("target-checks", &hooks)?;
    }
    verify(&s.record.consumed)?;
    s.record.checked = true;
    // One current baseline, not a full registry snapshot per iteration.
    let checked = s.state.join("checked-plan.next");
    fs::write(
        &checked,
        serde_json::to_vec(&json!({"functions":current["functions"],"target":current["target"]}))?,
    )?;
    fs::rename(checked, s.state.join("checked-plan.json"))?;
    Ok((info, current, capture_flags))
}
fn bound_receipt(capture: &Binding, report: &Binding, info: &Info) -> Result<()> {
    capture.verify()?;
    report.verify()?;
    let value = json_file(&report.path)?;
    ensure!(
        value["oracle_written"] == true
            && value["capture_sha256"] == capture.sha256
            && value["identity"]
                == serde_json::to_value(original_reference::Identity::from_info(info))?,
        "iteration requires a bound capture+identity receipt; legacy reuse remains available through run"
    );
    Ok(())
}
fn native_pair(s: &mut Session, capture: &Binding, report: &Binding, info: &Info) -> Result<()> {
    s.record.native_pass = Some(false);
    bound_receipt(capture, report, info)?;
    let expected = original_reference::Expected::load(&report.path, &capture.path, info)?;
    ensure!(
        expected.summary.source_binding_available,
        "iteration requires a bound capture+identity receipt; legacy replay remains available through run"
    );
    let path = s.dir.join("native.json");
    let mut args = vec!["run".into(), s.case.display().to_string()];
    push_pair(&mut args, "--capture", &capture.path);
    push_pair(&mut args, "--original-report", &report.path);
    push_pair(&mut args, "--report", &path);
    let result = s.tool("native", &args);
    if path.is_file() {
        s.record.native_report = Some(Binding::read(&path)?);
        let v = json_file(&path)?;
        let r = v
            .as_array()
            .and_then(|v| v.first())
            .context("native report array")?;
        s.record.native_pass = Some(result.is_ok() && r["pass"] == true);
        let original = json_file(&report.path)?;
        s.record.matching_prefix = Some(
            r["first_entry_order"]
                .as_array()
                .context("native trace")?
                .iter()
                .zip(
                    original["first_entry_order"]
                        .as_array()
                        .context("original trace")?,
                )
                .take_while(|(a, b)| a == b)
                .count(),
        );
        s.record.terminal = original["terminal"].as_str().map(str::to_owned);
        s.record.native_timing = Some(
            json!({"elapsed_ms":r["elapsed_ms"],"setup_ms":r["setup_ms"],"execution_ms":r["execution_ms"],"checks_ms":r["checks_ms"]}),
        );
        if s.record.native_pass == Some(true) {
            s.record.exercised_owners = s
                .record
                .owners
                .iter()
                .filter(|n| {
                    info.functions.iter().any(|f| {
                        &f.name == *n
                            && r["first_entry_order"]
                                .as_array()
                                .is_some_and(|v| v.contains(&json!(f.rva)))
                    })
                })
                .cloned()
                .collect();
        }
        println!(
            "native: {} entries, terminal {}, frontier {}; exercised selected owners {:?}",
            r["first_entry_order"].as_array().map_or(0, Vec::len),
            r["terminal"],
            r["trap_rva"],
            s.record.exercised_owners
        );
        if result.is_err()
            && let Some(error) = r["errors"]
                .as_array()
                .and_then(|v| v.first())
                .and_then(Value::as_str)
        {
            return result.context(format!("native comparison: {error}"));
        }
    }
    result
}
fn launch_count(report: &Value) -> Value {
    // A written oracle proves a root target execution. Without that proof failed
    // captures can be prelaunch or postlaunch: do not invent a count from exit status.
    if report["oracle_written"] == true {
        json!({"root":1,"children":report.get("pe_self_child").and_then(|v|v.get("processes")).and_then(Value::as_array).map(|v|v.len().saturating_sub(1))})
    } else {
        json!("unknown")
    }
}
pub fn discover(o: Discover) -> Result<()> {
    let mut s = Session::new(
        Common {
            case: o.common.case.clone(),
            state: o.common.state.clone(),
        },
        "original",
    )?;
    let result = (|| {
        let c = s.config()?;
        let (info, _, flags) = prepare(&mut s, &c, &o)?;
        if o.check_only {
            return Ok(());
        }
        verify(&s.record.consumed)?;
        // Freeze the exact authored request, not a mutable path to a future capture.
        let input = s.dir.join("request.bin");
        fs::copy(c.path("input"), &input)?;
        let out = s.dir.join("original.bin");
        let report = s.dir.join("original.json");
        let artifacts = s.dir.join("artifacts");
        let mut args = vec![
            "original".into(),
            "capture".into(),
            s.case.display().to_string(),
            "--progress".into(),
        ];
        args.extend(flags);
        push_pair(
            &mut args,
            "--repo",
            crate::toolchain::repository(o.repo.as_deref())?,
        );
        push_pair(&mut args, "--capture", &input);
        push_pair(&mut args, "--output", &out);
        push_pair(&mut args, "--report", &report);
        push_pair(&mut args, "--artifacts-dir", &artifacts);
        s.record.original_launches = json!("unknown");
        let capture_result = s.tool("original", &args);
        if report.is_file() {
            s.record.original_launches = launch_count(&json_file(&report)?);
        }
        capture_result?;
        verify(&s.record.consumed)?;
        let capture = Binding::read(out)?;
        let report = Binding::read(report)?;
        bound_receipt(&capture, &report, &info)?;
        s.record.capture = Some(capture.clone());
        s.record.original_report = Some(report.clone());
        s.record.terminal = json_file(&report.path)?["terminal"]
            .as_str()
            .map(str::to_owned);
        // Capture remains selectable when the immediately following native pair fails.
        native_pair(&mut s, &capture, &report, &info)?;
        verify(&s.record.consumed)?;
        Ok(())
    })();
    s.finish(result)
}
fn plan_identity(plan: &Value) -> Value {
    json!({"sha256":hash(&serde_json::to_vec(plan).expect("JSON plan")),"repo":plan["repo"]})
}
fn validate_path_option(args: &mut Vec<String>, flag: &str, expected: &Path) -> Result<()> {
    let mut i = 0;
    while i < args.len() {
        if args[i] == flag {
            let path = args
                .get(i + 1)
                .with_context(|| format!("missing {flag} path"))?;
            ensure!(
                Path::new(path).canonicalize()? == expected.canonicalize()?,
                "audit {flag} differs from actual build input"
            );
            args.drain(i..i + 2);
        } else {
            ensure!(
                !args[i].starts_with(&format!("{flag}=")),
                "use separate {flag} path arguments"
            );
            i += 1;
        }
    }
    Ok(())
}
fn compatible_reference(s: &Session, c: &Config, selector: &str, info: &Info) -> Result<Record> {
    let (flags, _) = parse_flags(c)?;
    for (path, r) in prior_records(&s.state)? {
        if selector != "latest" && r.step != selector {
            continue;
        }
        if r.kind != "original"
            || r.case != s.case
            || r.capture.is_none()
            || r.original_report.is_none()
        {
            continue;
        }
        let repo: PathBuf = serde_json::from_value(
            r.plan.as_ref().context("reference plan missing")?["repo"].clone(),
        )?;
        let request = fixture::read(&path.join("request.bin"))?;
        if input_view(&fixture::read(&c.path("input"))?)? != input_view(&request)? {
            ensure!(
                selector == "latest",
                "authored invocation/input changed; fresh discovery required"
            );
            continue;
        }
        let mut current = plan(info, c, &request, &flags, &repo)?;
        current["repo"] = json!(repo);
        if r.plan.as_ref() != Some(&plan_identity(&current)) {
            ensure!(
                selector == "latest",
                "selected reference original execution plan changed; fresh discovery required"
            );
            continue;
        }
        let capture = r.capture.as_ref().unwrap();
        let report = r.original_report.as_ref().unwrap();
        capture.verify()?;
        report.verify()?;
        bound_receipt(capture, report, info)?;
        return Ok(r);
    }
    bail!(
        "no compatible captured reference {selector}; prelaunch failures and native-pass status are not reference selectors"
    )
}
fn record_summary(record: &Record) -> Value {
    json!({"step":record.step,"kind":record.kind,"owners":record.owners,"checked":record.checked,
        "original_launches":record.original_launches,"native_pass":record.native_pass,
        "matching_prefix":record.matching_prefix,"terminal":record.terminal,"native_timing":record.native_timing,
        "exercised_owners":record.exercised_owners,"reference_step":record.reference_step,
        "capture":record.capture,"original_report":record.original_report,"native_report":record.native_report,
        "error":record.error,"phases":record.phases.iter().map(|p|json!({"name":p.name,"elapsed_ms":p.elapsed_ms,"status":p.status})).collect::<Vec<_>>()})
}
pub fn execute(command: Command) -> Result<()> {
    match command {
        Command::Replay { common, reference } => {
            let mut s = Session::new(common, "replay")?;
            let result = (|| {
                let c = s.config()?;
                parse_flags(&c)?;
                s.make("native-build", &["recon-iteration-native".into()])?;
                let info = native::inspect_for_reference(&case_library(&s.case)?, true)?;
                let r = compatible_reference(&s, &c, &reference, &info)?;
                let capture = r.capture.unwrap();
                let report = r.original_report.unwrap();
                println!(
                    "reference {} capture {} report {}",
                    r.step, capture.sha256, report.sha256
                );
                s.record.reference_step = Some(r.step.clone());
                s.record.owners = r.owners;
                s.record.plan = r.plan;
                s.record.consumed = consumed(&c, &info, &s.case, &[c.path("input")], false, None)?;
                native_pair(&mut s, &capture, &report, &info)?;
                verify(&s.record.consumed)?;
                Ok(())
            })();
            s.finish(result)
        }
        Command::Status { common } => {
            let s = Session::new(common, "status")?;
            let records = prior_records(&s.state)?;
            println!(
                "{}",
                serde_json::to_string_pretty(
                    &json!({"journal":s.state,"last":records.first().map(|(_,r)|record_summary(r)),"latest_capture":records.iter().find(|(_,r)|r.capture.is_some()).map(|(_,r)|&r.step)})
                )?
            );
            fs::remove_dir_all(&s.dir)?;
            Ok(())
        }
        Command::Latest { common } => {
            let mut s = Session::new(common, "latest")?;
            let c = s.config()?;
            let info = native::inspect_for_reference(&case_library(&s.case)?, true)?;
            let r = compatible_reference(&s, &c, "latest", &info)?;
            println!("{}", serde_json::to_string_pretty(&record_summary(&r))?);
            fs::remove_dir_all(&s.dir)?;
            Ok(())
        }
        Command::Export {
            common,
            reference,
            output,
        } => {
            let mut s = Session::new(common, "export")?;
            let c = s.config()?;
            let info = native::inspect_for_reference(&case_library(&s.case)?, true)?;
            let r = compatible_reference(&s, &c, &reference, &info)?;
            let capture = r.capture.as_ref().unwrap();
            let report = r.original_report.as_ref().unwrap();
            let expected = original_reference::Expected::load(&report.path, &capture.path, &info)?;
            ensure!(
                expected.summary.source_binding_available,
                "export requires bound reference"
            );
            let raw = json_file(&report.path)?;
            let c_order = raw["first_entry_order"]
                .as_array()
                .context("original trace array")?
                .iter()
                .map(|v| format!("0x{:x}", v.as_u64().unwrap()))
                .collect::<Vec<_>>()
                .join(", ");
            let comparison=prior_records(&s.state)?.into_iter().find(|(_,v)|v.reference_step.as_ref()==Some(&r.step)).map(|(_,v)|json!({"step":v.step,"pass":v.native_pass,"exercised_owners":v.exercised_owners,"report":v.native_report}));
            let delta = json!({"latest_native_replay":comparison,"review_required":true,"not_case_completion":true,"step":r.step,"native_pass":r.native_pass,
                "capture":capture,"original_report":report,"first_entry_order":raw["first_entry_order"],"first_entry_seen":info.first_entry_seen,
                "terminal":raw["terminal"],"c_initializer":format!("static const uint32_t order[] = {{{c_order}}};"),"exercised_owners":r.exercised_owners});
            if let Some(path) = output {
                write_new(&path, &delta)?;
            } else {
                println!("{}", serde_json::to_string_pretty(&delta)?);
            }
            fs::remove_dir_all(&s.dir)?;
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn inherited_jobserver_requires_live_inheritable_pipe_or_fifo() {
        assert!(!jobserver_available("-j4 --jobserver-auth=999998,999999"));
        let mut fds = [0; 2];
        assert_eq!(unsafe { libc::pipe(fds.as_mut_ptr()) }, 0);
        assert!(jobserver_available(&format!(
            "-j4 --jobserver-auth={},{}",
            fds[0], fds[1]
        )));
        unsafe {
            libc::fcntl(fds[0], libc::F_SETFD, libc::FD_CLOEXEC);
        }
        assert!(!jobserver_available(&format!(
            "--jobserver-auth={},{}",
            fds[0], fds[1]
        )));
        unsafe {
            libc::close(fds[0]);
            libc::close(fds[1]);
        }
    }
    #[test]
    fn finite_coverage_requires_explicit_baseline_and_changed_owner() {
        let old =
            json!({"functions":[{"name":"a","definition":4,"implementation":1,"code":[[1,9]]}]});
        let mut next = old.clone();
        next["functions"][0]["code"] = json!([[1, 10]]);
        assert!(selected_coverage(&old, None, &["a".into()], false).is_err());
        assert!(selected_coverage(&old, None, &[], true).is_ok());
        assert!(selected_coverage(&old, Some(&old), &[], false).is_ok());
        assert!(selected_coverage(&next, Some(&old), &[], false).is_err());
        assert!(selected_coverage(&next, Some(&old), &["a".into()], false).is_ok());
    }
    #[test]
    fn grouped_arguments_never_execute_or_expand() {
        assert_eq!(
            words("--host 'two words' \"$HOME\" `x` '' a\\ b").unwrap(),
            ["--host", "two words", "$HOME", "`x`", "", "a b"]
        );
        assert!(words("'open").is_err());
        assert!(words("tail\\").is_err());
    }
    #[test]
    fn flags_preserve_existing_format_runtime_and_direct_options() {
        let config = |capture: &str, audit: &str| {
            Config(BTreeMap::from([
                ("capture_flags".into(), capture.into()),
                ("audit_flags".into(), audit.into()),
            ]))
        };
        for flag in [
            "--capture",
            "--report",
            "--replace",
            "--progress",
            "--output=bad",
        ] {
            assert!(parse_flags(&config(flag, "--check-ownership --check-extents")).is_err());
        }
        assert!(parse_flags(&config("--host 'host.exe' --direct-noreturn --process-os-environment --pe-self-child", "--check-ownership --check-extents --original target --rebuilt rebuilt --map app.map --runtime filtered.json")).is_ok());
    }
    #[test]
    fn compact_plan_digest_and_input_separation() {
        let p = json!({"functions":[{"name":"a","code":[[1,9]]}],"repo":"/repo"});
        let id = plan_identity(&p);
        assert!(id.get("functions").is_none());
        let mut changed = p.clone();
        changed["functions"][0]["code"] = json!([[1, 10]]);
        assert_ne!(id, plan_identity(&changed));
        let f = fixture::Fixture::default();
        let mut g = f.clone();
        g.stdout = Some(fixture::Bytes(b"new".to_vec()));
        assert_eq!(input_view(&f).unwrap(), input_view(&g).unwrap());
        g.args.push("different".into());
        assert_ne!(input_view(&f).unwrap(), input_view(&g).unwrap());
    }
    #[test]
    fn unknown_launches_are_not_cli_attempts() {
        assert_eq!(launch_count(&json!({"error":"EOF"})), json!("unknown"));
        assert_eq!(launch_count(&json!({"oracle_written":true}))["root"], 1);
    }
    #[test]
    fn scoping_keeps_globals_callbacks_and_other_annotations() {
        let mut m = json!({"functions":{"a":{"register_imports":{"enabled":true}},"b":{"register_imports":{"enabled":true},"switches":{"4":[8]}}},"callback_tables":[{"anchor":1}],"register_imports":{"preservation_contracts":[{"owner":"external"}]}});
        let globals = m["register_imports"].clone();
        let callbacks = m["callback_tables"].clone();
        crate::projection::scope_metadata(&mut m, &json!({"accepted":["a"]})).unwrap();
        assert!(m["functions"]["b"].get("register_imports").is_none());
        assert!(m["functions"]["b"].get("switches").is_some());
        assert_eq!(m["register_imports"], globals);
        assert_eq!(m["callback_tables"], callbacks);
    }
}
