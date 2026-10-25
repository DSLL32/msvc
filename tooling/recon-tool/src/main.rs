#[cfg(not(target_arch = "x86"))]
compile_error!("recon-tool must be built for i686-unknown-linux-gnu");
mod abi;
mod assets;
mod debug;
mod fixture;
mod host_alloc;
mod native;
mod original;
mod original_image;
mod output;
mod pe_import;
mod process;
mod projection;
mod runtime;
mod status;
mod toolchain;
mod trap;
mod wine;
use anyhow::{Context, Result};
use clap::{Parser, Subcommand};
use std::{fs, path::PathBuf};
#[derive(Parser)]
#[command(
    version,
    about = "Shared i386 reconstruction case runner; native replay never launches Wine"
)]
struct Cli {
    #[command(subcommand)]
    command: Command,
}
#[derive(Subcommand)]
enum Command {
    /// Identify shipped i386 COFF runtime code without executing the image.
    IdentifyRuntime(runtime::Options),
    /// Inspect descriptors without running the case.
    Inspect { case: PathBuf },
    /// Generate graph-audit's legacy config from the single binary registry.
    RegistryExport(projection::Options),
    /// Run native case(s), asserting trace, terminal and available final oracle.
    Run {
        #[arg(required = true)]
        cases: Vec<PathBuf>,
        #[arg(long = "capture")]
        io: Option<PathBuf>,
        #[arg(long)]
        report: Option<PathBuf>,
        #[arg(long)]
        gdb: bool,
        #[arg(long)]
        gdb_script: Option<PathBuf>,
    },
    /// Run optional unit tests from the initialized setup fork checkpoint.
    Test {
        #[arg(required = true)]
        cases: Vec<PathBuf>,
        #[arg(long = "capture")]
        io: Option<PathBuf>,
        #[arg(long)]
        report: Option<PathBuf>,
        #[arg(long)]
        gdb: bool,
        #[arg(long)]
        gdb_script: Option<PathBuf>,
        #[arg(long = "test")]
        selected_test: Option<String>,
    },
    /// Explicit original-only Wine helper/capture commands.
    Original {
        #[command(subcommand)]
        command: OriginalCommand,
    },
    /// Author and inspect versioned uncompressed I/O fixtures.
    Capture {
        #[command(subcommand)]
        command: CaptureCommand,
    },
}
#[derive(Subcommand)]
enum OriginalCommand {
    /// Build a case-owned C helper for explicit original DIRECT capture only.
    BuildDirect(original::BuildDirect),
    /// Run an original compiler/linker/tool in the shared Wine prefix.
    Tool(toolchain::Tool),
    Capture(original::Capture),
    BuildHelper {
        #[arg(long, default_value = ".")]
        repo: PathBuf,
    },
}
#[derive(Subcommand)]
enum CaptureCommand {
    Pack {
        json: PathBuf,
        output: PathBuf,
    },
    Unpack {
        input: PathBuf,
        #[arg(short, long)]
        output: Option<PathBuf>,
    },
}
fn main() {
    if let Err(e) = execute() {
        eprintln!("recon-tool: {e:#}");
        std::process::exit(1)
    }
}
fn execute() -> Result<()> {
    host_alloc::verify()?;
    match Cli::parse().command {
        Command::IdentifyRuntime(options) => runtime::identify(options)?,
        Command::Inspect { case } => println!(
            "{}",
            serde_json::to_string_pretty(&native::inspect(&case.canonicalize()?)?)?
        ),
        Command::RegistryExport(options) => projection::export(options)?,
        Command::Original { command } => match command {
            OriginalCommand::BuildDirect(options) => {
                println!("{}", original::build_direct(&options)?.display())
            }
            OriginalCommand::Tool(options) => toolchain::run(options)?,
            OriginalCommand::Capture(options) => original::capture(options)?,
            OriginalCommand::BuildHelper { repo } => {
                println!("{}", original::build_helper(&repo)?.display())
            }
        },
        Command::Capture { command } => match command {
            CaptureCommand::Pack { json, output } => {
                let f: fixture::Fixture = serde_json::from_slice(&fs::read(json)?)?;
                fixture::write(&output, &f)?;
            }
            CaptureCommand::Unpack { input, output } => {
                let f = fixture::read(&input)?;
                let s = serde_json::to_string_pretty(&f)? + "\n";
                if let Some(p) = output {
                    fs::write(p, s)?
                } else {
                    print!("{s}")
                }
            }
        },
        command => {
            let (cases, io, report, tests, mut debug) = match command {
                Command::Run {
                    cases,
                    io,
                    report,
                    gdb,
                    gdb_script,
                } => (
                    cases,
                    io,
                    report,
                    false,
                    debug::Options {
                        enabled: gdb,
                        script: gdb_script,
                        selected_test: None,
                    },
                ),
                Command::Test {
                    cases,
                    io,
                    report,
                    gdb,
                    gdb_script,
                    selected_test,
                } => (
                    cases,
                    io,
                    report,
                    true,
                    debug::Options {
                        enabled: gdb,
                        script: gdb_script,
                        selected_test,
                    },
                ),
                _ => unreachable!(),
            };
            debug.prepare()?;
            anyhow::ensure!(
                !debug.enabled || cases.len() == 1,
                "debug mode requires one case"
            );
            anyhow::ensure!(
                !debug.enabled || !tests || debug.selected_test.is_some(),
                "debugging tests requires --test NAME"
            );
            anyhow::ensure!(
                io.is_none() || cases.len() == 1,
                "--capture requires one case"
            );
            let mut results = vec![];
            for case in cases {
                let path = if case.is_dir() {
                    case.join(".build/case.so")
                } else {
                    case
                };
                let inferred = path
                    .parent()
                    .and_then(|p| p.parent())
                    .map(|p| p.join("capture.bin"));
                let fixture = io
                    .as_deref()
                    .or_else(|| inferred.as_deref().filter(|p| p.exists()));
                let result = native::run(&path, fixture, tests, &debug)
                    .with_context(|| format!("case {}", path.display()))?;
                println!(
                    "{} {} {} ({:.3} ms; setup {:.3} ms, execution {:.3} ms, checks {:.3} ms)",
                    if result.pass { "PASS" } else { "FAIL" },
                    result.case,
                    result.terminal,
                    result.elapsed_ms,
                    result.setup_ms,
                    result.execution_ms,
                    result.checks_ms
                );
                for e in &result.errors {
                    eprintln!("  {e}")
                }
                results.push(result);
            }
            if let Some(p) = report {
                fs::write(p, serde_json::to_vec_pretty(&results)?)?;
            }
            anyhow::ensure!(results.iter().all(|r| r.pass), "one or more cases failed");
        }
    }
    Ok(())
}
