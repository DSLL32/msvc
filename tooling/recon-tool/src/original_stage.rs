//! Private progress images retain their original toolchain-local dependencies.
use crate::{original_image::Image, original_loader};
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::BTreeMap,
    fs,
    path::{Path, PathBuf},
};
#[derive(Serialize, Deserialize)]
pub struct Provenance {
    pub source: PathBuf,
    pub staged: PathBuf,
    pub sha256: String,
    pub source_sha256: String,
    pub role: String,
}
pub struct Staged {
    pub host: PathBuf,
    pub files: Vec<Provenance>,
    pub provider: Option<PathBuf>,
}
fn local(directory: &Path, name: &str) -> Result<Option<PathBuf>> {
    ensure!(
        !name.is_empty() && !name.contains(['/', '\\', ':']) && !matches!(name, "." | ".."),
        "invalid imported DLL filename {name:?}"
    );
    let mut matches = vec![];
    for entry in fs::read_dir(directory)? {
        let entry = entry?;
        if entry
            .file_name()
            .to_str()
            .is_some_and(|file| file.eq_ignore_ascii_case(name))
        {
            ensure!(
                entry.file_type()?.is_file(),
                "toolchain dependency is not a regular file"
            );
            matches.push(entry.path());
        }
    }
    ensure!(matches.len() <= 1, "ambiguous toolchain dependency {name}");
    Ok(matches.pop())
}
fn copy(
    source: &Path,
    root: &Path,
    role: &str,
    known: &mut BTreeMap<String, PathBuf>,
    files: &mut Vec<Provenance>,
) -> Result<PathBuf> {
    let source = source.canonicalize()?;
    let name = source
        .file_name()
        .and_then(|s| s.to_str())
        .context("non-ASCII dependency filename")?;
    ensure!(name.is_ascii(), "non-ASCII dependency filename");
    let key = name.to_ascii_uppercase();
    if let Some(previous) = known.get(&key) {
        ensure!(
            previous == &source,
            "host/target/dependency filename collision: {name}"
        );
        return Ok(root.join(name));
    }
    ensure!(
        local(root, name)?.is_none(),
        "private capture filename collision: {name}"
    );
    let bytes = fs::read(&source)?;
    let staged = root.join(name);
    let mut file = fs::OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&staged)?;
    std::io::Write::write_all(&mut file, &bytes)?;
    files.push(Provenance {
        source: source.clone(),
        staged: staged.clone(),
        sha256: crate::fixture::hex(&Sha256::digest(&bytes)),
        source_sha256: crate::fixture::hex(&Sha256::digest(&bytes)),
        role: role.into(),
    });
    known.insert(key, source);
    Ok(staged)
}
/// The exact finite source closure used by private staging. Read-only callers
/// may bind these inputs before capture; absence of local DLLs still means Wine.
pub fn input_sources(
    repo: &Path,
    declared: &Path,
    program: &Path,
    host: &Path,
    dll: bool,
    native_msvcrt: bool,
) -> Result<(Option<PathBuf>, Vec<PathBuf>)> {
    let directory = declared
        .parent()
        .context("original toolchain directory")?
        .canonicalize()?;
    // Only declared imports found beside the selected toolchain binary are staged.
    // System DLLs absent from that directory remain the Wine loader's concern.
    let mut sources = vec![];
    let mut selected_provider = None;
    let mut queue = vec![program.to_owned()];
    if dll {
        queue.push(host.to_owned());
    }
    if native_msvcrt {
        ensure!(
            directory.starts_with(repo.join("toolchains").canonicalize()?),
            "native MSVCRT requires an original toolchain directory"
        );
        let provider =
            local(&directory, "MSVCRT.DLL")?.context("selected toolchain lacks MSVCRT.DLL")?;
        selected_provider = Some(provider.clone());
        queue.push(provider);
    }
    let mut seen = std::collections::BTreeSet::new();
    while let Some(source) = queue.pop() {
        let source = source.canonicalize()?;
        if !seen.insert(source.clone()) {
            continue;
        }
        ensure!(
            seen.len() <= 128,
            "toolchain dependency closure exceeds 128 images"
        );
        let bytes = fs::read(&source)?;
        let image = Image::parse(&bytes)?;
        for name in original_loader::imports(&image, &bytes)? {
            if let Some(dependency) = local(&directory, &name)? {
                sources.push(dependency.clone());
                queue.push(dependency);
            }
        }
    }
    Ok((selected_provider, sources))
}

pub fn prepare(
    repo: &Path,
    declared: &Path,
    program: &Path,
    target: &Path,
    host: &Path,
    root: &Path,
    dll: bool,
    private_image: bool,
    native_msvcrt: bool,
) -> Result<Staged> {
    if !private_image {
        return Ok(Staged {
            host: host.to_owned(),
            files: vec![],
            provider: None,
        });
    }
    let mut known = BTreeMap::new();
    let mut files = vec![Provenance {
        source: program.canonicalize()?,
        staged: target.to_owned(),
        sha256: crate::fixture::hex(&Sha256::digest(fs::read(target)?)),
        source_sha256: crate::fixture::hex(&Sha256::digest(fs::read(program)?)),
        role: "target".into(),
    }];
    let mut selected_provider = None;
    let target_name = target
        .file_name()
        .and_then(|s| s.to_str())
        .context("target filename")?;
    known.insert(target_name.to_ascii_uppercase(), program.canonicalize()?);
    let staged_host = if dll {
        let name = host
            .file_name()
            .and_then(|s| s.to_str())
            .context("host filename")?;
        ensure!(
            !name.eq_ignore_ascii_case(target_name),
            "DLL/host filename collision"
        );
        copy(host, root, "host", &mut known, &mut files)?
    } else {
        target.to_owned()
    };
    let (provider, dependencies) =
        input_sources(repo, declared, program, host, dll, native_msvcrt)?;
    if let Some(provider) = provider {
        selected_provider = Some(copy(
            &provider,
            root,
            "native-msvcrt",
            &mut known,
            &mut files,
        )?);
    }
    for dependency in dependencies {
        copy(
            &dependency,
            root,
            "toolchain-import",
            &mut known,
            &mut files,
        )?;
    }
    Ok(Staged {
        host: staged_host,
        files,
        provider: selected_provider,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    fn repo() -> PathBuf {
        std::env::var_os("RECON_TOOL_TEST_REPO")
            .map(PathBuf::from)
            .unwrap_or_else(|| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../.."))
    }
    #[test]
    fn static_host_reuses_masked_target_and_stages_only_declared_local_imports() -> Result<()> {
        let repo = repo().canonicalize()?;
        let dir = repo.join("toolchains/5.10/bin");
        let root = crate::native::create_workspace()?;
        let result = (|| -> Result<()> {
            let original = dir.join("RCDLL.DLL");
            let target = root.join("RCDLL.DLL");
            fs::write(&target, b"already masked target")?;
            let staged = prepare(
                &repo,
                &original,
                &original,
                &target,
                &dir.join("RC.EXE"),
                &root,
                true,
                true,
                true,
            )?;
            assert_eq!(fs::read(&target)?, b"already masked target");
            assert_eq!(fs::read(staged.host)?, fs::read(dir.join("RC.EXE"))?);
            assert_eq!(staged.provider, Some(root.join("MSVCRT.DLL")));
            assert_eq!(
                fs::read(root.join("MSVCRT.DLL"))?,
                fs::read(dir.join("MSVCRT.DLL"))?
            );
            assert!(!root.join("KERNEL32.dll").exists());
            Ok(())
        })();
        fs::remove_dir_all(root)?;
        result
    }
    #[test]
    fn cvpack_import_closure_and_case_collisions() -> Result<()> {
        let repo = repo().canonicalize()?;
        let dir = repo.join("toolchains/5.10/bin");
        let root = crate::native::create_workspace()?;
        let result = (|| -> Result<()> {
            let original = dir.join("CVPACK.EXE");
            let target = root.join("CVPACK.EXE");
            fs::copy(&original, &target)?;
            let staged = prepare(
                &repo, &original, &original, &target, &target, &root, false, true, true,
            )?;
            assert!(staged.files.iter().any(|v| {
                v.source
                    .file_name()
                    .unwrap()
                    .to_string_lossy()
                    .eq_ignore_ascii_case("MSPDB50.DLL")
            }));
            fs::write(root.join("collision.dll"), [])?;
            fs::write(root.join("COLLISION.DLL"), [])?;
            assert!(local(&root, "collision.dll").is_err());
            assert!(local(&root, "../MSVCRT.DLL").is_err());
            let other = root.join("other");
            fs::create_dir(&other)?;
            fs::write(other.join("MSVCRT.DLL"), [])?;
            let mut known =
                BTreeMap::from([("MSVCRT.DLL".into(), dir.join("MSVCRT.DLL").canonicalize()?)]);
            assert!(
                copy(
                    &other.join("MSVCRT.DLL"),
                    &root,
                    "collision",
                    &mut known,
                    &mut vec![]
                )
                .is_err()
            );
            Ok(())
        })();
        fs::remove_dir_all(root)?;
        result
    }
}
