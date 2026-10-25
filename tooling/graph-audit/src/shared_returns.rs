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
    evidence: Value,
}
struct Pair {
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
impl SharedReturns {
    pub fn parse(
        cfg: &Value,
        metadata: &Value,
        image: &impl ImageView,
        drivers: &[Record],
        index: &[Record],
        project: &Path,
    ) -> Result<Self> {
        let Some(value) = cfg.get("shared_return_entries") else {
            return Ok(Self::default());
        };
        let rows: Vec<Declaration> =
            serde_json::from_value(value.clone()).context("shared_return_entries schema")?;
        ensure!(!rows.is_empty(), "empty shared_return_entries");
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
            let evidence = crate::register_imports::verified_evidence(project, &row.evidence)?;
            out.0.push(Pair {
                owner: owner.clone(),
                entry: entry.clone(),
                evidence,
            });
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
    pub fn report(&self) -> Option<Value> {
        (!self.0.is_empty()).then(||json!(self.0.iter().map(|p|json!({"owner":p.owner.name,"entry":p.entry.name,"original_range":[p.entry.start,p.entry.end],"bytes_hex":"c3","evidence":p.evidence,"additional_unique_code_bytes":0,"meaning":"two distinct function identities share only this terminal RET; no added call edge, runtime admission or frontier observation"})).collect::<Vec<_>>()))
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
