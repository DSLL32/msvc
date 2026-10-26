//! Finite semantic exit assertions stop only an exact final CALL's false continuation.
use crate::{
    decode::Decoder,
    flow::Graph,
    image::ImageView,
    index::{EntryNames, Record, num},
};
use anyhow::{Context, Result, ensure};
use serde::Deserialize;
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Declaration {
    owner: String,
    side: Side,
    site_offset: u32,
    target: String,
    role: Role,
}
#[derive(Deserialize, Clone, Copy)]
#[serde(rename_all = "lowercase")]
enum Side {
    Original,
    Rebuilt,
}
impl Side {
    fn name(self) -> &'static str {
        match self {
            Self::Original => "original",
            Self::Rebuilt => "rebuilt",
        }
    }
}
#[derive(Deserialize)]
#[serde(rename_all = "snake_case")]
enum Role {
    ProcessExit,
}
struct Resolved {
    owner: String,
    side: &'static str,
    site: u32,
    target: u32,
    report: Value,
}
#[derive(Default)]
pub struct Contracts(Vec<Resolved>);
fn endpoint(entries: &EntryNames, name: &str) -> Result<u32> {
    let addresses: Vec<_> = entries
        .iter()
        .filter(|(_, names)| names.contains(name))
        .map(|(&a, _)| a)
        .collect();
    ensure!(
        addresses.len() == 1,
        "terminating CALL needs unique mapped endpoint: {name}"
    );
    Ok(addresses[0])
}
fn row<'a>(cfg: &'a Value, name: &str) -> Result<&'a Value> {
    let rows: Vec<_> = cfg["function_registry"]
        .as_array()
        .context("terminating CALL requires canonical registry")?
        .iter()
        .filter(|r| r["name"].as_str() == Some(name))
        .collect();
    ensure!(
        rows.len() == 1,
        "terminating CALL needs unique canonical identity: {name}"
    );
    Ok(rows[0])
}
impl Contracts {
    #[allow(clippy::too_many_arguments)]
    pub fn parse<I: ImageView>(
        cfg: &Value,
        o: &I,
        r: &I,
        drivers: &[Record],
        oi: &[Record],
        ri: &[Record],
        original: &EntryNames,
        rebuilt: &EntryNames,
        selected: &BTreeSet<String>,
    ) -> Result<Self> {
        let Some(value) = cfg.get("terminating_calls") else {
            return Ok(Self::default());
        };
        let rows: Vec<Declaration> =
            serde_json::from_value(value.clone()).context("terminating_calls schema")?;
        ensure!(!rows.is_empty(), "empty terminating_calls");
        let decoder = Decoder::new()?;
        let mut out = Vec::new();
        let mut seen = BTreeSet::new();
        for decl in rows {
            let Role::ProcessExit = decl.role;
            ensure!(
                selected.contains(&decl.owner),
                "terminating CALL owner must be selected"
            );
            let owner_row = row(cfg, &decl.owner)?;
            ensure!(
                owner_row["definition"] == 4 && owner_row["implementation"] == 1,
                "terminating CALL owner must be implemented App"
            );
            let owners: Vec<_> = drivers.iter().filter(|v| v.name == decl.owner).collect();
            ensure!(
                owners.len() == 1 && !owners[0].pending_extent,
                "terminating CALL needs unique nonpending App driver"
            );
            let base = num(&cfg["target"]["base"])?;
            let canonical_code: Vec<(u32, u32)> =
                serde_json::from_value(owner_row["code"].clone())?;
            let code: Vec<_> = canonical_code
                .into_iter()
                .map(|(a, b)| {
                    Ok((
                        base.checked_add(a)
                            .context("terminating CALL code overflow")?,
                        base.checked_add(b)
                            .context("terminating CALL code overflow")?,
                    ))
                })
                .collect::<Result<_>>()?;
            ensure!(
                base.checked_add(num(&owner_row["rva"])?) == Some(owners[0].start)
                    && code == owners[0].code_ranges().collect::<Vec<_>>(),
                "terminating CALL App driver/canonical bounds mismatch"
            );
            let target_row = row(cfg, &decl.target)?;
            ensure!(
                target_row["definition"] == 3 && target_row["implementation"] == 0,
                "terminating CALL target must be classified CRT without implementation"
            );
            let original_target = endpoint(original, &decl.target)?;
            ensure!(
                base.checked_add(num(&target_row["rva"])?) == Some(original_target),
                "terminating CALL canonical target entry mismatch"
            );
            let (image, index, entries) = match decl.side {
                Side::Original => (o, oi, original),
                Side::Rebuilt => (r, ri, rebuilt),
            };
            let owner_address = endpoint(entries, &decl.owner)?;
            let bodies: Vec<_> = index
                .iter()
                .filter(|v| v.name == decl.owner && v.start == owner_address)
                .collect();
            ensure!(
                bodies.len() == 1 && !bodies[0].pending_extent,
                "terminating CALL needs unique actual owner body"
            );
            let bounds: BTreeSet<_> = index
                .iter()
                .filter(|v| v.start == owner_address && !v.pending_extent)
                .map(|v| (v.start, v.end, &v.code_chunks))
                .collect();
            ensure!(
                bounds.len() == 1,
                "terminating CALL owner has ambiguous endpoint bounds"
            );
            let body = bodies[0];
            let site = body
                .start
                .checked_add(decl.site_offset)
                .context("terminating CALL site overflow")?;
            ensure!(
                seen.insert((decl.owner.clone(), decl.side.name(), site)),
                "duplicate terminating CALL site"
            );
            let target = endpoint(entries, &decl.target)?;
            image
                .read(target, 1)
                .context("terminating CALL target must be initialized")?;
            if image.pe_sections().is_some() {
                ensure!(
                    image.file_backed_code(
                        target,
                        target
                            .checked_add(1)
                            .context("terminating CALL target overflow")?
                    ),
                    "terminating CALL target must be executable"
                );
            }
            let (_, end) = body
                .code_ranges()
                .find(|(lo, hi)| *lo <= site && site < *hi)
                .context("terminating CALL site outside owner")?;
            let i = decoder
                .one(image, site, (end - site) as usize)?
                .context("terminating CALL cannot decode")?;
            ensure!(
                i.m == "call"
                    && i.bytes.len() == 5
                    && i.bytes[0] == 0xe8
                    && i.imm() == Some(target)
                    && i.next() == end
                    && !body.contains_code(end),
                "terminating CALL must be exact final direct E8 at genuine body end"
            );
            let report = json!({"owner":decl.owner,"side":decl.side.name(),"site":site,"site_offset":decl.site_offset,
                "target":decl.target,"raw_target":target,"role":"process_exit","bytes_hex":i.bytes.iter().map(|b|format!("{b:02x}")).collect::<String>(),"body_end":end,
                "meaning":"finite reviewed process-exit assertion; raw callee and noRET retained, only this CALL's false continuation suppressed; no cleanup, body availability or runtime permission"});
            out.push(Resolved {
                owner: decl.owner,
                side: decl.side.name(),
                site,
                target,
                report,
            });
        }
        Ok(Self(out))
    }
    pub fn sites(&self, owner: &str, side: &str) -> BTreeMap<u32, u32> {
        self.0
            .iter()
            .filter(|r| r.owner == owner && r.side == side)
            .map(|r| (r.site, r.target))
            .collect()
    }
    pub fn validate_graph(&self, owner: &str, side: &str, graph: &Graph) -> Result<()> {
        let expected = self.sites(owner, side);
        ensure!(
            graph.terminating_calls == expected,
            "unconsumed/mismatched terminating CALL declaration in {owner}"
        );
        for (&site, &target) in &expected {
            ensure!(
                graph.calls.contains(&target)
                    && graph.instructions.contains_key(&site)
                    && !graph.successors.contains_key(&site),
                "terminating CALL must retain its raw callee/instruction without false continuation"
            );
        }
        Ok(())
    }
    pub fn validated_no_return(&self, owner: &str, side: &str, graph: &Graph) -> bool {
        let sites = self.sites(owner, side);
        if sites.is_empty()
            || !graph.returns.is_empty()
            || !graph.tails.is_empty()
            || graph.unresolved_tail
            || self.validate_graph(owner, side, graph).is_err()
        {
            return false;
        }
        // Only this explicit owner's existing normal CFG may close to its declared sinks.
        // No flag is inferred for unannotated tail-only wrappers or arbitrary empty RET sets.
        let mut closed: BTreeSet<_> = sites.keys().copied().collect();
        loop {
            let before = closed.len();
            for (&site, successors) in &graph.successors {
                if !successors.is_empty() && successors.iter().all(|v| closed.contains(v)) {
                    closed.insert(site);
                }
            }
            if closed.len() == before {
                break;
            }
        }
        closed.len() == graph.instructions.len()
            && graph.instructions.keys().all(|site| closed.contains(site))
    }
    pub fn report(&self) -> Option<Value> {
        (!self.0.is_empty()).then(|| json!(self.0.iter().map(|r| &r.report).collect::<Vec<_>>()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::flow::{self, Options};
    fn fixture() -> (
        crate::pe::Pe,
        Vec<Record>,
        Value,
        EntryNames,
        BTreeSet<String>,
    ) {
        let mut bytes = vec![0x90; 0x101];
        bytes[..10].copy_from_slice(&[0xb8, 1, 0, 0, 0, 0xe8, 0xf6, 0, 0, 0]);
        bytes[10] = 0xc3;
        bytes[0x100] = 0xcc;
        let image = crate::tests::image(&bytes);
        let owner = Record {
            name: "caller".into(),
            start: 0x401000,
            end: 0x40100a,
            shared: vec![],
            code_chunks: vec![],
            evidence: Value::Null,
            pending_extent: false,
        };
        let next = Record {
            name: "next".into(),
            start: 0x40100a,
            end: 0x40100b,
            ..owner.clone()
        };
        let entries = BTreeMap::from([
            (0x401000, BTreeSet::from(["caller".into()])),
            (0x401100, BTreeSet::from(["known_exit".into()])),
        ]);
        let cfg = json!({"target":{"base":0x400000},"function_registry":[
            {"name":"caller","rva":0x1000,"definition":4,"implementation":1,"code":[[0x1000,0x100a]]},
            {"name":"known_exit","rva":0x1100,"definition":3,"implementation":0,"code":[[0x1100,0x1101]]}],
            "terminating_calls":[{"owner":"caller","side":"rebuilt","site_offset":5,"target":"known_exit","role":"process_exit"}]});
        (
            image,
            vec![owner, next],
            cfg,
            entries,
            BTreeSet::from(["caller".into()]),
        )
    }
    fn parse(
        image: &crate::pe::Pe,
        records: &[Record],
        cfg: &Value,
        entries: &EntryNames,
        selected: &BTreeSet<String>,
    ) -> Result<Contracts> {
        Contracts::parse(
            cfg, image, image, records, records, records, entries, entries, selected,
        )
    }
    #[test]
    fn exact_final_call_keeps_raw_callee_and_no_ret_without_next_owner_edge() -> Result<()> {
        let (image, records, cfg, entries, selected) = fixture();
        let contract = parse(&image, &records, &cfg, &entries, &selected)?;
        let d = Decoder::new()?;
        let mut options = Options {
            fallthrough_entry: Some(0x40100a),
            record_dataflow: true,
            ..Default::default()
        };
        let original = flow::walk(&d, &image, 0x401000, 0x40100a, &options)?;
        assert_eq!(original.calls, BTreeSet::from([0x401100, 0x40100a]));
        assert_eq!(original.tails, BTreeSet::from([0x40100a]));
        options.terminating_calls = contract.sites("caller", "rebuilt");
        let graph = flow::walk(&d, &image, 0x401000, 0x40100a, &options)?;
        contract.validate_graph("caller", "rebuilt", &graph)?;
        assert_eq!(graph.calls, BTreeSet::from([0x401100]));
        assert!(graph.tails.is_empty() && graph.returns.is_empty());
        assert!(contract.validated_no_return("caller", "rebuilt", &graph));
        assert!(!contract.validated_no_return("caller", "original", &graph));
        assert!(!Contracts::default().validated_no_return("caller", "rebuilt", &graph));
        assert_eq!(graph.instructions[&0x401005].bytes, [0xe8, 0xf6, 0, 0, 0]);
        assert!(!graph.instructions.contains_key(&0x40100a));
        assert!(contract.sites("caller", "original").is_empty());
        assert_eq!(contract.report().unwrap()[0]["body_end"], 0x40100a);
        // No global name/classification inference: absence of the field keeps ordinary discovery.
        let mut absent = cfg.clone();
        absent.as_object_mut().unwrap().remove("terminating_calls");
        assert!(
            parse(&image, &records, &absent, &entries, &selected)?
                .sites("caller", "rebuilt")
                .is_empty()
        );
        Ok(())
    }
    #[test]
    fn rejects_wrong_classification_endpoint_owner_site_role_and_scope() {
        let (image, records, cfg, entries, selected) = fixture();
        for (key, value) in [
            ("site_offset", json!(0)),
            ("site_offset", json!(6)),
            ("site_offset", json!(u32::MAX)),
            ("owner", json!("next")),
            ("target", json!("missing")),
            ("side", json!("other")),
            ("role", json!("noreturn")),
        ] {
            let mut bad = cfg.clone();
            bad["terminating_calls"][0][key] = value;
            assert!(parse(&image, &records, &bad, &entries, &selected).is_err());
        }
        for (key, value) in [("definition", json!(4)), ("implementation", json!(1))] {
            let mut bad = cfg.clone();
            bad["function_registry"][1][key] = value;
            assert!(parse(&image, &records, &bad, &entries, &selected).is_err());
        }
        let mut duplicate = entries.clone();
        duplicate.insert(0x401101, BTreeSet::from(["known_exit".into()]));
        assert!(parse(&image, &records, &cfg, &duplicate, &selected).is_err());
        assert!(parse(&image, &records, &cfg, &entries, &BTreeSet::new()).is_err());
        let mut bad = cfg.clone();
        bad["terminating_calls"] = json!([
            bad["terminating_calls"][0].clone(),
            bad["terminating_calls"][0].clone()
        ]);
        assert!(parse(&image, &records, &bad, &entries, &selected).is_err());
        let mut aliases = records.clone();
        let mut alias = records[0].clone();
        alias.name = "other_name".into();
        alias.end += 1;
        aliases.push(alias);
        assert!(parse(&image, &aliases, &cfg, &entries, &selected).is_err());
        let mut long = records.clone();
        long[0].end += 1;
        assert!(parse(&image, &long, &cfg, &entries, &selected).is_err());
    }
    #[test]
    fn unreachable_static_call_does_not_seed_a_root_or_consume_the_declaration() -> Result<()> {
        let (mut image, records, cfg, entries, selected) = fixture();
        let offset = image.sections[0].off;
        image.data[offset] = 0xc3;
        let contract = parse(&image, &records, &cfg, &entries, &selected)?;
        let graph = flow::walk(
            &Decoder::new()?,
            &image,
            0x401000,
            0x40100a,
            &Options {
                terminating_calls: contract.sites("caller", "rebuilt"),
                ..Default::default()
            },
        )?;
        assert!(
            contract
                .validate_graph("caller", "rebuilt", &graph)
                .is_err()
        );
        assert!(!graph.instructions.contains_key(&0x401005));
        Ok(())
    }
    #[test]
    fn no_return_summary_rejects_cfg_cycles_and_unannotated_tail_wrappers() -> Result<()> {
        let (mut image, records, cfg, entries, selected) = fixture();
        let offset = image.sections[0].off;
        image.data[offset..offset + 5].copy_from_slice(&[0x85, 0xc0, 0x75, 0xfc, 0x90]);
        let contract = parse(&image, &records, &cfg, &entries, &selected)?;
        let d = Decoder::new()?;
        let graph = flow::walk(
            &d,
            &image,
            0x401000,
            0x40100a,
            &Options {
                terminating_calls: contract.sites("caller", "rebuilt"),
                record_dataflow: true,
                ..Default::default()
            },
        )?;
        contract.validate_graph("caller", "rebuilt", &graph)?;
        assert!(!contract.validated_no_return("caller", "rebuilt", &graph));
        image.data[offset + 0x10..offset + 0x15].copy_from_slice(&[0xe9, 0xeb, 0xff, 0xff, 0xff]);
        let wrapper = flow::walk(&d, &image, 0x401010, 0x401015, &Options::default())?;
        assert_eq!(wrapper.tails, BTreeSet::from([0x401000]));
        assert!(wrapper.returns.is_empty());
        assert!(!contract.validated_no_return("wrapper", "rebuilt", &wrapper));
        Ok(())
    }
}
