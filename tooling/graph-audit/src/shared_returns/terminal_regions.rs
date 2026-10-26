//! Exact shared terminal subgraphs, validated only from existing owner CFG facts.
use super::{Pair, TerminalLocation, canonical, unique};
use crate::{decode::Decoder, image::ImageView, index::Record};
use anyhow::{Context, Result, ensure};
use serde::Deserialize;
use serde_json::{Value, json};
use std::{
    collections::{BTreeMap, BTreeSet},
    path::Path,
};

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Declaration {
    owners: [String; 2],
    original: Region,
    #[serde(
        default,
        deserialize_with = "crate::register_imports::documentary_evidence"
    )]
    evidence: Option<Value>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Region {
    regions: Vec<TerminalLocation>,
    #[serde(default)]
    branches: Vec<Branch>,
    #[serde(default)]
    calls: Vec<Call>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Branch {
    site: u32,
    target: u32,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Call {
    site: u32,
    #[serde(default, deserialize_with = "optional_string")]
    target: Option<String>,
    #[serde(default, deserialize_with = "optional_string")]
    indirect_operand: Option<String>,
}
fn optional_string<'de, D: serde::Deserializer<'de>>(
    d: D,
) -> std::result::Result<Option<String>, D::Error> {
    String::deserialize(d).map(Some)
}
enum Target {
    Direct(String, u32),
    Indirect(String),
}

pub(super) fn parse(
    cfg: &Value,
    metadata: &Value,
    image: &impl ImageView,
    drivers: &[Record],
    index: &[Record],
    project: &Path,
    value: &Value,
) -> Result<Vec<Pair>> {
    let rows: Vec<Declaration> =
        serde_json::from_value(value.clone()).context("shared_terminal_regions schema")?;
    ensure!(!rows.is_empty(), "empty shared_terminal_regions");
    let mut seen = BTreeSet::new();
    let mut out = Vec::new();
    for row in rows {
        let mut names = row.owners.clone();
        names.sort();
        ensure!(
            seen.insert(names),
            "duplicate shared terminal-region owners"
        );
        let a = unique(drivers, &row.owners[0])?;
        let b = unique(drivers, &row.owners[1])?;
        canonical(cfg, a)?;
        canonical(cfg, b)?;
        let report = validate(image, a, b, &row.original, cfg, drivers, index, metadata)?;
        let evidence =
            crate::register_imports::verified_documentary_evidence(project, &row.evidence)?;
        let mut report = report;
        report["evidence"] = evidence.clone();
        out.push(Pair {
            regions: Some(report),
            jump: None,
            call: None,
            terminal: None,
            owner: a.clone(),
            entry: b.clone(),
            evidence,
        });
    }
    Ok(out)
}
#[allow(clippy::too_many_arguments)]
fn validate(
    image: &impl ImageView,
    a: &Record,
    b: &Record,
    region: &Region,
    cfg: &Value,
    drivers: &[Record],
    index: &[Record],
    metadata: &Value,
) -> Result<Value> {
    ensure!(
        a.name != b.name,
        "shared terminal regions require distinct owners"
    );
    let mut ranges = Vec::new();
    let mut bytes = BTreeMap::new();
    for part in &region.regions {
        let hex = &part.bytes_hex;
        ensure!(
            hex.is_ascii() && !hex.is_empty() && hex.len().is_multiple_of(2) && hex.len() <= 64,
            "shared terminal regions require 1..32 exact bytes"
        );
        let raw: Vec<u8> = (0..hex.len() / 2)
            .map(|i| u8::from_str_radix(&hex[i * 2..i * 2 + 2], 16))
            .collect::<std::result::Result<_, _>>()?;
        let end = part
            .start
            .checked_add(raw.len() as u32)
            .context("shared terminal-region overflow")?;
        ensure!(
            ranges.last().is_none_or(|&(_, hi)| hi <= part.start),
            "shared terminal regions must be sorted and disjoint"
        );
        ensure!(
            image.read(part.start, raw.len())? == raw,
            "shared terminal-region bytes differ"
        );
        for (offset, byte) in raw.into_iter().enumerate() {
            bytes.insert(part.start + offset as u32, byte);
        }
        ranges.push((part.start, end));
    }
    ensure!(
        !bytes.is_empty() && bytes.len() <= 32,
        "shared terminal regions require 1..32 total bytes"
    );
    let mut overlap = Vec::new();
    for (lo, hi) in a.code_ranges() {
        for (other_lo, other_hi) in b.code_ranges() {
            let begin = lo.max(other_lo);
            let end = hi.min(other_hi);
            if begin < end {
                overlap.push((begin, end));
            }
        }
    }
    overlap.sort();
    ensure!(
        ranges == overlap,
        "shared terminal regions must equal the complete two-owner code intersection"
    );
    for owner in [a, b] {
        for &(start, end) in &ranges {
            ensure!(
                owner.code_ranges().any(|(lo, hi)| lo <= start && hi == end),
                "shared terminal regions must end both owning chunks"
            );
        }
    }
    let mut branches = BTreeMap::new();
    for branch in &region.branches {
        ensure!(
            branches.insert(branch.site, branch.target).is_none(),
            "duplicate shared terminal-region branch"
        );
    }
    let mut calls = BTreeMap::new();
    for call in &region.calls {
        let target = match (&call.target, &call.indirect_operand) {
            (Some(name), None) if !name.is_empty() => {
                let target = unique(drivers, name)?;
                canonical(cfg, target)?;
                Target::Direct(name.clone(), target.start)
            }
            (None, Some(operand)) if !operand.is_empty() => Target::Indirect(operand.clone()),
            _ => anyhow::bail!(
                "shared terminal-region CALL needs exactly one direct target or opaque indirect operand"
            ),
        };
        ensure!(
            calls.insert(call.site, target).is_none() && !branches.contains_key(&call.site),
            "duplicate/conflicting shared terminal-region CALL"
        );
    }
    let decoder = Decoder::new()?;
    let mut common_edges = None;
    let mut common_returns = None;
    let mut call_report = Vec::new();
    for owner in [a, b] {
        let (graph, _) = crate::audit::structural_facts(
            &decoder,
            image,
            owner,
            metadata,
            "original",
            index,
            crate::flow::Options {
                record_dataflow: true,
                forbid_overlapping_instructions: true,
                ..Default::default()
            },
        )?;
        let mut successors = BTreeMap::new();
        let mut returns = BTreeSet::new();
        let mut used_branches = BTreeSet::new();
        let mut used_calls = BTreeSet::new();
        for &(lo, hi) in &ranges {
            let mut address = lo;
            while address < hi {
                let i = graph.instructions.get(&address).with_context(|| {
                    format!(
                        "shared terminal-region instruction not discovered in {} at {address:x}",
                        owner.name
                    )
                })?;
                ensure!(
                    i.next() <= hi
                        && i.bytes
                            .iter()
                            .enumerate()
                            .all(|(n, v)| bytes.get(&(address + n as u32)) == Some(v)),
                    "shared terminal-region instruction differs/truncates"
                );
                let next = graph.successors.get(&address).cloned().unwrap_or_default();
                if i.ret {
                    ensure!(
                        (i.bytes == [0xc3] || (i.bytes.len() == 3 && i.bytes[0] == 0xc2))
                            && next.is_empty(),
                        "shared terminal-region sink must be unprefixed nearRET"
                    );
                    returns.insert(address);
                } else if i.m == "call" {
                    let declared = calls
                        .get(&address)
                        .context("undeclared CALL in shared terminal region")?;
                    match declared {
                        Target::Direct(_, target) => ensure!(
                            i.imm() == Some(*target) && graph.calls.contains(target),
                            "shared terminal-region direct CALL target/raw edge mismatch"
                        ),
                        Target::Indirect(operand) => ensure!(
                            i.imm().is_none()
                                && i.text == *operand
                                && graph.indirect
                                && graph.indirect_calls.contains(&address),
                            "shared terminal-region opaque CALL operand/raw edge mismatch"
                        ),
                    }
                    ensure!(
                        next == BTreeSet::from([i.next()]),
                        "shared terminal-region CALL must retain normal next successor"
                    );
                    used_calls.insert(address);
                    call_report.push(match declared {
                        Target::Direct(name,target) => json!({"owner":owner.name,"site":address,"bytes_hex":i.bytes.iter().map(|b|format!("{b:02x}")).collect::<String>(),"target":name,"raw_target":target,"normal_successor":i.next()}),
                        Target::Indirect(operand) => json!({"owner":owner.name,"site":address,"bytes_hex":i.bytes.iter().map(|b|format!("{b:02x}")).collect::<String>(),"indirect_operand":operand,"raw_indirect":true,"normal_successor":i.next()}),
                    });
                } else if i.jump {
                    let target = branches
                        .get(&address)
                        .context("undeclared branch in shared terminal region")?;
                    ensure!(
                        i.imm() == Some(*target),
                        "shared terminal-region branch target mismatch"
                    );
                    let mut expected = BTreeSet::from([*target]);
                    if i.m != "jmp" {
                        expected.insert(i.next());
                    }
                    ensure!(
                        next == expected,
                        "shared terminal-region branch successors differ"
                    );
                    used_branches.insert(address);
                } else {
                    ensure!(
                        !i.control_transfer && next == BTreeSet::from([i.next()]),
                        "shared terminal-region unsupported transfer/fallthrough"
                    );
                }
                successors.insert(address, next);
                address = i.next();
            }
        }
        ensure!(
            used_branches == branches.keys().copied().collect()
                && used_calls == calls.keys().copied().collect(),
            "unconsumed shared terminal-region branch/CALL inventory"
        );
        ensure!(
            !returns.is_empty()
                && successors
                    .values()
                    .flatten()
                    .all(|target| successors.contains_key(target)),
            "shared terminal-region successors escape declared instruction union"
        );
        // A finite reverse closure over existing successor facts: cycles/dead ends cannot become return paths.
        let mut terminated = returns.clone();
        loop {
            let before = terminated.len();
            for (&site, targets) in &successors {
                if !targets.is_empty() && targets.iter().all(|t| terminated.contains(t)) {
                    terminated.insert(site);
                }
            }
            if terminated.len() == before {
                break;
            }
        }
        ensure!(
            terminated.len() == successors.len(),
            "shared terminal-region paths must be acyclic and terminate at nearRET"
        );
        if let Some(previous) = &common_edges {
            ensure!(
                *previous == successors,
                "shared terminal-region owner CFG edges differ"
            );
        }
        if let Some(previous) = &common_returns {
            ensure!(
                *previous == returns,
                "shared terminal-region owner RET starts differ"
            );
        }
        common_edges = Some(successors);
        common_returns = Some(returns);
    }
    Ok(
        json!({"owners":[a.name,b.name],"original_regions":region.regions.iter().map(|r|json!({"start":r.start,"bytes_hex":r.bytes_hex})).collect::<Vec<_>>(),
        "successors":common_edges,"near_ret_starts":common_returns,"raw_calls":call_report,"additional_unique_code_bytes":0,
        "meaning":"only this exact existing terminal subgraph is physically shared; raw calls and all per-owner graph/ABI checks remain distinct; no target/value/preservation/cleanup inference, execution permission, roots or body credit"}),
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture(raw: &[u8], branches: Value, calls: Value) -> (crate::pe::Pe, Vec<Record>, Value) {
        let mut bytes = vec![0x90; 0x62];
        bytes[..5].copy_from_slice(&[0x55, 0x89, 0xe5, 0xeb, 0x0b]);
        bytes[0x20..0x25].copy_from_slice(&[0x55, 0x89, 0xe5, 0xeb, 0xeb]);
        bytes[0x10..0x10 + raw.len()].copy_from_slice(raw);
        bytes[0x30..0x32].copy_from_slice(&[0xc9, 0xc3]);
        bytes[0x60] = 0xc3;
        let record = |name: &str, start, end, code_chunks| Record {
            name: name.into(),
            start,
            end,
            code_chunks,
            shared: vec![],
            evidence: Value::Null,
            pending_extent: false,
        };
        let chunks = vec![
            (0x401010, 0x401010 + raw.len() as u32),
            (0x401030, 0x401032),
        ];
        let records = vec![
            record("a", 0x401000, 0x401005, chunks.clone()),
            record("b", 0x401020, 0x401025, chunks),
            record("callee", 0x401060, 0x401061, vec![]),
        ];
        let registry:Vec<_>=records.iter().map(|r|json!({"name":r.name,"rva":r.start-0x400000,
            "definition":4,"implementation":1,"code":r.code_ranges().map(|(a,b)|[a-0x400000,b-0x400000]).collect::<Vec<_>>()})).collect();
        let cfg = json!({"target":{"base":0x400000},"function_registry":registry,"shared_terminal_regions":[{
            "owners":["a","b"],"original":{"regions":[{"start":0x401010,"bytes_hex":raw.iter().map(|b|format!("{b:02x}")).collect::<String>()},
            {"start":0x401030,"bytes_hex":"c9c3"}],"branches":branches,"calls":calls}}]});
        (crate::tests::image(&bytes), records, cfg)
    }
    fn parse_fixture(
        image: &crate::pe::Pe,
        records: &[Record],
        cfg: &Value,
    ) -> Result<super::super::SharedReturns> {
        super::super::SharedReturns::parse(cfg, &json!({}), image, records, records, Path::new("."))
    }
    #[test]
    fn disconnected_terminal_region_retains_both_real_owner_paths() -> Result<()> {
        let (image, records, cfg) = fixture(
            &[0x31, 0xc0, 0xeb, 0x1c],
            json!([{"site":0x401012,"target":0x401030}]),
            json!([]),
        );
        let policy = parse_fixture(&image, &records, &cfg)?;
        assert!(policy.permits(&records[0], &records[1]));
        assert!(policy.report().is_none() && policy.terminal_report().is_none());
        let report = policy.regions_report().unwrap();
        assert_eq!(report[0]["original_regions"].as_array().unwrap().len(), 2);
        assert_eq!(report[0]["near_ret_starts"], json!([0x401031]));
        assert_eq!(report[0]["additional_unique_code_bytes"], 0);
        assert!(crate::index::validate_chunk_ownership(&image, &records).is_err());
        crate::index::validate_chunk_ownership_with_shared(&image, &records, &policy)?;
        Ok(())
    }
    #[test]
    fn region_call_inventory_keeps_opaque_expression_and_raw_indirect_topology() -> Result<()> {
        let (image, records, cfg) = fixture(
            &[0xff, 0x16, 0xb8, 1, 0, 0, 0, 0xeb, 0x17],
            json!([{"site":0x401017,"target":0x401030}]),
            json!([{"site":0x401010,"indirect_operand":"dword ptr [esi]"}]),
        );
        let policy = parse_fixture(&image, &records, &cfg)?;
        let report = policy.regions_report().unwrap();
        assert_eq!(report[0]["raw_calls"].as_array().unwrap().len(), 2);
        assert_eq!(report[0]["raw_calls"][0]["raw_indirect"], true);
        let d = Decoder::new()?;
        for r in &records[..2] {
            let (g, _) = crate::audit::structural_facts(
                &d,
                &image,
                r,
                &json!({}),
                "original",
                &records,
                crate::flow::Options {
                    record_dataflow: true,
                    ..Default::default()
                },
            )?;
            assert!(g.indirect && g.calls.is_empty());
            assert_eq!(g.indirect_calls, BTreeSet::from([0x401010]));
            assert_eq!(g.successors[&0x401010], BTreeSet::from([0x401012]));
        }
        for replacement in [
            json!([]),
            json!([{"site":0x401011,"indirect_operand":"dword ptr [esi]"}]),
            json!([{"site":0x401010,"indirect_operand":"dword ptr [edi]"}]),
            json!([{"site":0x401010,"indirect_operand":"dword ptr [esi]","target":"callee"}]),
            json!([{"site":0x401010,"indirect_operand":null}]),
            json!([{"site":0x401010,"target":"callee"}]),
        ] {
            let mut bad = cfg.clone();
            bad["shared_terminal_regions"][0]["original"]["calls"] = replacement;
            assert!(parse_fixture(&image, &records, &bad).is_err());
        }
        Ok(())
    }
    #[test]
    fn region_direct_call_inventory_reuses_canonical_target_qualification() -> Result<()> {
        let (image, records, cfg) = fixture(
            &[0xe8, 0x4b, 0, 0, 0, 0xeb, 0x19],
            json!([{"site":0x401015,"target":0x401030}]),
            json!([{"site":0x401010,"target":"callee"}]),
        );
        let report = parse_fixture(&image, &records, &cfg)?
            .regions_report()
            .unwrap();
        assert_eq!(report[0]["raw_calls"][0]["raw_target"], 0x401060);
        let mut bad = cfg.clone();
        bad["function_registry"][2]["implementation"] = json!(0);
        assert!(parse_fixture(&image, &records, &bad).is_err());
        let mut bad = cfg.clone();
        bad["shared_terminal_regions"][0]["original"]["calls"][0]["target"] = json!("a");
        assert!(parse_fixture(&image, &records, &bad).is_err());
        Ok(())
    }
    #[test]
    fn rejects_partial_intersection_unconsumed_edges_and_cycles() {
        let (image, records, cfg) = fixture(
            &[0x31, 0xc0, 0xeb, 0x1c],
            json!([{"site":0x401012,"target":0x401030}]),
            json!([]),
        );
        for regions in [
            json!([{"start":0x401030,"bytes_hex":"c9c3"}]),
            json!([{"start":0x401010,"bytes_hex":"31c0eb1c"}]),
            json!([{"start":0x401010,"bytes_hex":"31c0eb1c"},{"start":0x401031,"bytes_hex":"c3"}]),
        ] {
            let mut bad = cfg.clone();
            bad["shared_terminal_regions"][0]["original"]["regions"] = regions;
            assert!(parse_fixture(&image, &records, &bad).is_err());
        }
        for branches in [
            json!([]),
            json!([{"site":0x401013,"target":0x401030}]),
            json!([{"site":0x401012,"target":0x401031}]),
            json!([{"site":0x401012,"target":0x401030},{"site":0x401030,"target":0x401031}]),
        ] {
            let mut bad = cfg.clone();
            bad["shared_terminal_regions"][0]["original"]["branches"] = branches;
            assert!(parse_fixture(&image, &records, &bad).is_err());
        }
        // A discovered loop with a real exit is still outside a closed acyclic return subgraph.
        let (image, records, cfg) = fixture(
            &[0x85, 0xc0, 0x75, 0xfc, 0xeb, 0x1a],
            json!([{"site":0x401012,"target":0x401010},{"site":0x401014,"target":0x401030}]),
            json!([]),
        );
        assert!(
            parse_fixture(&image, &records, &cfg)
                .err()
                .unwrap()
                .to_string()
                .contains("acyclic")
        );
    }
}
