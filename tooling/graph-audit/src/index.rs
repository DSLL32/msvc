use crate::{
    Args, Profile,
    pe::{Pe, u32_at},
};
use anyhow::{Context, Result, bail, ensure};
use serde::{Deserialize, Serialize};
use serde_json::Value;
use sha2::{Digest, Sha256};
use std::{
    collections::{BTreeMap, BTreeSet, HashMap},
    fs,
    path::Path,
};
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Record {
    pub name: String,
    pub start: u32,
    pub end: u32,
    #[serde(default)]
    pub shared: Vec<(u32, u32)>,
    #[serde(default)]
    pub evidence: Value,
    #[serde(default)]
    pub pending_extent: bool,
}
pub fn json(path: &Path) -> Result<Value> {
    Ok(serde_json::from_slice(
        &fs::read(path).with_context(|| format!("read {}", path.display()))?,
    )?)
}
pub fn records(v: &Value) -> Result<Vec<Record>> {
    Ok(serde_json::from_value(v.clone())?)
}
pub fn num(v: &Value) -> Result<u32> {
    v.as_u64()
        .and_then(|v| u32::try_from(v).ok())
        .context("expected uint32")
}
pub fn hash(b: &[u8]) -> String {
    format!("{:x}", Sha256::digest(b))
}
pub fn map(path: &Path, first: bool) -> Result<BTreeMap<String, u32>> {
    let re = regex::Regex::new(r"^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})")?;
    let mut out = BTreeMap::new();
    for line in fs::read_to_string(path)?.lines() {
        if let Some(c) = re.captures(line) {
            let value = u32::from_str_radix(&c[2], 16)?;
            if first {
                out.entry(c[1].into()).or_insert(value);
            } else {
                out.insert(c[1].into(), value);
            }
        }
    }
    Ok(out)
}
pub fn imports(pe: &Pe, raw: bool) -> Result<Vec<Record>> {
    let directory = u32_at(&pe.data, pe.optional + 104)?;
    if directory == 0 {
        return Ok(Vec::new());
    }
    let mut p = pe.base + directory;
    let mut lookup = HashMap::new();
    for _ in 0..10000 {
        if pe.word(p + 12)? == 0 {
            break;
        }
        let names = pe.word(p)?;
        let iat = pe.word(p + 16)?;
        for n in 0..10000 {
            let v = pe.word(pe.base + names + 4 * n)?;
            if v == 0 {
                break;
            }
            let name = if v & 0x80000000 != 0 {
                format!("ordinal_{}", v & 65535)
            } else {
                pe.string(pe.base + v + 2)?
            };
            lookup.insert(pe.base + iat + 4 * n, name);
        }
        p += 20;
    }
    let mut result = Vec::new();
    for s in &pe.sections {
        if s.flags & 0x20 == 0 {
            continue;
        }
        let length = if raw { s.size } else { s.virtual_size } as usize;
        let code = pe
            .data
            .get(s.off..(s.off + length).min(pe.data.len()))
            .context("bad code section")?;
        for i in 0..code.len().saturating_sub(5) {
            if code[i..i + 2] == [255, 37]
                && let Some(name) = lookup.get(&u32_at(code, i + 2)?)
            {
                let start = s.start + i as u32;
                result.push(Record {
                    name: format!("import:{name}"),
                    start,
                    end: start + 6,
                    shared: vec![],
                    evidence: Value::Null,
                    pending_extent: false,
                });
            }
        }
    }
    Ok(result)
}
pub struct Inputs {
    pub o: Pe,
    pub r: Pe,
    pub oi: Vec<Record>,
    pub ri: Vec<Record>,
    pub drivers: Vec<Record>,
    pub accepted: BTreeSet<String>,
    pub publics: BTreeMap<String, u32>,
}
impl Inputs {
    pub fn load(a: &Args) -> Result<Self> {
        let cfg = json(&a.config.clone().unwrap_or(a.project.join("functions.json")))?;
        let name = match a.profile {
            Profile::Cl => "cl",
            Profile::C1 => "c1",
            Profile::C1xx => "c1xx",
            Profile::C2 => "c2",
            Profile::Link => "link",
            Profile::Rc => "rc",
            Profile::Rcpp => "rcpp",
            _ => bail!("CVPACK has a separate graph profile"),
        };
        let fallback = match a.profile {
            Profile::C1 => "C13232.EXE",
            Profile::C1xx => "C1XX3232.EXE",
            Profile::C2 => "C23232.EXE",
            Profile::Cl => "CL.EXE",
            Profile::Link => "LINK.EXE",
            Profile::Rc => "RC.EXE",
            _ => "RCPP.EXE",
        };
        let original = a.original.clone().unwrap_or_else(|| {
            cfg["target"]["path"]
                .as_str()
                .map(|p| a.repo.join(p))
                .unwrap_or(a.repo.join("toolchains/1.0-nt/bin").join(fallback))
        });
        let o = Pe::open(&original)?;
        ensure!(
            Some(hash(&o.data).as_str()) == cfg["target"]["sha256"].as_str(),
            "target changed"
        );
        let r = Pe::open(
            &a.rebuilt
                .clone()
                .unwrap_or(a.project.join(format!("build/{name}.exe"))),
        )?;
        let mut publics = map(
            &a.map
                .clone()
                .unwrap_or(a.project.join(format!("build/{name}.map"))),
            a.profile == Profile::Rc,
        )?;
        let drivers = records(
            &cfg[if a.profile == Profile::Rcpp {
                "functions"
            } else {
                "driver"
            }],
        )?;
        if a.profile == Profile::Link {
            for d in cfg["driver"].as_array().context("driver array")? {
                if let Some(s) = d["symbol"].as_str()
                    && let Some(v) = publics.get(s).copied()
                {
                    let n = d["name"].as_str().context("driver name")?;
                    ensure!(!publics.contains_key(n), "logical symbol already public");
                    publics.insert(n.into(), v);
                }
            }
        }
        let accepted = serde_json::from_value(cfg["accepted"].clone())?;
        let driver_addresses: BTreeSet<_> = drivers.iter().map(|r| r.start).collect();
        let driver_names: BTreeSet<_> = drivers.iter().map(|r| r.name.as_str()).collect();
        if matches!(a.profile, Profile::C1 | Profile::C2) {
            ensure!(
                driver_addresses.len() == drivers.len(),
                "duplicate driver entry address"
            );
            ensure!(
                driver_names.len() == drivers.len(),
                "duplicate driver function name"
            );
        }
        if a.profile == Profile::Link {
            ensure!(
                driver_names.len() == drivers.len(),
                "Duplicate reconstructed graph vertex name"
            );
        }
        if a.profile == Profile::C1xx {
            for d in &drivers {
                ensure!(
                    d.start < d.end
                        && o.sections.iter().any(|s| s.flags & 0x20 != 0
                            && s.start <= d.start
                            && d.end <= s.start + s.virtual_size),
                    "invalid original extent: {}",
                    d.name
                );
            }
        }
        let runtime = if a.profile == Profile::Rcpp {
            Vec::new()
        } else {
            records(&json(
                &a.runtime.clone().unwrap_or(a.project.join("runtime.json")),
            )?)?
        };
        if a.profile == Profile::C2 {
            let mut ordered = drivers.clone();
            ordered.sort_by_key(|d| d.start);
            for pair in ordered.windows(2) {
                ensure!(pair[0].end <= pair[1].start, "overlapping driver extents");
            }
            for d in &drivers {
                for l in &runtime {
                    ensure!(
                        d.start == l.start || d.end <= l.start || l.end <= d.start,
                        "driver extent overlaps CRT: {} / {}",
                        d.name,
                        l.name
                    );
                }
            }
        }
        let mut oi: Vec<_> = runtime
            .into_iter()
            .filter(|r| match a.profile {
                Profile::C1 | Profile::C1xx => !driver_addresses.contains(&r.start),
                Profile::Rc => !driver_names.contains(r.name.as_str()),
                _ => true,
            })
            .collect();
        oi.extend(imports(&o, a.profile == Profile::Rcpp)?);
        oi.extend(drivers.clone());
        let mut ri = if a.profile == Profile::Rcpp {
            Vec::new()
        } else {
            records(&json(
                &a.rebuilt_runtime
                    .clone()
                    .unwrap_or(a.project.join("build/runtime.json")),
            )?)?
        };
        if a.profile == Profile::Rc {
            ri.retain(|r| !driver_names.contains(r.name.as_str()));
        }
        ri.extend(imports(&r, a.profile == Profile::Rcpp)?);
        let driver_publics: BTreeSet<_> = drivers
            .iter()
            .filter_map(|r| publics.get(&r.name).copied())
            .collect();
        ri.retain(|r| {
            if matches!(a.profile, Profile::C1 | Profile::C1xx) && driver_publics.contains(&r.start)
            {
                return false;
            }
            r.name.starts_with("import:")
                || match publics.get(&r.name) {
                    Some(v) => *v == r.start,
                    None => !matches!(a.profile, Profile::Link | Profile::Rc),
                }
        });
        let addresses: BTreeSet<_> = publics.values().copied().chain([r.code_end()?]).collect();
        for (i, d) in drivers.iter().enumerate() {
            let symbol = if a.profile == Profile::Rcpp {
                cfg["functions"][i]["symbol"].as_str().unwrap_or(&d.name)
            } else {
                &d.name
            };
            if let Some(start) = publics.get(symbol) {
                let end = addresses
                    .range((
                        std::ops::Bound::Excluded(*start),
                        std::ops::Bound::Unbounded,
                    ))
                    .next()
                    .copied()
                    .context("no function end")?;
                ri.push(Record {
                    name: d.name.clone(),
                    start: *start,
                    end,
                    shared: vec![],
                    evidence: Value::Null,
                    pending_extent: false,
                });
            } else {
                ensure!(a.profile != Profile::Rcpp, "missing public {symbol}");
            }
        }
        if a.profile == Profile::Rcpp {
            ensure!(
                driver_names.len() == drivers.len(),
                "duplicate logical function names"
            );
            for idx in [&oi, &ri] {
                let mut sorted = idx.iter().collect::<Vec<_>>();
                sorted.sort_by_key(|r| (r.start, r.end));
                for p in sorted.windows(2) {
                    ensure!(
                        p[0].end <= p[1].start,
                        "overlapping extents: {} / {}",
                        p[0].name,
                        p[1].name
                    );
                }
            }
        }
        for (n, v) in cfg["crt_defaults"].as_object().into_iter().flatten() {
            let expected = num(&v["value"])?;
            ensure!(
                o.word(num(&v["address"])?)? == expected
                    && r.word(*publics.get(n).context("missing CRT global")?)? == expected,
                "CRT default differs {n}"
            );
        }
        for v in cfg["runtime_data"].as_array().into_iter().flatten() {
            let n = v["name"].as_str().context("CRT global name")?;
            let count = num(&v["bytes"])? as usize;
            let expected = integer_bytes(v["value"].as_u64().context("CRT global value")?, count)?;
            ensure!(
                count <= 8
                    && o.read(num(&v["original"])?, count)? == &expected[..count]
                    && r.read(*publics.get(n).context("missing CRT global")?, count)?
                        == &expected[..count],
                "CRT global differs {n}"
            );
        }
        Ok(Self {
            o,
            r,
            oi,
            ri,
            drivers,
            accepted,
            publics,
        })
    }
}
pub fn first_names(records: &[Record]) -> HashMap<u32, String> {
    let mut out = HashMap::new();
    for r in records {
        out.entry(r.start).or_insert_with(|| r.name.clone());
    }
    out
}
pub fn identity(names: &HashMap<u32, String>, target: u32) -> String {
    names
        .get(&target)
        .cloned()
        .unwrap_or_else(|| format!("0x{target:x}"))
}

pub(crate) fn integer_bytes(value: u64, width: usize) -> Result<[u8; 8]> {
    ensure!(width <= 8, "unsupported CRT global width {width}");
    ensure!(
        width == 8 || value < (1u64 << (width * 8)),
        "CRT global value does not fit declared {width}-byte width"
    );
    Ok(value.to_le_bytes())
}
