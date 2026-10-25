//! Explicit paired CALL boundaries. These facts classify only the selected
//! instructions; they provide no callee identity, ABI, preservation or permission.
use crate::{
    decode::{Decoder, Inst},
    flow::Graph,
    image::ImageView,
    imports::{Catalog, Identity, Symbol},
    index::{Inputs, Record},
};
use anyhow::{Context, Result, ensure};
use serde::Deserialize;
use serde_json::{Value, json};
use std::{
    collections::{BTreeMap, BTreeSet},
    path::Path,
};

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Contract {
    owner: String,
    name: String,
    original: Original,
    rebuilt: Rebuilt,
    evidence: Value,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Original {
    site: u32,
    target: u32,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Rebuilt {
    site_offset: u32,
    import: NamedImport,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct NamedImport {
    dll: String,
    name: String,
}
struct Bound {
    name: String,
    original: Inst,
    rebuilt: Inst,
    report: Value,
}
#[derive(Default)]
pub struct Policy(BTreeMap<String, Vec<Bound>>);
fn owner<'a>(records: &'a [Record], name: &str) -> Result<&'a Record> {
    let found: Vec<_> = records.iter().filter(|r| r.name == name).collect();
    ensure!(
        found.len() == 1 && !found[0].pending_extent,
        "compatibility CALL owner missing, ambiguous or pending: {name}"
    );
    Ok(found[0])
}
fn instruction(d: &Decoder, image: &impl ImageView, r: &Record, site: u32) -> Result<Inst> {
    let end = r
        .code_ranges()
        .find(|&(s, e)| s <= site && site < e)
        .map(|(_, e)| e)
        .context("compatibility CALL outside owner code")?;
    let i = d
        .one(image, site, (end - site).min(15) as usize)?
        .context("undecodable compatibility CALL")?;
    ensure!(
        i.next() <= end && i.m == "call",
        "compatibility site is not an owned CALL"
    );
    Ok(i)
}
fn direct(i: &Inst) -> Option<u32> {
    (i.m == "call" && i.bytes.len() == 5 && i.bytes[0] == 0xe8)
        .then(|| i.imm())
        .flatten()
}
fn imported<'a>(i: &Inst, catalog: &'a Catalog) -> Option<&'a Identity> {
    if let Some(target) = direct(i) {
        catalog.thunks.get(&target)
    } else if i.m == "call" && i.bytes.starts_with(&[0xff, 0x15]) {
        catalog.absolute_transfer(i)
    } else {
        None
    }
}
fn facts(i: &Inst) -> Value {
    json!({"site":i.a,"bytes_hex":i.bytes.iter().map(|b|format!("{b:02x}")).collect::<String>(),"direct_target":i.imm(),"absolute_slot":i.mem().map(|m|m.disp)})
}
impl Policy {
    pub fn parse(
        metadata: &Value,
        x: &Inputs,
        selected: &BTreeSet<String>,
        d: &Decoder,
        project: &Path,
    ) -> Result<Self> {
        let Some(value) = metadata.get("compatibility_calls") else {
            return Ok(Self::default());
        };
        let contracts: Vec<Contract> =
            serde_json::from_value(value.clone()).context("compatibility_calls schema")?;
        ensure!(
            !contracts.is_empty(),
            "empty compatibility_calls declaration"
        );
        let catalog = Catalog::load(x.r.require_pe()?)?;
        let mut out = Self::default();
        for c in contracts {
            ensure!(
                selected.contains(&c.owner),
                "compatibility CALL owner not selected/accepted: {}",
                c.owner
            );
            ensure!(
                !c.name.is_empty()
                    && c.name
                        .bytes()
                        .all(|b| b.is_ascii_alphanumeric() || b"_-".contains(&b)),
                "invalid compatibility CALL role name"
            );
            let left = owner(&x.drivers, &c.owner)?;
            let right = owner(&x.ri, &c.owner)?;
            let original = instruction(d, &x.o, left, c.original.site)?;
            ensure!(
                direct(&original) == Some(c.original.target),
                "compatibility original CALL target/kind mismatch"
            );
            let site = right
                .start
                .checked_add(c.rebuilt.site_offset)
                .context("compatibility rebuilt site overflow")?;
            let rebuilt = instruction(d, &x.r, right, site)?;
            ensure!(
                !c.rebuilt.import.dll.is_empty() && !c.rebuilt.import.name.is_empty(),
                "empty compatibility import identity"
            );
            let expected = Identity {
                dll: c.rebuilt.import.dll.to_ascii_lowercase(),
                symbol: Symbol::Name {
                    name: c.rebuilt.import.name,
                },
            };
            let actual = imported(&rebuilt, &catalog)
                .context("compatibility rebuilt CALL is not a standard PE import")?;
            ensure!(
                actual == &expected,
                "compatibility rebuilt import identity mismatch"
            );
            let evidence = crate::register_imports::verified_evidence(project, &c.evidence)?;
            let report = json!({"name":c.name,"original":facts(&original),"rebuilt":facts(&rebuilt),"rebuilt_import":actual,"evidence":evidence,"meaning":"paired compatibility CALL boundary only; no callee equivalence, ABI, preservation or execution permission"});
            let rows = out.0.entry(c.owner).or_default();
            ensure!(
                rows.iter().all(|r| r.name != c.name
                    && r.original.a != original.a
                    && r.rebuilt.a != rebuilt.a),
                "duplicate compatibility CALL name/site"
            );
            rows.push(Bound {
                name: c.name,
                original,
                rebuilt,
                report,
            });
        }
        Ok(out)
    }
    pub fn sites(&self, owner: &str, rebuilt: bool) -> BTreeMap<u32, String> {
        self.0
            .get(owner)
            .into_iter()
            .flatten()
            .map(|r| {
                (
                    if rebuilt { r.rebuilt.a } else { r.original.a },
                    r.name.clone(),
                )
            })
            .collect()
    }
    pub fn checked(&self, owner: &str, original: &Graph, rebuilt: &Graph) -> Result<Option<Value>> {
        let Some(rows) = self.0.get(owner) else {
            return Ok(None);
        };
        ensure!(
            original.compatibility_calls == self.sites(owner, false)
                && rebuilt.compatibility_calls == self.sites(owner, true),
            "unconsumed compatibility CALL declaration for {owner}"
        );
        for row in rows {
            for (graph, expected) in [(original, &row.original), (rebuilt, &row.rebuilt)] {
                ensure!(
                    graph
                        .instructions
                        .get(&expected.a)
                        .is_some_and(|i| i.bytes == expected.bytes),
                    "compatibility CALL is not a discovered instruction start"
                );
            }
        }
        Ok(Some(Value::Array(
            rows.iter().map(|r| r.report.clone()).collect(),
        )))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn exact_call_kinds_and_imports() -> Result<()> {
        let d = Decoder::new()?;
        let p = crate::tests::image(&[0xe8, 0, 0, 0, 0, 0xff, 0x15, 0x20, 0x10, 0x40, 0, 0xc3]);
        let a = d.one(&p, 0x401000, 5)?.unwrap();
        assert_eq!(direct(&a), Some(0x401005));
        let id = Identity {
            dll: "kernel32.dll".into(),
            symbol: Symbol::Name {
                name: "LocalAlloc".into(),
            },
        };
        let catalog = Catalog {
            thunks: BTreeMap::from([(0x401005, id.clone())]),
            slots: BTreeMap::from([(0x401020, id.clone())]),
        };
        assert_eq!(imported(&a, &catalog), Some(&id));
        let b = d.one(&p, 0x401005, 6)?.unwrap();
        assert_eq!(direct(&b), None);
        assert_eq!(imported(&b, &catalog), Some(&id));
        assert!(imported(&d.one(&p, 0x40100b, 1)?.unwrap(), &catalog).is_none());
        Ok(())
    }
    #[test]
    fn boundary_keeps_unrelated_calls_and_instruction_flow() -> Result<()> {
        // Two calls to the same target: declaring one never suppresses the other.
        let p = crate::tests::image(&[0xe8, 11, 0, 0, 0, 0xe8, 6, 0, 0, 0, 0xff, 0xd0, 0xc3]);
        let options = crate::flow::Options {
            compatibility_calls: BTreeMap::from([(0x401000, "allocate".into())]),
            record_dataflow: true,
            ..Default::default()
        };
        let g = crate::flow::walk(&Decoder::new()?, &p, 0x401000, 0x40100d, &options)?;
        assert_eq!(g.calls, BTreeSet::from([0x401010]));
        assert_eq!(g.compatibility_calls, options.compatibility_calls);
        assert!(g.indirect);
        assert_eq!(g.returns, BTreeSet::from([0]));
        assert_eq!(g.instructions.len(), 4);
        assert!(g.successors[&0x401000].contains(&0x401005));
        Ok(())
    }
    #[test]
    fn schema_rejects_typos() {
        assert!(serde_json::from_value::<Vec<Contract>>(json!([{"owner":"x","name":"a","original":{"site":1,"target":2},"rebuilt":{"site_offset":0,"import":{"dll":"k","name":"a"}},"evidnce":{}}])).is_err());
    }
}
