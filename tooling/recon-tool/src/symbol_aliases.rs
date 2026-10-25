//! Metadata-only alias generation; no Wine, target code or extra function body.
use anyhow::{Result, ensure};
use std::{
    path::{Path, PathBuf},
    process::Command,
};
#[derive(clap::Args)]
pub struct Options {
    pub map: PathBuf,
    #[arg(long)]
    pub coff: PathBuf,
    #[arg(long)]
    pub native_rsp: PathBuf,
}
pub fn build(options: Options) -> Result<()> {
    let status = Command::new("python3")
        .arg(Path::new(env!("CARGO_MANIFEST_DIR")).join("symbol-aliases.py"))
        .arg(options.map)
        .arg("--coff")
        .arg(options.coff)
        .arg("--native-rsp")
        .arg(options.native_rsp)
        .status()?;
    ensure!(status.success(), "symbol-alias generation failed: {status}");
    Ok(())
}
