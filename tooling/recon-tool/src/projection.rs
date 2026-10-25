//! Graph-audit inputs derived from the single binary registry plus finite policy.
use crate::{abi::Info, runtime};
use anyhow::{Context, Result, bail, ensure};
use serde_json::{Value, json};
use std::{collections::HashMap, fs, path::PathBuf};

#[derive(clap::Args)]
pub struct Options {
    pub case: PathBuf,
    #[arg(long)]
    pub image_base: u32,
    /// Select a finite policy.accepted_scopes view; never adds accepted functions.
    #[arg(long)]
    pub accepted_scope: Option<String>,
    #[arg(long)]
    pub policy: Option<PathBuf>,
    #[arg(long)]
    pub metadata_output: Option<PathBuf>,
    /// Resolve explicit rebuilt_symbol address leaves against this PE and MAP.
    #[arg(long, requires = "rebuilt_map")]
    pub rebuilt_image: Option<PathBuf>,
    #[arg(long, requires = "rebuilt_image")]
    pub rebuilt_map: Option<PathBuf>,
    /// Project only policy.ret_checks for graph-audit's independent RET check.
    #[arg(long)]
    pub ret_checks_output: Option<PathBuf>,
    /// Project verified original CRT/import extents; no matching or source credit.
    #[arg(long)]
    pub runtime_output: Option<PathBuf>,
    #[arg(short, long)]
    pub output: Option<PathBuf>,
}

fn resolve(value: &mut Value, publics: &HashMap<String, Vec<u32>>) -> Result<()> {
    match value {
        Value::Object(object) if object.contains_key("rebuilt_symbol") => {
            ensure!(
                object
                    .keys()
                    .all(|k| matches!(k.as_str(), "rebuilt_symbol" | "offset")),
                "rebuilt_symbol address contains unrelated fields"
            );
            let name = object["rebuilt_symbol"]
                .as_str()
                .context("rebuilt_symbol must be a string")?;
            let matches = publics
                .get(name)
                .with_context(|| format!("rebuilt MAP lacks symbol {name}"))?;
            ensure!(matches.len() == 1, "ambiguous rebuilt MAP symbol {name}");
            let offset = object
                .get("offset")
                .map(|v| v.as_i64().context("address offset must be an integer"))
                .transpose()?
                .unwrap_or(0);
            let address = i64::from(matches[0])
                .checked_add(offset)
                .context("rebuilt address overflow")?;
            *value = json!(u32::try_from(address).context("rebuilt address outside PE32")?);
        }
        Value::Object(object) => {
            for child in object.values_mut() {
                resolve(child, publics)?
            }
            for (key, payload) in [
                ("switches", "targets"),
                ("resolved_register_calls", "identity"),
                ("direct_import_calls", "identity"),
            ] {
                if let Some(Value::Array(rows)) = object.get(key) {
                    let mut sites = serde_json::Map::new();
                    for row in rows {
                        ensure!(
                            row.as_object().is_some_and(|v| v.len() == 2
                                && v.contains_key("site")
                                && v.contains_key(payload)),
                            "{key} row requires site/{payload}"
                        );
                        let site = row["site"]
                            .as_u64()
                            .filter(|v| *v <= u32::MAX as u64)
                            .context("site must resolve to PE32 address")?;
                        if payload == "targets" {
                            ensure!(
                                row[payload].as_array().is_some_and(|a| a
                                    .iter()
                                    .all(|n| n.as_u64().is_some_and(|n| n <= u32::MAX as u64))),
                                "switch targets must resolve to PE32 addresses"
                            );
                        }
                        ensure!(
                            sites
                                .insert(site.to_string(), row[payload].clone())
                                .is_none(),
                            "duplicate {key} site"
                        );
                    }
                    object.insert(key.into(), Value::Object(sites));
                }
            }
        }
        Value::Array(array) => {
            for child in array {
                resolve(child, publics)?
            }
        }
        _ => {}
    }
    Ok(())
}

fn metadata(policy: &Value, info: &Info) -> Result<Value> {
    let mut metadata = policy
        .get("graph_metadata")
        .cloned()
        .unwrap_or_else(|| json!({"schema":1,"functions":{},"callback_tables":[]}));
    ensure!(
        metadata.is_object() && metadata["schema"] == 1,
        "graph_metadata must be an object with schema 1"
    );
    let functions = metadata
        .get("functions")
        .context("graph_metadata.functions missing")?
        .as_object()
        .context("graph_metadata.functions must be an object")?;
    for (name, annotations) in functions {
        ensure!(
            info.functions.iter().any(|f| f.name == *name),
            "graph_metadata has unknown function {name}"
        );
        let annotations = annotations
            .as_object()
            .context("function metadata must be an object")?;
        for identity in [
            "name",
            "symbol",
            "native",
            "native_symbol",
            "original_symbol",
            "start",
            "end",
            "rva",
            "entry_rva",
            "code",
            "code_chunks",
            "extent",
            "extents",
            "implementation",
            "status",
            "category",
            "definition",
        ] {
            ensure!(
                !annotations.contains_key(identity),
                "graph_metadata cannot override registry identity {name}.{identity}"
            );
        }
    }
    if metadata.get("callback_tables").is_none() {
        metadata["callback_tables"] = json!([]);
    }
    ensure!(
        metadata["callback_tables"].is_array(),
        "graph_metadata.callback_tables must be an array"
    );
    // Existing VC1 pointer-table declarations remain a compact authoring form.
    // Explicit graph_metadata tables are authoritative when supplied.
    if policy.get("graph_metadata").is_none() {
        for table in policy["tables"].as_array().into_iter().flatten() {
            if table["kind"] != "function_pointers" {
                continue;
            }
            let functions = table["functions"]
                .as_array()
                .context("callback table functions")?;
            let begin = table["original"]
                .as_u64()
                .context("callback table original address")?;
            let stride = table
                .get("stride")
                .map(|v| v.as_u64().context("callback table stride"))
                .transpose()?
                .unwrap_or(4);
            let pointer_offset = table
                .get("pointer_offset")
                .map(|v| v.as_u64().context("callback table pointer_offset"))
                .transpose()?
                .unwrap_or(0);
            ensure!(
                stride >= 4 && pointer_offset <= stride - 4,
                "invalid callback pointer layout"
            );
            let end = begin
                .checked_add(
                    (functions.len() as u64)
                        .checked_mul(stride)
                        .context("callback size overflow")?,
                )
                .context("callback extent overflow")?;
            let name = table["name"].as_str().context("callback table name")?;
            metadata["callback_tables"].as_array_mut().unwrap().push(json!({"label":name,"original":[begin,end],"rebuilt_begin":name,"count":functions.len(),"stride":stride,"pointer_offset":pointer_offset,"require_accepted":false}));
        }
    }
    Ok(metadata)
}

fn registry(info: &Info, image_base: u32, mut policy: Value) -> Result<Value> {
    ensure!(policy.is_object(), "policy must be a JSON object");
    for field in ["target", "accepted", "driver"] {
        ensure!(
            policy.get(field).is_none(),
            "policy must not duplicate registry field {field}"
        );
    }
    policy.as_object_mut().unwrap().remove("graph_metadata");
    policy.as_object_mut().unwrap().remove("ret_checks");
    let add = |rva: u32| image_base.checked_add(rva).context("original VA overflow");
    policy["target"] = json!({"path":info.original_path,"sha256":info.original_sha256,"base":image_base,"entry":add(info.original_entry_rva)?});
    let audited = qualified_runtime_names(info, &policy)?;
    policy["accepted"] = json!(
        info.functions
            .iter()
            .filter(|f| f.definition == 4 && f.implementation == 1
                || f.definition == 3 && f.native_symbol.is_some() && !f.code.is_empty()
                || audited.contains(f.name.as_str()))
            .map(|f| &f.name)
            .collect::<Vec<_>>()
    );
    let mut drivers = vec![];
    for f in &info.functions {
        if f.definition != 4 && !(f.definition == 3 && f.native_symbol.is_some()) {
            continue;
        }
        if f.code.is_empty() && (f.definition == 3 || f.definition == 4 && f.implementation == 0) {
            continue;
        }
        let entry = f
            .code
            .iter()
            .find(|r| r[0] <= f.rva && f.rva < r[1])
            .with_context(|| format!("driver function {} entry not in code extents", f.name))?;
        ensure!(
            entry[0] == f.rva,
            "graph-audit requires primary code extent to begin at entry: {}",
            f.name
        );
        let mut row = json!({"name":f.name,"symbol":f.original_symbol.as_deref().unwrap_or(&f.name),"native":f.native_symbol,"start":add(entry[0])?,"end":add(entry[1])?,"shared":f.text_data.iter().map(|r|Ok([add(r[0])?,add(r[1])?])).collect::<Result<Vec<_>>>()?,"code_chunks":f.code.iter().filter(|r|*r!=entry).map(|r|Ok([add(r[0])?,add(r[1])?])).collect::<Result<Vec<_>>>()?});
        if f.definition == 3 {
            row["category"] = json!("runtime");
        } else if f.implementation == 0 {
            row["pending_extent"] = json!(true);
            if let Some(known) = policy["function_properties"][&f.name].get("pending_extent") {
                ensure!(
                    known.is_boolean(),
                    "pending_extent qualification must be boolean"
                );
                row["pending_extent"] = known.clone();
            }
        }
        drivers.push(row);
    }
    policy["driver"] = json!(drivers);
    // Preserve every discovered entry for overlap/identity checks, including
    // pending entries that truthfully have no known code extent.
    policy["function_registry"] = serde_json::to_value(&info.functions)?;
    Ok(policy)
}

fn qualified_runtime_names<'a>(
    info: &Info,
    policy: &'a Value,
) -> Result<std::collections::HashSet<&'a str>> {
    let audited = policy
        .get("runtime_audit_names")
        .map(|v| v.as_array().context("runtime_audit_names must be an array"))
        .transpose()?;
    let mut qualified = std::collections::HashSet::new();
    for name in audited.into_iter().flatten() {
        let name = name.as_str().context("runtime audit name must be string")?;
        ensure!(
            qualified.insert(name)
                && info
                    .functions
                    .iter()
                    .any(|f| f.name == name && matches!(f.definition, 1..=3) && !f.code.is_empty()),
            "runtime audit qualification lacks unique classified extent: {name}"
        );
    }
    Ok(qualified)
}

fn runtime_records(info: &Info, image_base: u32, policy: &Value) -> Result<Value> {
    let qualified = qualified_runtime_names(info, policy)?;
    let add = |rva: u32| image_base.checked_add(rva).context("runtime VA overflow");
    let mut records = vec![];
    for function in info
        .functions
        .iter()
        .filter(|f| matches!(f.definition, 1..=3))
    {
        if function.code.is_empty() {
            continue;
        }
        let primary = function
            .code
            .iter()
            .find(|r| r[0] <= function.rva && function.rva < r[1])
            .with_context(|| {
                format!(
                    "runtime {} entry outside verified code extents",
                    function.name
                )
            })?;
        let mut row = json!({"name":function.name,"start":add(primary[0])?,"end":add(primary[1])?,"shared":function.text_data.iter().map(|r|Ok([add(r[0])?,add(r[1])?])).collect::<Result<Vec<_>>>()?,"code_chunks":function.code.iter().filter(|r| *r != primary).map(|r|Ok([add(r[0])?,add(r[1])?])).collect::<Result<Vec<_>>>()?,"category":"runtime"});
        if qualified.contains(function.name.as_str()) {
            row["evidence"] =
                json!("binary.c classified extent; finite runtime_audit_names graph qualification");
        }
        records.push(row);
    }
    Ok(json!(records))
}

fn accepted_view(config: &mut Value, info: &Info, policy: &Value, scope: &str) -> Result<()> {
    let names = policy["accepted_scopes"]
        .get(scope)
        .with_context(|| format!("unknown accepted scope {scope}"))?
        .as_array()
        .context("accepted scope must be an array")?;
    ensure!(!names.is_empty(), "accepted scope must not be empty");
    let accepted = config["accepted"].as_array().unwrap();
    let mut selected = std::collections::HashSet::new();
    for name in names {
        let name = name
            .as_str()
            .context("accepted scope name must be string")?;
        ensure!(
            selected.insert(name)
                && info.functions.iter().any(|f| f.name == name)
                && accepted.iter().any(|v| v.as_str() == Some(name)),
            "unaccepted/unknown/duplicate accepted scope function {name}"
        );
    }
    config["accepted"]
        .as_array_mut()
        .unwrap()
        .retain(|name| name.as_str().is_some_and(|name| selected.contains(name)));
    Ok(())
}

pub fn export(options: Options) -> Result<()> {
    let path = if options.case.is_dir() {
        options.case.join(".build/case.so")
    } else {
        options.case
    };
    let info = crate::native::inspect(&path.canonicalize()?)?;
    let policy = match options.policy {
        Some(path) => serde_json::from_slice(&fs::read(path)?)?,
        None => json!({}),
    };
    let mut config = registry(&info, options.image_base, policy.clone())?;
    if let Some(scope) = options.accepted_scope {
        accepted_view(&mut config, &info, &policy, &scope)?;
    }
    if let Some(path) = options.metadata_output {
        let mut metadata = metadata(&policy, &info)?;
        let publics = match (options.rebuilt_image, options.rebuilt_map) {
            (Some(image), Some(map)) => runtime::map_addresses(&image, &map)?,
            (None, None) => HashMap::new(),
            _ => bail!("rebuilt image and MAP must be supplied together"),
        };
        resolve(&mut metadata, &publics)?;
        fs::write(path, serde_json::to_string_pretty(&metadata)? + "\n")?;
    }
    if let Some(path) = options.runtime_output {
        fs::write(
            path,
            serde_json::to_string_pretty(&runtime_records(&info, options.image_base, &policy)?)?
                + "\n",
        )?;
    }
    if let Some(path) = options.ret_checks_output {
        let checks = policy
            .get("ret_checks")
            .context("policy.ret_checks missing")?;
        ensure!(
            checks["schema"] == 1 && checks["functions"].is_object(),
            "ret_checks requires schema1/functions"
        );
        for (name, check) in checks["functions"].as_object().unwrap() {
            ensure!(
                info.functions.iter().any(|f| f.name == *name),
                "RET check has unknown function {name}"
            );
            ensure!(
                check
                    .as_object()
                    .is_some_and(|o| o.len() == 1 && o.contains_key("expected_cleanup"))
                    && check["expected_cleanup"]
                        .as_array()
                        .is_some_and(|a| !a.is_empty()
                            && a.iter()
                                .all(|n| n.as_u64().is_some_and(|n| n <= u16::MAX as u64))),
                "invalid RET check for {name}"
            );
        }
        fs::write(path, serde_json::to_string_pretty(checks)? + "\n")?;
    }
    let text = serde_json::to_string_pretty(&config)? + "\n";
    if let Some(output) = options.output {
        fs::write(output, text)?;
    } else {
        print!("{text}");
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    fn info() -> Info {
        serde_json::from_value(json!({"binary":"test","original_path":"tool.exe","original_sha256":"","original_entry_rva":16,"case":"case","description":"","execution":0,"terminal":0,"frontier_rva":0,"first_entry_order":[],"first_entry_seen":[],"functions":[{"name":"method","rva":16,"native_symbol":"_ZNmethod","original_symbol":"?method@@","definition":4,"implementation":1,"code":[[16,20]],"text_data":[]},{"name":"pending","rva":32,"native_symbol":"pending","original_symbol":null,"definition":4,"implementation":0,"code":[],"text_data":[]}],"tests":[],"module":"libbinary.so"})).unwrap()
    }
    #[test]
    fn accepted_views_intersect_verified_scope_without_changing_identity() {
        let i = info();
        let p = json!({"accepted_scopes":{"imports":["method"]}});
        let mut v = registry(&i, 0, p.clone()).unwrap();
        let driver = v["driver"].clone();
        accepted_view(&mut v, &i, &p, "imports").unwrap();
        assert_eq!(v["accepted"], json!(["method"]));
        assert_eq!(v["driver"], driver);
        assert!(
            accepted_view(
                &mut v,
                &i,
                &json!({"accepted_scopes":{"bad":["unknown"]}}),
                "bad"
            )
            .is_err()
        );
        assert!(accepted_view(&mut v, &i, &p, "missing").is_err());
        assert!(
            accepted_view(
                &mut v,
                &i,
                &json!({"accepted_scopes":{"bad":["pending"]}}),
                "bad"
            )
            .is_err()
        );
        assert!(accepted_view(&mut v, &i, &json!({"accepted_scopes":{"bad":[]}}), "bad").is_err());
    }
    #[test]
    fn original_runtime_projection_is_classified_and_has_no_app_credit() {
        let mut i = info();
        let mut crt = i.functions[0].clone();
        crt.name = "crt".into();
        crt.definition = 3;
        crt.implementation = 0;
        crt.native_symbol = None;
        crt.code.push([30, 40]);
        crt.text_data.push([20, 24]);
        i.functions.push(crt);
        let records = runtime_records(&i, 0x400000, &json!({})).unwrap();
        assert_eq!(records.as_array().unwrap().len(), 1);
        assert!(records[0].get("evidence").is_none());
        assert!(
            runtime_records(&i, 0, &json!({"runtime_audit_names":["crt"]})).unwrap()[0]
                .get("evidence")
                .is_some()
        );
        assert!(runtime_records(&i, 0, &json!({"runtime_audit_names":["method"]})).is_err());
        assert_eq!(records[0]["start"], 0x400010);
        assert_eq!(records[0]["code_chunks"][0], json!([0x40001e, 0x400028]));
        assert_eq!(records[0]["shared"][0], json!([0x400014, 0x400018]));
        let v = registry(&i, 0x400000, json!({})).unwrap();
        assert_eq!(v["function_registry"].as_array().unwrap().len(), 3);
        assert!(!v["accepted"].as_array().unwrap().contains(&json!("crt")));
        let qualified = registry(&i, 0, json!({"runtime_audit_names":["crt"]})).unwrap();
        assert!(
            qualified["accepted"]
                .as_array()
                .unwrap()
                .contains(&json!("crt"))
        );
        i.functions[2].native_symbol = Some("native_crt".into());
        assert!(
            registry(&i, 0, json!({})).unwrap()["accepted"]
                .as_array()
                .unwrap()
                .contains(&json!("crt"))
        );
        assert_eq!(v["function_registry"][1]["code"], json!([]));
        let mut i = info();
        i.functions[1].code = vec![[32, 33]];
        assert_eq!(
            registry(&i, 0, json!({})).unwrap()["driver"][1]["pending_extent"],
            true
        );
        assert_eq!(
            registry(
                &i,
                0,
                json!({"function_properties":{"pending":{"pending_extent":false}}})
            )
            .unwrap()["driver"][1]["pending_extent"],
            false
        );
    }
    #[test]
    fn aliases_pending_extents_and_strict_known_extents() {
        let mut i = info();
        let v = registry(&i, 0x400000, json!({})).unwrap();
        assert_eq!(v["driver"].as_array().unwrap().len(), 1);
        assert_eq!(v["driver"][0]["symbol"], "?method@@");
        assert_eq!(v["driver"][0]["native"], "_ZNmethod");
        i.functions[1].implementation = 1;
        assert!(registry(&i, 0x400000, json!({})).is_err());
        assert!(registry(&info(), 0, json!({"driver":[]})).is_err());
    }
    #[test]
    fn finite_metadata_and_explicit_relative_addresses() {
        let p = json!({"graph_metadata":{"schema":1,"functions":{"method":{"original":{"roots":[17]},"rebuilt":{"roots":[{"rebuilt_symbol":"?method@@","offset":-2}]} }},"register_imports":{"strict":true},"callback_tables":[]}});
        let mut v = metadata(&p, &info()).unwrap();
        resolve(&mut v, &HashMap::from([("?method@@".into(), vec![100])])).unwrap();
        assert_eq!(v["functions"]["method"]["rebuilt"]["roots"][0], 98);
        assert_eq!(v["register_imports"]["strict"], true);
        for bad in [
            json!({"schema":1,"functions":{"method":{"start":10}}}),
            json!({"schema":1,"functions":{"unknown":{}}}),
        ] {
            assert!(metadata(&json!({"graph_metadata":bad}), &info()).is_err());
        }
        let mut address = json!({"rebuilt_symbol":"x","offset":0});
        assert!(resolve(&mut address, &HashMap::new()).is_err());
        assert!(resolve(&mut address, &HashMap::from([("x".into(), vec![1, 2])])).is_err());
    }
}
