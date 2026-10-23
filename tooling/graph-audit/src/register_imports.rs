//! Opt-in, intraprocedural must-values over the already-discovered instruction
//! CFG. This is deliberately not a general x86 emulator or ABI inference pass.
use crate::{
    decode::{Inst, Op},
    flow::Graph,
    imports::{Catalog, Identity},
    index::Record,
};
use anyhow::{Context, Result, ensure};
use capstone::arch::x86::X86Reg::*;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::{BTreeMap, BTreeSet, VecDeque};

#[derive(Default, Deserialize)]
#[serde(deny_unknown_fields)]
struct Policy {
    #[serde(default)]
    require_all_register_calls_resolved: bool,
}
pub fn strict_policy(value: &Value) -> Result<bool> {
    Ok(if value.is_null() {
        Policy::default()
    } else {
        serde_json::from_value(value.clone())?
    }
    .require_all_register_calls_resolved)
}
const NAMES: [&str; 3] = ["ebx", "esi", "edi"];
type State = [Option<Identity>; 3];
fn tracked(r: u16) -> Option<usize> {
    match u32::from(r) {
        X86_REG_EBX | X86_REG_BX | X86_REG_BL | X86_REG_BH => Some(0),
        X86_REG_ESI | X86_REG_SI | X86_REG_SIL => Some(1),
        X86_REG_EDI | X86_REG_DI | X86_REG_DIL => Some(2),
        _ => None,
    }
}
fn full(r: u16) -> Option<usize> {
    match u32::from(r) {
        X86_REG_EBX => Some(0),
        X86_REG_ESI => Some(1),
        X86_REG_EDI => Some(2),
        _ => None,
    }
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Target {
    #[serde(default, skip_serializing_if = "Option::is_none")]
    function: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    import: Option<Identity>,
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Evidence {
    path: std::path::PathBuf,
    sha256: String,
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Contract {
    target: Target,
    preserves: Vec<String>,
    evidence: Evidence,
}
#[derive(Default, Deserialize)]
#[serde(deny_unknown_fields)]
struct Configuration {
    #[serde(default)]
    preservation_contracts: Vec<Contract>,
}
#[derive(Default)]
pub struct Contracts {
    functions: BTreeMap<u32, [bool; 3]>,
    imports: BTreeMap<Identity, [bool; 3]>,
}
impl Contracts {
    pub fn parse(
        metadata: &Value,
        index: &[Record],
        catalog: &Catalog,
        project: &std::path::Path,
    ) -> Result<Self> {
        let configuration = &metadata["register_imports"];
        let records = if configuration.is_null() {
            Configuration::default()
        } else {
            serde_json::from_value::<Configuration>(configuration.clone())?
        }
        .preservation_contracts;
        let mut out = Self::default();
        let mut targets = BTreeSet::new();
        let base = project
            .canonicalize()
            .context("resolve preservation evidence project")?;
        let mut evidence_hashes = BTreeMap::new();
        for c in records {
            ensure!(
                !c.evidence.path.is_absolute(),
                "preservation evidence must be project-relative"
            );
            let path = base
                .join(&c.evidence.path)
                .canonicalize()
                .context("resolve preservation evidence file")?;
            ensure!(
                path.starts_with(&base) && path.is_file(),
                "preservation evidence escapes project or is not a file"
            );
            ensure!(
                c.evidence.sha256.len() == 64
                    && c.evidence
                        .sha256
                        .bytes()
                        .all(|v| v.is_ascii_digit() || (b'a'..=b'f').contains(&v)),
                "invalid preservation evidence SHA256"
            );
            let actual = if let Some(hash) = evidence_hashes.get(&path) {
                hash
            } else {
                let hash = crate::index::hash(&std::fs::read(&path)?);
                evidence_hashes.entry(path.clone()).or_insert(hash)
            };
            ensure!(
                *actual == c.evidence.sha256,
                "stale preservation evidence SHA256: {}",
                c.evidence.path.display()
            );
            ensure!(
                c.target.function.is_some() != c.target.import.is_some(),
                "preservation target must name exactly one function/import"
            );
            let mut mask = [false; 3];
            ensure!(!c.preserves.is_empty(), "empty preservation contract");
            for r in &c.preserves {
                let n = NAMES
                    .iter()
                    .position(|v| v == r)
                    .context("only ebx/esi/edi preservation is supported")?;
                ensure!(!mask[n], "duplicate preservation register");
                mask[n] = true;
            }
            let key = serde_json::to_string(&c.target)?;
            ensure!(targets.insert(key), "duplicate preservation target");
            if let Some(name) = c.target.function {
                let matches = index.iter().filter(|r| r.name == name).collect::<Vec<_>>();
                ensure!(
                    !matches.is_empty(),
                    "preservation function missing from verified index: {name}"
                );
                ensure!(
                    matches.iter().all(|r| r.start == matches[0].start),
                    "ambiguous preservation function: {name}"
                );
                // Distinct logical aliases must all agree; never union claims.
                out.functions
                    .entry(matches[0].start)
                    .and_modify(|old| {
                        for n in 0..3 {
                            old[n] &= mask[n];
                        }
                    })
                    .or_insert(mask);
            } else if let Some(id) = c.target.import {
                ensure!(
                    catalog.slots.values().any(|v| v == &id),
                    "preservation import missing from verified IAT: {id:?}"
                );
                out.imports.insert(id, mask);
            }
        }
        Ok(out)
    }
    fn mask(&self, i: &Inst, g: &Graph, before: &State) -> [bool; 3] {
        if let Some(id) = g.import_calls.get(&i.a) {
            self.imports.get(id).copied().unwrap_or_default()
        } else if let Some(a) = i.imm() {
            self.functions.get(&a).copied().unwrap_or_default()
        } else if let Some((Op::Reg(r), 4)) = i.ops.first() {
            full(*r)
                .and_then(|n| before[n].as_ref())
                .and_then(|id| self.imports.get(id))
                .copied()
                .unwrap_or_default()
        } else {
            [false; 3]
        }
    }
}
#[derive(Serialize)]
pub struct Report {
    pub resolved_register_calls: BTreeMap<u32, Identity>,
    pub unresolved_register_calls: BTreeSet<u32>,
    pub other_unresolved_calls: BTreeSet<u32>,
    pub direct_import_calls: BTreeMap<u32, Identity>,
    pub roots: BTreeSet<u32>,
}
fn is_call(i: &Inst) -> bool {
    i.m.split_whitespace()
        .last()
        .is_some_and(|v| v == "call" || v == "lcall")
}
fn transfer(
    i: &Inst,
    before: &State,
    g: &Graph,
    catalog: &Catalog,
    contracts: &Contracts,
) -> State {
    let mut out = before.clone();
    for r in &i.writes {
        if let Some(n) = tracked(*r) {
            out[n] = None;
        }
    }
    if i.m == "mov"
        && let [(Op::Reg(dst), 4), (src, 4)] = i.ops.as_slice()
        && let Some(n) = full(*dst)
    {
        out[n] = match src {
            Op::Reg(r) => full(*r).and_then(|n| before[n].clone()),
            Op::Mem(m) if m.segment == 0 && m.base == 0 && m.index == 0 && i.address_size == 4 => {
                catalog.slots.get(&m.disp).cloned()
            }
            _ => None,
        };
    }
    if is_call(i) {
        let mask = contracts.mask(i, g, before);
        for n in 0..3 {
            if !mask[n] {
                out[n] = None;
            }
        }
    }
    // Software interrupts can return, but their register contract is unknown.
    if ["int", "int1", "int3", "into", "sysenter", "syscall"].contains(&i.m.as_str()) {
        out = Default::default();
    }
    out
}
pub fn resolve(
    g: &mut Graph,
    catalog: &Catalog,
    contracts: &Contracts,
    require_all: bool,
) -> Result<Report> {
    let mut incoming = BTreeMap::<u32, State>::new();
    let mut pending = VecDeque::new();
    for &root in &g.roots {
        incoming.insert(root, Default::default());
        pending.push_back(root);
    }
    while let Some(a) = pending.pop_front() {
        let i = g
            .instructions
            .get(&a)
            .context("dataflow root/edge absent from discovered instructions")?;
        let state = transfer(i, &incoming[&a], g, catalog, contracts);
        for &to in g.successors.get(&a).into_iter().flatten() {
            ensure!(
                g.instructions.contains_key(&to),
                "dataflow edge absent from discovered instructions"
            );
            let changed = if let Some(old) = incoming.get_mut(&to) {
                let mut changed = false;
                for n in 0..3 {
                    if old[n] != state[n] && old[n].is_some() {
                        old[n] = None;
                        changed = true;
                    }
                }
                changed
            } else {
                incoming.insert(to, state.clone());
                true
            };
            if changed {
                pending.push_back(to);
            }
        }
    }
    ensure!(
        incoming.len() == g.instructions.len(),
        "discovered instruction missing from dataflow CFG"
    );
    let mut report = Report {
        resolved_register_calls: BTreeMap::new(),
        unresolved_register_calls: BTreeSet::new(),
        other_unresolved_calls: BTreeSet::new(),
        direct_import_calls: g.import_calls.clone(),
        roots: g.roots.clone(),
    };
    for &a in &g.indirect_calls {
        let i = &g.instructions[&a];
        if let Some((Op::Reg(r), _)) = i.ops.first() {
            if let Some(id) = full(*r).and_then(|n| incoming[&a][n].clone()) {
                report.resolved_register_calls.insert(a, id);
            } else {
                report.unresolved_register_calls.insert(a);
            }
        } else {
            report.other_unresolved_calls.insert(a);
        }
    }
    ensure!(
        !require_all || report.unresolved_register_calls.is_empty(),
        "unresolved register import calls: {:?}",
        report.unresolved_register_calls
    );
    for (&a, id) in &report.resolved_register_calls {
        g.imports.insert(id.clone());
        g.indirect_calls.remove(&a);
    }
    g.indirect = g.unresolved_tail || !g.indirect_calls.is_empty();
    Ok(report)
}
