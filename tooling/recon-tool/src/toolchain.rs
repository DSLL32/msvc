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

pub(crate) fn repository(explicit: Option<&Path>) -> Result<PathBuf> {
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

fn watcom_executable(root: &Path, tool: &str) -> Result<PathBuf> {
    let linker = tool.eq_ignore_ascii_case("WLINK") || tool.eq_ignore_ascii_case("WLINK.EXE");
    let assembler = tool.eq_ignore_ascii_case("WASM") || tool.eq_ignore_ascii_case("WASM.EXE");
    let directory = if linker {
        ".patched"
    } else if assembler {
        ".patched/binnt"
    } else {
        "binnt"
    };
    let program = executable(&root.join(directory), tool)?;
    if linker || assembler {
        use sha2::{Digest, Sha256};
        let (expected, label) = if linker {
            (
                "509c749b2258029a25a0071e34fa13b76a2fd4b267591a2bb12e9a7be5fd665b",
                "WLINK GS-patched image",
            )
        } else {
            (
                "8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d",
                "WASM loader",
            )
        };
        anyhow::ensure!(
            crate::fixture::hex(&Sha256::digest(fs::read(&program)?)) == expected,
            "Watcom {label} differs"
        );
        if assembler {
            // This loader derives the sibling binb path from its own location.
            // Qualify the same unmasked pair used by ordinary CF capture.
            let payload = executable(&root.join(".patched/binb"), "WASM")?;
            anyhow::ensure!(
                crate::fixture::hex(&Sha256::digest(fs::read(payload)?))
                    == "f7189b9de3de853139d381816ab5ed9aefdd6a4fd988e29a023f1f5fbd640de9",
                "Watcom WASM GS-patched CF payload differs"
            );
        }
    }
    Ok(program)
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
        program = watcom_executable(&root, tool)?;
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

#[cfg(test)]
mod tests {
    use super::*;
    fn shipped_root() -> PathBuf {
        Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../../toolchains/watcom10.0a")
            .canonicalize()
            .unwrap()
    }
    #[test]
    fn qualified_wasm_selection_preserves_other_tools_and_launch_inputs() -> Result<()> {
        let root = shipped_root();
        let program = root.join(".patched/binnt/WASM.EXE");
        for name in ["WASM", "wasm", "WASM.EXE", "wasm.exe"] {
            assert_eq!(watcom_executable(&root, name)?, program);
        }
        assert_eq!(
            watcom_executable(&root, "WLINK")?,
            root.join(".patched/WLINK.EXE")
        );
        assert_eq!(
            watcom_executable(&root, "WCC386")?,
            root.join("binnt/WCC386.EXE")
        );
        let repo = root.parent().unwrap().parent().unwrap();
        let mut launch = command(repo, "watcom10.0a", "WASM")?;
        let tail = ["-q", "-fo=out.obj", "input.asm"];
        launch.args(tail);
        let args: Vec<_> = launch.get_args().map(|v| v.to_str().unwrap()).collect();
        assert_eq!(
            args,
            [
                r"R:\toolchains\watcom10.0a\.patched\binnt\WASM.EXE",
                tail[0],
                tail[1],
                tail[2]
            ]
        );
        let env: std::collections::HashMap<_, _> = launch.get_envs().collect();
        assert_eq!(
            env[std::ffi::OsStr::new("WINEPREFIX")],
            Some(crate::wine::prefix()?.as_os_str())
        );
        assert_eq!(
            env[std::ffi::OsStr::new("WATCOM")],
            Some(std::ffi::OsStr::new(r"R:\toolchains\watcom10.0a"))
        );
        assert_eq!(
            env[std::ffi::OsStr::new("WINEPATH")],
            Some(std::ffi::OsStr::new(r"R:\toolchains\watcom10.0a\binnt"))
        );
        assert!(launch.get_current_dir().is_none());
        Ok(())
    }
    #[test]
    fn loader_and_sibling_payload_are_both_required_and_hash_bound() -> Result<()> {
        let source = shipped_root();
        let root = PathBuf::from(std::env::var_os("HOME").unwrap())
            .join(".ai-tmp/msvc2")
            .join(format!(
                "wasm-tool-hashes-{}-{}",
                std::process::id(),
                std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)?
                    .as_nanos()
            ));
        let loader = root.join(".patched/binnt/WASM.EXE");
        let payload = root.join(".patched/binb/WASM.EXE");
        fs::create_dir_all(loader.parent().unwrap())?;
        fs::create_dir_all(payload.parent().unwrap())?;
        let result = (|| -> Result<()> {
            assert!(watcom_executable(&root, "WASM").is_err());
            fs::copy(source.join(".patched/binnt/WASM.EXE"), &loader)?;
            assert!(watcom_executable(&root, "WASM").is_err());
            fs::copy(source.join(".patched/binb/WASM.EXE"), &payload)?;
            assert!(watcom_executable(&root, "WASM").is_ok());
            fs::write(&loader, b"changed loader")?;
            assert!(
                watcom_executable(&root, "WASM")
                    .unwrap_err()
                    .to_string()
                    .contains("loader differs")
            );
            fs::copy(source.join(".patched/binnt/WASM.EXE"), &loader)?;
            fs::write(&payload, b"changed payload")?;
            assert!(
                watcom_executable(&root, "WASM")
                    .unwrap_err()
                    .to_string()
                    .contains("payload differs")
            );
            for invalid in ["../WASM", r"binnt\WASM.EXE", ""] {
                assert!(watcom_executable(&root, invalid).is_err());
            }
            Ok(())
        })();
        fs::remove_dir_all(&root)?;
        result
    }
}
