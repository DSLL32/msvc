mod audit;
mod cf;
mod cvpack;
mod cvpack_registry;
mod decode;
mod flow;
mod image;
mod imports;
mod index;
mod instruction_export;
mod pe;
mod register_imports;
mod ret_checks;
mod watcom_map;
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
#[derive(Clone, Copy, Debug, ValueEnum, PartialEq, Eq, serde::Serialize)]
#[serde(rename_all = "lowercase")]
pub enum MapFormat {
    Msvc,
    Watcom10,
}
#[derive(Parser, Debug)]
#[command(
    about = "Read-only x86 function graph audits over verified PE and Watcom CF images.",
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
    #[arg(
        long,
        help = "Run the existing RC FILE/locale/Kanji runtime data checks (RC CFG only)"
    )]
    pub check_runtime_data: bool,
    #[arg(
        long,
        help = "Assert explicit RET cleanup sets for selected accepted functions without reducing other audit scopes"
    )]
    pub ret_checks: Option<PathBuf>,
    #[arg(
        long,
        help = "Export exact decoded instruction facts for the successful paired CFG selection; not execution permission"
    )]
    pub export_instructions: bool,
    #[arg(
        long,
        value_name = "NAME",
        help = "Export original-only decoded facts for an explicitly evidenced canonical CRT owner; not permission or paired equivalence"
    )]
    pub export_original_support_instructions: Vec<String>,
    #[arg(
        long,
        help = "Compare verified DLL/name-or-ordinal identities of direct IAT and import-thunk transfers"
    )]
    pub check_imports: bool,
    #[arg(
        long,
        requires = "check_imports",
        help = "Conservatively resolve EBX/ESI/EDI/EBP IAT calls with explicit callee-preservation evidence"
    )]
    pub resolve_register_imports: bool,
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
        help = "Structurally check canonical CRT/import extents without accepting graph bodies"
    )]
    pub check_registry_support_extents: bool,
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
    #[arg(long, value_enum, default_value = "msvc")]
    pub map_format: MapFormat,
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
    /// Suppress successful stdout output; --report still writes the full report.
    #[arg(short = 'q', long)]
    pub quiet: bool,
}
fn run() -> Result<()> {
    let args = Args::parse();
    anyhow::ensure!(
        args.discovery != Discovery::Legacy
            || !(args.check_imports
                || args.resolve_register_imports
                || args.export_instructions
                || !args.export_original_support_instructions.is_empty()
                || args.check_ret
                || args.check_runtime_data
                || args.ret_checks.is_some()
                || args.check_references
                || args.check_callbacks
                || args.check_ownership
                || args.check_extents
                || args.check_instruction_overlap
                || args.include_evidenced_runtime
                || args.check_accepted_extents
                || args.check_registry_support_extents
                || args.metadata.is_some()
                || args.map_format != MapFormat::Msvc),
        "optional checks/metadata require --discovery cfg; legacy mode already applies exact Python preset checks"
    );
    anyhow::ensure!(
        args.profile != Profile::Cvpack || args.map_format == MapFormat::Msvc,
        "integrated CVPACK profile requires MSVC MAP format"
    );
    anyhow::ensure!(
        args.profile != Profile::Cvpack || args.export_original_support_instructions.is_empty(),
        "original support instruction export requires a standalone CFG profile"
    );
    anyhow::ensure!(
        args.profile != Profile::Cvpack || !args.export_instructions,
        "instruction export requires a standalone CFG profile"
    );
    anyhow::ensure!(
        args.profile != Profile::Cvpack || args.ret_checks.is_none(),
        "--ret-checks requires a standalone CFG profile"
    );
    if args.profile == Profile::Cvpack
        && let Some(path) = &args.metadata
    {
        anyhow::ensure!(
            crate::register_imports::enabled_owners(&crate::index::json(path)?)?.is_empty()
                && crate::register_imports::expected_site_owners(&crate::index::json(path)?)?
                    .is_empty(),
            "per-owner register imports require a standalone CFG profile"
        );
    }
    anyhow::ensure!(
        !args.check_registry_support_extents || args.profile != Profile::Cvpack,
        "registry support extent checks require a standalone CFG profile"
    );
    anyhow::ensure!(
        !args.check_runtime_data || args.profile == Profile::Rc,
        "--check-runtime-data requires the RC CFG profile"
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
            (args.check_runtime_data, "runtime_data"),
            (args.ret_checks.is_some(), "selected_ret_cleanup"),
            (args.check_imports, "known_imports"),
            (
                args.resolve_register_imports || report.get("register_import_inputs").is_some(),
                "register_imports",
            ),
            (args.check_references, "function_references"),
            (args.check_callbacks, "callback_tables"),
            (args.check_ownership, "extent_ownership"),
            (args.check_extents, "whole_extent_boundaries"),
            (args.include_evidenced_runtime, "evidenced_runtime_graphs"),
            (args.check_accepted_extents, "accepted_index_extents"),
            (
                args.check_registry_support_extents,
                "registry_support_extents",
            ),
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
    if !args.quiet {
        println!("{}", serde_json::to_string(&report)?);
    }
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
