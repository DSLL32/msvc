//! Explicit fresh-pair reference, separate from cached descriptor expectations.
use crate::{abi::Info, fixture};
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    fs,
    io::Write,
    path::{Path, PathBuf},
};

#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Identity {
    case: String,
    binary: String,
    original_path: String,
    original_sha256: String,
    execution: u32,
    first_entry_seen: Vec<u32>,
}
impl Identity {
    pub fn from_info(info: &Info) -> Self {
        Self {
            case: info.case.clone(),
            binary: info.binary.clone(),
            original_path: info.original_path.clone(),
            original_sha256: info.original_sha256.to_lowercase(),
            execution: info.execution,
            first_entry_seen: info.first_entry_seen.clone(),
        }
    }
}
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub(crate) struct DirectExit {
    pub callback_observed: bool,
    pub callback_pc: u32,
    pub pid: u32,
    pub tid: u32,
    pub expected_exit: u32,
    pub actual_exit: u32,
}
#[derive(Clone, Deserialize, Serialize)]
pub struct Summary {
    pub report: PathBuf,
    pub report_sha256: String,
    pub capture_sha256: String,
    pub source_binding_available: bool,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub direct_noreturn: Option<DirectExit>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub comparison_derivation: Option<Derivation>,
}
#[derive(clap::Args)]
pub struct Derive {
    #[arg(long)]
    pub original_report: PathBuf,
    #[arg(long)]
    pub original_capture: PathBuf,
    #[arg(long = "capture")]
    pub comparison_capture: PathBuf,
    #[arg(short, long)]
    pub output: PathBuf,
    /// Explain the separately reviewed comparison change; this grants no acceptance.
    #[arg(long)]
    pub reason: String,
}
#[derive(Clone, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
struct FileBinding {
    path: PathBuf,
    sha256: String,
}
#[derive(Clone, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Derivation {
    schema: u32,
    kind: String,
    parent_report: FileBinding,
    parent_capture: FileBinding,
    comparison_capture_sha256: String,
    unchanged_target_inputs_sha256: String,
    changed_output_rules: Vec<serde_json::Value>,
    added_harness_assets: Vec<serde_json::Value>,
    reason: String,
}
#[derive(Deserialize)]
struct Receipt {
    #[serde(default)]
    direct_noreturn: Option<DirectExit>,
    case: String,
    terminal: String,
    first_entry_order: Vec<u32>,
    #[serde(default)]
    oracle_written: bool,
    #[serde(default)]
    identity: Option<Identity>,
    #[serde(default)]
    capture_sha256: Option<String>,
    #[serde(default)]
    comparison_derivation: Option<Derivation>,
}
#[derive(Clone)]
pub struct Expected {
    case: String,
    identity: Option<Identity>,
    order: Vec<u32>,
    terminal: Terminal,
    pub summary: Summary,
}
#[derive(Clone)]
enum Terminal {
    Frontier(u32),
    Exit(u32),
    Direct,
}
fn hash(bytes: &[u8]) -> String {
    fixture::hex(&Sha256::digest(bytes))
}

fn comparison_changes(
    parent: &fixture::Fixture,
    next: &fixture::Fixture,
) -> Result<(String, Vec<serde_json::Value>, Vec<serde_json::Value>)> {
    use serde_json::json;
    parent.validate()?;
    next.validate()?;
    let input_view = |f: &fixture::Fixture| -> Result<serde_json::Value> {
        let mut value = serde_json::to_value(f)?;
        value["assets"] = json!([]);
        value["output_files"] = json!([]);
        if value["partial"].is_object() {
            value["partial"]["output_files"] = json!([]);
        }
        Ok(value)
    };
    let inputs = input_view(parent)?;
    ensure!(
        inputs == input_view(next)?,
        "comparison derivation changed target inputs, invocation, status or streams"
    );
    let mut changed = vec![];
    for (scope, before, after) in [
        (
            "final",
            parent.output_files.as_slice(),
            next.output_files.as_slice(),
        ),
        (
            "partial",
            parent
                .partial
                .as_ref()
                .map_or(&[][..], |p| p.output_files.as_slice()),
            next.partial
                .as_ref()
                .map_or(&[][..], |p| p.output_files.as_slice()),
        ),
    ] {
        ensure!(
            before.len() == after.len(),
            "comparison derivation changed {scope} output inventory"
        );
        for (a, b) in before.iter().zip(after) {
            ensure!(
                a.path == b.path && a.absent == b.absent,
                "comparison derivation changed output identity or absence"
            );
            let (a, b) = (serde_json::to_value(a)?, serde_json::to_value(b)?);
            if a != b {
                changed.push(json!({"scope":scope,"path":a["path"],"before":a,"after":b}));
            }
        }
    }
    for asset in &parent.assets {
        ensure!(
            next.assets
                .iter()
                .any(|a| a.path == asset.path && a.data == asset.data),
            "comparison derivation removed or modified an existing harness asset"
        );
    }
    let added = next
        .assets
        .iter()
        .filter(|a| !parent.assets.iter().any(|p| p.path == a.path))
        .map(|a| json!({"path":a.path,"bytes":a.data.0.len(),"sha256":hash(&a.data.0)}))
        .collect::<Vec<_>>();
    ensure!(
        !changed.is_empty() || !added.is_empty(),
        "comparison derivation has no policy or asset changes"
    );
    Ok((hash(&serde_json::to_vec(&inputs)?), changed, added))
}

fn proof(
    parent_raw: &[u8],
    parent_capture: &[u8],
    next_capture: &[u8],
    parent: &fixture::Fixture,
    next: &fixture::Fixture,
    report_path: PathBuf,
    capture_path: PathBuf,
    reason: &str,
) -> Result<Derivation> {
    let receipt: Receipt = serde_json::from_slice(parent_raw)?;
    ensure!(
        receipt.comparison_derivation.is_none(),
        "derive from the untouched original report, not another derivative"
    );
    ensure!(
        receipt.oracle_written
            && receipt.capture_sha256.as_deref() == Some(hash(parent_capture).as_str()),
        "parent original report does not bind the original capture"
    );
    let identity = receipt
        .identity
        .context("comparison derivation requires modern original identity binding")?;
    ensure!(
        identity.case == receipt.case,
        "parent original case identity differs"
    );
    ensure!(
        !reason.trim().is_empty() && reason.len() <= 4096 && !reason.contains('\0'),
        "derivation reason is empty or invalid"
    );
    let (inputs, changed, added) = comparison_changes(parent, next)?;
    Ok(Derivation {
        schema: 1,
        kind: "output-comparison-only".into(),
        parent_report: FileBinding {
            path: report_path,
            sha256: hash(parent_raw),
        },
        parent_capture: FileBinding {
            path: capture_path,
            sha256: hash(parent_capture),
        },
        comparison_capture_sha256: hash(next_capture),
        unchanged_target_inputs_sha256: inputs,
        changed_output_rules: changed,
        added_harness_assets: added,
        reason: reason.into(),
    })
}

pub fn derive(options: Derive) -> Result<()> {
    let report = options.original_report.canonicalize()?;
    let capture = options.original_capture.canonicalize()?;
    let parent_raw = fs::read(&report)?;
    ensure!(
        parent_raw.len() <= 8 * 1024 * 1024,
        "original report exceeds 8 MiB"
    );
    let parent_capture = fs::read(&capture)?;
    let next_capture = fs::read(&options.comparison_capture)?;
    let parent = fixture::decode(&parent_capture)?;
    let next = fixture::decode(&next_capture)?;
    let derivation = proof(
        &parent_raw,
        &parent_capture,
        &next_capture,
        &parent,
        &next,
        report,
        capture,
        &options.reason,
    )?;
    let mut output: serde_json::Value = serde_json::from_slice(&parent_raw)?;
    output["capture_sha256"] = serde_json::json!(derivation.comparison_capture_sha256);
    output["comparison_derivation"] = serde_json::to_value(derivation)?;
    let mut file = fs::OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&options.output)
        .context("derived comparison report must have a new output path")?;
    file.write_all(serde_json::to_string_pretty(&output)?.as_bytes())?;
    file.write_all(b"\n")?;
    Ok(())
}

fn validate_derivation(raw: &[u8], next_capture: &[u8], info: &Info) -> Result<Option<Derivation>> {
    let receipt: Receipt = serde_json::from_slice(raw)?;
    let Some(derivation) = receipt.comparison_derivation else {
        return Ok(None);
    };
    let report = derivation.parent_report.path.canonicalize()?;
    let capture = derivation.parent_capture.path.canonicalize()?;
    let parent_raw = fs::read(&report)?;
    ensure!(
        parent_raw.len() <= 8 * 1024 * 1024,
        "parent report exceeds 8 MiB"
    );
    let parent_capture = fs::read(&capture)?;
    ensure!(
        hash(&parent_raw) == derivation.parent_report.sha256
            && hash(&parent_capture) == derivation.parent_capture.sha256,
        "derived comparison parent bytes changed"
    );
    let expected = proof(
        &parent_raw,
        &parent_capture,
        next_capture,
        &fixture::decode(&parent_capture)?,
        &fixture::decode(next_capture)?,
        report.clone(),
        capture,
        &derivation.reason,
    )?;
    ensure!(
        derivation == expected,
        "derived comparison proof differs from actual parent/comparison fixtures"
    );
    let parent_value: serde_json::Value = serde_json::from_slice(&parent_raw)?;
    let mut observation: serde_json::Value = serde_json::from_slice(raw)?;
    observation
        .as_object_mut()
        .context("derived report must be an object")?
        .remove("comparison_derivation");
    observation["capture_sha256"] = parent_value["capture_sha256"].clone();
    ensure!(
        observation == parent_value,
        "derived comparison report changed original observations"
    );
    Expected::parse(&parent_raw, &parent_capture, report, info)?;
    Ok(Some(derivation))
}
impl Expected {
    pub fn load(path: &Path, capture: &Path, info: &Info) -> Result<Self> {
        let path = path.canonicalize()?;
        let raw = fs::read(&path)?;
        ensure!(
            raw.len() <= 8 * 1024 * 1024,
            "original report exceeds 8 MiB"
        );
        let capture_raw = fs::read(capture)?;
        let derivation = validate_derivation(&raw, &capture_raw, info)?;
        let mut expected = Self::parse(&raw, &capture_raw, path, info)?;
        expected.summary.comparison_derivation = derivation;
        Ok(expected)
    }
    fn parse(raw: &[u8], capture: &[u8], path: PathBuf, info: &Info) -> Result<Self> {
        let receipt: Receipt =
            serde_json::from_slice(raw).context("invalid explicit original report")?;
        ensure!(
            receipt.oracle_written,
            "original report has no fixture oracle"
        );
        ensure!(
            receipt.case == info.case,
            "original report case differs from native case"
        );
        let capture_sha256 = hash(capture);
        ensure!(
            receipt.identity.is_some() == receipt.capture_sha256.is_some(),
            "original report has incomplete source/capture binding"
        );
        if let Some(expected) = &receipt.capture_sha256 {
            ensure!(
                *expected == capture_sha256,
                "original report capture SHA256 differs"
            );
        }
        if let Some(identity) = &receipt.identity {
            ensure!(
                *identity == Identity::from_info(info),
                "original report binary/case/execution identity differs"
            );
        }
        ensure!(
            receipt.direct_noreturn.is_none()
                || (info.execution == 1
                    && receipt.terminal.starts_with("exit:")
                    && receipt.identity.is_some()
                    && receipt.capture_sha256.is_some()),
            "nonreturn DIRECT proof requires a bound DIRECT exit receipt"
        );
        let terminal = if let Some(text) = receipt.terminal.strip_prefix("frontier:") {
            let rva = u32::from_str_radix(text, 16).context("invalid original frontier")?;
            ensure!(
                receipt.terminal == format!("frontier:{rva:x}"),
                "noncanonical original frontier"
            );
            ensure!(
                info.functions
                    .iter()
                    .any(|f| f.rva == rva && f.definition == 4 && f.implementation == 0),
                "original frontier is not an unimplemented App registry entry"
            );
            Terminal::Frontier(rva)
        } else if let Some(text) = receipt.terminal.strip_prefix("exit:") {
            let status: u32 = text.parse().context("invalid original exit")?;
            ensure!(
                receipt.terminal == format!("exit:{status}")
                    && (info.execution == 0
                        || (info.execution == 1
                            && receipt.direct_noreturn.as_ref().is_some_and(|proof| proof
                                .callback_observed
                                && proof.callback_pc != 0
                                && proof.pid != 0
                                && proof.tid != 0
                                && proof.expected_exit == status
                                && proof.actual_exit == status))),
                "original exit differs from execution contract"
            );
            Terminal::Exit(status)
        } else {
            ensure!(
                receipt.terminal == "direct:pass" && info.execution == 1,
                "unsupported original terminal/execution contract"
            );
            Terminal::Direct
        };
        ensure!(
            receipt.first_entry_order.len() <= info.functions.len(),
            "original entry order exceeds registry"
        );
        let mut seen = std::collections::HashSet::new();
        for rva in &receipt.first_entry_order {
            ensure!(
                seen.insert(*rva)
                    && info
                        .functions
                        .iter()
                        .any(|f| f.rva == *rva && f.definition == 4),
                "original order contains duplicate or unknown App entry {rva:x}"
            );
        }
        Ok(Self {
            case: receipt.case,
            identity: receipt.identity,
            order: receipt.first_entry_order,
            terminal,
            summary: Summary {
                report: path,
                report_sha256: hash(raw),
                capture_sha256,
                source_binding_available: receipt.capture_sha256.is_some(),
                direct_noreturn: receipt.direct_noreturn.clone(),
                comparison_derivation: None,
            },
        })
    }
    pub fn apply(&self, info: &mut Info, fixture: &mut fixture::Fixture) -> Result<()> {
        ensure!(
            self.case == info.case,
            "original report case changed before execution"
        );
        if let Some(identity) = &self.identity {
            ensure!(
                *identity == Identity::from_info(info),
                "original report identity changed before execution"
            );
        }
        info.first_entry_order = self.order.clone();
        match self.terminal {
            Terminal::Frontier(rva) => {
                info.terminal = 1;
                info.frontier_rva = rva;
            }
            Terminal::Exit(status) => {
                info.terminal = 0;
                fixture.exit_code = Some(status as i32);
            }
            Terminal::Direct => {
                info.terminal = 0;
            }
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::{Value, json};
    fn info() -> Info {
        serde_json::from_value(json!({
            "binary":"link", "original_path":"toolchains/5.10/bin/LINK.EXE",
            "original_sha256":"ab".repeat(32), "original_entry_rva":0x1000,
            "case":"hello-win32", "description":"control", "execution":0,
            "terminal":1,"frontier_rva":0x616d0,"first_entry_order":[0x1000,0x616d0],
            "first_entry_seen":[],"tests":[],"module":"/scratch/case.so",
            "functions":([0x1000,0x31f30,0x616d0].map(|rva|json!({"name":format!("f{rva:x}"),
                "rva":rva,"native_symbol":null,"original_symbol":null,"definition":4,
                "implementation":0,"code":[[rva,rva+1]],"text_data":[]})))
        }))
        .unwrap()
    }
    fn report(info: &Info, bound: bool) -> Value {
        let mut report = json!({"case":info.case,"terminal":"frontier:31f30",
            "first_entry_order":[0x1000,0x31f30],"oracle_written":true});
        if bound {
            report["identity"] = serde_json::to_value(Identity::from_info(info)).unwrap();
            report["capture_sha256"] = json!(hash(b"actual capture"));
        }
        report
    }
    fn parse(value: &Value, info: &Info) -> Result<Expected> {
        Expected::parse(
            &serde_json::to_vec(value)?,
            b"actual capture",
            PathBuf::from("receipt.json"),
            info,
        )
    }
    #[test]
    fn direct_process_exit_requires_explicit_callback_proof() {
        let mut info = info();
        info.execution = 1;
        let mut receipt = report(&info, true);
        receipt["terminal"] = json!("exit:4294967295");
        assert!(parse(&receipt, &info).is_err());
        receipt["direct_noreturn"] = json!({"callback_observed":true,
            "callback_pc":0x10001000u32,"pid":123,"tid":124,
            "expected_exit":u32::MAX,"actual_exit":u32::MAX});
        let expected = parse(&receipt, &info).unwrap();
        let mut fixture = fixture::Fixture::default();
        expected.apply(&mut info, &mut fixture).unwrap();
        assert_eq!(fixture.exit_code, Some(-1));
        assert!(expected.summary.direct_noreturn.is_some());
        for (pointer, value) in [
            ("/direct_noreturn/callback_observed", json!(false)),
            ("/direct_noreturn/callback_pc", json!(0)),
            ("/direct_noreturn/tid", json!(0)),
            ("/direct_noreturn/actual_exit", json!(4)),
            ("/direct_noreturn/expected_exit", json!(4)),
            ("/terminal", json!("direct:pass")),
        ] {
            let mut bad = receipt.clone();
            *bad.pointer_mut(pointer).unwrap() = value;
            assert!(parse(&bad, &info).is_err(), "{pointer}");
        }
    }

    #[test]
    fn fresh_original_expectations_override_provisional_descriptor() {
        for bound in [false, true] {
            let mut info = info();
            let expected = parse(&report(&info, bound), &info).unwrap();
            assert_eq!(expected.summary.source_binding_available, bound);
            let mut fixture = fixture::Fixture::default();
            expected.apply(&mut info, &mut fixture).unwrap();
            assert_eq!(info.frontier_rva, 0x31f30);
            assert_eq!(info.first_entry_order, [0x1000, 0x31f30]);
            assert_ne!(info.frontier_rva, 0x616d0);
            assert_ne!(info.first_entry_order, [0x1000, 0x616d0]);
        }
    }
    #[test]
    fn source_capture_case_and_execution_bindings_are_enforced() {
        let info = info();
        let good = report(&info, true);
        for (pointer, value) in [
            ("/capture_sha256", json!("00".repeat(32))),
            ("/case", json!("different")),
            ("/identity/binary", json!("different")),
            ("/identity/original_sha256", json!("00".repeat(32))),
            ("/identity/execution", json!(1)),
            ("/identity/first_entry_seen", json!([0x1000])),
            ("/identity/case", json!("different")),
            ("/oracle_written", json!(false)),
            ("/terminal", json!("frontier:31F30")),
            ("/first_entry_order", json!([0x31f30, 0x31f30])),
            ("/first_entry_order", json!([0xdeadbeefu32])),
            ("/identity", Value::Null),
            ("/capture_sha256", Value::Null),
        ] {
            let mut bad = good.clone();
            *bad.pointer_mut(pointer).unwrap() = value;
            assert!(parse(&bad, &info).is_err(), "{pointer}");
        }
    }
    #[test]
    fn genuine_full_dword_exit_and_direct_terminals_keep_their_domains() {
        let mut info = info();
        let mut receipt = report(&info, false);
        receipt["terminal"] = json!("exit:4294967295");
        let expected = parse(&receipt, &info).unwrap();
        let mut fixture = fixture::Fixture::default();
        expected.apply(&mut info, &mut fixture).unwrap();
        assert_eq!(info.terminal, 0);
        assert_eq!(fixture.exit_code.unwrap() as u32, u32::MAX);
        receipt["terminal"] = json!("direct:pass");
        assert!(parse(&receipt, &info).is_err());
        info.execution = 1;
        assert!(parse(&receipt, &info).is_ok());
    }
}
