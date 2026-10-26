//! Explicit CF transport, using the existing stopped-loader controller.
//! Entry-stop produces diagnostic evidence only, never a fixture oracle.
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    fs,
    path::{Path, PathBuf},
    process::{Command, Stdio},
};

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Header {
    header_file_offset: u32,
    image_file_offset: u32,
    initialized_size: u32,
    relocation_offset: u32,
    allocation_size: u32,
    entry_offset: u32,
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Target {
    path: String,
    sha256: String,
    coordinates: String,
    analysis_base: u32,
    header: Header,
    initialized_sha256: String,
    relocation_sha256: String,
    relocation_count: u32,
    relocation_groups: u32,
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Loader {
    path: String,
    sha256: String,
    protocol: String,
    #[serde(default)]
    setup_catalog: Option<serde_json::Value>,
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Layout {
    code: Vec<[u32; 2]>,
    text_data: Vec<[u32; 2]>,
    #[serde(
        default,
        deserialize_with = "optional_evidence",
        skip_serializing_if = "Option::is_none"
    )]
    evidence: Option<Evidence>,
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Evidence {
    path: String,
    sha256: String,
}
fn optional_evidence<'de, D>(deserializer: D) -> Result<Option<Evidence>, D::Error>
where
    D: serde::Deserializer<'de>,
{
    // Absence is allowed; an explicitly supplied null is malformed.
    Evidence::deserialize(deserializer).map(Some)
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Profile {
    schema: u32,
    backend: String,
    mode: String,
    target: Target,
    loader: Loader,
    runtime_catalog: Option<serde_json::Value>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    code_layout: Option<Layout>,
}
fn hash(bytes: &[u8]) -> String {
    crate::fixture::hex(&Sha256::digest(bytes))
}
fn launch_executable(command: &Command) -> Result<String> {
    Ok(command
        .get_program()
        .to_str()
        .context("Wine executable is not UTF-8")?
        .to_owned())
}
fn raw_command_line(loader: &str, tail: &str, declared: Option<&str>) -> Result<String> {
    ensure!(
        !loader.is_empty()
            && loader.is_ascii()
            && !loader
                .chars()
                .any(|c| c.is_whitespace() || matches!(c, '"' | '\0'))
            && tail.is_ascii()
            && !tail.contains('\0')
            && loader.len() + tail.len() + 5 < 4096,
        "CF command-line domain requires an ASCII loader path without whitespace/quotes and bounded ANSI tail"
    );
    // Match the helper's actual sprintf recipe; the capture uses its recorded
    // buffer, not this prelaunch validation copy.
    let text = format!("\"{loader}\" {tail}");
    ensure!(
        declared.is_none_or(|value| value == text),
        "raw_command_line differs from constructed CF launch text; use an inputs-only fixture without the captured raw_command_line for a fresh invocation"
    );
    Ok(text)
}

fn validate(
    profile: &Profile,
    original_path: &str,
    original_sha256: &str,
    original_entry_rva: u32,
) -> Result<()> {
    ensure!(
        profile.schema == 1
            && profile.backend == "watcom-cf"
            && ((profile.mode == "entry-stop" && profile.runtime_catalog.is_none())
                || (profile.mode == "debugger"
                    && profile.runtime_catalog.is_none()
                    && profile.code_layout.is_some())),
        "unsupported CF profile/mode; entry-stop admits no instruction catalog"
    );
    ensure!(
        profile.target.path == original_path
            && profile.target.sha256 == original_sha256
            && profile.target.coordinates == "image-offset"
            && profile.target.analysis_base == 0
            && profile.target.header.entry_offset == original_entry_rva,
        "CF profile differs from binary_descriptor identity/coordinates"
    );
    if profile.loader.protocol == "watcom10.0a-loader-v1" {
        let target = Path::new(&profile.target.path);
        let loader = Path::new(&profile.loader.path);
        ensure!(
            target
                .components()
                .all(|c| matches!(c, std::path::Component::Normal(_)))
                && target.parent().and_then(Path::file_name) == Some(std::ffi::OsStr::new("binb"))
                && loader
                    == target
                        .parent()
                        .unwrap()
                        .parent()
                        .context("CF toolchain root")?
                        .join("binnt")
                        .join(target.file_name().context("CF filename")?)
                && profile.loader.sha256
                    == "8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d"
                && profile.loader.protocol == "watcom10.0a-loader-v1"
                && profile.loader.setup_catalog.is_none(),
            "ordinary CF requires the hash-bound sibling Win32 loader"
        );
        if profile.mode == "entry-stop" {
            ensure!(
                profile.code_layout.is_none(),
                "identity-only CF profile does not supply mask ranges"
            );
            return Ok(());
        }
        ensure!(
            profile.mode == "debugger",
            "ordinary loader uses debugger mode"
        );
        let layout = profile
            .code_layout
            .as_ref()
            .context("CF CODE layout missing")?;
        ensure!(
            !layout.code.is_empty() && layout.code.len() <= 64 && layout.text_data.len() <= 65536,
            "bounded CF CODE/text data layout required"
        );
        let mut end = 0;
        for &[start, limit] in &layout.code {
            ensure!(
                end <= start && start < limit && limit <= profile.target.header.relocation_offset,
                "invalid CF CODE envelope"
            );
            end = limit;
        }
        for &[start, end] in &layout.text_data {
            ensure!(
                start < end && end <= profile.target.header.relocation_offset,
                "invalid CF text data"
            );
        }
        if let Some(evidence) = &layout.evidence {
            ensure!(
                !evidence.path.is_empty()
                    && Path::new(&evidence.path)
                        .components()
                        .all(|c| matches!(c, std::path::Component::Normal(_)))
                    && crate::fixture::unhex(&evidence.sha256)?.len() == 32,
                "CF layout evidence must be repository-relative and hash-bound"
            );
        }
        return Ok(());
    }
    ensure!(
        profile.code_layout.is_none(),
        "code layout belongs to ordinary debugger mode"
    );
    // A finite reviewed loader protocol, not an arbitrary list of executable PCs.
    // Other CF loaders need their own bounded geometry/byte qualification.
    let h = &profile.target.header;
    let expected = match profile.loader.protocol.as_str() {
        "watcom10.0a-wcc-entry-v1" => (
            "WCC386.EXE",
            "c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b",
            [0x50, 0x2200, 0x8209c, 0x7c448, 0x91000, 0x2fe00],
            "67510f8687dc06331c2bd85b56382a3be729be7a434f4330db06dcbffb2747c7",
            "a9aac7fd5813bb859f95bbe287ddfcc2ee32c38bae5103fd4fedcb74738d3640",
            11801,
            8,
        ),
        "watcom10.0a-wpp-entry-v1" => (
            "WPP386.EXE",
            "647f4e754cd27fc76b3c44a47227ae3baa4ff2e2f253a8ec27216414653b1e6f",
            [0x50, 0x2200, 0xc3a44, 0xbc438, 0xe0000, 0x664f4],
            "1f96d6ad4cf3deb8078ab02fe58e0d56b51001b058beb39ee9eb1ffeadae7e07",
            "9829d0e260bb569d85ee056419390a15e84bcf613c07c9417b4d43aaccc63f9b",
            15085,
            12,
        ),
        _ => anyhow::bail!("unqualified CF loader protocol"),
    };
    ensure!(
        profile.loader.setup_catalog.is_none()
            && profile.loader.path == format!("toolchains/watcom10.0a/binnt/{}", expected.0)
            && profile.loader.sha256
                == "8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d"
            && profile.target.path == format!("toolchains/watcom10.0a/binb/{}", expected.0)
            && profile.target.sha256 == expected.1
            && [
                h.header_file_offset,
                h.image_file_offset,
                h.initialized_size,
                h.relocation_offset,
                h.allocation_size,
                h.entry_offset
            ] == expected.2
            && profile.target.initialized_sha256 == expected.3
            && profile.target.relocation_sha256 == expected.4
            && profile.target.relocation_count == expected.5
            && profile.target.relocation_groups == expected.6,
        "unqualified CF loader protocol or payload geometry"
    );
    Ok(())
}

/// Project hash-bound format identity for graph-audit.
/// Registry classification, graph audits and masking remain separate.
pub(crate) fn graph_image(path: &Path, info: &crate::abi::Info) -> Result<serde_json::Value> {
    ensure!(
        !path.as_os_str().is_empty()
            && path
                .components()
                .all(|c| matches!(c, std::path::Component::Normal(_))),
        "CF projection profile must be relative to the graph project/current directory"
    );
    let raw = fs::read(path)?;
    ensure!(raw.len() <= 65536, "CF profile exceeds 64 KiB");
    let profile: Profile = serde_json::from_slice(&raw)?;
    ensure!(
        matches!(profile.mode.as_str(), "entry-stop" | "debugger"),
        "CF graph projection requires a format identity/layout profile without an instruction catalog"
    );
    validate(
        &profile,
        &info.original_path,
        &info.original_sha256,
        info.original_entry_rva,
    )?;
    Ok(serde_json::json!({
        "format":"watcom-cf", "coordinates":"image-offset", "analysis_base":0,
        "initialized_sha256":profile.target.initialized_sha256,
        "relocation_sha256":profile.target.relocation_sha256,
        "loader":{"path":profile.loader.path,"sha256":profile.loader.sha256},
        "layout_evidence":{"path":path,"sha256":hash(&raw)}
    }))
}

pub fn capture(
    options: &crate::original::Capture,
    info: &crate::abi::Info,
    f: &crate::fixture::Fixture,
    root: &Path,
    cwd: &Path,
    program: &Path,
    environment: &Path,
) -> Result<serde_json::Value> {
    ensure!(
        options.host.is_none()
            && options.direct_helper.is_none()
            && !options.native_msvcrt
            && !options.entry_stop
            && options.progress
            && info.execution == 0,
        "CF capture uses ordinary IO --progress; live entry-stop and instruction-admission modes are retired"
    );
    ensure!(
        f.timeout_ms > 0 && f.timeout_ms <= 300000,
        "CF controller deadline must be 1..300000 ms"
    );
    let raw = fs::read(options.cf_profile.as_ref().context("CF profile missing")?)?;
    ensure!(raw.len() <= 65536, "CF profile exceeds 64 KiB");
    let profile: Profile = serde_json::from_slice(&raw)?;
    validate(
        &profile,
        &info.original_path,
        &info.original_sha256,
        info.original_entry_rva,
    )?;
    ensure!(
        profile.mode == "debugger",
        "CF capture needs debugger mode with a code layout; identity-only profiles are static projection inputs"
    );
    let target = options.repo.join(&profile.target.path).canonicalize()?;
    ensure!(
        target == program,
        "CF loader derives its sibling binb payload; --program cannot relocate it"
    );
    let loader = options.repo.join(&profile.loader.path).canonicalize()?;
    ensure!(
        hash(&fs::read(&loader)?) == profile.loader.sha256,
        "CF loader SHA256 differs from profile"
    );
    let helper = options
        .helper
        .clone()
        .unwrap_or_else(|| options.repo.join("tooling/recon-tool/.build/cf-broker.exe"))
        .canonicalize()
        .context("build explicit CF helper first: original build-cf-helper --repo REPO")?;
    let stdin = root.join("original-stdin");
    fs::write(&stdin, f.stdin.as_ref().map_or(&[][..], |b| b.0.as_slice()))?;
    let args = f
        .args
        .iter()
        .skip(1)
        .map(|a| crate::original::quote(a))
        .collect::<Vec<_>>()
        .join(" ");
    ensure!(
        args.is_ascii() && args.len() <= 2048,
        "CF ANSI command tail invalid/too large"
    );
    let raw_command_line = raw_command_line(
        &crate::wine::windows(&loader, &options.repo)?,
        &args,
        f.raw_command_line.as_deref(),
    )?;
    // Copy shared Wine host transport only; case env is the separate Win32 block.
    let wine = crate::wine::command(&options.repo)?;
    let mut command = Command::new("python3");
    for (key, value) in wine.get_envs() {
        if let Some(value) = value {
            command.env(key, value);
        } else {
            command.env_remove(key);
        }
    }
    let package = Path::new(env!("CARGO_MANIFEST_DIR"));
    let mut sources = serde_json::Map::new();
    let source_names = [
        "frontend.py",
        "debugger.py",
        "broker.py",
        "cf_identity.py",
        "ipc.py",
    ];
    for name in source_names {
        let path = package.join("watcom").join(name);
        sources.insert(
            name.into(),
            serde_json::json!({"path":path,"sha256":hash(&fs::read(&path)?)}),
        );
    }
    sources.insert(
        "helper".into(),
        serde_json::json!({"path":helper,"sha256":hash(&fs::read(&helper)?)}),
    );
    let request = serde_json::json!({"repo":options.repo,"profile":profile,"profile_sha256":hash(&raw),
        "target_env":f.env,"registry":info.functions,"first_entry_seen":info.first_entry_seen,"sources":sources,"root":root,"cwd":cwd,"target":target,"loader":loader,"helper":helper,
        "wine":launch_executable(&wine)?,"environment":environment,"arguments":args,
        "raw_command_line":raw_command_line,
        "stdin":if f.stdin_kind==crate::fixture::StdinKind::Null {"NUL".into()}else{crate::wine::windows(&stdin,&options.repo)?},
        "timeout_ms":f.timeout_ms,"seconds":(f.timeout_ms as f64/1000.+1.).min(300.)});
    let request_path = root.join("cf-request");
    fs::write(&request_path, serde_json::to_vec(&request)?)?;
    let result = command
        .arg(package.join("watcom/frontend.py"))
        .arg(request_path)
        .current_dir(cwd)
        .stdin(Stdio::null())
        .output()?;
    let raw = fs::read(root.join("cf-result")).unwrap_or_default();
    ensure!(
        result.status.success(),
        "CF backend failed: {}\n{}\n{}",
        result.status,
        String::from_utf8_lossy(&result.stderr),
        String::from_utf8_lossy(&raw)
    );
    ensure!(raw.len() <= 4 * 1024 * 1024, "CF report exceeds 4 MiB");
    let report: serde_json::Value = serde_json::from_slice(&raw)?;
    ensure!(
        report["passed"] == true
            && report["oracle_written"] == !options.entry_stop
            && (!options.entry_stop || report["original_cf_instructions_executed"] == 0),
        "invalid CF entry-stop receipt"
    );
    ensure!(
        report["raw_command_line"] == raw_command_line,
        "CF report lacks exact actual command input"
    );
    Ok(report)
}

pub fn build_helper(repo: &Path) -> Result<PathBuf> {
    let repo = repo.canonicalize()?;
    let root = repo.join("tooling/recon-tool");
    let build = root.join(".build");
    fs::create_dir_all(&build)?;
    let exe = build.join("cf-broker.exe");
    let object = build.join("cf-broker.obj");
    let mut command = crate::toolchain::command(&repo, "1.0-nt", "CL.EXE")?;
    let result = command
        .args(["/nologo", "/Od", "/G3"])
        .arg(format!("/Fe{}", crate::wine::windows(&exe, &repo)?))
        .arg(format!("/Fo{}", crate::wine::windows(&object, &repo)?))
        .arg(crate::wine::windows(&root.join("wine/cf-broker.c"), &repo)?)
        .args(["/link", "/base:0x400000"])
        .current_dir(&build)
        .output()?;
    fs::write(
        build.join("cf-build.log"),
        [result.stdout.as_slice(), result.stderr.as_slice()].concat(),
    )?;
    ensure!(
        result.status.success(),
        "CF helper build failed: {}\n{}\n{}",
        result.status,
        String::from_utf8_lossy(&result.stdout),
        String::from_utf8_lossy(&result.stderr)
    );
    Ok(exe)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn cf_raw_command_line_matches_helper_recipe_and_declared_input() {
        let loader = r"R:\toolchains\watcom10.0a\binnt\WPP386.EXE";
        let raw = raw_command_line(loader, "?", None).unwrap();
        assert_eq!(raw, format!("\"{loader}\" ?"));
        assert_eq!(raw_command_line(loader, "?", Some(&raw)).unwrap(), raw);
        assert!(raw_command_line(loader, "?", Some("WPP386.EXE ?")).is_err());
        assert_eq!(
            raw_command_line(loader, "", None).unwrap(),
            format!("\"{loader}\" ")
        );
        for bad in ["", "R:\\a b.exe", "R:\\a\"b.exe", "R:\\a\0b.exe", "é.exe"] {
            assert!(raw_command_line(bad, "?", None).is_err());
        }
        assert!(raw_command_line(loader, "x\0y", None).is_err());
        assert!(raw_command_line(loader, &"x".repeat(4096), None).is_err());
    }
    fn valid(value: &serde_json::Value) -> bool {
        let p: Profile = match serde_json::from_value(value.clone()) {
            Ok(p) => p,
            Err(_) => return false,
        };
        validate(
            &p,
            "toolchains/watcom10.0a/binb/WCC386.EXE",
            "c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b",
            0x2fe00,
        )
        .is_ok()
    }
    #[test]
    fn ordinary_debugger_uses_format_identity_and_layout_without_instruction_catalog() {
        let mut value: serde_json::Value =
            serde_json::from_str(include_str!("../tests/cf-entry-profile.json")).unwrap();
        value["mode"] = serde_json::json!("debugger");
        value["loader"]["protocol"] = serde_json::json!("watcom10.0a-loader-v1");
        value["code_layout"] = serde_json::json!({"code":[[0,0x704c4]],"text_data":[],
            "evidence":{"path":"decomp/watcom10.0a/wcc386/harness/cf-layout.json","sha256":"a".repeat(64)}});
        assert!(valid(&value));
        for (path, replacement) in [
            ("/runtime_catalog", serde_json::json!({"instructions":[]})),
            ("/code_layout/code", serde_json::json!([[0, 0x8209c]])),
            (
                "/code_layout/evidence/path",
                serde_json::json!("../layout.json"),
            ),
            ("/loader/sha256", serde_json::json!("0".repeat(64))),
            (
                "/loader/path",
                serde_json::json!("toolchains/watcom10.0a/binnt/WPP386.EXE"),
            ),
        ] {
            let mut bad = value.clone();
            *bad.pointer_mut(path).unwrap() = replacement;
            assert!(!valid(&bad), "{path}");
        }
        for evidence in [
            serde_json::Value::Null,
            serde_json::json!({}),
            serde_json::json!({"path":"layout.json","sha256":"invalid"}),
        ] {
            let mut bad = value.clone();
            bad["code_layout"]["evidence"] = evidence;
            assert!(!valid(&bad));
        }
        value["code_layout"]
            .as_object_mut()
            .unwrap()
            .remove("evidence");
        assert!(valid(&value));
        let mut bad = value.clone();
        bad["code_layout"]["code"] = serde_json::json!([[0, 0x8209c]]);
        assert!(!valid(&bad));
        let mut bad = value.clone();
        bad["target"]["sha256"] = serde_json::json!("0".repeat(64));
        assert!(!valid(&bad));
        value["mode"] = serde_json::json!("entry-stop");
        value.as_object_mut().unwrap().remove("code_layout");
        assert!(valid(&value)); // Format-only projection, no execution admission.
    }

    #[test]
    fn wpp_profile_has_its_own_finite_geometry_and_loader_name() {
        let value: serde_json::Value =
            serde_json::from_str(include_str!("../tests/cf-wpp-profile.json")).unwrap();
        let check = |value: serde_json::Value| {
            let profile: Profile = serde_json::from_value(value).unwrap();
            validate(
                &profile,
                &profile.target.path,
                &profile.target.sha256,
                profile.target.header.entry_offset,
            )
            .is_ok()
        };
        assert!(check(value.clone()));
        for (path, replacement) in [
            (
                "/loader/path",
                serde_json::json!("toolchains/watcom10.0a/binnt/WCC386.EXE"),
            ),
            (
                "/loader/protocol",
                serde_json::json!("watcom10.0a-wcc-entry-v1"),
            ),
            ("/target/header/entry_offset", serde_json::json!(0x2fe00)),
            ("/target/relocation_count", serde_json::json!(11801)),
            (
                "/target/relocation_sha256",
                serde_json::json!("0".repeat(64)),
            ),
        ] {
            let mut bad = value.clone();
            *bad.pointer_mut(path).unwrap() = replacement;
            assert!(!check(bad), "{path}");
        }
    }

    #[test]
    fn launch_executable_is_a_json_string_for_python_subprocess() {
        let command = Command::new("/usr/bin/wine");
        let value = serde_json::json!({"wine":launch_executable(&command).unwrap()});
        assert_eq!(value["wine"].as_str(), Some("/usr/bin/wine"));
        use std::os::unix::ffi::OsStringExt;
        let invalid = Command::new(std::ffi::OsString::from_vec(vec![b'/', 255]));
        assert!(launch_executable(&invalid).is_err());
    }

    #[test]
    fn graph_projection_uses_finite_identity_and_project_relative_profile() {
        let value: serde_json::Value =
            serde_json::from_str(include_str!("../tests/cf-entry-profile.json")).unwrap();
        let mut info: crate::abi::Info = serde_json::from_value(serde_json::json!({
            "binary":"WCC386.EXE", "case":"metadata", "description":"", "execution":0,
            "original_path":value["target"]["path"], "original_sha256":value["target"]["sha256"],
            "original_entry_rva":0x2fe00, "terminal":1,"frontier_rva":0x47c77,
            "first_entry_order":[],"first_entry_seen":[],"functions":[],"tests":[],"module":"metadata.so"
        })).unwrap();
        let path = Path::new("tests/cf-entry-profile.json");
        let projected = graph_image(path, &info).unwrap();
        assert_eq!(projected["format"], "watcom-cf");
        assert_eq!(projected["analysis_base"], 0);
        assert_eq!(projected["loader"]["sha256"], value["loader"]["sha256"]);
        assert_eq!(
            projected["layout_evidence"]["path"],
            "tests/cf-entry-profile.json"
        );
        assert_eq!(
            projected["layout_evidence"]["sha256"],
            hash(&fs::read(path).unwrap())
        );
        assert!(projected.get("runtime_catalog").is_none());
        assert!(graph_image(&path.canonicalize().unwrap(), &info).is_err());
        assert!(graph_image(Path::new("../profile.json"), &info).is_err());
        info.original_entry_rva = 0x1a99b;
        assert!(graph_image(path, &info).is_err());
        info.original_entry_rva = 0x2fe00;
        info.original_sha256 = "0".repeat(64);
        assert!(graph_image(path, &info).is_err());
    }

    #[test]
    fn entry_stop_profile_is_finite_and_bound_to_descriptor() {
        let value: serde_json::Value =
            serde_json::from_str(include_str!("../tests/cf-entry-profile.json")).unwrap();
        assert!(valid(&value));
        for (pointer, replacement) in [
            ("/schema", serde_json::json!(2)),
            ("/backend", serde_json::json!("pe")),
            ("/mode", serde_json::json!("run")),
            ("/runtime_catalog", serde_json::json!({"permit":"all"})),
            ("/target/path", serde_json::json!("../other")),
            ("/target/analysis_base", serde_json::json!(0x400000)),
            ("/target/coordinates", serde_json::json!("preferred-va")),
            ("/target/header/entry_offset", serde_json::json!(0x1a99b)),
            ("/target/relocation_count", serde_json::json!(11800)),
            ("/target/sha256", serde_json::json!("0".repeat(64))),
            ("/loader/protocol", serde_json::json!("arbitrary")),
            ("/loader/sha256", serde_json::json!("0".repeat(64))),
            (
                "/loader/path",
                serde_json::json!("toolchains/watcom10.0a/binnt/WPP386.EXE"),
            ),
        ] {
            let mut bad = value.clone();
            *bad.pointer_mut(pointer).unwrap() = replacement;
            assert!(!valid(&bad), "{pointer}");
        }
        let mut unknown = value.clone();
        unknown["instructions"] = serde_json::json!(["arbitrary"]);
        assert!(!valid(&unknown));
        let profile: Profile = serde_json::from_value(value).unwrap();
        assert!(validate(&profile, "different", &profile.target.sha256, 0x2fe00).is_err());
        assert!(validate(&profile, &profile.target.path, "different", 0x2fe00).is_err());
        assert!(
            validate(
                &profile,
                &profile.target.path,
                &profile.target.sha256,
                0x1a99b
            )
            .is_err()
        );
    }
}
