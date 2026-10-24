//! Original-toolchain selection. Native run/test never calls this module.
use anyhow::{Context, Result, bail};
use clap::Args;
use std::{
    ffi::OsString,
    fs,
    os::unix::process::CommandExt,
    path::{Path, PathBuf},
    process::Command,
};

#[derive(Args)]
pub struct Tool {
    /// Repository root; inferred from cwd or recon-tool's location by default.
    #[arg(long)]
    pub repo: Option<PathBuf>,
    pub toolchain: String,
    pub tool: String,
    /// Original arguments, passed unchanged (use -- before tool switches).
    #[arg(trailing_var_arg = true, allow_hyphen_values = true)]
    pub args: Vec<OsString>,
}

fn repository(explicit: Option<&Path>) -> Result<PathBuf> {
    if let Some(root) = explicit {
        return root.canonicalize().context("repository root");
    }
    for start in [std::env::current_dir()?, std::env::current_exe()?] {
        for root in start.ancestors() {
            if root.join("toolchains").is_dir()
                && root.join("tooling/recon-tool/Cargo.toml").is_file()
            {
                return Ok(root.to_owned());
            }
        }
    }
    bail!("cannot locate repository; supply --repo")
}

fn executable(directory: &Path, name: &str) -> Result<PathBuf> {
    anyhow::ensure!(
        !name.is_empty() && !name.contains(['/', '\\']) && name != "." && name != "..",
        "tool must be a filename"
    );
    let name = if Path::new(name).extension().is_none() {
        format!("{name}.EXE")
    } else {
        name.to_owned()
    };
    for entry in fs::read_dir(directory)
        .with_context(|| format!("tool directory {}", directory.display()))?
    {
        let entry = entry?;
        if entry
            .file_name()
            .to_str()
            .is_some_and(|file| file.eq_ignore_ascii_case(&name))
            && entry.file_type()?.is_file()
        {
            return Ok(entry.path());
        }
    }
    bail!("tool {name} not found in {}", directory.display())
}

pub fn command(repo: &Path, name: &str, tool: &str) -> Result<Command> {
    anyhow::ensure!(
        matches!(name, "1.0-nt" | "5.10" | "watcom10.0a"),
        "unknown toolchain {name}; supported: 1.0-nt, 5.10, watcom10.0a"
    );
    let root = repo.join("toolchains").join(name);
    let mut command = crate::wine::command(repo)?;
    let program;
    if name == "watcom10.0a" {
        let patched_linker =
            tool.eq_ignore_ascii_case("WLINK") || tool.eq_ignore_ascii_case("WLINK.EXE");
        program = executable(
            &root.join(if patched_linker { ".patched" } else { "binnt" }),
            tool,
        )?;
        if patched_linker {
            use sha2::{Digest, Sha256};
            anyhow::ensure!(
                crate::fixture::hex(&Sha256::digest(fs::read(&program)?))
                    == "509c749b2258029a25a0071e34fa13b76a2fd4b267591a2bb12e9a7be5fd665b",
                "Watcom WLINK GS-patched image differs"
            );
        }
        for key in [
            "WCC",
            "WCC386",
            "WPP",
            "WPP386",
            "WLINK",
            "WLINK_LNK",
            "INCLUDE",
            "LIB",
        ] {
            command.env_remove(key);
        }
        command
            .env("WATCOM", crate::wine::windows(&root, repo)?)
            .env("WINEPATH", crate::wine::windows(&root.join("binnt"), repo)?);
    } else {
        program = executable(&root.join("bin"), tool)?;
        command
            .env(
                "INCLUDE",
                crate::wine::windows(&root.join("include"), repo)?,
            )
            .env("LIB", crate::wine::windows(&root.join("lib"), repo)?)
            .env("WINEPATH", crate::wine::windows(&root.join("bin"), repo)?);
        if name == "5.10" {
            command.env("WINEDLLOVERRIDES", "msvcrt=n");
        }
    }
    command.arg(crate::wine::windows(&program, repo)?);
    Ok(command)
}

pub fn run(options: Tool) -> Result<()> {
    let repo = repository(options.repo.as_deref())?;
    let mut command = command(&repo, &options.toolchain, &options.tool)?;
    command.args(options.args);
    // Replace the launcher, preserving the tool's streams, status and signals.
    Err(command.exec()).context("execute original tool through Wine")
}
