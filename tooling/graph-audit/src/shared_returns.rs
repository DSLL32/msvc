//! Explicit ownership of a single shared RET instruction, not a function alias.
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
struct Pair {
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
    let hex = &location.bytes_hex;
    ensure!(
        hex.len() == 6 && hex.is_ascii(),
        "shared terminal epilogue requires exactly two register POPs and RET"
    );
    let bytes = (0..3)
        .map(|i| u8::from_str_radix(&hex[i * 2..i * 2 + 2], 16))
        .collect::<std::result::Result<Vec<_>, _>>()?;
    ensure!(
        bytes[..2]
            .iter()
            .all(|b| (0x58..=0x5f).contains(b) && *b != 0x5c)
            && bytes[2] == 0xc3,
        "shared terminal epilogue requires two non-ESP register POPs then C3"
    );
    let start = location.start;
    let end = start
        .checked_add(3)
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
        image.read(start, 3)? == bytes,
        "shared terminal epilogue bytes differ"
    );
    let d = Decoder::new()?;
    for r in [a, b] {
        ensure!(
            r.code_ranges().any(|(lo, hi)| lo <= start && hi == end),
            "shared terminal epilogue must terminate both owning code chunks"
        );
        let g = crate::audit::structural_record(&d, image, r, metadata, "original", index, true)?;
        for (n, byte) in bytes.iter().enumerate() {
            ensure!(
                g.instructions.get(&(start + n as u32)).is_some_and(
                    |i| i.bytes == [*byte] && if n == 2 { i.ret } else { i.m == "pop" }
                ),
                "shared terminal epilogue is not a complete discovered instruction sequence in {}",
                r.name
            );
        }
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
                    terminal: Some((row.original.start, row.original.bytes_hex)),
                    owner: a.clone(),
                    entry: b.clone(),
                    evidence,
                });
            }
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
        self.0.iter().any(|p|p.terminal.is_some()).then(||json!(self.0.iter().filter_map(|p|p.terminal.as_ref().map(|(start,bytes)|json!({"owners":[p.owner.name,p.entry.name],"original_range":[start,start+3],"bytes_hex":bytes,"evidence":p.evidence,"additional_unique_code_bytes":0,"meaning":"only this decoded terminal epilogue is physically shared; function identities, control edges and ABI checks remain distinct; no runtime permission"}))).collect::<Vec<_>>()))
    }
    pub fn report(&self) -> Option<Value> {
        self.0.iter().any(|p|p.terminal.is_none()).then(||json!(self.0.iter().filter(|p|p.terminal.is_none()).map(|p|json!({"owner":p.owner.name,"entry":p.entry.name,"original_range":[p.entry.start,p.entry.end],"bytes_hex":"c3","evidence":p.evidence,"additional_unique_code_bytes":0,"meaning":"two distinct function identities share only this terminal RET; no added call edge, runtime admission or frontier observation"})).collect::<Vec<_>>()))
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
        for hex in ["5c5bc3", "5a5b90", "c3", "5a5bc300"] {
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
