//! Opt-in, intraprocedural must-values over the already-discovered instruction
//! CFG. This is deliberately not a general x86 emulator or ABI inference pass.
use crate::{
    decode::{Inst, Op},
    flow::Graph,
    imports::{Catalog, Identity},
    index::Record,
    pe::Pe,
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
    enabled: Option<bool>,
    #[serde(default)]
    require_all_register_calls_resolved: bool,
    #[serde(default)]
    expected_sites: Option<ExpectedSites>,
}
/// New metadata can only add resolution to selected owners. It cannot disable
/// the existing global CLI flag or silently broaden an unaccepted graph scope.
pub fn enabled_owners(metadata: &Value) -> Result<BTreeSet<String>> {
    let mut enabled = BTreeSet::new();
    for (name, function) in metadata["functions"].as_object().into_iter().flatten() {
        let value = &function["register_imports"];
        if value.is_null() {
            continue;
        }
        ensure!(
            value.is_object(),
            "register import owner policy must be an object: {name}"
        );
        let policy: Policy = serde_json::from_value(value.clone())?;
        if value.get("enabled").is_some() {
            ensure!(
                policy.enabled == Some(true),
                "register import owner enabled must be true; disabling is unsupported: {name}"
            );
            enabled.insert(name.clone());
        }
    }
    Ok(enabled)
}
pub fn strict_policy(value: &Value) -> Result<bool> {
    Ok(if value.is_null() {
        Policy::default()
    } else {
        serde_json::from_value(value.clone())?
    }
    .require_all_register_calls_resolved)
}
/// Optional exact assertions over already computed report facts. These cannot
/// create identities or change must-analysis, graph scope, or call semantics.
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct ExpectedSites {
    original: SiteInventory,
    rebuilt: SiteInventory,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct SiteInventory {
    resolved_register_calls: BTreeMap<u32, Identity>,
    unresolved_register_calls: Vec<u32>,
    other_unresolved_calls: Vec<u32>,
    direct_import_calls: BTreeMap<u32, Identity>,
}
impl SiteInventory {
    fn check(&self, actual: &Report, owner: &str, side: &str) -> Result<()> {
        let unresolved: BTreeSet<_> = self.unresolved_register_calls.iter().copied().collect();
        let other: BTreeSet<_> = self.other_unresolved_calls.iter().copied().collect();
        ensure!(
            unresolved.len() == self.unresolved_register_calls.len()
                && other.len() == self.other_unresolved_calls.len(),
            "duplicate expected import site for {side} {owner}"
        );
        ensure!(
            self.resolved_register_calls == actual.resolved_register_calls,
            "{side} {owner} expected resolved register call sites differ"
        );
        ensure!(
            unresolved == actual.unresolved_register_calls,
            "{side} {owner} expected unresolved register call sites differ"
        );
        ensure!(
            other == actual.other_unresolved_calls,
            "{side} {owner} expected opaque call sites differ"
        );
        ensure!(
            self.direct_import_calls == actual.direct_import_calls,
            "{side} {owner} expected direct import call sites differ"
        );
        Ok(())
    }
}
pub fn expected_site_owners(metadata: &Value) -> Result<BTreeSet<String>> {
    let mut owners = BTreeSet::new();
    for (name, function) in metadata["functions"].as_object().into_iter().flatten() {
        let value = &function["register_imports"];
        if value.is_null() {
            continue;
        }
        let policy: Policy = serde_json::from_value(value.clone())?;
        if value.get("expected_sites").is_some() {
            ensure!(
                policy.expected_sites.is_some(),
                "expected import sites must contain both original and rebuilt inventories: {name}"
            );
            owners.insert(name.clone());
        }
    }
    Ok(owners)
}
pub fn check_expected_sites(
    value: &Value,
    original: &Report,
    rebuilt: &Report,
    owner: &str,
) -> Result<bool> {
    if value.is_null() {
        return Ok(false);
    }
    let policy: Policy = serde_json::from_value(value.clone())?;
    if let Some(expected) = policy.expected_sites {
        expected.original.check(original, owner, "original")?;
        expected.rebuilt.check(rebuilt, owner, "rebuilt")?;
        Ok(true)
    } else {
        Ok(false)
    }
}
const NAMES: [&str; 4] = ["ebx", "esi", "edi", "ebp"];
type State = [Option<Identity>; 4];
fn tracked(r: u16) -> Option<usize> {
    match u32::from(r) {
        X86_REG_EBX | X86_REG_BX | X86_REG_BL | X86_REG_BH => Some(0),
        X86_REG_ESI | X86_REG_SI | X86_REG_SIL => Some(1),
        X86_REG_EDI | X86_REG_DI | X86_REG_DIL => Some(2),
        X86_REG_EBP | X86_REG_BP | X86_REG_BPL => Some(3),
        _ => None,
    }
}
fn full(r: u16) -> Option<usize> {
    match u32::from(r) {
        X86_REG_EBX => Some(0),
        X86_REG_ESI => Some(1),
        X86_REG_EDI => Some(2),
        X86_REG_EBP => Some(3),
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
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct CallbackLocation {
    site: u32,
    slot: u32,
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct CallbackContract {
    owner: String,
    original: CallbackLocation,
    rebuilt: CallbackLocation,
    preserves: Vec<String>,
    evidence: Evidence,
}
#[derive(Clone, Copy)]
pub enum Side {
    Original,
    Rebuilt,
}
impl CallbackContract {
    fn location(&self, side: Side) -> &CallbackLocation {
        match side {
            Side::Original => &self.original,
            Side::Rebuilt => &self.rebuilt,
        }
    }
}
#[derive(Clone, Serialize)]
pub struct CallbackReport {
    slot: u32,
    preserves: Vec<String>,
    evidence: Evidence,
}
fn preservation_mask(preserves: &[String]) -> Result<[bool; 4]> {
    ensure!(!preserves.is_empty(), "empty preservation contract");
    let mut mask = [false; 4];
    for r in preserves {
        let n = NAMES
            .iter()
            .position(|v| v == r)
            .context("only ebx/esi/edi/ebp preservation is supported")?;
        ensure!(!mask[n], "duplicate preservation register");
        mask[n] = true;
    }
    Ok(mask)
}
struct EvidenceFiles {
    base: std::path::PathBuf,
    hashes: BTreeMap<std::path::PathBuf, String>,
}
impl EvidenceFiles {
    fn new(project: &std::path::Path) -> Result<Self> {
        Ok(Self {
            base: project
                .canonicalize()
                .context("resolve preservation evidence project")?,
            hashes: BTreeMap::new(),
        })
    }
    fn verify(&mut self, evidence: &Evidence) -> Result<()> {
        ensure!(
            !evidence.path.is_absolute(),
            "preservation evidence must be project-relative"
        );
        let path = self
            .base
            .join(&evidence.path)
            .canonicalize()
            .context("resolve preservation evidence file")?;
        ensure!(
            path.starts_with(&self.base) && path.is_file(),
            "preservation evidence escapes project or is not a file"
        );
        ensure!(
            evidence.sha256.len() == 64
                && evidence
                    .sha256
                    .bytes()
                    .all(|v| v.is_ascii_digit() || (b'a'..=b'f').contains(&v)),
            "invalid preservation evidence SHA256"
        );
        let actual = if let Some(hash) = self.hashes.get(&path) {
            hash
        } else {
            let hash = crate::index::hash(&std::fs::read(&path)?);
            self.hashes.entry(path).or_insert(hash)
        };
        ensure!(
            *actual == evidence.sha256,
            "stale preservation evidence SHA256: {}",
            evidence.path.display()
        );
        Ok(())
    }
}
#[derive(Default, Deserialize)]
#[serde(deny_unknown_fields)]
struct Configuration {
    #[serde(default)]
    preservation_contracts: Vec<Contract>,
    #[serde(default)]
    callback_preservation_contracts: Vec<CallbackContract>,
}
#[derive(Default)]
pub struct Contracts {
    functions: BTreeMap<u32, [bool; 4]>,
    imports: BTreeMap<Identity, [bool; 4]>,
    callbacks: BTreeMap<String, Vec<CallbackContract>>,
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
        };
        let mut out = Self::default();
        let mut targets = BTreeSet::new();
        let mut evidence_files = EvidenceFiles::new(project)?;
        for c in records.preservation_contracts {
            evidence_files.verify(&c.evidence)?;
            ensure!(
                c.target.function.is_some() != c.target.import.is_some(),
                "preservation target must name exactly one function/import"
            );
            let mask = preservation_mask(&c.preserves)?;
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
                        for n in 0..4 {
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
        let mut original_sites = BTreeSet::new();
        let mut rebuilt_sites = BTreeSet::new();
        for c in records.callback_preservation_contracts {
            evidence_files.verify(&c.evidence)?;
            preservation_mask(&c.preserves)?;
            ensure!(
                index.iter().any(|r| r.name == c.owner),
                "callback preservation owner missing from verified index: {}",
                c.owner
            );
            ensure!(
                original_sites.insert(c.original.site) && rebuilt_sites.insert(c.rebuilt.site),
                "duplicate callback preservation site/owner"
            );
            out.callbacks.entry(c.owner.clone()).or_default().push(c);
        }
        Ok(out)
    }
    pub fn validate_callback_owners(&self, selected: &BTreeSet<String>) -> Result<()> {
        for owner in self.callbacks.keys() {
            ensure!(
                selected.contains(owner),
                "callback preservation owner is not audited: {owner}"
            );
        }
        Ok(())
    }
    fn callback_reports(
        &self,
        owner: &Record,
        g: &Graph,
        catalog: &Catalog,
        pe: &Pe,
        side: Side,
    ) -> Result<BTreeMap<u32, CallbackReport>> {
        let mut reports = BTreeMap::new();
        for contract in self.callbacks.get(&owner.name).into_iter().flatten() {
            let loc = contract.location(side);
            ensure!(
                owner.contains_code(loc.site),
                "callback preservation site outside owner: {}",
                owner.name
            );
            let i = g
                .instructions
                .get(&loc.site)
                .context("callback preservation site absent from discovered owner CFG")?;
            ensure!(
                g.indirect_calls.contains(&loc.site),
                "callback preservation site is not an opaque indirect call"
            );
            ensure!(
                i.m == "call"
                    && i.bytes.len() == 6
                    && i.bytes[..2] == [0xff, 0x15]
                    && i.address_size == 4,
                "callback preservation requires exact unprefixed absolute32 CALL"
            );
            let [(Op::Mem(m), 4)] = i.ops.as_slice() else {
                anyhow::bail!("callback preservation requires absolute32 memory CALL");
            };
            ensure!(
                m.segment == 0 && m.base == 0 && m.index == 0 && m.disp == loc.slot,
                "callback preservation slot/operand differs"
            );
            let end = loc
                .slot
                .checked_add(4)
                .context("callback preservation slot overflow")?;
            ensure!(
                pe.sections.iter().any(|s| s.flags & 0x40000000 != 0
                    && s.start <= loc.slot
                    && s.start
                        .checked_add(s.virtual_size)
                        .is_some_and(|limit| end <= limit)),
                "callback preservation slot is not fully mapped readable memory"
            );
            // Reject any overlap, including misaligned references into a known IAT slot.
            ensure!(
                !catalog
                    .slots
                    .keys()
                    .any(|slot| u64::from(*slot) < u64::from(end)
                        && u64::from(loc.slot) < u64::from(*slot) + 4),
                "callback preservation slot overlaps verified IAT"
            );
            reports.insert(
                loc.site,
                CallbackReport {
                    slot: loc.slot,
                    preserves: contract.preserves.clone(),
                    evidence: contract.evidence.clone(),
                },
            );
        }
        Ok(reports)
    }
    fn mask(
        &self,
        i: &Inst,
        g: &Graph,
        before: &State,
        callbacks: &BTreeMap<u32, CallbackReport>,
    ) -> [bool; 4] {
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
        } else if let Some(c) = callbacks.get(&i.a) {
            // Validated above: only retain existing must-values. This call remains
            // opaque and contributes no callee/import identity or value seed.
            preservation_mask(&c.preserves).expect("validated callback mask")
        } else {
            [false; 4]
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
    #[serde(skip_serializing_if = "BTreeMap::is_empty")]
    pub callback_preservation: BTreeMap<u32, CallbackReport>,
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
    callbacks: &BTreeMap<u32, CallbackReport>,
) -> State {
    let mut out = before.clone();
    for r in &i.writes {
        if let Some(n) = tracked(*r) {
            out[n] = None;
        }
    }
    // Capstone 5 omits EBP from ENTER's implicit write list. Both frame
    // operations load/change EBP from state outside this value domain.
    if ["enter", "leave"].contains(&i.m.as_str()) {
        out[3] = None;
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
        let mask = contracts.mask(i, g, before, callbacks);
        for n in 0..4 {
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
    owner: &Record,
    pe: &Pe,
    side: Side,
) -> Result<Report> {
    let callbacks = contracts.callback_reports(owner, g, catalog, pe, side)?;
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
        let state = transfer(i, &incoming[&a], g, catalog, contracts, &callbacks);
        for &to in g.successors.get(&a).into_iter().flatten() {
            ensure!(
                g.instructions.contains_key(&to),
                "dataflow edge absent from discovered instructions"
            );
            let changed = if let Some(old) = incoming.get_mut(&to) {
                let mut changed = false;
                for n in 0..4 {
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
        callback_preservation: callbacks,
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

#[cfg(test)]
mod tests {
    #[test]
    fn owner_enable_is_explicit_and_additive() {
        use serde_json::json;
        assert!(super::enabled_owners(&json!({"functions":{"a":{"register_imports":{"require_all_register_calls_resolved":true}}}})).unwrap().is_empty());
        assert_eq!(super::enabled_owners(&json!({"functions":{"a":{"register_imports":{"enabled":true,"require_all_register_calls_resolved":true}}}})).unwrap(), ["a".to_string()].into());
        assert!(
            super::strict_policy(
                &json!({"enabled":true,"require_all_register_calls_resolved":true})
            )
            .unwrap()
        );
    }
    #[test]
    fn owner_enable_rejects_disable_and_malformed_policy() {
        use serde_json::json;
        for value in [json!(false), json!(null), json!("true"), json!(1)] {
            assert!(
                super::enabled_owners(
                    &json!({"functions":{"a":{"register_imports":{"enabled":value}}}})
                )
                .is_err()
            );
        }
        assert!(
            super::enabled_owners(
                &json!({"functions":{"a":{"register_imports":{"enabled":true,"seed":"invented"}}}})
            )
            .is_err()
        );
    }
    #[test]
    fn owner_policy_typo_cannot_silently_disable_resolution() {
        use serde_json::json;
        for value in [
            json!({"enabld":true}),
            json!({"seed":"invented"}),
            json!([]),
            json!("true"),
        ] {
            assert!(
                super::enabled_owners(&json!({"functions":{"a":{"register_imports":value}}}))
                    .is_err()
            );
        }
    }
    #[test]
    fn ebp_alias_family_includes_low_alias() {
        use capstone::arch::x86::X86Reg::*;
        for r in [X86_REG_EBP, X86_REG_BP, X86_REG_BPL] {
            assert_eq!(super::tracked(r as u16), Some(3));
        }
        assert_eq!(super::full(X86_REG_BP as u16), None);
        assert_eq!(super::full(X86_REG_BPL as u16), None);
    }
}

#[cfg(test)]
mod site_inventory_tests {
    use super::*;
    fn report() -> Report {
        Report {
            resolved_register_calls: BTreeMap::new(),
            unresolved_register_calls: BTreeSet::new(),
            other_unresolved_calls: BTreeSet::new(),
            direct_import_calls: BTreeMap::new(),
            roots: BTreeSet::new(),
            callback_preservation: BTreeMap::new(),
        }
    }
    #[test]
    fn inventory_asserts_sites_without_resolving_or_ignoring_opaque_calls() {
        let mut actual = report();
        assert!(!check_expected_sites(&Value::Null, &actual, &actual, "f").unwrap());
        actual.other_unresolved_calls.insert(7);
        let expected: SiteInventory=serde_json::from_value(serde_json::json!({"resolved_register_calls":{},"unresolved_register_calls":[],"other_unresolved_calls":[7],"direct_import_calls":{}})).unwrap();
        assert!(expected.check(&actual, "f", "original").is_ok());
        actual.unresolved_register_calls.insert(9);
        assert!(
            expected
                .check(&actual, "f", "original")
                .unwrap_err()
                .to_string()
                .contains("unresolved register")
        );
        assert!(actual.other_unresolved_calls.contains(&7));
        let bad = serde_json::json!({"functions":{"f":{"register_imports":{"enabled":true,"expected_sites":{"original":{},"rebuilt":{}}}}}});
        assert!(expected_site_owners(&bad).is_err());
        assert!(
            expected_site_owners(
                &serde_json::json!({"functions":{"f":{"register_imports":{"expected_sites":null}}}})
            )
            .is_err()
        );
    }
}
