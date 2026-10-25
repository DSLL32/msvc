//! Exact capture-owned staged program input, stored as source plus INT3 ranges.
use crate::fixture::{self, Fixture};
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    fs,
    io::Write,
    os::unix::fs::PermissionsExt,
    path::{Component, Path, PathBuf},
};

const MAX_IMAGE: u64 = 32 * 1024 * 1024;
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ProgramImage {
    pub logical_path: String,
    pub source_path: String,
    pub source_sha256: String,
    pub sha256: String,
    pub size: u64,
    pub int3_ranges: Vec<[u64; 2]>,
}
fn hash(bytes: &[u8]) -> String {
    fixture::hex(&Sha256::digest(bytes))
}
impl ProgramImage {
    fn relative(&self) -> Result<PathBuf> {
        let bytes = self.logical_path.as_bytes();
        ensure!(
            bytes.len() > 3
                && bytes[0].eq_ignore_ascii_case(&b'T')
                && bytes[1] == b':'
                && matches!(bytes[2], b'/' | b'\\')
                && self.logical_path.is_ascii(),
            "program image requires its observed drive-absolute T: path"
        );
        let relative = self.logical_path[3..].replace('\\', "/");
        fixture::safe_relative(&relative)?;
        Ok(relative.into())
    }
    pub fn validate(&self) -> Result<()> {
        self.relative()?;
        let source = Path::new(&self.source_path);
        ensure!(
            source.is_absolute()
                && !self.source_path.contains('\0')
                && !source
                    .components()
                    .any(|c| matches!(c, Component::ParentDir | Component::CurDir)),
            "program image source must be an absolute normalized readonly dependency"
        );
        ensure!(
            fixture::unhex(&self.source_sha256)?.len() == 32
                && fixture::unhex(&self.sha256)?.len() == 32,
            "invalid program image hash"
        );
        ensure!(
            self.size > 0 && self.size <= MAX_IMAGE && self.int3_ranges.len() <= 65536,
            "program image exceeds input bounds"
        );
        let mut end = 0;
        for [start, limit] in &self.int3_ranges {
            ensure!(
                end <= *start && start < limit && *limit <= self.size,
                "invalid or overlapping program image INT3 ranges"
            );
            end = *limit;
        }
        Ok(())
    }
    pub fn capture(
        source: &Path,
        original: &[u8],
        staged: &[u8],
        logical_path: String,
    ) -> Result<Self> {
        ensure!(
            original.len() == staged.len(),
            "staged program changed file length; only INT3 masking is supported"
        );
        let mut ranges = Vec::new();
        let mut cursor = 0;
        while cursor < original.len() {
            if original[cursor] == staged[cursor] {
                cursor += 1;
                continue;
            }
            let start = cursor;
            while cursor < original.len() && original[cursor] != staged[cursor] {
                ensure!(
                    staged[cursor] == 0xcc,
                    "staged program has a non-INT3 transform at raw offset {cursor:x}"
                );
                cursor += 1;
            }
            ranges.push([start as u64, cursor as u64]);
        }
        let value = Self {
            logical_path,
            source_path: source
                .canonicalize()?
                .to_str()
                .context("program source UTF8")?
                .into(),
            source_sha256: hash(original),
            sha256: hash(staged),
            size: original.len() as u64,
            int3_ranges: ranges,
        };
        value.validate()?;
        Ok(value)
    }
    fn path(&self, root: &Path, cwd: &Path) -> Result<PathBuf> {
        self.validate()?;
        let path = root.join(self.relative()?);
        ensure!(
            !path.starts_with(cwd) && !cwd.starts_with(&path),
            "program image must be outside application cwd"
        );
        Ok(path)
    }
    fn bytes(&self) -> Result<Vec<u8>> {
        ensure!(
            fs::metadata(&self.source_path)?.len() == self.size,
            "program source length changed"
        );
        let mut bytes = fs::read(&self.source_path)?;
        ensure!(
            hash(&bytes) == self.source_sha256,
            "program source hash changed"
        );
        for [start, end] in &self.int3_ranges {
            bytes[*start as usize..*end as usize].fill(0xcc);
        }
        ensure!(
            hash(&bytes) == self.sha256,
            "program mask recipe differs from captured staged image"
        );
        Ok(bytes)
    }
}
pub fn materialize(f: &Fixture, root: &Path, cwd: &Path) -> Result<()> {
    let Some(image) = &f.program_image else {
        return Ok(());
    };
    let path = image.path(root, cwd)?;
    let parent = path.parent().context("program image parent")?;
    let mut ancestor = parent;
    while !ancestor.exists() {
        ancestor = ancestor.parent().context("program image ancestor")?;
    }
    let canonical_root = root.canonicalize()?;
    ensure!(
        ancestor.canonicalize()?.starts_with(&canonical_root),
        "program image parent escapes workspace"
    );
    let bytes = image.bytes()?;
    fs::create_dir_all(parent)?;
    ensure!(
        parent.canonicalize()?.starts_with(&canonical_root),
        "program image parent escapes workspace"
    );
    let mut file = fs::OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&path)?;
    file.write_all(&bytes)?;
    file.set_permissions(fs::Permissions::from_mode(0o444))?;
    Ok(())
}
pub fn verify(f: &Fixture, root: &Path, cwd: &Path) -> Result<()> {
    let Some(image) = &f.program_image else {
        return Ok(());
    };
    let path = image.path(root, cwd)?;
    ensure!(
        path.canonicalize()?.starts_with(root.canonicalize()?),
        "program image escapes workspace"
    );
    let metadata = fs::symlink_metadata(&path)?;
    ensure!(
        metadata.is_file()
            && metadata.permissions().mode() & 0o222 == 0
            && metadata.len() == image.size,
        "program image is not the unchanged readonly input"
    );
    ensure!(
        hash(&fs::read(path)?) == image.sha256,
        "program image input changed"
    );
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{
        os::unix::fs::symlink,
        time::{SystemTime, UNIX_EPOCH},
    };
    struct Case {
        parent: PathBuf,
        root: PathBuf,
        cwd: PathBuf,
        source: PathBuf,
        original: Vec<u8>,
        staged: Vec<u8>,
        fixture: Fixture,
    }
    impl Case {
        fn new() -> Self {
            let parent = PathBuf::from(std::env::var_os("HOME").unwrap())
                .join(".ai-tmp")
                .join(format!(
                    "program-image-{}-{}",
                    std::process::id(),
                    SystemTime::now()
                        .duration_since(UNIX_EPOCH)
                        .unwrap()
                        .as_nanos()
                ));
            let root = parent.join("workspace");
            let cwd = root.join("cwd");
            fs::create_dir_all(&cwd).unwrap();
            let source = parent.join("original.exe");
            let original: Vec<u8> = (0..128).collect();
            let mut staged = original.clone();
            staged[4..8].fill(0xcc);
            staged[16..20].fill(0xcc);
            fs::write(&source, &original).unwrap();
            let image =
                ProgramImage::capture(&source, &original, &staged, "T:\\run-old\\WLIB.EXE".into())
                    .unwrap();
            let mut fixture = Fixture::default();
            fixture.args = vec![image.logical_path.clone(), "?".into()];
            fixture.readonly_dependencies.push(fixture::Dependency {
                path: image.source_path.clone(),
                sha256: image.source_sha256.clone(),
            });
            fixture.program_image = Some(image);
            Self {
                parent,
                root,
                cwd,
                source,
                original,
                staged,
                fixture,
            }
        }
    }
    impl Drop for Case {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.parent);
        }
    }
    #[test]
    fn exact_masked_file_is_a_readonly_sibling_input_without_a_blob() {
        let case = Case::new();
        case.fixture.validate().unwrap();
        assert_eq!(
            case.fixture.program_image.as_ref().unwrap().int3_ranges,
            [[4, 8], [16, 20]]
        );
        materialize(&case.fixture, &case.root, &case.cwd).unwrap();
        let path = case.root.join("run-old/WLIB.EXE");
        assert_eq!(fs::read(&path).unwrap(), case.staged);
        assert_eq!(fs::read(&case.source).unwrap(), case.original);
        assert_eq!(fs::read_dir(&case.cwd).unwrap().count(), 0);
        verify(&case.fixture, &case.root, &case.cwd).unwrap();
        // The captured ranges are the entire recipe: no current registry is used.
        let packed = serde_json::to_vec(&case.fixture).unwrap();
        assert!(packed.len() < 4096);
        let replay: Fixture = serde_json::from_slice(&packed).unwrap();
        assert_eq!(replay.program_image.unwrap().bytes().unwrap(), case.staged);
        fs::set_permissions(&path, fs::Permissions::from_mode(0o644)).unwrap();
        assert!(verify(&case.fixture, &case.root, &case.cwd).is_err());
        fs::write(&path, b"mutated").unwrap();
        fs::set_permissions(&path, fs::Permissions::from_mode(0o444)).unwrap();
        assert!(verify(&case.fixture, &case.root, &case.cwd).is_err());
    }
    #[test]
    fn arbitrary_transforms_source_changes_and_wrong_hashes_are_rejected() {
        let case = Case::new();
        let mut bad = case.staged.clone();
        bad[32] = 0x90;
        assert!(
            ProgramImage::capture(
                &case.source,
                &case.original,
                &bad,
                "T:\\run-old\\WLIB.EXE".into()
            )
            .is_err()
        );
        assert!(
            ProgramImage::capture(
                &case.source,
                &case.original,
                &bad[..127],
                "T:\\run-old\\WLIB.EXE".into()
            )
            .is_err()
        );
        let mut image = case.fixture.program_image.clone().unwrap();
        image.sha256 = "00".repeat(32);
        assert!(image.bytes().is_err());
        fs::write(&case.source, &case.staged).unwrap();
        assert!(
            case.fixture
                .program_image
                .as_ref()
                .unwrap()
                .bytes()
                .is_err()
        );
    }
    #[test]
    fn paths_ranges_and_dependency_bindings_are_checked() {
        let case = Case::new();
        for path in [
            "T:\\..\\escape.exe",
            "T:\\cwd\\inside.exe",
            "R:\\source.exe",
            "T:relative.exe",
        ] {
            let mut fixture = case.fixture.clone();
            fixture.program_image.as_mut().unwrap().logical_path = path.into();
            assert!(
                materialize(&fixture, &case.root, &case.cwd).is_err(),
                "{path}"
            );
        }
        for ranges in [vec![[4, 8], [7, 12]], vec![[4, 129]], vec![[8, 4]]] {
            let mut image = case.fixture.program_image.clone().unwrap();
            image.int3_ranges = ranges;
            assert!(image.validate().is_err());
        }
        let mut fixture = case.fixture.clone();
        fixture.readonly_dependencies.clear();
        assert!(fixture.validate().is_err());
        let mut fixture = case.fixture.clone();
        fixture.args[0] = "another.exe".into();
        assert!(fixture.validate().is_err());
    }
    #[test]
    fn symlink_escape_is_rejected_before_creating_outside_directories() {
        let mut case = Case::new();
        let outside = case.parent.join("outside");
        fs::create_dir(&outside).unwrap();
        symlink(&outside, case.root.join("run-old")).unwrap();
        case.fixture.program_image.as_mut().unwrap().logical_path =
            "T:\\run-old\\deeper\\WLIB.EXE".into();
        assert!(materialize(&case.fixture, &case.root, &case.cwd).is_err());
        assert!(!outside.join("deeper").exists());
    }
}
