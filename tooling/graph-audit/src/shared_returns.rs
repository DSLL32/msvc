//! Exact shared terminal instructions preserve distinct function identities.
mod terminal_regions;

use crate::{
    decode::Decoder,
    image::ImageView,
    index::{Record, num},
};
use anyhow::{Context, Result, ensure};
use serde::Deserialize;
use serde_json::{Value, json};
use std::{collections::BTreeSet, path::Path};

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Declaration {
    owner: String,
    entry: String,
    #[serde(
        default,
        deserialize_with = "crate::register_imports::documentary_evidence"
    )]
    evidence: Option<Value>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct TerminalDeclaration {
    owners: [String; 2],
    original: TerminalLocation,
    #[serde(
        default,
        deserialize_with = "crate::register_imports::documentary_evidence"
    )]
    evidence: Option<Value>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct TerminalLocation {
    start: u32,
    bytes_hex: String,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct CallReturnDeclaration {
    owners: [String; 2],
    original: TerminalLocation,
    call: TransferRole,
    #[serde(
        default,
        deserialize_with = "crate::register_imports::documentary_evidence"
    )]
    evidence: Option<Value>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct JumpTailDeclaration {
    owners: [String; 2],
    original: TerminalLocation,
    jump: TransferRole,
    #[serde(default, deserialize_with = "optional_call")]
    call: Option<TransferRole>,
    #[serde(
        default,
        deserialize_with = "crate::register_imports::documentary_evidence"
    )]
    evidence: Option<Value>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct TransferRole {
    offset: u32,
    target: String,
}
fn optional_call<'de, D: serde::Deserializer<'de>>(
    deserializer: D,
) -> std::result::Result<Option<TransferRole>, D::Error> {
    TransferRole::deserialize(deserializer).map(Some)
}
struct ResolvedTransfer {
    offset: u32,
    target: String,
    address: u32,
}
#[derive(Clone, Copy)]
enum TerminalTransfer<'a> {
    Call(&'a ResolvedTransfer),
    Jump(&'a ResolvedTransfer),
    CallJump(&'a ResolvedTransfer, &'a ResolvedTransfer),
}
struct Pair {
    regions: Option<Value>,
    jump: Option<ResolvedTransfer>,
    call: Option<ResolvedTransfer>,
    terminal: Option<(u32, String)>,
    owner: Record,
    entry: Record,
    evidence: Value,
}
#[derive(Default)]
pub struct SharedReturns(Vec<Pair>);
fn unique<'a>(records: &'a [Record], name: &str) -> Result<&'a Record> {
    let found: Vec<_> = records.iter().filter(|r| r.name == name).collect();
    ensure!(
        found.len() == 1 && !found[0].pending_extent,
        "shared RET requires unique nonpending App driver: {name}"
    );
    Ok(found[0])
}
fn canonical(cfg: &Value, r: &Record) -> Result<()> {
    let rows: Vec<_> = cfg["function_registry"]
        .as_array()
        .context("shared RET requires canonical registry")?
        .iter()
        .filter(|v| v["name"].as_str() == Some(&r.name))
        .collect();
    ensure!(
        rows.len() == 1,
        "shared RET canonical name missing/ambiguous"
    );
    let row = rows[0];
    ensure!(
        row["definition"] == 4 && row["implementation"] == 1,
        "shared RET requires implemented canonical App identity"
    );
    let base = num(&cfg["target"]["base"])?;
    ensure!(
        base.checked_add(num(&row["rva"])?) == Some(r.start),
        "shared RET canonical entry mismatch"
    );
    let code: Vec<(u32, u32)> = serde_json::from_value(row["code"].clone())?;
    let physical: Vec<_> = code
        .into_iter()
        .map(|(a, b)| {
            Ok((
                base.checked_add(a)
                    .context("shared RET registry overflow")?,
                base.checked_add(b)
                    .context("shared RET registry overflow")?,
            ))
        })
        .collect::<Result<_>>()?;
    ensure!(
        physical == r.code_ranges().collect::<Vec<_>>(),
        "shared RET driver/canonical extent mismatch"
    );
    Ok(())
}
fn transfer_endpoint(
    cfg: &Value,
    image: &impl ImageView,
    name: &str,
    allow_crt: bool,
) -> Result<u32> {
    let rows: Vec<_> = cfg["function_registry"]
        .as_array()
        .context("shared transfer requires canonical registry")?
        .iter()
        .filter(|row| row["name"].as_str() == Some(name))
        .collect();
    ensure!(
        rows.len() == 1
            && ((rows[0]["definition"] == 4
                && matches!(rows[0]["implementation"].as_u64(), Some(0 | 1)))
                || (allow_crt && rows[0]["definition"] == 3 && rows[0]["implementation"] == 0)),
        "shared transfer requires a unique canonical endpoint of the permitted class"
    );
    let address = num(&cfg["target"]["base"])?
        .checked_add(num(&rows[0]["rva"])?)
        .context("shared transfer endpoint overflow")?;
    image
        .read(address, 1)
        .context("shared transfer endpoint is not initialized")?;
    if image.pe_sections().is_some() || allow_crt {
        ensure!(
            image.file_backed_code(
                address,
                address
                    .checked_add(1)
                    .context("shared transfer endpoint overflow")?
            ),
            "shared transfer endpoint is not executable file-backed code"
        );
    }
    Ok(address)
}
fn app_endpoint(cfg: &Value, image: &impl ImageView, name: &str) -> Result<u32> {
    transfer_endpoint(cfg, image, name, false)
}
fn call_endpoint(cfg: &Value, image: &impl ImageView, name: &str) -> Result<u32> {
    transfer_endpoint(cfg, image, name, true)
}

fn jump_target(
    cfg: &Value,
    image: &impl ImageView,
    role: TransferRole,
) -> Result<ResolvedTransfer> {
    let address = app_endpoint(cfg, image, &role.target)?;
    Ok(ResolvedTransfer {
        offset: role.offset,
        target: role.target,
        address,
    })
}
fn validate_pair(
    image: &impl ImageView,
    owner: &Record,
    entry: &Record,
    index: &[Record],
    metadata: &Value,
) -> Result<()> {
    ensure!(
        owner.name != entry.name
            && owner.start < entry.start
            && entry.start.checked_add(1) == Some(entry.end)
            && owner.end == entry.end
            && entry.code_chunks.is_empty(),
        "shared RET must be the exact one-byte suffix of a distinct owner"
    );
    ensure!(
        image.read(entry.start, 1)? == [0xc3],
        "shared RET byte must be unprefixed C3"
    );
    let d = Decoder::new()?;
    for r in [owner, entry] {
        let g = crate::audit::structural_record(&d, image, r, metadata, "original", index, true)?;
        ensure!(
            g.instructions
                .get(&entry.start)
                .is_some_and(|i| i.ret && i.bytes == [0xc3]),
            "shared RET is not a discovered instruction start in {}",
            r.name
        );
    }
    Ok(())
}
fn validate_terminal(
    image: &impl ImageView,
    a: &Record,
    b: &Record,
    location: &TerminalLocation,
    index: &[Record],
    metadata: &Value,
) -> Result<()> {
    validate_terminal_role(image, a, b, location, index, metadata, None)
}
fn validate_terminal_role(
    image: &impl ImageView,
    a: &Record,
    b: &Record,
    location: &TerminalLocation,
    index: &[Record],
    metadata: &Value,
    transfer: Option<TerminalTransfer<'_>>,
) -> Result<()> {
    let hex = &location.bytes_hex;
    ensure!(
        hex.is_ascii() && !hex.is_empty() && hex.len() <= 64 && hex.len().is_multiple_of(2),
        "shared terminal epilogue requires 1..32 exact bytes"
    );
    let bytes = (0..hex.len() / 2)
        .map(|i| u8::from_str_radix(&hex[i * 2..i * 2 + 2], 16))
        .collect::<std::result::Result<Vec<_>, _>>()?;
    let start = location.start;
    let end = start
        .checked_add(bytes.len() as u32)
        .context("shared terminal epilogue overflow")?;
    ensure!(
        a.name != b.name,
        "shared terminal epilogue requires distinct owners"
    );
    let mut overlaps = Vec::new();
    for (left, right) in a.code_ranges() {
        for (lo, hi) in b.code_ranges() {
            let begin = left.max(lo);
            let finish = right.min(hi);
            if begin < finish {
                overlaps.push((begin, finish));
            }
        }
    }
    ensure!(
        overlaps == [(start, end)],
        "shared terminal epilogue must be the exact entire two-owner code overlap"
    );
    ensure!(
        image.read(start, bytes.len())? == bytes,
        "shared terminal epilogue bytes differ"
    );
    let d = Decoder::new()?;
    for r in [a, b] {
        ensure!(
            r.code_ranges().any(|(lo, hi)| lo <= start && hi == end),
            "shared terminal epilogue must terminate both owning code chunks"
        );
        let (g, _) = crate::audit::structural_facts(
            &d,
            image,
            r,
            metadata,
            "original",
            index,
            crate::flow::Options {
                record_dataflow: true,
                forbid_overlapping_instructions: true,
                ..Default::default()
            },
        )?;
        let mut address = start;
        let mut consumed_call = false;
        let mut consumed_jump = false;
        while address < end {
            let i = g.instructions.get(&address).with_context(|| format!(
                "shared terminal epilogue is not a discovered instruction sequence in {} at {address:x}", r.name
            ))?;
            ensure!(
                i.next() <= end
                    && i.bytes == bytes[(address - start) as usize..(i.next() - start) as usize],
                "shared terminal epilogue truncates/differs from decoded instruction"
            );
            if i.next() == end {
                if let Some(TerminalTransfer::Jump(jump) | TerminalTransfer::CallJump(_, jump)) =
                    transfer
                {
                    ensure!(
                        start.checked_add(jump.offset) == Some(address)
                            && i.bytes.len() == 5
                            && i.bytes[0] == 0xe9
                            && i.jump
                            && i.m == "jmp"
                            && i.imm() == Some(jump.address)
                            && g.tails.contains(&jump.address)
                            && g.calls.contains(&jump.address)
                            && !g.successors.contains_key(&address),
                        "shared JMP tail requires its exact terminal direct JMP and both raw tail/callee facts"
                    );
                    consumed_jump = true;
                } else {
                    ensure!(
                        i.ret && (i.bytes == [0xc3] || (i.bytes.len() == 3 && i.bytes[0] == 0xc2)),
                        "shared terminal epilogue must end in unprefixed near RET"
                    );
                }
            } else {
                let declared_call =
                    match transfer {
                        Some(
                            TerminalTransfer::Call(call) | TerminalTransfer::CallJump(call, _),
                        ) if start.checked_add(call.offset) == Some(address) => Some(call),
                        _ => None,
                    };
                if let Some(call) = declared_call {
                    ensure!(
                        i.bytes.len() == 5
                            && i.bytes[0] == 0xe8
                            && i.m == "call"
                            && i.imm() == Some(call.address)
                            && g.calls.contains(&call.address),
                        "shared return tail requires its exact decoded direct CALL and raw callee edge"
                    );
                    consumed_call = true;
                } else {
                    ensure!(
                        !i.control_transfer,
                        "shared terminal epilogue must fall through without control transfers"
                    );
                }
                ensure!(
                    g.successors.get(&address) == Some(&BTreeSet::from([i.next()])),
                    "shared terminal tail requires contiguous decoded fallthrough"
                );
            }
            address = i.next();
        }
        let consumed = match transfer {
            None => true,
            Some(TerminalTransfer::Call(_)) => consumed_call,
            Some(TerminalTransfer::Jump(_)) => consumed_jump,
            Some(TerminalTransfer::CallJump(_, _)) => consumed_call && consumed_jump,
        };
        ensure!(
            consumed,
            "shared tail transfer declaration was not consumed"
        );
    }
    Ok(())
}

impl SharedReturns {
    pub fn parse(
        cfg: &Value,
        metadata: &Value,
        image: &impl ImageView,
        drivers: &[Record],
        index: &[Record],
        project: &Path,
    ) -> Result<Self> {
        let rows: Vec<Declaration> = if let Some(value) = cfg.get("shared_return_entries") {
            let rows: Vec<Declaration> =
                serde_json::from_value(value.clone()).context("shared_return_entries schema")?;
            ensure!(!rows.is_empty(), "empty shared_return_entries");
            rows
        } else {
            vec![]
        };
        let mut out = Self::default();
        let mut entries = BTreeSet::new();
        for row in rows {
            ensure!(
                entries.insert(row.entry.clone()),
                "duplicate shared RET entry"
            );
            let owner = unique(drivers, &row.owner)?;
            let entry = unique(drivers, &row.entry)?;
            canonical(cfg, owner)?;
            canonical(cfg, entry)?;
            validate_pair(image, owner, entry, index, metadata)?;
            let evidence =
                crate::register_imports::verified_documentary_evidence(project, &row.evidence)?;
            out.0.push(Pair {
                regions: None,
                jump: None,
                call: None,
                terminal: None,
                owner: owner.clone(),
                entry: entry.clone(),
                evidence,
            });
        }
        if let Some(value) = cfg.get("shared_terminal_epilogues") {
            let rows: Vec<TerminalDeclaration> = serde_json::from_value(value.clone())
                .context("shared_terminal_epilogues schema")?;
            ensure!(!rows.is_empty(), "empty shared_terminal_epilogues");
            let mut seen = BTreeSet::new();
            for row in rows {
                let mut names = row.owners.clone();
                names.sort();
                ensure!(
                    seen.insert(names),
                    "duplicate shared terminal epilogue owners"
                );
                let a = unique(drivers, &row.owners[0])?;
                let b = unique(drivers, &row.owners[1])?;
                canonical(cfg, a)?;
                canonical(cfg, b)?;
                validate_terminal(image, a, b, &row.original, index, metadata)?;
                let evidence =
                    crate::register_imports::verified_documentary_evidence(project, &row.evidence)?;
                out.0.push(Pair {
                    regions: None,
                    jump: None,
                    call: None,
                    terminal: Some((row.original.start, row.original.bytes_hex)),
                    owner: a.clone(),
                    entry: b.clone(),
                    evidence,
                });
            }
        }
        if let Some(value) = cfg.get("shared_call_return_tails") {
            let rows: Vec<CallReturnDeclaration> =
                serde_json::from_value(value.clone()).context("shared_call_return_tails schema")?;
            ensure!(!rows.is_empty(), "empty shared_call_return_tails");
            let mut seen = BTreeSet::new();
            for row in rows {
                let mut names = row.owners.clone();
                names.sort();
                ensure!(
                    seen.insert(names),
                    "duplicate shared CALL/return-tail owners"
                );
                let a = unique(drivers, &row.owners[0])?;
                let b = unique(drivers, &row.owners[1])?;
                canonical(cfg, a)?;
                canonical(cfg, b)?;
                let target = unique(drivers, &row.call.target)?;
                canonical(cfg, target)?;
                let call = ResolvedTransfer {
                    offset: row.call.offset,
                    target: row.call.target,
                    address: target.start,
                };
                validate_terminal_role(
                    image,
                    a,
                    b,
                    &row.original,
                    index,
                    metadata,
                    Some(TerminalTransfer::Call(&call)),
                )?;
                let evidence =
                    crate::register_imports::verified_documentary_evidence(project, &row.evidence)?;
                out.0.push(Pair {
                    regions: None,
                    terminal: Some((row.original.start, row.original.bytes_hex)),
                    call: Some(call),
                    jump: None,
                    owner: a.clone(),
                    entry: b.clone(),
                    evidence,
                });
            }
        }
        if let Some(value) = cfg.get("shared_jump_tails") {
            let rows: Vec<JumpTailDeclaration> =
                serde_json::from_value(value.clone()).context("shared_jump_tails schema")?;
            ensure!(!rows.is_empty(), "empty shared_jump_tails");
            let mut seen = BTreeSet::new();
            for row in rows {
                let mut names = row.owners.clone();
                names.sort();
                ensure!(seen.insert(names), "duplicate shared JMP-tail owners");
                let a = unique(drivers, &row.owners[0])?;
                let b = unique(drivers, &row.owners[1])?;
                canonical(cfg, a)?;
                canonical(cfg, b)?;
                let jump = jump_target(cfg, image, row.jump)?;
                let call = row
                    .call
                    .map(|role| {
                        let target = unique(drivers, &role.target)?;
                        canonical(cfg, target)?;
                        Ok::<_, anyhow::Error>(ResolvedTransfer {
                            offset: role.offset,
                            target: role.target,
                            address: target.start,
                        })
                    })
                    .transpose()?;
                let transfer = match &call {
                    Some(call) => TerminalTransfer::CallJump(call, &jump),
                    None => TerminalTransfer::Jump(&jump),
                };
                validate_terminal_role(
                    image,
                    a,
                    b,
                    &row.original,
                    index,
                    metadata,
                    Some(transfer),
                )?;
                let evidence =
                    crate::register_imports::verified_documentary_evidence(project, &row.evidence)?;
                out.0.push(Pair {
                    regions: None,
                    terminal: Some((row.original.start, row.original.bytes_hex)),
                    jump: Some(jump),
                    call,
                    owner: a.clone(),
                    entry: b.clone(),
                    evidence,
                });
            }
        }
        if let Some(value) = cfg.get("shared_terminal_regions") {
            out.0.extend(terminal_regions::parse(
                cfg, metadata, image, drivers, index, project, value,
            )?);
        }
        Ok(out)
    }
    pub fn permits(&self, a: &Record, b: &Record) -> bool {
        let same = |a: &Record, b: &Record| {
            a.name == b.name
                && a.start == b.start
                && a.end == b.end
                && a.code_chunks == b.code_chunks
        };
        self.0.iter().any(|p| {
            (same(a, &p.owner) && same(b, &p.entry)) || (same(b, &p.owner) && same(a, &p.entry))
        })
    }
    pub fn terminal_report(&self) -> Option<Value> {
        self.0.iter().any(|p|p.terminal.is_some() && p.call.is_none() && p.jump.is_none()).then(||json!(self.0.iter().filter(|p|p.call.is_none() && p.jump.is_none()).filter_map(|p|p.terminal.as_ref().map(|(start,bytes)|json!({"owners":[p.owner.name,p.entry.name],"original_range":[start,start+(bytes.len()/2) as u32],"bytes_hex":bytes,"evidence":p.evidence,"additional_unique_code_bytes":0,"meaning":"only this decoded terminal epilogue is physically shared; function identities, control edges and ABI checks remain distinct; no runtime permission"}))).collect::<Vec<_>>()))
    }
    pub fn call_return_report(&self) -> Option<Value> {
        self.0.iter().any(|p|p.call.is_some() && p.jump.is_none()).then(||json!(self.0.iter().filter(|p|p.jump.is_none()).filter_map(|p|p.call.as_ref().map(|call| {
            let (start,bytes) = p.terminal.as_ref().expect("validated shared CALL terminal");
            json!({"owners":[p.owner.name,p.entry.name],"original_range":[start,start+(bytes.len()/2) as u32],
                "bytes_hex":bytes,"call":{"offset":call.offset,"target":call.target},
                "raw_call_edges":[{"owner":p.owner.name,"site":start+call.offset,"target":call.address},
                    {"owner":p.entry.name,"site":start+call.offset,"target":call.address}],
                "evidence":p.evidence,"additional_unique_code_bytes":0,
                "meaning":"only this exact decoded call/return tail is physically shared; both raw caller edges and all ABI checks remain distinct; no runtime permission"})
        })).collect::<Vec<_>>()))
    }
    pub fn jump_report(&self) -> Option<Value> {
        self.0.iter().any(|p|p.jump.is_some()).then(||json!(self.0.iter().filter_map(|p|p.jump.as_ref().map(|jump| {
            let (start,bytes)=p.terminal.as_ref().expect("validated shared JMP terminal");
            let mut report = json!({"owners":[p.owner.name,p.entry.name],"original_range":[start,start+(bytes.len()/2) as u32],
                "bytes_hex":bytes,"jump":{"offset":jump.offset,"target":jump.target},
                "raw_tail_edges":[{"owner":p.owner.name,"site":start+jump.offset,"target":jump.address},
                    {"owner":p.entry.name,"site":start+jump.offset,"target":jump.address}],
                "evidence":p.evidence,"additional_unique_code_bytes":0,
                "meaning":"only this exact decoded terminal JMP tail is physically shared; both raw edges remain; no cleanup/noreturn inference, endpoint availability or runtime permission"});
            if let Some(call) = &p.call {
                report["call"] = json!({"offset":call.offset,"target":call.target});
                report["raw_call_edges"] = json!([
                    {"owner":p.owner.name,"site":start+call.offset,"target":call.address},
                    {"owner":p.entry.name,"site":start+call.offset,"target":call.address}]);
            }
            report
        })).collect::<Vec<_>>()))
    }
    pub fn regions_report(&self) -> Option<Value> {
        let rows: Vec<_> = self.0.iter().filter_map(|p| p.regions.as_ref()).collect();
        (!rows.is_empty()).then(|| json!(rows))
    }
    pub fn report(&self) -> Option<Value> {
        self.0.iter().any(|p|p.terminal.is_none() && p.regions.is_none()).then(||json!(self.0.iter().filter(|p|p.terminal.is_none() && p.regions.is_none()).map(|p|json!({"owner":p.owner.name,"entry":p.entry.name,"original_range":[p.entry.start,p.entry.end],"bytes_hex":"c3","evidence":p.evidence,"additional_unique_code_bytes":0,"meaning":"two distinct function identities share only this terminal RET; no added call edge, runtime admission or frontier observation"})).collect::<Vec<_>>()))
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    fn record(name: &str, start: u32, end: u32) -> Record {
        Record {
            name: name.into(),
            start,
            end,
            shared: vec![],
            code_chunks: vec![],
            evidence: Value::Null,
            pending_extent: false,
        }
    }
    #[test]
    fn exact_shared_return_is_not_whole_function_alias() -> Result<()> {
        let p = crate::tests::image(&[0x90, 0xc3]);
        let a = record("owner", 0x401000, 0x401002);
        let b = record("entry", 0x401001, 0x401002);
        let index = vec![a.clone(), b.clone()];
        validate_pair(&p, &a, &b, &index, &json!({}))?;
        assert!(crate::index::validate_chunk_ownership(&p, &index).is_err());
        let policy = SharedReturns(vec![Pair {
            regions: None,
            jump: None,
            call: None,
            terminal: None,
            owner: a.clone(),
            entry: b.clone(),
            evidence: Value::Null,
        }]);
        assert!(policy.permits(&a, &b));
        assert!(policy.permits(&b, &a));
        let mut c = b.clone();
        c.name = "other".into();
        assert!(!policy.permits(&a, &c));
        assert!(crate::index::validate_chunk_ownership_with_shared(&p, &index, &policy).is_ok());
        Ok(())
    }
    #[test]
    fn two_real_owners_can_share_only_the_exact_reached_pop_pop_ret() -> Result<()> {
        let mut bytes = vec![0x90; 0x17];
        bytes[..5].copy_from_slice(&[0x53, 0x52, 0x5a, 0x5b, 0xc3]);
        bytes[0x10..].copy_from_slice(&[0x53, 0x52, 0xe9, 0xeb, 0xff, 0xff, 0xff]);
        let p = crate::tests::image(&bytes);
        let a = record("a", 0x401000, 0x401005);
        let mut b = record("b", 0x401010, 0x401017);
        b.code_chunks.push((0x401002, 0x401005));
        let index = [a.clone(), b.clone()];
        let location = TerminalLocation {
            start: 0x401002,
            bytes_hex: "5a5bc3".into(),
        };
        validate_terminal(&p, &a, &b, &location, &index, &json!({}))?;
        for hex in ["5a5b90", "c3", "5a5bc300", "", "5a5bc", "gg5bc3"] {
            assert!(
                validate_terminal(
                    &p,
                    &a,
                    &b,
                    &TerminalLocation {
                        start: location.start,
                        bytes_hex: hex.into()
                    },
                    &index,
                    &json!({})
                )
                .is_err()
            );
        }
        let mut wrong = b.clone();
        wrong.code_chunks[0].1 += 1;
        assert!(
            validate_terminal(
                &p,
                &a,
                &wrong,
                &location,
                &[a.clone(), wrong.clone()],
                &json!({})
            )
            .is_err()
        );
        Ok(())
    }
    fn suffix_case(suffix: &[u8]) -> Result<()> {
        let start = 0x401002;
        let end = start + suffix.len() as u32;
        let mut bytes = vec![0x90; 0x47];
        bytes[..2].copy_from_slice(&[0x53, 0x52]);
        bytes[2..2 + suffix.len()].copy_from_slice(suffix);
        bytes[0x40..].copy_from_slice(&[0x53, 0x52, 0xe9, 0xbb, 0xff, 0xff, 0xff]);
        let p = crate::tests::image(&bytes);
        let a = record("a", 0x401000, end);
        let mut b = record("b", 0x401040, 0x401047);
        b.code_chunks.push((start, end));
        let location = TerminalLocation {
            start,
            bytes_hex: suffix.iter().map(|v| format!("{v:02x}")).collect(),
        };
        validate_terminal(&p, &a, &b, &location, &[a.clone(), b.clone()], &json!({}))
    }
    #[test]
    fn decoded_multibyte_terminal_epilogue_retains_complete_fallthrough() -> Result<()> {
        suffix_case(&[0x8a, 0x45, 0xfc, 0xc9, 0x5a, 0x5b, 0xc3])?;
        suffix_case(&[0x90, 0xc2, 0x08, 0x00])?;
        suffix_case(&[0x90; 32]).expect_err("missing final RET");
        let mut bounded = [0x90; 32];
        bounded[31] = 0xc3;
        suffix_case(&bounded)?;
        Ok(())
    }
    #[test]
    fn shared_suffix_rejects_control_transfers_and_nonordinary_returns() {
        for bytes in [
            vec![0xeb, 0x00, 0xc3], // jump to next is still a branch
            vec![0x74, 0x00, 0xc3],
            vec![0xe8, 0, 0, 0, 0, 0xc3],
            vec![0xf3, 0xe8, 0, 0, 0, 0, 0xc3],
            vec![0xcd, 0x80, 0xc3],
            vec![0xcc, 0xc3],
            vec![0x0f, 0x34, 0xc3],
            vec![0xc9, 0xcb],
            vec![0x90, 0xf3, 0xc3],
            vec![0x8a, 0x45, 0xc3], // final C3 is an operand
        ] {
            assert!(suffix_case(&bytes).is_err(), "accepted {bytes:02x?}");
        }
    }
    fn call_tail_fixture() -> (crate::pe::Pe, Vec<Record>, Value) {
        let mut bytes = vec![0x90; 0x82];
        bytes[..8].copy_from_slice(&[0x53, 0xe8, 0x7a, 0, 0, 0, 0x5b, 0xc3]);
        bytes[0x20..0x26].copy_from_slice(&[0x53, 0xe9, 0xdb, 0xff, 0xff, 0xff]);
        bytes[0x80] = 0xc3;
        bytes[0x81] = 0xc3;
        let image = crate::tests::image(&bytes);
        let a = record("a", 0x401000, 0x401008);
        let mut b = record("b", 0x401020, 0x401026);
        b.code_chunks.push((0x401001, 0x401008));
        let target = record("callee", 0x401080, 0x401081);
        let other = record("other", 0x401081, 0x401082);
        let records = vec![a, b, target, other];
        let registry: Vec<_> = records
            .iter()
            .map(|r| {
                json!({"name":r.name,"rva":r.start-0x400000,
            "definition":4,"implementation":1,
            "code":r.code_ranges().map(|(a,b)|[a-0x400000,b-0x400000]).collect::<Vec<_>>() })
            })
            .collect();
        let cfg = json!({"target":{"base":0x400000},"function_registry":registry,
            "shared_call_return_tails":[{"owners":["a","b"],
                "original":{"start":0x401001,"bytes_hex":"e87a0000005bc3"},
                "call":{"offset":0,"target":"callee"}}]});
        (image, records, cfg)
    }
    #[test]
    fn exact_shared_call_tail_preserves_both_raw_caller_edges() -> Result<()> {
        let (image, records, cfg) = call_tail_fixture();
        let policy =
            SharedReturns::parse(&cfg, &json!({}), &image, &records, &records, Path::new("."))?;
        assert!(policy.permits(&records[0], &records[1]));
        assert!(policy.terminal_report().is_none());
        let report = policy.call_return_report().unwrap();
        assert_eq!(report[0]["raw_call_edges"].as_array().unwrap().len(), 2);
        let d = Decoder::new()?;
        for r in &records[..2] {
            let g = crate::audit::structural_record(
                &d,
                &image,
                r,
                &json!({}),
                "original",
                &records,
                true,
            )?;
            assert_eq!(g.calls, BTreeSet::from([0x401080]));
        }
        let location = TerminalLocation {
            start: 0x401001,
            bytes_hex: "e87a0000005bc3".into(),
        };
        assert!(
            validate_terminal(
                &image,
                &records[0],
                &records[1],
                &location,
                &records,
                &json!({})
            )
            .is_err()
        );
        Ok(())
    }
    #[test]
    fn shared_call_tail_rejects_wrong_unconsumed_or_unqualified_targets() {
        let (image, records, cfg) = call_tail_fixture();
        for (field, value) in [
            ("target", json!("other")),
            ("target", json!("missing")),
            ("offset", json!(1)),
            ("offset", json!(6)),
            ("offset", Value::Null),
        ] {
            let mut bad = cfg.clone();
            bad["shared_call_return_tails"][0]["call"][field] = value;
            assert!(
                SharedReturns::parse(&bad, &json!({}), &image, &records, &records, Path::new("."))
                    .is_err()
            );
        }
        let mut bad = cfg.clone();
        bad["function_registry"][2]["implementation"] = json!(0);
        assert!(
            SharedReturns::parse(&bad, &json!({}), &image, &records, &records, Path::new("."))
                .is_err()
        );
        let mut bad = cfg.clone();
        bad["function_registry"][2]["rva"] = json!(0x1081);
        assert!(
            SharedReturns::parse(&bad, &json!({}), &image, &records, &records, Path::new("."))
                .is_err()
        );
        let mut duplicate = records.clone();
        duplicate.push(records[2].clone());
        assert!(
            SharedReturns::parse(
                &cfg,
                &json!({}),
                &image,
                &duplicate,
                &duplicate,
                Path::new(".")
            )
            .is_err()
        );
    }
    fn jump_tail_fixture() -> (crate::pe::Pe, Vec<Record>, Value) {
        let (_, mut records, mut cfg) = call_tail_fixture();
        let mut bytes = vec![0x90; 0x82];
        bytes[..8].copy_from_slice(&[0x53, 0x90, 0x90, 0xe9, 0x78, 0, 0, 0]);
        bytes[0x20..0x26].copy_from_slice(&[0x53, 0xe9, 0xdb, 0xff, 0xff, 0xff]);
        bytes[0x80] = 0xcc;
        bytes[0x81] = 0xc3;
        records.retain(|r| r.name != "callee");
        cfg.as_object_mut()
            .unwrap()
            .remove("shared_call_return_tails");
        cfg["function_registry"][2]["implementation"] = json!(0);
        cfg["function_registry"][2]["code"] = json!([]);
        cfg["shared_jump_tails"] = json!([{"owners":["a","b"],
            "original":{"start":0x401001,"bytes_hex":"9090e978000000"},
            "jump":{"offset":2,"target":"callee"}}]);
        (crate::tests::image(&bytes), records, cfg)
    }
    #[test]
    fn exact_shared_jump_tail_keeps_pending_endpoint_and_both_raw_edges() -> Result<()> {
        let (image, records, cfg) = jump_tail_fixture();
        let policy =
            SharedReturns::parse(&cfg, &json!({}), &image, &records, &records, Path::new("."))?;
        assert!(policy.permits(&records[0], &records[1]));
        assert!(policy.call_return_report().is_none() && policy.terminal_report().is_none());
        assert_eq!(cfg["function_registry"][2]["implementation"], 0);
        assert_eq!(cfg["function_registry"][2]["code"], json!([]));
        assert_eq!(
            policy.jump_report().unwrap()[0]["raw_tail_edges"]
                .as_array()
                .unwrap()
                .len(),
            2
        );
        let d = Decoder::new()?;
        for r in &records[..2] {
            let g = crate::audit::structural_record(
                &d,
                &image,
                r,
                &json!({}),
                "original",
                &records,
                true,
            )?;
            assert_eq!(g.tails, BTreeSet::from([0x401080]));
            assert_eq!(g.calls, BTreeSet::from([0x401080]));
            assert!(g.returns.is_empty());
        }
        Ok(())
    }
    #[test]
    fn shared_jump_tail_requires_terminal_offset_and_unique_app_identity() {
        let (image, records, cfg) = jump_tail_fixture();
        for (field, value) in [
            ("target", json!("other")),
            ("target", json!("missing")),
            ("offset", json!(0)),
            ("offset", json!(3)),
        ] {
            let mut bad = cfg.clone();
            bad["shared_jump_tails"][0]["jump"][field] = value;
            assert!(
                SharedReturns::parse(&bad, &json!({}), &image, &records, &records, Path::new("."))
                    .is_err()
            );
        }
        let mut bad = cfg.clone();
        bad["function_registry"][2]["definition"] = json!(0);
        assert!(
            SharedReturns::parse(&bad, &json!({}), &image, &records, &records, Path::new("."))
                .is_err()
        );
        let mut bad = cfg.clone();
        bad["function_registry"][2]["rva"] = json!(0xfffffff0u32);
        assert!(
            SharedReturns::parse(&bad, &json!({}), &image, &records, &records, Path::new("."))
                .is_err()
        );
        let mut bad = cfg.clone();
        let duplicate = bad["function_registry"][2].clone();
        bad["function_registry"]
            .as_array_mut()
            .unwrap()
            .push(duplicate);
        assert!(
            SharedReturns::parse(&bad, &json!({}), &image, &records, &records, Path::new("."))
                .is_err()
        );
    }
    fn call_jump_tail_fixture() -> (crate::pe::Pe, Vec<Record>, Value) {
        let (_, mut records, mut cfg) = jump_tail_fixture();
        let mut bytes = vec![0x90; 0x82];
        bytes[..12].copy_from_slice(&[0x53, 0xe8, 0x7b, 0, 0, 0, 0x90, 0xe9, 0x74, 0, 0, 0]);
        bytes[0x20..0x26].copy_from_slice(&[0x53, 0xe9, 0xdb, 0xff, 0xff, 0xff]);
        bytes[0x80] = 0xcc;
        bytes[0x81] = 0xc3;
        records[0].end = 0x40100c;
        records[1].code_chunks = vec![(0x401001, 0x40100c)];
        cfg["function_registry"][0]["code"] = json!([[0x1000, 0x100c]]);
        cfg["function_registry"][1]["code"] = json!([[0x1020, 0x1026], [0x1001, 0x100c]]);
        cfg["shared_jump_tails"][0]["original"]["bytes_hex"] = json!("e87b00000090e974000000");
        cfg["shared_jump_tails"][0]["jump"]["offset"] = json!(6);
        cfg["shared_jump_tails"][0]["call"] = json!({"offset":0,"target":"other"});
        (crate::tests::image(&bytes), records, cfg)
    }
    #[test]
    fn shared_jump_tail_optional_call_keeps_both_raw_roles_and_pending_target() -> Result<()> {
        let (image, records, cfg) = call_jump_tail_fixture();
        let policy =
            SharedReturns::parse(&cfg, &json!({}), &image, &records, &records, Path::new("."))?;
        assert!(policy.permits(&records[0], &records[1]));
        assert!(policy.call_return_report().is_none());
        let report = policy.jump_report().unwrap();
        assert_eq!(report[0]["raw_call_edges"].as_array().unwrap().len(), 2);
        assert_eq!(report[0]["raw_tail_edges"].as_array().unwrap().len(), 2);
        assert_eq!(cfg["function_registry"][2]["implementation"], 0);
        assert_eq!(cfg["function_registry"][2]["code"], json!([]));
        let d = Decoder::new()?;
        for r in &records[..2] {
            let g = crate::audit::structural_record(
                &d,
                &image,
                r,
                &json!({}),
                "original",
                &records,
                true,
            )?;
            assert_eq!(g.tails, BTreeSet::from([0x401080]));
            assert_eq!(g.calls, BTreeSet::from([0x401080, 0x401081]));
            assert!(g.returns.is_empty());
        }
        Ok(())
    }
    #[test]
    fn shared_jump_call_requires_both_explicit_consumed_and_qualified_roles() {
        let (image, records, cfg) = call_jump_tail_fixture();
        let check = |bad: &Value| {
            SharedReturns::parse(bad, &json!({}), &image, &records, &records, Path::new("."))
                .is_err()
        };
        for value in [
            Value::Null,
            json!({}),
            json!({"offset":1,"target":"other"}),
            json!({"offset":6,"target":"other"}),
            json!({"offset":0,"target":"callee"}),
            json!({"offset":0,"target":"missing"}),
        ] {
            let mut bad = cfg.clone();
            bad["shared_jump_tails"][0]["call"] = value;
            assert!(check(&bad));
        }
        let mut bad = cfg.clone();
        bad["shared_jump_tails"][0]
            .as_object_mut()
            .unwrap()
            .remove("call");
        assert!(check(&bad));
        let mut bad = cfg.clone();
        bad["shared_jump_tails"][0]["jump"]["offset"] = json!(0);
        assert!(check(&bad));
        let mut bad = cfg.clone();
        bad["function_registry"][3]["implementation"] = json!(0);
        assert!(check(&bad));
    }
    #[test]
    fn rejects_nonret_interior_and_operand_entries() {
        for (bytes, start, end) in [
            (vec![0x90, 0x90], 0x401001, 0x401002),
            (vec![0x90, 0xc3, 0x90], 0x401001, 0x401002),
            (vec![0xb0, 0xc3], 0x401001, 0x401002),
            (vec![0x90, 0xc2, 0, 0], 0x401001, 0x401004),
        ] {
            let p = crate::tests::image(&bytes);
            let a = record("owner", 0x401000, 0x401000 + bytes.len() as u32);
            let b = record("entry", start, end);
            assert!(validate_pair(&p, &a, &b, &[a.clone(), b.clone()], &json!({})).is_err());
        }
    }
}
