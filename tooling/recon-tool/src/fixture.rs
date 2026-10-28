use anyhow::{Context, Result, bail};
use serde::{Deserialize, Deserializer, Serialize, Serializer};
use std::{
    collections::BTreeMap,
    fs,
    path::{Component, Path},
};

const MAGIC: &[u8; 8] = b"RECONIO\0";
const VERSION: u32 = 2;
#[derive(Clone, Default, Debug, PartialEq, Eq)]
pub struct Bytes(pub Vec<u8>);
impl Serialize for Bytes {
    fn serialize<S: Serializer>(&self, s: S) -> std::result::Result<S::Ok, S::Error> {
        if s.is_human_readable() {
            s.serialize_str(&format!("hex:{}", hex(&self.0)))
        } else {
            s.serialize_bytes(&self.0)
        }
    }
}
impl<'de> Deserialize<'de> for Bytes {
    fn deserialize<D: Deserializer<'de>>(d: D) -> std::result::Result<Self, D::Error> {
        struct V;
        impl<'de> serde::de::Visitor<'de> for V {
            type Value = Bytes;
            fn expecting(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
                f.write_str("text:... or hex:... string, or CBOR bytes")
            }
            fn visit_str<E: serde::de::Error>(self, s: &str) -> std::result::Result<Bytes, E> {
                if let Some(v) = s.strip_prefix("text:") {
                    Ok(Bytes(v.as_bytes().to_vec()))
                } else if let Some(v) = s.strip_prefix("hex:") {
                    unhex(v).map(Bytes).map_err(E::custom)
                } else {
                    Err(E::custom("byte strings require text: or hex: prefix"))
                }
            }
            fn visit_bytes<E: serde::de::Error>(self, b: &[u8]) -> std::result::Result<Bytes, E> {
                Ok(Bytes(b.to_vec()))
            }
            fn visit_byte_buf<E: serde::de::Error>(
                self,
                b: Vec<u8>,
            ) -> std::result::Result<Bytes, E> {
                Ok(Bytes(b))
            }
        }
        d.deserialize_any(V)
    }
}
pub fn hex(b: &[u8]) -> String {
    b.iter().map(|v| format!("{v:02x}")).collect()
}
pub fn unhex(s: &str) -> Result<Vec<u8>> {
    if s.len() % 2 != 0 || !s.is_ascii() {
        bail!("invalid hexadecimal bytes")
    }
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).context("invalid hexadecimal bytes"))
        .collect()
}
#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(deny_unknown_fields)]
pub struct InputFile {
    pub path: String,
    pub data: Bytes,
    #[serde(default)]
    pub mode: Option<u32>,
    #[serde(default)]
    pub modified_unix_seconds: Option<i64>,
}
#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(deny_unknown_fields)]
pub struct AssetFile {
    pub path: String,
    pub data: Bytes,
}
#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(deny_unknown_fields)]
pub struct OutputFile {
    pub path: String,
    #[serde(default)]
    pub absent: bool,
    #[serde(default)]
    pub data: Option<Bytes>,
    #[serde(default)]
    pub sha256: Option<String>,
    /// Only explicitly identified nondeterministic byte fields may be zeroed.
    #[serde(default)]
    pub zero_ranges: Vec<[u64; 2]>,
}
#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(tag = "kind", rename_all = "snake_case", deny_unknown_fields)]
pub enum Action {
    Signal { signal: i32 },
    Stdin { data: Bytes },
    CloseStdin,
}
#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(deny_unknown_fields)]
pub struct Event {
    #[serde(default)]
    pub after_ms: Option<u64>,
    #[serde(default)]
    pub stdout_contains: Option<Bytes>,
    pub action: Action,
}
#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(deny_unknown_fields)]
pub struct Dependency {
    pub path: String,
    pub sha256: String,
}
#[derive(Clone, Serialize, Deserialize, Debug, Default)]
#[serde(deny_unknown_fields)]
pub struct Partial {
    #[serde(default)]
    pub stdout: Option<Bytes>,
    #[serde(default)]
    pub stderr: Option<Bytes>,
    #[serde(default)]
    pub output_files: Vec<OutputFile>,
    #[serde(default)]
    pub reject_unexpected_files: bool,
}
#[derive(Clone, Copy, Serialize, Deserialize, Debug, Default, PartialEq, Eq)]
#[serde(rename_all = "snake_case")]
pub enum StdinKind {
    #[default]
    File,
    Pipe,
    Null,
}
/// Exact original CreateProcessA environment input, separate from CRT _environ.
#[derive(Clone, Serialize, Deserialize, Debug, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct ProcessOsEnvironment {
    pub ansi: Bytes,
    pub provider_sha256: String,
}
impl ProcessOsEnvironment {
    pub fn validate(&self) -> Result<()> {
        let bytes = &self.ansi.0;
        anyhow::ensure!(
            (3..=32767).contains(&bytes.len())
                && bytes.is_ascii()
                && bytes.ends_with(&[0, 0])
                && bytes[0] != 0,
            "process_os_environment requires a nonempty ASCII MULTISZ of at most 32767 bytes"
        );
        let mut names = std::collections::BTreeSet::new();
        for entry in bytes[..bytes.len() - 1].split_inclusive(|v| *v == 0) {
            let entry = &entry[..entry.len() - 1];
            let at = entry
                .iter()
                .enumerate()
                .skip(usize::from(entry.first() == Some(&b'=')))
                .find(|(_, v)| **v == b'=')
                .map(|(i, _)| i)
                .context("process_os_environment entry lacks assignment")?;
            anyhow::ensure!(
                at > 0 && names.insert(entry[..at].to_ascii_uppercase()),
                "process_os_environment has empty or duplicate names"
            );
        }
        anyhow::ensure!(
            self.provider_sha256.len() == 64
                && self
                    .provider_sha256
                    .bytes()
                    .all(|v| v.is_ascii_digit() || (b'a'..=b'f').contains(&v)),
            "process_os_environment requires a lowercase provider SHA256"
        );
        Ok(())
    }
    pub fn entries(&self) -> Result<Vec<std::ffi::CString>> {
        self.validate()?;
        self.ansi.0[..self.ansi.0.len() - 1]
            .split_inclusive(|v| *v == 0)
            .map(|entry| std::ffi::CString::new(&entry[..entry.len() - 1]).map_err(Into::into))
            .collect()
    }
    pub fn from_entries(entries: &[std::ffi::CString], provider_sha256: String) -> Result<Self> {
        let mut bytes = Vec::new();
        for entry in entries {
            bytes.extend_from_slice(entry.as_bytes_with_nul());
        }
        bytes.push(0);
        let result = Self {
            ansi: Bytes(bytes),
            provider_sha256,
        };
        result.validate()?;
        Ok(result)
    }
    /// Caller owns the returned process-lifetime storage through target exit.
    pub unsafe fn stage(&self, loaded: &crate::abi::Loaded) -> Result<Vec<u16>> {
        self.validate()?;
        let wide: Vec<u16> = self.ansi.0.iter().map(|v| u16::from(*v)).collect();
        let provider = std::ffi::CString::new(self.provider_sha256.as_str())?;
        type Bind = unsafe extern "C" fn(*const u16, usize, *const libc::c_char);
        let bind = unsafe {
            loaded
                .library
                .get::<Bind>(b"recon_bind_process_os_environment\0")
        }
        .context("process_os_environment requires shared native input staging")?;
        unsafe { bind(wide.as_ptr(), wide.len(), provider.as_ptr()) };
        Ok(wide)
    }
}
#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(deny_unknown_fields)]
pub struct Fixture {
    #[serde(default = "version")]
    pub version: u32,
    #[serde(default)]
    pub args: Vec<String>,
    /// Exact ANSI text passed to the original CreateProcessA, independent of argv.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub raw_command_line: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub process_os_environment: Option<ProcessOsEnvironment>,
    #[serde(default)]
    pub stdin: Option<Bytes>,
    #[serde(default)]
    pub stdin_kind: StdinKind,
    #[serde(default)]
    pub hold_stdin_open: bool,
    /// Give stderr the same destination as stdout before invoking the target.
    #[serde(default, skip_serializing_if = "is_false")]
    pub stderr_to_stdout: bool,
    #[serde(default)]
    pub stdout: Option<Bytes>,
    #[serde(default)]
    pub stderr: Option<Bytes>,
    #[serde(default)]
    pub exit_code: Option<i32>,
    #[serde(default)]
    pub input_files: Vec<InputFile>,
    #[serde(default)]
    pub assets: Vec<AssetFile>,
    #[serde(default)]
    pub output_files: Vec<OutputFile>,
    #[serde(default)]
    pub env: BTreeMap<String, Option<String>>,
    #[serde(default)]
    pub events: Vec<Event>,
    #[serde(default)]
    pub reject_unexpected_files: bool,
    #[serde(default = "timeout")]
    pub timeout_ms: u64,
    #[serde(default)]
    pub logical_cwd: Option<String>,
    #[serde(default)]
    pub logical_drive: Option<String>,
    #[serde(default)]
    pub partial: Option<Partial>,
    #[serde(default)]
    pub readonly_dependencies: Vec<Dependency>,
    /// Exact file membership, using the hashes already in readonly_dependencies.
    #[serde(default)]
    pub readonly_tree_roots: Vec<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub program_image: Option<crate::program_image::ProgramImage>,
}
fn version() -> u32 {
    VERSION
}
fn is_false(value: &bool) -> bool {
    !value
}
fn timeout() -> u64 {
    30000
}
impl Default for Fixture {
    fn default() -> Self {
        serde_json::from_str("{}").unwrap()
    }
}
pub fn safe_relative(s: &str) -> Result<()> {
    if s.is_empty()
        || s.contains('\0')
        || s.contains('\\')
        || !Path::new(s)
            .components()
            .all(|c| matches!(c, Component::Normal(_)))
    {
        bail!("unsafe relative path: {s:?}")
    }
    Ok(())
}
impl Fixture {
    pub fn validate(&self) -> Result<()> {
        if self.version != VERSION {
            bail!("unsupported fixture version {}", self.version)
        }
        if self.timeout_ms == 0 {
            bail!("timeout_ms must be positive")
        }
        if let Some(environment) = &self.process_os_environment {
            environment.validate()?;
        }
        if let Some(text) = &self.raw_command_line {
            if !text.is_ascii() || text.contains('\0') || text.len() > 32766 {
                bail!("raw_command_line must be ASCII without NUL and at most 32766 bytes")
            }
        }
        if let Some(d) = &self.logical_drive {
            if d.len() != 1 || !d.as_bytes()[0].is_ascii_alphabetic() {
                bail!("logical_drive must be one letter")
            }
        }
        if let Some(p) = &self.partial {
            let mut f = self.clone();
            f.partial = None;
            f.stdout = p.stdout.clone();
            f.stderr = p.stderr.clone();
            f.output_files = p.output_files.clone();
            f.reject_unexpected_files = p.reject_unexpected_files;
            f.validate()?;
        }
        let mut paths = std::collections::BTreeSet::new();
        for f in &self.input_files {
            safe_relative(&f.path)?;
            if !paths.insert(&f.path) {
                bail!("duplicate input {}", f.path)
            }
            if let Some(mode) = f.mode {
                if mode & !0o777 != 0 {
                    bail!("invalid input mode")
                }
            }
        }
        paths.clear();
        for asset in &self.assets {
            safe_relative(&asset.path)?;
            let canonical: std::path::PathBuf = Path::new(&asset.path).components().collect();
            if canonical.to_str() != Some(asset.path.as_str()) || !paths.insert(&asset.path) {
                bail!("duplicate/noncanonical asset path {}", asset.path)
            }
        }
        for asset in &self.assets {
            let mut parent = Path::new(&asset.path).parent();
            while let Some(path) = parent.filter(|p| !p.as_os_str().is_empty()) {
                if paths
                    .iter()
                    .any(|name| name.as_str() == path.to_str().unwrap_or(""))
                {
                    bail!("asset file/directory collision: {}", asset.path)
                }
                parent = path.parent();
            }
        }
        paths.clear();
        for f in &self.output_files {
            safe_relative(&f.path)?;
            if !paths.insert(&f.path) {
                bail!("duplicate output {}", f.path)
            }
            if f.absent && (f.data.is_some() || f.sha256.is_some() || !f.zero_ranges.is_empty()) {
                bail!("absent output {} has content assertions", f.path)
            }
            if let Some(s) = &f.sha256 {
                if unhex(s)?.len() != 32 {
                    bail!("invalid sha256 for {}", f.path)
                }
            }
            for [a, b] in &f.zero_ranges {
                if a >= b {
                    bail!("invalid zero range")
                }
            }
        }
        for (k, v) in &self.env {
            if k.is_empty()
                || k.contains(['=', '\0'])
                || v.as_ref().is_some_and(|s| s.contains('\0'))
            {
                bail!("invalid environment entry")
            }
        }
        for a in &self.args {
            if a.contains('\0') {
                bail!("NUL in argument")
            }
        }
        for d in &self.readonly_dependencies {
            if !Path::new(&d.path).is_absolute() || unhex(&d.sha256)?.len() != 32 {
                bail!("invalid readonly dependency")
            }
        }
        if let Some(image) = &self.program_image {
            image.validate()?;
            anyhow::ensure!(
                self.readonly_dependencies
                    .iter()
                    .any(|d| d.path == image.source_path && d.sha256 == image.source_sha256),
                "program image source is not a bound readonly dependency"
            );
            anyhow::ensure!(
                self.args.first() == Some(&image.logical_path),
                "program image path differs from captured argv0"
            );
        }
        let mut tree_roots = std::collections::BTreeSet::new();
        for root in &self.readonly_tree_roots {
            let path = Path::new(root);
            anyhow::ensure!(
                path.is_absolute()
                    && !root.contains('\0')
                    && !path
                        .components()
                        .any(|c| matches!(c, Component::ParentDir | Component::CurDir)),
                "readonly tree root must be absolute and normalized: {root}"
            );
            anyhow::ensure!(
                tree_roots.insert(path),
                "duplicate readonly tree root: {root}"
            );
            anyhow::ensure!(
                self.readonly_dependencies.iter().any(|d| Path::new(&d.path)
                    .strip_prefix(path)
                    .is_ok_and(|p| !p.as_os_str().is_empty())),
                "readonly tree root has no declared dependencies: {root}"
            );
        }
        if self.hold_stdin_open && self.stdin_kind != StdinKind::Pipe {
            bail!("hold_stdin_open requires stdin_kind: pipe")
        }
        if self.stdin_kind == StdinKind::Null && self.stdin.is_some() {
            bail!("stdin_kind: null cannot contain stdin bytes (omit stdin)")
        }
        let mut closed = false;
        for e in &self.events {
            if matches!(e.action, Action::Stdin { .. } | Action::CloseStdin)
                && self.stdin_kind != StdinKind::Pipe
            {
                bail!("stdin/close_stdin events require stdin_kind: pipe")
            }
            if matches!(e.action, Action::Stdin { .. }) && closed {
                bail!("stdin event follows close_stdin")
            }
            if matches!(e.action, Action::CloseStdin) {
                closed = true;
            }
            if e.after_ms.is_none() && e.stdout_contains.is_none() {
                bail!("event needs after_ms or stdout_contains")
            }
            if let Action::Signal { signal } = e.action {
                if signal <= 0 || signal >= 65 {
                    bail!("invalid signal")
                }
            }
        }
        Ok(())
    }
}
pub fn read(path: &Path) -> Result<Fixture> {
    let b = fs::read(path).with_context(|| format!("read {}", path.display()))?;
    decode(&b)
}
/// Decode the same captured bytes that a provenance caller has hashed.
pub fn decode(b: &[u8]) -> Result<Fixture> {
    if b.len() < 20 || &b[..8] != MAGIC || u32::from_le_bytes(b[8..12].try_into()?) != VERSION {
        bail!("invalid capture.bin header")
    }
    let len = u64::from_le_bytes(b[12..20].try_into()?);
    if len != b.len() as u64 - 20 {
        bail!("capture.bin length mismatch")
    }
    let mut cursor = std::io::Cursor::new(&b[20..]);
    let f: Fixture = ciborium::from_reader(&mut cursor)?;
    if cursor.position() != len {
        bail!("trailing CBOR data")
    }
    f.validate()?;
    Ok(f)
}
pub fn write(path: &Path, f: &Fixture) -> Result<()> {
    f.validate()?;
    let mut data = vec![];
    ciborium::into_writer(f, &mut data)?;
    let mut b = MAGIC.to_vec();
    b.extend(VERSION.to_le_bytes());
    b.extend((data.len() as u64).to_le_bytes());
    b.extend(data);
    fs::write(path, b)?;
    Ok(())
}

#[cfg(test)]
mod transport_tests {
    use super::*;
    #[test]
    fn stream_routing_preserves_legacy_format_and_round_trips() {
        // A version-2 payload without the new field is still valid unchanged.
        let mut payload = Vec::new();
        ciborium::into_writer(&serde_json::json!({"version": 2}), &mut payload).unwrap();
        let mut legacy = MAGIC.to_vec();
        legacy.extend(2u32.to_le_bytes());
        legacy.extend((payload.len() as u64).to_le_bytes());
        legacy.extend_from_slice(&payload);
        let mut f = decode(&legacy).unwrap();
        assert!(!f.stderr_to_stdout);
        assert!(
            serde_json::to_value(&f)
                .unwrap()
                .get("stderr_to_stdout")
                .is_none()
        );

        f.stderr_to_stdout = true;
        let value = serde_json::to_value(&f).unwrap();
        assert_eq!(value["stderr_to_stdout"], true);
        payload.clear();
        ciborium::into_writer(&f, &mut payload).unwrap();
        let mut merged = MAGIC.to_vec();
        merged.extend(VERSION.to_le_bytes());
        merged.extend((payload.len() as u64).to_le_bytes());
        merged.extend(payload);
        assert!(decode(&merged).unwrap().stderr_to_stdout);
        assert!(
            serde_json::from_value::<Fixture>(serde_json::json!({"stderr_to_stdout": "true"}))
                .is_err()
        );
    }
    #[test]
    fn raw_command_line_is_optional_bounded_exact_input() {
        let mut f = Fixture::default();
        assert!(
            serde_json::to_value(&f)
                .unwrap()
                .get("raw_command_line")
                .is_none()
        );
        for text in ["", "R:\\requested.exe\t ?  "] {
            f.raw_command_line = Some(text.into());
            assert!(f.validate().is_ok());
            let value = serde_json::to_value(&f).unwrap();
            assert_eq!(value["raw_command_line"], text);
        }
        f.raw_command_line = Some("x".repeat(32766));
        assert!(f.validate().is_ok());
        for text in ["x\0y".into(), "é".into(), "x".repeat(32767)] {
            f.raw_command_line = Some(text);
            assert!(f.validate().is_err());
        }
    }
    #[test]
    fn null_input_is_a_device_not_empty_file_bytes() {
        let f: Fixture = serde_json::from_value(serde_json::json!({"stdin_kind":"null"})).unwrap();
        assert!(f.validate().is_ok());
        for value in [
            serde_json::json!({"stdin_kind":"null","stdin":"hex:"}),
            serde_json::json!({"stdin_kind":"null","events":[{"after_ms":0,"action":{"kind":"close_stdin"}}]}),
        ] {
            let f: Fixture = serde_json::from_value(value).unwrap();
            assert!(f.validate().is_err());
        }
    }
}

#[cfg(test)]
mod asset_tests {
    use super::*;
    #[test]
    fn asset_paths_are_canonical_and_collision_free() {
        for paths in [
            vec!["../x"],
            vec!["/x"],
            vec!["x\\y"],
            vec!["x//y"],
            vec!["x/"],
            vec!["x\0y"],
            vec!["x", "x"],
            vec!["x", "x/y"],
        ] {
            let mut f = Fixture::default();
            f.assets = paths
                .into_iter()
                .map(|path| AssetFile {
                    path: path.into(),
                    data: Bytes(vec![]),
                })
                .collect();
            assert!(f.validate().is_err(), "{f:?}");
        }
        let mut f = Fixture::default();
        f.assets = vec![AssetFile {
            path: "nested/empty.bin".into(),
            data: Bytes(vec![]),
        }];
        assert!(f.validate().is_ok());
    }
}

#[cfg(test)]
mod process_os_environment_tests {
    use super::*;
    fn input(bytes: &[u8]) -> ProcessOsEnvironment {
        ProcessOsEnvironment {
            ansi: Bytes(bytes.to_vec()),
            provider_sha256: "ab".repeat(32),
        }
    }
    #[test]
    fn process_os_environment_is_optional_and_preserves_exact_multisz() {
        assert!(
            serde_json::to_value(Fixture::default())
                .unwrap()
                .get("process_os_environment")
                .is_none()
        );
        let original = input(b"=R:=R:\\case\0A=one\0EMPTY=\0\0");
        original.validate().unwrap();
        let mut f = Fixture::default();
        f.process_os_environment = Some(original.clone());
        let mut encoded = vec![];
        ciborium::into_writer(&f, &mut encoded).unwrap();
        let decoded: Fixture = ciborium::from_reader(encoded.as_slice()).unwrap();
        assert_eq!(decoded.process_os_environment, Some(original));
    }
    #[test]
    fn process_os_environment_rejects_ambiguous_or_invalid_blocks() {
        for bytes in [
            b"".as_slice(),
            b"\0\0",
            b"A=x\0",
            b"A=x\0\0B=y\0\0",
            b"A=x\0a=y\0\0",
            b"A\0\0",
            b"=x\0\0",
            b"A=\xff\0\0",
        ] {
            assert!(input(bytes).validate().is_err(), "{bytes:?}");
        }
        assert!(input(&[b'A'; 32768]).validate().is_err());
        for hash in ["a".repeat(63), "AB".repeat(32), "gg".repeat(32)] {
            let mut f = input(b"A=x\0\0");
            f.provider_sha256 = hash;
            assert!(f.validate().is_err());
        }
    }
    #[test]
    fn process_os_environment_child_uses_actual_accepted_vector() {
        let entries = [
            std::ffi::CString::new("A=one").unwrap(),
            std::ffi::CString::new("LINK_REPRO_NAME=\\deflib.rsp").unwrap(),
        ];
        let child = ProcessOsEnvironment::from_entries(&entries, "ab".repeat(32)).unwrap();
        assert_eq!(child.ansi.0, b"A=one\0LINK_REPRO_NAME=\\deflib.rsp\0\0");
        assert_ne!(child, input(b"A=one\0\0"));
        let vector = child.entries().unwrap();
        assert_eq!(vector.len(), 2);
        assert_eq!(vector[1].as_bytes(), b"LINK_REPRO_NAME=\\deflib.rsp");
        drop(child);
        assert_eq!(vector[0].as_bytes(), b"A=one");
    }
}
