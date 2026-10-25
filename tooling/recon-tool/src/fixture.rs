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
#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(deny_unknown_fields)]
pub struct Fixture {
    #[serde(default = "version")]
    pub version: u32,
    #[serde(default)]
    pub args: Vec<String>,
    #[serde(default)]
    pub stdin: Option<Bytes>,
    #[serde(default)]
    pub stdin_kind: StdinKind,
    #[serde(default)]
    pub hold_stdin_open: bool,
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
}
fn version() -> u32 {
    VERSION
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
