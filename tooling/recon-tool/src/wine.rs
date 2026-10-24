//! Shared Wine transport for explicit original builds and reference runs.
use anyhow::{Context, Result, bail};
use std::{
    path::{Path, PathBuf},
    process::Command,
};
pub fn prefix() -> Result<PathBuf> {
    Ok(
        PathBuf::from(std::env::var_os("HOME").context("HOME unset")?)
            .join(".ai-tmp/vc1.0-capture/wine"),
    )
}
pub fn windows(path: &Path, repo: &Path) -> Result<String> {
    let scratch =
        PathBuf::from(std::env::var_os("HOME").context("HOME unset")?).join(".ai-tmp/msvc2");
    for (base, drive) in [
        (repo, 'R'),
        (scratch.as_path(), 'S'),
        (Path::new("/tmp/recon"), 'T'),
    ] {
        if let Ok(relative) = path.strip_prefix(base) {
            return Ok(format!(
                "{drive}:\\{}",
                relative
                    .to_str()
                    .context("non-UTF8 Windows path")?
                    .replace('/', "\\")
            ));
        }
    }
    bail!(
        "original path outside shared R:/S:/T: mappings: {}",
        path.display()
    )
}
pub fn command(repo: &Path) -> Result<Command> {
    let prefix = prefix()?;
    anyhow::ensure!(
        prefix.join("system.reg").is_file(),
        "shared Wine prefix is not initialized"
    );
    for (drive, expected) in [
        ("r:", repo.to_owned()),
        (
            "s:",
            PathBuf::from(std::env::var_os("HOME").context("HOME unset")?).join(".ai-tmp/msvc2"),
        ),
        ("t:", PathBuf::from("/tmp/recon")),
    ] {
        let actual = prefix
            .join("dosdevices")
            .join(drive)
            .canonicalize()
            .with_context(|| format!("shared Wine drive {drive}"))?;
        if actual != expected.canonicalize()? {
            bail!("shared Wine drive {drive} differs from expected mapping")
        }
    }
    let mut c = Command::new("wine");
    c.env("WINEPREFIX", prefix)
        .env("WINEARCH", "win32")
        .env("WINEDEBUG", "-all")
        .env_remove("CL")
        .env_remove("_CL_");
    Ok(c)
}
