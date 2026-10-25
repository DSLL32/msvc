//! Opt-in decoded instruction facts from the existing paired CFG audit.
//! These are neither execution permissions nor independent acceptance evidence.
use crate::{
    flow::{Graph, Options},
    image::ImageView,
    index::{Inputs, Record},
};
use anyhow::{Context, Result, ensure};
use serde::Serialize;
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};

/// Hash the exact parsed semantic value, not source-file whitespace or C source.
pub fn semantic_hash(value: &impl Serialize) -> Result<String> {
    Ok(crate::index::hash(&serde_json::to_vec(
        &serde_json::to_value(value)?,
    )?))
}

pub fn side(
    image: &impl ImageView,
    record: &Record,
    graph: &Graph,
    options: &Options,
    checked_ret: Option<&BTreeSet<u32>>,
) -> Result<Value> {
    ensure!(
        !record.pending_extent,
        "cannot export pending_extent owner {}",
        record.name
    );
    let mut instructions = Vec::with_capacity(graph.instructions.len());
    for (&address, instruction) in &graph.instructions {
        ensure!(
            address == instruction.a,
            "instruction export address mismatch"
        );
        ensure!(
            (1..=15).contains(&instruction.bytes.len())
                && instruction.size as usize == instruction.bytes.len(),
            "instruction export invalid length at {address:x}"
        );
        let end = address
            .checked_add(instruction.size)
            .context("instruction export endpoint overflow")?;
        ensure!(
            record.code_ranges().any(|(a, b)| a <= address && end <= b),
            "instruction export outside owner code chunks at {address:x}"
        );
        ensure!(
            image.read(address, instruction.bytes.len())? == instruction.bytes,
            "instruction export source bytes differ at {address:x}"
        );
        instructions.push(json!({"address":address,"bytes_hex":instruction.bytes.iter().map(|b|format!("{b:02x}")).collect::<String>()}));
    }
    ensure!(
        graph.roots.contains(&record.start),
        "instruction export missing entry root"
    );
    Ok(json!({
        "record":record,
        "record_sha256":semantic_hash(record)?,
        "effective_roots":graph.roots,
        "declared_roots":options.roots,
        "declared_text_data":options.data,
        "inferred_tables":graph.tables,
        "instructions":instructions,
        "local_ret_cleanup_set":graph.returns,
        "checked_ret_cleanup_set":checked_ret,
    }))
}

pub fn finish(
    inputs: &Inputs,
    metadata: &Value,
    functions: BTreeMap<String, Value>,
    compared: &BTreeMap<String, Value>,
) -> Result<Value> {
    ensure!(
        functions.keys().eq(compared.keys()),
        "instruction export owner selection differs from checked graph selection"
    );
    Ok(json!({
        "schema":1,
        "semantics":"decoded instruction facts only; not execution permission or durable acceptance",
        "inputs":{
            "images":{"original":inputs.o.provenance(),"rebuilt":inputs.r.provenance()},
            "config_sha256":inputs.config_semantic_sha256.as_ref().context("missing instruction export config binding")?,
            "original_index_sha256":semantic_hash(&inputs.oi)?,
            "rebuilt_index_sha256":semantic_hash(&inputs.ri)?,
            "accepted_names_sha256":semantic_hash(&inputs.accepted)?,
            "public_symbols_sha256":semantic_hash(&inputs.publics)?,
            "metadata_sha256":semantic_hash(metadata)?,
        },
        "functions":functions,
    }))
}

/// Explicit original CRT selectors never turn endpoint identities into bodies.
pub fn support_records(
    config: &Value,
    image: &impl ImageView,
    index: &[Record],
    selected: &[String],
    project: &std::path::Path,
) -> Result<Vec<Record>> {
    if selected.is_empty() {
        return Ok(vec![]);
    }
    let registry = config["function_registry"]
        .as_array()
        .context("original support export requires function_registry")?;
    let base = crate::index::num(&config["target"]["base"])?;
    let mut names = BTreeSet::new();
    let mut result = Vec::new();
    for name in selected {
        ensure!(
            !name.is_empty() && names.insert(name),
            "empty or duplicate original support selector: {name}"
        );
        let entries: Vec<_> = registry
            .iter()
            .filter(|entry| entry["name"] == name.as_str())
            .collect();
        ensure!(
            entries.len() == 1,
            "missing or ambiguous original support registry owner: {name}"
        );
        let entry = entries[0];
        ensure!(
            entry["definition"] == 3 && entry["implementation"] == 0,
            "original support owner must be classified CRT without implementation: {name}"
        );
        let records: Vec<_> = index.iter().filter(|record| &record.name == name).collect();
        ensure!(
            records.len() == 1,
            "missing or ambiguous original support index owner: {name}"
        );
        let record = records[0];
        ensure!(
            !record.pending_extent,
            "original support owner extent is pending: {name}"
        );
        let convert = |value: &Value| -> Result<Vec<(u32, u32)>> {
            let ranges: Vec<(u32, u32)> = serde_json::from_value(value.clone())?;
            ranges
                .into_iter()
                .map(|(a, b)| {
                    ensure!(a < b, "invalid original support canonical interval: {name}");
                    Ok((
                        base.checked_add(a)
                            .context("original support address overflow")?,
                        base.checked_add(b)
                            .context("original support address overflow")?,
                    ))
                })
                .collect()
        };
        let code = convert(&entry["code"])?;
        let shared = convert(&entry["text_data"])?;
        ensure!(
            !code.is_empty()
                && code[0].0
                    == base
                        .checked_add(crate::index::num(&entry["rva"])?)
                        .context("original support entry overflow")?
                && code == record.code_ranges().collect::<Vec<_>>()
                && shared == record.shared,
            "original support canonical/index ranges differ: {name}"
        );
        ensure!(
            code.iter().all(|&(a, b)| image.file_backed_code(a, b))
                && shared.iter().all(|&(a, b)| image.text_data_backed(a, b)),
            "original support ranges lack initialized ownership: {name}"
        );
        for app in registry.iter().filter(|entry| entry["definition"] == 4) {
            let address = base
                .checked_add(crate::index::num(&app["rva"])?)
                .context("application entry overflow")?;
            ensure!(
                !record.contains_code(address),
                "original support includes an application entry: {name}"
            );
        }
        let mut record = record.clone();
        record.evidence = crate::register_imports::verified_evidence(project, &record.evidence)
            .with_context(|| format!("original support witness {name}"))?;
        result.push(record);
    }
    // Entry-only pins have no owned interval. All real declared bodies still
    // constrain a support selection; a support label cannot override an owner.
    let owners: Vec<_> = index
        .iter()
        .filter(|record| record.start != record.end || !record.code_chunks.is_empty())
        .cloned()
        .collect();
    crate::index::validate_chunk_ownership(image, &owners).context("original support ownership")?;
    result.sort_by(|a, b| a.name.cmp(&b.name));
    Ok(result)
}

pub fn support_side(
    image: &impl ImageView,
    record: &Record,
    graph: &Graph,
    options: &Options,
) -> Result<Value> {
    let mut digests = Vec::new();
    for (kind, ranges) in [
        (
            "declared-code-interval",
            record.code_ranges().collect::<Vec<_>>(),
        ),
        ("declared-shared-data", record.shared.clone()),
    ] {
        for (start, end) in ranges {
            digests.push(json!({"kind":kind,"start":start,"end":end,
                "sha256":crate::index::hash(image.read(start,(end-start) as usize)?)}));
        }
    }
    Ok(
        json!({"classification":"CRT","selection_source":"explicit original-only support selector and hash-bound witness; no admission",
        "evidence":record.evidence,"declared_range_sha256":digests,
        "original":side(image,record,graph,options,None)?,
        "direct_tails":graph.tails,"unresolved_tail":graph.unresolved_tail}),
    )
}

pub fn finish_support(
    inputs: &Inputs,
    metadata: &Value,
    functions: BTreeMap<String, Value>,
) -> Result<Value> {
    ensure!(
        functions
            .keys()
            .eq(inputs.original_support.iter().map(|record| &record.name)),
        "original support export selection differs"
    );
    Ok(json!({"schema":1,
        "semantics":"original-only decoded support facts under the existing calls-may-return CFG model; no paired equivalence, checked ABI, application acceptance or execution permission",
        "inputs":{"images":{"original":inputs.o.provenance()},
            "config_sha256":inputs.config_semantic_sha256.as_ref().context("missing support config binding")?,
            "original_index_sha256":semantic_hash(&inputs.oi)?,"metadata_sha256":semantic_hash(metadata)?,
            "selected_support_sha256":semantic_hash(&inputs.original_support)?},
        "functions":functions}))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{decode::Decoder, flow};
    struct Bytes(Vec<u8>);
    impl ImageView for Bytes {
        fn read(&self, a: u32, n: usize) -> Result<&[u8]> {
            let a = a.checked_sub(0x1000).context("below image")? as usize;
            self.0.get(a..a + n).context("outside source bytes")
        }
        fn file_backed_code(&self, a: u32, b: u32) -> bool {
            0x1000 <= a && a < b && b <= 0x1000 + self.0.len() as u32
        }
    }
    fn fixture(bytes: Vec<u8>) -> (Bytes, Record, Options, Graph) {
        let image = Bytes(bytes);
        let record = Record {
            name: "owner".into(),
            start: 0x1000,
            end: 0x1000 + image.0.len() as u32,
            code_chunks: vec![],
            shared: vec![],
            evidence: Value::Null,
            pending_extent: false,
        };
        let options = Options {
            record_dataflow: true,
            strict: true,
            ..Default::default()
        };
        let graph = flow::walk(
            &Decoder::new().unwrap(),
            &image,
            record.start,
            record.end,
            &options,
        )
        .unwrap();
        (image, record, options, graph)
    }
    #[test]
    fn support_selection_requires_canonical_crt_extent_and_bound_witness() {
        let (image, mut record, _, _) = fixture(vec![0x90, 0xc3]);
        record.evidence = json!({"path":"fixtures/register-preservation.json","sha256":crate::index::hash(include_bytes!("../fixtures/register-preservation.json"))});
        let cfg = json!({"target":{"base":4096},"function_registry":[{"name":"owner","definition":3,"implementation":0,"rva":0,"code":[[0,2]],"text_data":[]}]});
        let names = vec!["owner".to_string()];
        let project = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
        assert_eq!(
            support_records(&cfg, &image, &[record.clone()], &names, project)
                .unwrap()
                .len(),
            1
        );
        assert!(
            support_records(&json!({}), &image, &[], &[], project)
                .unwrap()
                .is_empty()
        );
        for (key, value) in [
            ("definition", json!(4)),
            ("definition", json!(0)),
            ("implementation", json!(1)),
            ("code", json!([])),
            ("code", json!([[0, 1]])),
            ("rva", json!(1)),
        ] {
            let mut wrong = cfg.clone();
            wrong["function_registry"][0][key] = value;
            assert!(
                support_records(&wrong, &image, &[record.clone()], &names, project).is_err(),
                "{key}"
            );
        }
        for evidence in [
            Value::Null,
            json!("unchecked"),
            json!({"path":"fixtures/register-preservation.json","sha256":"0".repeat(64)}),
            json!({"path":"/etc/passwd","sha256":"0".repeat(64)}),
        ] {
            let mut wrong = record.clone();
            wrong.evidence = evidence;
            assert!(support_records(&cfg, &image, &[wrong], &names, project).is_err());
        }
        let mut wrong = record.clone();
        wrong.pending_extent = true;
        assert!(support_records(&cfg, &image, &[wrong], &names, project).is_err());
        assert!(
            support_records(
                &cfg,
                &image,
                &[record.clone(), record.clone()],
                &names,
                project
            )
            .is_err()
        );
        assert!(
            support_records(
                &cfg,
                &image,
                &[record.clone()],
                &["owner".into(), "owner".into()],
                project
            )
            .is_err()
        );
        let mut wrong = cfg.clone();
        wrong["function_registry"]
            .as_array_mut()
            .unwrap()
            .push(json!({"name":"app","definition":4,"implementation":0,"rva":1}));
        assert!(support_records(&wrong, &image, &[record], &names, project).is_err());
    }

    #[test]
    fn support_tail_and_local_ret_are_facts_without_a_counterpart_or_checked_abi() {
        for code in [vec![0xe9, 0x0b, 0, 0, 0], vec![0xc3]] {
            let (image, record, options, graph) = fixture(code.clone());
            let report = support_side(&image, &record, &graph, &options).unwrap();
            assert!(report.get("rebuilt").is_none());
            assert!(report["original"]["checked_ret_cleanup_set"].is_null());
            assert_eq!(
                report["declared_range_sha256"][0]["sha256"],
                crate::index::hash(&code)
            );
            if code[0] == 0xe9 {
                assert_eq!(report["original"]["local_ret_cleanup_set"], json!([]));
                assert_eq!(report["direct_tails"], json!([0x1010]));
            } else {
                assert_eq!(report["original"]["local_ret_cleanup_set"], json!([0]));
            }
        }
    }

    #[test]
    fn exact_bytes_roots_and_checked_vs_local_cleanup() {
        let (image, record, options, graph) = fixture(vec![0x90, 0xc3]);
        let report = side(&image, &record, &graph, &options, None).unwrap();
        assert_eq!(
            report["instructions"],
            json!([{"address":4096,"bytes_hex":"90"},{"address":4097,"bytes_hex":"c3"}])
        );
        assert_eq!(report["effective_roots"], json!([4096]));
        assert_eq!(report["local_ret_cleanup_set"], json!([0]));
        assert_eq!(report["checked_ret_cleanup_set"], Value::Null);
        assert_eq!(
            side(
                &image,
                &record,
                &graph,
                &options,
                Some(&BTreeSet::from([0]))
            )
            .unwrap()["checked_ret_cleanup_set"],
            json!([0])
        );
    }
    #[test]
    fn source_byte_length_address_extent_and_pending_guards() {
        let (image, mut record, options, mut graph) = fixture(vec![0x90, 0xc3]);
        graph.instructions.get_mut(&0x1000).unwrap().bytes[0] = 0x91;
        assert!(
            side(&image, &record, &graph, &options, None)
                .unwrap_err()
                .to_string()
                .contains("source bytes differ")
        );
        graph.instructions.get_mut(&0x1000).unwrap().bytes[0] = 0x90;
        graph.instructions.get_mut(&0x1000).unwrap().size = 0;
        assert!(
            side(&image, &record, &graph, &options, None)
                .unwrap_err()
                .to_string()
                .contains("invalid length")
        );
        graph.instructions.get_mut(&0x1000).unwrap().size = 1;
        graph.instructions.get_mut(&0x1000).unwrap().a = 0x1001;
        assert!(
            side(&image, &record, &graph, &options, None)
                .unwrap_err()
                .to_string()
                .contains("address mismatch")
        );
        graph.instructions.get_mut(&0x1000).unwrap().a = 0x1000;
        record.end = 0x1001;
        assert!(
            side(&image, &record, &graph, &options, None)
                .unwrap_err()
                .to_string()
                .contains("outside owner code chunks")
        );
        record.end = 0x1002;
        record.pending_extent = true;
        assert!(
            side(&image, &record, &graph, &options, None)
                .unwrap_err()
                .to_string()
                .contains("pending_extent")
        );
    }
    #[test]
    fn data_unreachable_bytes_and_chunk_gaps_are_not_generated_code() {
        let (image, record, mut options, _) = fixture(vec![0xeb, 1, 0x42, 0xc3, 0xc3]);
        options.data.push((0x1002, 0x1003));
        let graph = flow::walk(
            &Decoder::new().unwrap(),
            &image,
            record.start,
            record.end,
            &options,
        )
        .unwrap();
        let report = side(&image, &record, &graph, &options, None).unwrap();
        assert_eq!(
            report["instructions"],
            json!([{"address":4096,"bytes_hex":"eb01"},{"address":4099,"bytes_hex":"c3"}])
        );
        let (image, mut record, mut options, _) = fixture(vec![0xeb, 2, 0x90, 0x90, 0xc3]);
        record.end = 0x1002;
        record.code_chunks = vec![(0x1004, 0x1005)];
        options.code_chunks = record.code_chunks.clone();
        let graph = flow::walk(
            &Decoder::new().unwrap(),
            &image,
            record.start,
            record.end,
            &options,
        )
        .unwrap();
        let report = side(&image, &record, &graph, &options, None).unwrap();
        assert_eq!(
            report["instructions"],
            json!([{"address":4096,"bytes_hex":"eb02"},{"address":4100,"bytes_hex":"c3"}])
        );
        assert_eq!(report["effective_roots"], json!([4096]));
    }
    #[test]
    fn aliases_and_valid_overlapping_streams_remain_facts() {
        let (image, mut record, options, graph) = fixture(vec![0xeb, 0xff, 0xc3, 0xc3]);
        let first = side(&image, &record, &graph, &options, None).unwrap();
        assert_eq!(first["instructions"].as_array().unwrap().len(), 3);
        assert_eq!(first["instructions"][0]["address"], 4096);
        assert_eq!(first["instructions"][1]["address"], 4097);
        record.name = "alias".into();
        let alias = side(&image, &record, &graph, &options, None).unwrap();
        assert_eq!(first["instructions"], alias["instructions"]);
        assert_ne!(first["record_sha256"], alias["record_sha256"]);
    }
    #[test]
    fn semantic_binding_uses_canonical_consumed_json() {
        assert_eq!(
            semantic_hash(&json!({"b":2,"a":1})).unwrap(),
            "43258cff783fe7036d8a43033f830adfc60ec037382473548ac742b888292777"
        );
    }
}
