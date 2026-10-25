//! Capture-owned harness bytes live outside the application's working tree.
use crate::{abi::Loaded, fixture::Fixture};
use anyhow::{Context, Result, ensure};
use std::{
    collections::BTreeSet,
    ffi::CString,
    fs,
    os::unix::fs::PermissionsExt,
    path::{Path, PathBuf},
};
pub fn root(f: &Fixture, workspace: &Path) -> Option<PathBuf> {
    (!f.assets.is_empty()).then(|| workspace.join(".recon-assets"))
}
pub fn materialize(f: &Fixture, workspace: &Path, cwd: &Path) -> Result<()> {
    let Some(root) = root(f, workspace) else {
        return Ok(());
    };
    ensure!(
        !root.starts_with(cwd) && !cwd.starts_with(&root),
        "harness assets must be outside the application working tree"
    );
    fs::create_dir(&root)?;
    for asset in &f.assets {
        let path = root.join(&asset.path);
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent)?;
        }
        fs::write(&path, &asset.data.0)?;
        fs::set_permissions(path, fs::Permissions::from_mode(0o444))?;
    }
    Ok(())
}
fn walk(
    root: &Path,
    path: &Path,
    files: &mut BTreeSet<String>,
    dirs: &mut BTreeSet<String>,
) -> Result<()> {
    for entry in fs::read_dir(root.join(path))? {
        let entry = entry?;
        let relative = path.join(entry.file_name());
        let name = relative
            .to_str()
            .context("nonUTF8 harness asset path")?
            .to_owned();
        let kind = entry.file_type()?;
        if kind.is_dir() {
            dirs.insert(name);
            walk(root, &relative, files, dirs)?;
        } else {
            ensure!(
                kind.is_file(),
                "harness assets cannot contain symlinks or special files"
            );
            files.insert(name);
        }
    }
    Ok(())
}
pub fn verify(f: &Fixture, workspace: &Path) -> Result<()> {
    let Some(root) = root(f, workspace) else {
        return Ok(());
    };
    let mut expected_dirs = BTreeSet::new();
    let mut expected_files = BTreeSet::new();
    for asset in &f.assets {
        expected_files.insert(asset.path.clone());
        let mut parent = Path::new(&asset.path).parent();
        while let Some(path) = parent.filter(|p| !p.as_os_str().is_empty()) {
            expected_dirs.insert(path.to_str().context("asset path UTF8")?.to_owned());
            parent = path.parent();
        }
        let path = root.join(&asset.path);
        let metadata = fs::symlink_metadata(&path)?;
        ensure!(
            metadata.is_file() && metadata.permissions().mode() & 0o222 == 0,
            "asset is not a read-only regular file: {}",
            asset.path
        );
        ensure!(
            fs::read(path)? == asset.data.0,
            "harness asset changed: {}",
            asset.path
        );
    }
    let mut files = BTreeSet::new();
    let mut dirs = BTreeSet::new();
    walk(&root, Path::new(""), &mut files, &mut dirs)?;
    ensure!(
        files == expected_files && dirs == expected_dirs,
        "harness asset inventory changed"
    );
    Ok(())
}
pub fn configure(loaded: &Loaded, f: &Fixture, workspace: &Path, cwd: &Path) -> Result<()> {
    type Configure =
        unsafe extern "C" fn(*const libc::c_char, *const libc::c_char, *const libc::c_char) -> i32;
    let configure = unsafe {
        loaded
            .library
            .get::<Configure>(b"recon_case_configure_paths\0")
    };
    match configure {
        Ok(configure) => {
            let cwd = CString::new(cwd.as_os_str().as_encoded_bytes())?;
            let logical = f
                .logical_cwd
                .as_ref()
                .map(|v| CString::new(v.as_str()))
                .transpose()?;
            let assets = root(f, workspace)
                .map(|v| CString::new(v.as_os_str().as_encoded_bytes()))
                .transpose()?;
            ensure!(
                unsafe {
                    configure(
                        cwd.as_ptr(),
                        logical.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
                        assets.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
                    )
                } != 0,
                "configure native fixture paths"
            );
            Ok(())
        }
        Err(_) => {
            ensure!(
                f.assets.is_empty(),
                "libbinary.so must include shared native fixture-path support for assets"
            );
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::fixture::{AssetFile, Bytes};
    #[test]
    fn separate_readonly_assets_reject_mutation_and_extra_entries() -> Result<()> {
        let root = crate::native::create_workspace()?;
        let result = (|| -> Result<()> {
            let cwd = root.join("work");
            fs::create_dir(&cwd)?;
            let mut f = Fixture::default();
            f.assets = vec![AssetFile {
                path: "nested/raw.bin".into(),
                data: Bytes(vec![0, 255, 42]),
            }];
            materialize(&f, &root, &cwd)?;
            verify(&f, &root)?;
            ensure!(fs::read_dir(&cwd)?.next().is_none(), "assets polluted cwd");
            let asset = root.join(".recon-assets/nested/raw.bin");
            fs::set_permissions(&asset, fs::Permissions::from_mode(0o644))?;
            fs::write(&asset, [0, 255, 43])?;
            fs::set_permissions(&asset, fs::Permissions::from_mode(0o444))?;
            ensure!(verify(&f, &root).is_err(), "modified asset accepted");
            fs::write(&asset, [0, 255, 42])?;
            fs::write(root.join(".recon-assets/extra"), [])?;
            ensure!(verify(&f, &root).is_err(), "extra asset accepted");
            Ok(())
        })();
        fs::remove_dir_all(root)?;
        result
    }
}
