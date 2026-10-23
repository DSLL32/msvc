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
