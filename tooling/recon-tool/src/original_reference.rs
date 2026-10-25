//! Explicit fresh-pair reference, separate from cached descriptor expectations.
use crate::{abi::Info, fixture};
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    fs,
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
#[derive(Clone, Deserialize, Serialize)]
pub struct Summary {
    pub report: PathBuf,
    pub report_sha256: String,
    pub capture_sha256: String,
    pub source_binding_available: bool,
}
#[derive(Deserialize)]
struct Receipt {
    case: String,
    terminal: String,
    first_entry_order: Vec<u32>,
    #[serde(default)]
    oracle_written: bool,
    #[serde(default)]
    identity: Option<Identity>,
    #[serde(default)]
    capture_sha256: Option<String>,
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
impl Expected {
    pub fn load(path: &Path, capture: &Path, info: &Info) -> Result<Self> {
        let path = path.canonicalize()?;
        let raw = fs::read(&path)?;
        ensure!(
            raw.len() <= 8 * 1024 * 1024,
            "original report exceeds 8 MiB"
        );
        Self::parse(&raw, &fs::read(capture)?, path, info)
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
                info.execution == 0 && receipt.terminal == format!("exit:{status}"),
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
