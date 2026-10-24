#[cfg(not(target_arch = "x86"))]
compile_error!("recon-tool must be built for i686-unknown-linux-gnu");
mod abi;
mod debug;
mod fixture;
mod native;
mod original;
mod original_image;
mod runtime;
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
    RegistryExport {
        case: PathBuf,
        #[arg(long)]
        image_base: u32,
        #[arg(long)]
        policy: Option<PathBuf>,
        /// Project declared callback tables for graph-audit alongside the registry.
        #[arg(long)]
        metadata_output: Option<PathBuf>,
        #[arg(short, long)]
        output: Option<PathBuf>,
    },
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
    match Cli::parse().command {
        Command::IdentifyRuntime(options) => runtime::identify(options)?,
        Command::Inspect { case } => println!(
            "{}",
            serde_json::to_string_pretty(&native::inspect(&case.canonicalize()?)?)?
        ),
        Command::RegistryExport {
            case,
            image_base,
            policy,
            metadata_output,
            output,
        } => {
            let path = if case.is_dir() {
                case.join(".build/case.so")
            } else {
                case
            };
            let info = native::inspect(&path.canonicalize()?)?;
            let mut v = if let Some(policy) = policy {
                serde_json::from_slice::<serde_json::Value>(&fs::read(policy)?)?
            } else {
                serde_json::json!({})
            };
            anyhow::ensure!(v.is_object(), "policy must be a JSON object");
            for field in ["target", "accepted", "driver"] {
                anyhow::ensure!(
                    v.get(field).is_none(),
                    "policy must not duplicate registry field {field}"
                )
            }
            let add = |rva: u32| image_base.checked_add(rva).context("original VA overflow");
            v["target"] = serde_json::json!({"sha256":info.original_sha256,"base":image_base,"entry":add(info.original_entry_rva)?});
            v["accepted"] = serde_json::to_value(
                info.functions
                    .iter()
                    .filter(|f| {
                        f.definition == 4 && f.implementation == 1 || matches!(f.definition, 1..=3)
                    })
                    .map(|f| &f.name)
                    .collect::<Vec<_>>(),
            )?;
            let mut drivers = vec![];
            for f in &info.functions {
                if f.definition != 4 && !(f.definition == 3 && f.native_symbol.is_some()) {
                    continue;
                }
                let entry = f
                    .code
                    .iter()
                    .find(|r| r[0] <= f.rva && f.rva < r[1])
                    .context("driver function entry not in code extents")?;
                anyhow::ensure!(
                    entry[0] == f.rva,
                    "graph-audit requires primary code extent to begin at entry"
                );
                let mut row = serde_json::json!({"name":f.name,"start":add(entry[0])?,"end":add(entry[1])?,"shared":f.text_data.iter().map(|r|Ok([add(r[0])?,add(r[1])?])).collect::<Result<Vec<_>>>()?,"code_chunks":f.code.iter().filter(|r|*r!=entry).map(|r|Ok([add(r[0])?,add(r[1])?])).collect::<Result<Vec<_>>>()?});
                if f.definition == 3 {
                    row["category"] = serde_json::json!("runtime")
                };
                drivers.push(row);
            }
            v["driver"] = serde_json::to_value(drivers)?;
            if let Some(path) = metadata_output {
                let mut tables = vec![];
                for table in v["tables"].as_array().into_iter().flatten() {
                    if table["kind"] != "function_pointers" {
                        continue;
                    }
                    let functions = table["functions"]
                        .as_array()
                        .context("callback table functions")?;
                    let begin = table["original"]
                        .as_u64()
                        .context("callback table original address")?;
                    let end = begin
                        .checked_add(
                            (functions.len() as u64)
                                .checked_mul(4)
                                .context("callback table size overflow")?,
                        )
                        .context("callback table extent overflow")?;
                    let name = table["name"].as_str().context("callback table name")?;
                    tables.push(serde_json::json!({"label":name,"original":[begin,end],"rebuilt_begin":name,"count":functions.len(),"require_accepted":false}));
                }
                anyhow::ensure!(!tables.is_empty(), "missing declared callback tables");
                fs::write(
                    path,
                    serde_json::to_string_pretty(
                        &serde_json::json!({"schema":1,"functions":{},"callback_tables":tables}),
                    )? + "\n",
                )?;
            }
            let text = serde_json::to_string_pretty(&v)? + "\n";
            if let Some(output) = output {
                fs::write(output, text)?
            } else {
                print!("{text}")
            }
        }
        Command::Original { command } => match command {
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
