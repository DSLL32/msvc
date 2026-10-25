//! Explicit expected cleanup assertions; never a graph-selection or ABI waiver.
use crate::index::Record;
use anyhow::{Context, Result, ensure};
use serde::{
    Deserialize,
    de::{self, MapAccess, Visitor},
};
use std::{
    collections::{BTreeMap, BTreeSet},
    fmt,
    path::Path,
};

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Policy {
    schema: u32,
    #[serde(deserialize_with = "unique_functions")]
    functions: BTreeMap<String, Contract>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Contract {
    expected_cleanup: Vec<u16>,
}
fn unique_functions<'de, D: serde::Deserializer<'de>>(
    d: D,
) -> Result<BTreeMap<String, Contract>, D::Error> {
    struct Functions;
    impl<'de> Visitor<'de> for Functions {
        type Value = BTreeMap<String, Contract>;
        fn expecting(&self, f: &mut fmt::Formatter) -> fmt::Result {
            f.write_str("unique logical function names")
        }
        fn visit_map<M: MapAccess<'de>>(self, mut map: M) -> Result<Self::Value, M::Error> {
            let mut result = BTreeMap::new();
            while let Some((name, contract)) = map.next_entry::<String, Contract>()? {
                if result.insert(name.clone(), contract).is_some() {
                    return Err(de::Error::custom(format!(
                        "duplicate RET checks function: {name}"
                    )));
                }
            }
            Ok(result)
        }
    }
    d.deserialize_map(Functions)
}
pub struct Checks {
    pub input_sha256: String,
    functions: BTreeMap<String, BTreeSet<u32>>,
}
impl Checks {
    pub fn load(
        path: &Path,
        progress: &[Record],
        selected: &BTreeSet<String>,
        original: &[Record],
        rebuilt: &[Record],
    ) -> Result<Self> {
        let bytes =
            std::fs::read(path).with_context(|| format!("read RET checks {}", path.display()))?;
        Self::parse(&bytes, progress, selected, original, rebuilt)
    }
    fn parse(
        bytes: &[u8],
        progress: &[Record],
        selected: &BTreeSet<String>,
        original: &[Record],
        rebuilt: &[Record],
    ) -> Result<Self> {
        let policy: Policy = serde_json::from_slice(bytes).context("parse RET checks")?;
        ensure!(policy.schema == 1, "unsupported RET checks schema");
        ensure!(!policy.functions.is_empty(), "RET checks has no functions");
        let mut functions = BTreeMap::new();
        for (name, contract) in policy.functions {
            ensure!(
                !name.trim().is_empty(),
                "RET checks has empty function name"
            );
            ensure!(
                selected.contains(&name),
                "RET checks function is not selected/accepted: {name}"
            );
            for (side, records) in [
                ("selection", progress),
                ("original", original),
                ("rebuilt", rebuilt),
            ] {
                let matches: Vec<_> = records.iter().filter(|r| r.name == name).collect();
                ensure!(
                    matches.len() == 1,
                    "RET checks {side} function missing or ambiguous: {name}"
                );
                ensure!(
                    !matches[0].pending_extent,
                    "RET checks {side} function has pending extent: {name}"
                );
            }
            ensure!(
                !contract.expected_cleanup.is_empty(),
                "RET checks expected cleanup is empty: {name}"
            );
            let expected: BTreeSet<u32> = contract
                .expected_cleanup
                .iter()
                .copied()
                .map(u32::from)
                .collect();
            ensure!(
                expected.len() == contract.expected_cleanup.len(),
                "RET checks duplicate expected cleanup: {name}"
            );
            functions.insert(name, expected);
        }
        Ok(Self {
            input_sha256: crate::index::hash(bytes),
            functions,
        })
    }
    pub fn expected(&self, name: &str) -> Option<&BTreeSet<u32>> {
        self.functions.get(name)
    }
    pub fn len(&self) -> usize {
        self.functions.len()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn record() -> Record {
        Record {
            name: "f".into(),
            start: 0x401000,
            end: 0x401001,
            shared: vec![],
            code_chunks: vec![],
            evidence: serde_json::Value::Null,
            pending_extent: false,
        }
    }
    #[test]
    fn scoped_policy_is_strict_and_requires_paired_selected_owner() {
        let r = record();
        let selected = BTreeSet::from(["f".into()]);
        let parse = |s: &str| {
            Checks::parse(
                s.as_bytes(),
                std::slice::from_ref(&r),
                &selected,
                std::slice::from_ref(&r),
                std::slice::from_ref(&r),
            )
        };
        assert_eq!(
            parse(r#"{"schema":1,"functions":{"f":{"expected_cleanup":[8]}}}"#)
                .unwrap()
                .expected("f"),
            Some(&BTreeSet::from([8]))
        );
        for invalid in [
            r#"{"schema":2,"functions":{"f":{"expected_cleanup":[8]}}}"#,
            r#"{"schema":1,"functions":{}}"#,
            r#"{"schema":1,"functions":{"f":{"expected_cleanup":[]}}}"#,
            r#"{"schema":1,"functions":{"f":{"expected_cleanup":[8,8]}}}"#,
            r#"{"schema":1,"functions":{"f":{"expected_cleanup":[65536]}}}"#,
            r#"{"schema":1,"functions":{"f":{"expected_cleanup":[-1]}}}"#,
            r#"{"schema":1,"functions":{"f":{"expected_cleanups":[8]}}}"#,
            r#"{"schema":1,"functions":{"f":{"expected_cleanup":[8],"disable":true}}}"#,
            r#"{"schema":1,"functions":{"g":{"expected_cleanup":[8]}}}"#,
            r#"{"schema":1,"functions":{"f":{"expected_cleanup":[8]},"f":{"expected_cleanup":[8]}}}"#,
        ] {
            assert!(parse(invalid).is_err(), "{invalid}");
        }
        let bytes = br#"{"schema":1,"functions":{"f":{"expected_cleanup":[8]}}}"#;
        assert!(
            Checks::parse(
                bytes,
                &[r.clone(), r.clone()],
                &selected,
                std::slice::from_ref(&r),
                std::slice::from_ref(&r)
            )
            .is_err()
        );
        assert!(
            Checks::parse(
                bytes,
                std::slice::from_ref(&r),
                &selected,
                std::slice::from_ref(&r),
                &[]
            )
            .is_err()
        );
        let mut pending = r.clone();
        pending.pending_extent = true;
        assert!(
            Checks::parse(
                bytes,
                std::slice::from_ref(&r),
                &selected,
                std::slice::from_ref(&r),
                &[pending]
            )
            .is_err()
        );
    }
}
