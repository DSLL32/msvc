//! Adapt canonical registry facts to CVPACK's existing strict audit policy.
//! This grants no support availability or application credit.
use crate::{
    index::{hash, num},
    pe::Pe,
};
use anyhow::{Context, Result, ensure};
use serde_json::{Value, json};
use std::{
    collections::{BTreeMap, BTreeSet},
    path::Path,
};

pub fn configuration(input: &Value, metadata: &Value, runtime: &Value) -> Result<Value> {
    ensure!(
        metadata["schema"] == 1,
        "unsupported CVPACK graph metadata schema"
    );
    let base = num(&input["target"]["base"])?;
    let registry = input["function_registry"]
        .as_array()
        .context("CVPACK registry projection requires function_registry")?;
    let drivers = input["driver"]
        .as_array()
        .context("CVPACK registry driver")?;
    let accepted: BTreeSet<String> = serde_json::from_value(input["accepted"].clone())?;
    let mut functions = Vec::new();
    let mut names = BTreeSet::new();
    let mut used = BTreeSet::new();
    for item in registry.iter().filter(|r| r["definition"] == 4) {
        let name = item["name"].as_str().context("CVPACK application name")?;
        ensure!(
            names.insert(name),
            "duplicate CVPACK registry application: {name}"
        );
        let start = base
            .checked_add(num(&item["rva"])?)
            .context("CVPACK registry address overflow")?;
        let implemented = item["implementation"] == 1;
        ensure!(
            accepted.contains(name) == implemented,
            "CVPACK registry application acceptance differs: {name}"
        );
        let symbol = item["original_symbol"].as_str().unwrap_or(name);
        let mut row = json!({"name":name,"symbol":symbol,"original":start,"kind":"application","status":if implemented{"accepted"}else{"stub"},"extent":null,"text_data":[]});
        if implemented {
            let found: Vec<_> = drivers.iter().filter(|d| d["name"] == name).collect();
            ensure!(
                found.len() == 1,
                "CVPACK accepted driver missing or ambiguous: {name}"
            );
            let driver = found[0];
            ensure!(
                num(&driver["start"])? == start,
                "CVPACK driver entry differs: {name}"
            );
            ensure!(
                driver["code_chunks"].as_array().is_none_or(Vec::is_empty),
                "CVPACK registry adapter does not support fragmented bodies"
            );
            row["extent"] = json!([driver["start"], driver["end"]]);
            row["text_data"] = driver["shared"].clone();
            row["symbol"] = driver
                .get("symbol")
                .cloned()
                .unwrap_or_else(|| json!(symbol));
            used.insert(name);
        }
        let policy = metadata["functions"][name]
            .as_object()
            .with_context(|| format!("missing CVPACK function policy: {name}"))?;
        for (key, value) in policy {
            ensure!(
                [
                    "abi",
                    "stack_cleanup",
                    "roots",
                    "alignment_padding",
                    "switches",
                    "switch_tables",
                    "rebuilt_extra_roots",
                    "compiler_seh_scaffolding",
                    "compiler_switch_counts",
                    "return_bits",
                    "source"
                ]
                .contains(&key.as_str()),
                "unsupported CVPACK function policy field: {name}.{key}"
            );
            row[key] = value.clone();
        }
        functions.push(row);
    }
    ensure!(
        used.len() == drivers.len(),
        "CVPACK driver contains non-application or duplicate rows"
    );
    let known: Vec<_> = runtime["functions"]
        .as_array()
        .context("original runtime functions")?
        .iter()
        .chain(
            runtime["imports"]
                .as_array()
                .context("original runtime imports")?,
        )
        .collect();
    let mut support = Vec::new();
    let mut restored = BTreeSet::new();
    for name in input["restored_runtime_names"]
        .as_array()
        .context("CVPACK restored_runtime_names")?
    {
        let name = name.as_str().context("restored runtime name")?;
        ensure!(
            restored.insert(name),
            "duplicate restored runtime name: {name}"
        );
        ensure!(
            !runtime["callee_identity_anchors"]
                .as_array()
                .into_iter()
                .flatten()
                .any(|r| r["name"] == name),
            "identity-only anchor cannot be restored support: {name}"
        );
        let found: Vec<_> = known.iter().filter(|r| r["name"] == name).collect();
        let ranges: BTreeSet<_> = found
            .iter()
            .map(|r| Ok((num(&r["start"])?, num(&r["end"])?)))
            .collect::<Result<_>>()?;
        ensure!(
            ranges.len() == 1,
            "restored runtime identity missing or ambiguous: {name}"
        );
        let record = found[0];
        support.push(json!({"name":name,"start":record["start"],"end":record["end"]}));
    }
    let mut groups = BTreeMap::<(u32, u32), BTreeSet<String>>::new();
    for record in &known {
        groups
            .entry((num(&record["start"])?, num(&record["end"])?))
            .or_default()
            .insert(record["name"].as_str().context("runtime name")?.into());
    }
    let aliases: Vec<_> = groups
        .into_iter()
        .filter(|(_, names)| names.len() > 1)
        .map(|((a, b), names)| json!({"start":a,"end":b,"names":names,"evidence":"runtime.json"}))
        .collect();
    Ok(
        json!({"target":input["target"],"functions":functions,"support_unmasked":support,"runtime_aliases":aliases}),
    )
}

pub fn original_runtime(
    mut runtime: Value,
    input: &Value,
    original: &Pe,
    project: &Path,
    repo: &Path,
) -> Result<Value> {
    ensure!(
        runtime["image_sha256"] == hash(&original.data),
        "original runtime image identity differs"
    );
    ensure!(
        runtime["library_sha256"]
            == hash(&std::fs::read(repo.join("toolchains/1.0-nt/lib/LIBC.LIB"))?),
        "original runtime archive identity differs"
    );
    let declared = runtime["callee_identity_anchors"]
        .as_array()
        .cloned()
        .unwrap_or_default();
    if !declared.is_empty() {
        let policy = crate::index::json(&project.join("runtime-policy.json"))?;
        let anchors = policy["identity_only_anchors"]
            .as_array()
            .context("CVPACK identity-only anchor policy")?;
        ensure!(
            anchors.len() == declared.len(),
            "CVPACK identity-only anchor count differs"
        );
        for anchor in anchors {
            let name = anchor["name"]
                .as_str()
                .context("identity-only anchor name")?;
            let found: Vec<_> = declared.iter().filter(|r| r["name"] == name).collect();
            ensure!(
                found.len() == 1,
                "identity-only anchor missing or ambiguous: {name}"
            );
            for key in ["bytes_sha256", "classification"] {
                ensure!(
                    anchor[key] == found[0][key],
                    "identity-only anchor declaration differs: {name}.{key}"
                );
            }
            ensure!(
                anchor["accepted"] == false
                    && anchor["unmask"] == false
                    && anchor["classification"] == "reviewed-variant-identity-only",
                "identity-only anchor has availability or credit: {name}"
            );
            let registry = input["function_registry"]
                .as_array()
                .context("CVPACK function registry")?;
            let entries: Vec<_> = registry.iter().filter(|r| r["name"] == name).collect();
            ensure!(
                entries.len() == 1,
                "identity-only anchor registry entry missing or ambiguous: {name}"
            );
            let entry = entries[0];
            ensure!(
                entry["definition"] == 0 && entry["implementation"] == 0,
                "identity-only anchor registry classification differs: {name}"
            );
            let chunks = entry["code"]
                .as_array()
                .context("identity-only anchor code intervals")?;
            ensure!(
                chunks.len() == 1,
                "identity-only anchor needs one exact canonical extent: {name}"
            );
            let interval = chunks[0]
                .as_array()
                .context("identity-only anchor interval")?;
            ensure!(interval.len() == 2, "invalid identity-only anchor interval");
            let base = num(&input["target"]["base"])?;
            let start = base
                .checked_add(num(&interval[0])?)
                .context("identity-only anchor address overflow")?;
            let end = base
                .checked_add(num(&interval[1])?)
                .context("identity-only anchor address overflow")?;
            ensure!(
                num(&entry["rva"])? == num(&interval[0])?
                    && start == num(&found[0]["start"])?
                    && end == num(&found[0]["end"])?,
                "identity-only anchor canonical extent differs: {name}"
            );
            ensure!(start < end, "invalid identity-only anchor extent");
            ensure!(
                hash(original.read(start, (end - start) as usize)?) == anchor["bytes_sha256"],
                "identity-only anchor body differs: {name}"
            );
            runtime["functions"]
                .as_array_mut()
                .context("original runtime functions")?
                .push(json!({"name":name,"start":start,"end":end}));
        }
    }
    Ok(runtime)
}
pub fn rebuilt_runtime(flat: Value, rebuilt: &Pe) -> Result<Value> {
    ensure!(
        flat.is_array(),
        "CVPACK registry requires flat shared runtime identifier output"
    );
    let records: Vec<crate::index::Record> = serde_json::from_value(flat.clone())?;
    for record in &records {
        ensure!(
            record.start <= record.end
                && (record.start == record.end
                    || rebuilt.file_backed_code(record.start, record.end)),
            "invalid identified rebuilt runtime extent: {}",
            record.name
        );
    }
    let imports = crate::index::imports(rebuilt, false, None)?;
    Ok(json!({"functions":flat,"imports":imports}))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn richer_runtime_aliases_preserve_vertices() {
        let left = BTreeMap::from([
            (1, BTreeSet::from(["fprintf".into()])),
            (2, BTreeSet::from(["other".into()])),
        ]);
        let right = BTreeMap::from([(3, BTreeSet::from(["fprintf".into(), "fwprintf".into()]))]);
        assert!(crate::audit::equivalent(
            &BTreeSet::from([1]),
            &BTreeSet::from([3]),
            &left,
            &right
        ));
        assert!(!crate::audit::equivalent(
            &BTreeSet::from([1, 2]),
            &BTreeSet::from([3]),
            &left,
            &right
        ));
        assert!(!crate::audit::equivalent(
            &BTreeSet::from([2]),
            &BTreeSet::from([3]),
            &left,
            &right
        ));
    }
}
