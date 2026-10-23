mod audit;
mod cvpack;
mod decode;
mod flow;
mod index;
mod pe;
use anyhow::Result;
use clap::{Parser, ValueEnum};
use std::path::PathBuf;
#[derive(Clone, Copy, Debug, ValueEnum, PartialEq, Eq, serde::Serialize)]
#[serde(rename_all = "lowercase")]
pub enum Profile {
    Cl,
    C1,
    C1xx,
    C2,
    Link,
    Rc,
    Rcpp,
    Cvpack,
}
#[derive(Clone, Copy, Debug, ValueEnum, PartialEq, Eq)]
pub enum Discovery {
    Cfg,
    Legacy,
}
#[derive(Parser, Debug)]
#[command(
    about = "Read-only PE function graph audits and explicit optional checks.",
    version
)]
pub struct Args {
    #[arg(long, value_enum)]
    pub profile: Profile,
    #[arg(long)]
    pub project: PathBuf,
    #[arg(long, value_enum, default_value = "cfg")]
    pub discovery: Discovery,
    #[arg(long)]
    pub check_ret: bool,
    #[arg(long)]
    pub check_references: bool,
    #[arg(long)]
    pub check_callbacks: bool,
    #[arg(long)]
    pub check_ownership: bool,
    #[arg(long)]
    pub check_extents: bool,
    #[arg(
        long,
        help = "Also compare accepted, explicitly evidenced runtime graph vertices from verified indexes"
    )]
    pub include_evidenced_runtime: bool,
    #[arg(
        long,
        help = "Check instruction extents of every accepted source/runtime/import index record on both sides"
    )]
    pub check_accepted_extents: bool,
    #[arg(
        long,
        help = "Assume instructions cannot overlap within a function (not a universal x86 rule)"
    )]
    pub check_instruction_overlap: bool,
    #[arg(
        long,
        help = "Optional target graph roots/tables/callback metadata JSON"
    )]
    pub metadata: Option<PathBuf>,
    #[arg(long, default_value = ".")]
    pub repo: PathBuf,
    #[arg(long)]
    pub original: Option<PathBuf>,
    #[arg(long)]
    pub rebuilt: Option<PathBuf>,
    #[arg(long)]
    pub map: Option<PathBuf>,
    #[arg(long)]
    pub config: Option<PathBuf>,
    #[arg(long)]
    pub runtime: Option<PathBuf>,
    #[arg(long)]
    pub rebuilt_runtime: Option<PathBuf>,
    #[arg(
        long,
        help = "Write a deterministic report; no report files are overwritten otherwise"
    )]
    pub report: Option<PathBuf>,
}
fn run() -> Result<()> {
    let args = Args::parse();
    anyhow::ensure!(
        args.discovery != Discovery::Legacy
            || !(args.check_ret
                || args.check_references
                || args.check_callbacks
                || args.check_ownership
                || args.check_extents
                || args.check_instruction_overlap
                || args.include_evidenced_runtime
                || args.check_accepted_extents
                || args.metadata.is_some()),
        "optional checks/metadata require --discovery cfg; legacy mode already applies exact Python preset checks"
    );
    let mut report = if args.profile == Profile::Cvpack {
        cvpack::run(&args)?
    } else {
        audit::run(&args)?
    };
    report["discovery"] = serde_json::json!(match args.discovery {
        Discovery::Cfg => "cfg",
        Discovery::Legacy => "legacy",
    });
    let checks = if args.discovery == Discovery::Legacy {
        vec!["python_compatibility_preset"]
    } else {
        let mut checks = vec!["call_graph"];
        for (enabled, name) in [
            (args.check_ret, "ret_cleanup"),
            (args.check_references, "function_references"),
            (args.check_callbacks, "callback_tables"),
            (args.check_ownership, "extent_ownership"),
            (args.check_extents, "whole_extent_boundaries"),
            (args.include_evidenced_runtime, "evidenced_runtime_graphs"),
            (args.check_accepted_extents, "accepted_index_extents"),
            (
                args.check_instruction_overlap,
                "nonoverlapping_instruction_assumption",
            ),
        ] {
            if enabled {
                checks.push(name)
            }
        }
        checks
    };
    report["checks"] = serde_json::json!(checks);
    if let Some(path) = &args.report {
        std::fs::write(path, serde_json::to_vec_pretty(&report)?)?;
    }
    println!("{}", serde_json::to_string(&report)?);
    Ok(())
}
fn main() {
    if let Err(e) = run() {
        eprintln!("graph-audit: {e:#}");
        std::process::exit(1)
    }
}

#[cfg(test)]
mod tests;
