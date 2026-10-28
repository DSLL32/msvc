use crate::{
    Args, MapFormat, Profile,
    image::{Image, ImageView},
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
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq)]
pub struct Record {
    pub name: String,
    pub start: u32,
    pub end: u32,
    #[serde(default)]
    pub shared: Vec<(u32, u32)>,
    #[serde(default)]
    pub code_chunks: Vec<(u32, u32)>,
    #[serde(default)]
    pub evidence: Value,
    #[serde(default)]
    pub pending_extent: bool,
}
impl Record {
    pub fn code_ranges(&self) -> impl Iterator<Item = (u32, u32)> + '_ {
        std::iter::once((self.start, self.end)).chain(self.code_chunks.iter().copied())
    }
    pub fn contains_code(&self, address: u32) -> bool {
        self.code_ranges().any(|(a, b)| a <= address && address < b)
    }
}
pub fn json(path: &Path) -> Result<Value> {
    Ok(serde_json::from_slice(
        &fs::read(path).with_context(|| format!("read {}", path.display()))?,
    )?)
}
pub fn records(v: &Value) -> Result<Vec<Record>> {
    Ok(serde_json::from_value(v.clone())?)
}
fn append_original_drivers(original: &mut Vec<Record>, drivers: &[Record]) {
    // Registry-backed native CRT bodies can occur in both projections. Keep
    // one complete identical Record, not two copies of the same named body.
    // Conflicts, distinct aliases and different evidence remain untouched.
    original.retain(|record| !drivers.contains(record));
    original.extend_from_slice(drivers);
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
pub fn imports(
    pe: &Pe,
    raw: bool,
    strict: Option<&crate::imports::Catalog>,
) -> Result<Vec<Record>> {
    let directory = u32_at(&pe.data, pe.optional + 104)?;
    if directory == 0 {
        return Ok(Vec::new());
    }
    let lookup = if let Some(catalog) = strict {
        catalog
            .slots
            .iter()
            .map(|(&slot, id)| (slot, id.legacy_name()))
            .collect::<HashMap<_, _>>()
    } else {
        let mut p = pe.base + directory;
        let mut lookup = HashMap::new();
        for _ in 0..10000 {
            if pe.word(p + 12)? == 0 {
                break;
            }
            let names = pe.word(p)?;
            let iat = pe.word(p + 16)?;
            // OriginalFirstThunk may be omitted; FirstThunk then holds lookup RVAs.
            let names = if names == 0 { iat } else { names };
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
        lookup
    };
    let mut result = Vec::new();
    for s in &pe.sections {
        if !s.is_code() {
            continue;
        }
        let length = if raw {
            s.size
        } else {
            s.size.min(s.virtual_size)
        } as usize;
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
                    code_chunks: vec![],
                    evidence: Value::Null,
                    pending_extent: false,
                });
            }
        }
    }
    Ok(result)
}
pub type EntryNames = BTreeMap<u32, BTreeSet<String>>;
pub struct Inputs {
    pub terminating_calls: crate::terminating_calls::Contracts,
    pub shared_return_entries: crate::shared_returns::SharedReturns,
    pub o: Image,
    pub r: Image,
    pub oi: Vec<Record>,
    pub ri: Vec<Record>,
    pub drivers: Vec<Record>,
    pub registry_support: Vec<Record>,
    pub original_support: Vec<Record>,
    pub identity_only_entries: Vec<Value>,
    pub rebuilt_extent_checks: Vec<Value>,
    pub original_entry_names: EntryNames,
    pub rebuilt_entry_names: EntryNames,
    pub original_import_entries: BTreeSet<u32>,
    pub rebuilt_import_entries: BTreeSet<u32>,
    pub accepted: BTreeSet<String>,
    pub publics: BTreeMap<String, u32>,
    pub original_imports: Option<crate::imports::Catalog>,
    pub rebuilt_imports: Option<crate::imports::Catalog>,
    pub map_provenance: Option<Value>,
    pub config_semantic_sha256: Option<String>,
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
        let mut o = Image::open_original(&original, &cfg["target"]["image"], &a.project, &a.repo)?;
        ensure!(
            !o.is_cf() || (a.discovery == crate::Discovery::Cfg && a.profile == Profile::Cl),
            "CF images require standalone cl CFG metadata profile"
        );
        ensure!(
            !o.is_cf() || !a.check_imports,
            "CF has no PE IAT; --check-imports/register import resolution is unsupported"
        );
        ensure!(
            !o.is_cf() || !a.check_callbacks,
            "CF callback-table checks require fixup-aware pointer/null interpretation and are unsupported"
        );
        ensure!(
            Some(hash(o.source_bytes()).as_str()) == cfg["target"]["sha256"].as_str(),
            "target changed"
        );
        let mut r = Image::open_pe(
            &a.rebuilt
                .clone()
                .unwrap_or(a.project.join(format!("build/{name}.exe"))),
        )?;
        let original_imports = a
            .check_imports
            .then(|| crate::imports::Catalog::load(o.require_pe()?))
            .transpose()
            .context("original PE imports")?;
        let rebuilt_imports = a
            .check_imports
            .then(|| crate::imports::Catalog::load(r.require_pe()?))
            .transpose()
            .context("rebuilt PE imports")?;
        let map_path = a
            .map
            .clone()
            .unwrap_or(a.project.join(format!("build/{name}.map")));
        let (mut publics, absolute_publics, map_provenance) = match a.map_format {
            MapFormat::Msvc => (
                map(&map_path, a.profile == Profile::Rc)?,
                BTreeMap::new(),
                None,
            ),
            MapFormat::Watcom10 => {
                let parsed = crate::watcom_map::parse(&map_path, r.require_pe()?)?;
                (parsed.addresses, parsed.absolutes, Some(parsed.provenance))
            }
            MapFormat::Tlink32 => {
                let parsed = crate::tlink_map::parse(&map_path, r.require_pe()?)?;
                (parsed.addresses, BTreeMap::new(), Some(parsed.provenance))
            }
        };
        let raw_publics = publics.clone();
        let driver_key = if a.profile == Profile::Rcpp && cfg.get("driver").is_none() {
            "functions"
        } else {
            "driver"
        };
        let drivers = records(&cfg[driver_key])?;
        for driver in cfg[driver_key].as_array().into_iter().flatten() {
            let symbol = driver
                .get("symbol")
                .unwrap_or(&driver["name"])
                .as_str()
                .context("driver MAP symbol")?;
            ensure!(
                !absolute_publics.contains_key(symbol),
                "absolute Watcom MAP symbol cannot be a function: {symbol}"
            );
        }
        symbol_aliases(&cfg[driver_key], &mut publics)?;
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
                        && o.pe_sections().into_iter().flatten().any(|s| s.is_code()
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
        for record in &runtime {
            ensure!(
                !absolute_publics.contains_key(&record.name),
                "absolute Watcom MAP symbol cannot be a runtime function: {}",
                record.name
            );
        }
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
        if let Some(pe) = o.as_pe() {
            oi.extend(imports(
                pe,
                a.profile == Profile::Rcpp,
                original_imports.as_ref(),
            )?);
        }
        append_original_drivers(&mut oi, &drivers);
        let mut ri = if a.profile == Profile::Rcpp {
            Vec::new()
        } else {
            records(&json(
                &a.rebuilt_runtime
                    .clone()
                    .unwrap_or(a.project.join("build/runtime.json")),
            )?)?
        };
        for record in &ri {
            ensure!(
                !absolute_publics.contains_key(&record.name),
                "absolute Watcom MAP symbol cannot be a runtime function: {}",
                record.name
            );
        }
        if a.profile == Profile::Rc {
            ri.retain(|r| !driver_names.contains(r.name.as_str()));
        }
        ri.extend(imports(
            r.require_pe()?,
            a.profile == Profile::Rcpp,
            rebuilt_imports.as_ref(),
        )?);
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
        let mut rebuilt_extent_checks = Vec::new();
        let rebuilt_map_sha = cfg[driver_key]
            .as_array()
            .into_iter()
            .flatten()
            .any(|row| row.get("rebuilt_extent").is_some())
            .then(|| fs::read(&map_path).map(|bytes| hash(&bytes)))
            .transpose()?;
        for (i, d) in drivers.iter().enumerate() {
            let symbol = &d.name;
            if let Some(start) = publics.get(symbol) {
                let end = addresses
                    .range((
                        std::ops::Bound::Excluded(*start),
                        std::ops::Bound::Unbounded,
                    ))
                    .next()
                    .copied()
                    .context("no function end")?;
                let (end, check) = checked_rebuilt_extent(
                    &r,
                    &cfg[driver_key][i],
                    *start,
                    end,
                    rebuilt_map_sha.as_deref().unwrap_or(""),
                    &a.project,
                )?;
                if let Some(check) = check {
                    rebuilt_extent_checks.push(check);
                }
                ri.push(Record {
                    name: d.name.clone(),
                    start: *start,
                    end,
                    shared: vec![],
                    code_chunks: match cfg[driver_key][i].get("rebuilt_code_chunks") {
                        None => vec![],
                        Some(value) => serde_json::from_value(value.clone())?,
                    },
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
        for check in &rebuilt_extent_checks {
            let begin = num(&check["end"])?;
            let end = num(&check["map_envelope_end"])?;
            ensure!(
                ri.iter()
                    .all(|record| record.code_ranges().all(|(a, b)| b <= begin || end <= a)),
                "rebuilt padding overlaps declared code ownership"
            );
        }
        let mut registry_entries = registry_entry_names(
            &cfg,
            &raw_publics,
            &absolute_publics,
            &oi,
            &ri,
            original_imports.as_ref(),
            rebuilt_imports.as_ref(),
        )?;
        let identity_only_entries = identity_only_entries(
            &cfg,
            &raw_publics,
            &absolute_publics,
            &mut registry_entries,
            &a.project,
        )?;
        validate_registry_entry_bytes(&o, &registry_entries.original, "original")?;
        validate_registry_entry_bytes(&r, &registry_entries.rebuilt, "rebuilt")?;
        if cfg.get("function_registry").is_some() {
            ensure!(
                a.discovery != crate::Discovery::Legacy,
                "function_registry identity mappings require cfg discovery"
            );
        }
        let terminating_calls = crate::terminating_calls::Contracts::parse(
            &cfg,
            &o,
            &r,
            &drivers,
            &oi,
            &ri,
            &registry_entries.original,
            &registry_entries.rebuilt,
            &accepted,
        )?;
        o.bind_code(&oi)?;
        r.bind_code(&ri)?;
        let shared_metadata = if cfg.get("shared_return_entries").is_some()
            || cfg.get("shared_terminal_epilogues").is_some()
            || cfg.get("shared_call_return_tails").is_some()
            || cfg.get("shared_jump_tails").is_some()
            || cfg.get("shared_terminal_regions").is_some()
        {
            ensure!(
                a.discovery != crate::Discovery::Legacy,
                "shared RET entries require cfg discovery"
            );
            a.metadata
                .as_ref()
                .map(|path| json(path))
                .transpose()?
                .unwrap_or_default()
        } else {
            Value::Null
        };
        let shared_return_entries = crate::shared_returns::SharedReturns::parse(
            &cfg,
            &shared_metadata,
            &o,
            &drivers,
            &oi,
            &a.project,
        )?;
        for (side, pe, index) in [("original", &o, &oi), ("rebuilt", &r, &ri)] {
            if pe.is_cf() || index.iter().any(|record| !record.code_chunks.is_empty()) {
                ensure!(
                    a.discovery != crate::Discovery::Legacy,
                    "code_chunks require cfg discovery; legacy mode cannot audit fragmented functions"
                );
                if side == "original" {
                    validate_chunk_ownership_with_shared(pe, index, &shared_return_entries)
                } else {
                    validate_chunk_ownership(pe, index)
                }
                .with_context(|| format!("{side} code chunk ownership"))?;
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
        let registry_support = if a.check_registry_support_extents {
            registry_support_records(&cfg, &o)?
        } else {
            Vec::new()
        };
        let original_support = crate::instruction_export::support_records(
            &cfg,
            &o,
            &oi,
            &a.export_original_support_instructions,
            &a.project,
        )?;
        Ok(Self {
            terminating_calls,
            original_support,
            identity_only_entries,
            rebuilt_extent_checks,
            registry_support,
            shared_return_entries,
            o,
            r,
            oi,
            ri,
            drivers,
            original_entry_names: registry_entries.original,
            rebuilt_entry_names: registry_entries.rebuilt,
            original_import_entries: registry_entries.original_imports,
            rebuilt_import_entries: registry_entries.rebuilt_imports,
            accepted,
            publics,
            original_imports,
            rebuilt_imports,
            map_provenance,
            config_semantic_sha256: (a.export_instructions
                || !a.export_original_support_instructions.is_empty())
            .then(|| crate::instruction_export::semantic_hash(&cfg))
            .transpose()?,
        })
    }
}
/// Resolve explicit logical names against immutable real MAP keys. Different
/// real keys may alias one address; metadata may not invent alias chains or map
/// two logical vertices to the same real symbol key.
pub(crate) fn symbol_aliases(records: &Value, publics: &mut BTreeMap<String, u32>) -> Result<()> {
    let records = records
        .as_array()
        .context("function records must be an array")?;
    if !records.iter().any(|r| r.get("symbol").is_some()) {
        return Ok(());
    }
    let raw = publics.clone();
    let mut names = BTreeSet::new();
    let mut symbols = BTreeSet::new();
    let mut aliases = Vec::new();
    for record in records {
        let name = record["name"]
            .as_str()
            .filter(|s| !s.is_empty())
            .context("logical function name must be nonempty")?;
        ensure!(names.insert(name), "duplicate logical function name {name}");
        let explicit = record.get("symbol");
        let symbol = match explicit {
            None => name,
            Some(value) => value
                .as_str()
                .filter(|s| !s.is_empty())
                .context("symbol must be a nonempty MAP name")?,
        };
        ensure!(
            symbols.insert(symbol),
            "multiple logical functions map to MAP symbol {symbol}"
        );
        if explicit.is_some() {
            let address = *raw.get(symbol).with_context(|| {
                format!("missing actual MAP symbol {symbol} for logical {name}")
            })?;
            if symbol != name {
                ensure!(
                    !raw.contains_key(name),
                    "logical name {name} conflicts with an actual MAP public"
                );
                aliases.push((name.to_string(), address));
            }
        }
    }
    publics.extend(aliases);
    Ok(())
}

pub fn validate_chunk_ownership(pe: &impl ImageView, records: &[Record]) -> Result<()> {
    validate_chunk_ownership_with_shared(
        pe,
        records,
        &crate::shared_returns::SharedReturns::default(),
    )
}
pub(crate) fn validate_chunk_ownership_with_shared(
    pe: &impl ImageView,
    records: &[Record],
    shared: &crate::shared_returns::SharedReturns,
) -> Result<()> {
    for record in records {
        let mut ranges = record.code_ranges().collect::<Vec<_>>();
        ranges.sort();
        for &(a, b) in &ranges {
            ensure!(
                pe.file_backed_code(a, b),
                "{} invalid/non-file-backed code chunk {a:x}..{b:x}",
                record.name
            );
        }
        ensure!(
            ranges.windows(2).all(|w| w[0].1 <= w[1].0),
            "{} overlapping code chunks",
            record.name
        );
        for &(a, b) in &record.code_chunks {
            ensure!(
                records
                    .iter()
                    .flat_map(|r| &r.shared)
                    .all(|&(c, d)| b <= c || d <= a),
                "{} code chunk intersects declared shared text data",
                record.name
            );
        }
        for other in records {
            if std::ptr::eq(record, other) {
                continue;
            }
            let mut other_ranges = other.code_ranges().collect::<Vec<_>>();
            other_ranges.sort();
            if record.start == other.start && ranges == other_ranges {
                continue;
            } // exact physical aliases
            if shared.permits(record, other) {
                continue;
            }
            ensure!(
                ranges
                    .iter()
                    .all(|&(a, b)| other_ranges.iter().all(|&(c, d)| b <= c || d <= a)),
                "code chunk ownership overlaps {} / {}",
                record.name,
                other.name
            );
        }
    }
    Ok(())
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

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct RebuiltExtent {
    end: Value,
    padding_hex: String,
    image_sha256: String,
    map_sha256: String,
    object: Value,
    #[serde(
        default,
        deserialize_with = "crate::register_imports::documentary_evidence"
    )]
    evidence: Option<Value>,
}

fn checked_rebuilt_extent(
    image: &Image,
    driver: &Value,
    start: u32,
    envelope_end: u32,
    map_sha256: &str,
    project: &Path,
) -> Result<(u32, Option<Value>)> {
    let Some(value) = driver.get("rebuilt_extent") else {
        return Ok((envelope_end, None));
    };
    let name = driver["name"].as_str().context("rebuilt extent owner")?;
    let proof: RebuiltExtent =
        serde_json::from_value(value.clone()).context("rebuilt_extent contract")?;
    ensure!(
        proof.image_sha256 == hash(image.source_bytes()) && proof.map_sha256 == map_sha256,
        "rebuilt extent image/MAP binding differs: {name}"
    );
    let end = if let Some(object) = proof.end.as_object() {
        ensure!(
            object
                .keys()
                .all(|key| matches!(key.as_str(), "rebuilt_symbol" | "offset"))
                && object.get("rebuilt_symbol").and_then(Value::as_str)
                    == driver.get("symbol").unwrap_or(&driver["name"]).as_str(),
            "rebuilt extent must use its own MAP symbol"
        );
        let offset = object
            .get("offset")
            .map(|v| {
                v.as_i64()
                    .context("rebuilt extent offset must be an integer")
            })
            .transpose()?
            .unwrap_or(0);
        u32::try_from(
            i64::from(start)
                .checked_add(offset)
                .context("rebuilt extent offset overflow")?,
        )
        .context("rebuilt extent end outside PE32")?
    } else {
        num(&proof.end)?
    };
    ensure!(
        start < end
            && (2..=30).contains(&proof.padding_hex.len())
            && proof.padding_hex.len().is_multiple_of(2)
            && proof.padding_hex.bytes().all(|b| b == b'0')
            && end.checked_add((proof.padding_hex.len() / 2) as u32) == Some(envelope_end),
        "rebuilt extent requires exactly declared 1..15 zero alignment bytes: {name}"
    );
    ensure!(
        image.file_backed_code(end, envelope_end)
            && image
                .read(end, (envelope_end - end) as usize)?
                .iter()
                .all(|b| *b == 0),
        "rebuilt extent alignment byte differs: {name}"
    );
    let object = crate::register_imports::verified_evidence(project, &proof.object)
        .with_context(|| format!("rebuilt extent object {name}"))?;
    let evidence = crate::register_imports::verified_documentary_evidence(project, &proof.evidence)
        .with_context(|| format!("rebuilt extent witness {name}"))?;
    Ok((
        end,
        Some(serde_json::json!({"name":name,"start":start,"end":end,
        "map_envelope_end":envelope_end,"padding_hex":proof.padding_hex,"image_sha256":proof.image_sha256,
        "map_sha256":map_sha256,"object":object,"evidence":evidence})),
    ))
}

/// Entry identities have no body, extent, availability or acceptance meaning.
#[derive(Default)]
struct RegistryEntries {
    original: EntryNames,
    rebuilt: EntryNames,
    original_imports: BTreeSet<u32>,
    rebuilt_imports: BTreeSet<u32>,
}
fn validate_registry_entry_bytes(image: &Image, entries: &EntryNames, side: &str) -> Result<()> {
    for (&address, names) in entries {
        let end = address
            .checked_add(1)
            .context("function_registry entry overflow")?;
        if let Some(pe) = image.as_pe() {
            ensure!(
                pe.file_backed_code(address, end),
                "function_registry {side} entry is not executable file-backed code: {address:x} {names:?}"
            );
        } else {
            image.read(address, 1).with_context(|| {
                format!("function_registry {side} entry is not initialized: {address:x}")
            })?;
        }
    }
    Ok(())
}
fn registry_optional_str<'a>(item: &'a Value, key: &str) -> Result<Option<&'a str>> {
    match item.get(key) {
        None | Some(Value::Null) => Ok(None),
        Some(Value::String(value)) => Ok(Some(value)),
        _ => bail!("function_registry {key} must be a string or null"),
    }
}
fn registry_optional_ordinal(item: &Value) -> Result<Option<u16>> {
    match item.get("import_ordinal") {
        None | Some(Value::Null) => Ok(None),
        Some(value) => Ok(Some(
            value.as_u64().and_then(|v| u16::try_from(v).ok()).context(
                "function_registry import_ordinal must be an unsigned16-bit integer or null",
            )?,
        )),
    }
}
fn validate_registry_import(
    item: &Value,
    catalog: &crate::imports::Catalog,
    address: u32,
    side: &str,
) -> Result<()> {
    let name = item["name"].as_str().context("classified import name")?;
    let identity = catalog
        .thunks
        .get(&address)
        .with_context(|| format!("classified {side} import is not a verified PE thunk: {name}"))?;
    ensure!(
        matches!(
            (&identity.symbol, num(&item["definition"])?),
            (crate::imports::Symbol::Name { .. }, 1) | (crate::imports::Symbol::Ordinal { .. }, 2)
        ),
        "classified {side} import name/ordinal kind differs: {name}"
    );
    if let Some(dll) = registry_optional_str(item, "import_dll")? {
        ensure!(
            identity.dll
                == dll
                    .rsplit(['\\', '/'])
                    .next()
                    .unwrap_or(dll)
                    .to_ascii_lowercase(),
            "classified {side} import DLL differs: {name}"
        );
    }
    if let Some(declared) = registry_optional_str(item, "import_name")? {
        ensure!(
            matches!(&identity.symbol,crate::imports::Symbol::Name{name} if name==declared),
            "classified {side} import name differs: {name}"
        );
    }
    if let Some(ordinal) = registry_optional_ordinal(item)? {
        if num(&item["definition"])? == 2 {
            ensure!(
                matches!(&identity.symbol,crate::imports::Symbol::Ordinal{ordinal:o} if *o==ordinal),
                "classified {side} import ordinal differs: {name}"
            );
        } else {
            ensure!(ordinal == 0, "named import has nonzero ordinal: {name}");
        }
    }
    Ok(())
}
fn registry_entry_names(
    cfg: &Value,
    publics: &BTreeMap<String, u32>,
    absolutes: &BTreeMap<String, u32>,
    original: &[Record],
    rebuilt: &[Record],
    original_catalog: Option<&crate::imports::Catalog>,
    rebuilt_catalog: Option<&crate::imports::Catalog>,
) -> Result<RegistryEntries> {
    let Some(registry) = cfg.get("function_registry") else {
        return Ok(RegistryEntries::default());
    };
    let registry = registry
        .as_array()
        .context("function_registry must be an array")?;
    let base = num(&cfg["target"]["base"])?;
    let mut seen = BTreeSet::new();
    let mut out = RegistryEntries::default();
    let mut original_bodies = BTreeSet::new();
    let mut rebuilt_bodies = BTreeSet::new();
    for item in registry {
        let definition = num(&item["definition"])?;
        ensure!(definition <= 4, "unsupported function_registry definition");
        if definition == 0 {
            continue;
        }
        let name = item["name"]
            .as_str()
            .filter(|s| !s.is_empty())
            .context("function_registry name")?;
        if definition == 4 {
            ensure!(
                seen.insert(name),
                "duplicate function_registry application identity: {name}"
            );
        }
        let address = base
            .checked_add(num(&item["rva"])?)
            .context("function_registry entry overflow")?;
        ensure!(
            if definition == 4 {
                original
                    .iter()
                    .all(|r| r.name != name || r.start == address)
            } else {
                !original.iter().any(|r| r.name == name)
                    || original
                        .iter()
                        .any(|r| r.name == name && r.start == address)
            },
            "function_registry original identity conflicts with index: {name}"
        );
        out.original.entry(address).or_default().insert(name.into());
        if [1, 2].contains(&definition) {
            registry_optional_str(item, "import_dll")?;
            registry_optional_str(item, "import_name")?;
            registry_optional_ordinal(item)?;
            if let Some(catalog) = original_catalog {
                validate_registry_import(item, catalog, address, "original")?;
            }
            out.original_imports.insert(address);
        } else {
            original_bodies.insert(address);
        }
        let symbol = registry_optional_str(item, "original_symbol")?.unwrap_or(name);
        ensure!(
            !symbol.is_empty() && !absolutes.contains_key(symbol),
            "function_registry symbol is empty or absolute: {name}"
        );
        if let Some(&address) = publics.get(symbol) {
            ensure!(
                if definition == 4 {
                    rebuilt.iter().all(|r| r.name != name || r.start == address)
                } else {
                    !rebuilt.iter().any(|r| r.name == name)
                        || rebuilt.iter().any(|r| r.name == name && r.start == address)
                },
                "function_registry rebuilt identity conflicts with index: {name}"
            );
            out.rebuilt.entry(address).or_default().insert(name.into());
            if [1, 2].contains(&definition) {
                if let Some(catalog) = rebuilt_catalog {
                    validate_registry_import(item, catalog, address, "rebuilt")?;
                }
                out.rebuilt_imports.insert(address);
            } else {
                rebuilt_bodies.insert(address);
            }
        }
    }
    ensure!(
        out.original_imports.is_disjoint(&original_bodies)
            && out.rebuilt_imports.is_disjoint(&rebuilt_bodies),
        "conflicting function_registry import and App/CRT entry classifications"
    );
    Ok(out)
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct IdentityOnlyEntry {
    name: String,
    #[serde(
        default,
        deserialize_with = "crate::register_imports::documentary_evidence"
    )]
    evidence: Option<Value>,
}

/// Explicit correlation names have no body, classification or ABI meaning.
fn identity_only_entries(
    cfg: &Value,
    publics: &BTreeMap<String, u32>,
    absolutes: &BTreeMap<String, u32>,
    entries: &mut RegistryEntries,
    project: &Path,
) -> Result<Vec<Value>> {
    let Some(value) = cfg.get("identity_only_entries") else {
        return Ok(vec![]);
    };
    let selected: Vec<IdentityOnlyEntry> = serde_json::from_value(value.clone())
        .context("identity_only_entries must be an array of name/evidence objects")?;
    ensure!(
        !selected.is_empty(),
        "identity_only_entries must not be empty"
    );
    let registry = cfg["function_registry"]
        .as_array()
        .context("identity_only_entries requires function_registry")?;
    let base = num(&cfg["target"]["base"])?;
    let mut seen = BTreeSet::new();
    let mut result = Vec::new();
    for selected in selected {
        let name = selected.name;
        ensure!(
            !name.trim().is_empty() && seen.insert(name.clone()),
            "empty or duplicate identity-only endpoint: {name}"
        );
        let candidates: Vec<_> = registry
            .iter()
            .filter(|item| item["name"] == name)
            .collect();
        ensure!(
            candidates.len() == 1,
            "missing or ambiguous identity-only registry endpoint: {name}"
        );
        let item = candidates[0];
        ensure!(
            item["definition"] == 0
                && item["implementation"] == 0
                && item["code"].as_array().is_some_and(Vec::is_empty)
                && item["text_data"].as_array().is_some_and(Vec::is_empty)
                && registry_optional_str(item, "native_symbol")?.is_none(),
            "identity-only endpoint requires Unknown unimplemented registry entry without owned code/data or native body: {name}"
        );
        ensure!(
            !cfg["accepted"]
                .as_array()
                .into_iter()
                .flatten()
                .any(|v| v.as_str() == Some(&name))
                && !cfg["driver"]
                    .as_array()
                    .into_iter()
                    .flatten()
                    .any(|v| v["name"] == name),
            "identity-only endpoint must not be accepted or have a driver body: {name}"
        );
        let rva = num(&item["rva"])?;
        let original = base
            .checked_add(rva)
            .context("identity-only endpoint address overflow")?;
        let symbol = registry_optional_str(item, "original_symbol")?
            .filter(|s| !s.is_empty())
            .context("identity-only endpoint needs an explicit original_symbol")?;
        ensure!(
            !absolutes.contains_key(symbol),
            "identity-only endpoint MAP symbol is absolute: {name}"
        );
        let rebuilt = *publics.get(symbol).with_context(|| {
            format!("identity-only endpoint MAP symbol missing: {name} ({symbol})")
        })?;
        for other in registry {
            if std::ptr::eq(other, item) {
                continue;
            }
            ensure!(
                num(&other["rva"])? != rva,
                "identity-only endpoint shares an original address with another registry identity: {name}"
            );
            if let Some(other_symbol) =
                registry_optional_str(other, "original_symbol")?.or_else(|| other["name"].as_str())
            {
                ensure!(
                    publics.get(other_symbol) != Some(&rebuilt),
                    "identity-only endpoint shares a rebuilt address with another registry identity: {name}"
                );
            }
            let code: Vec<(u32, u32)> = serde_json::from_value(other["code"].clone())
                .context("identity-only endpoint registry code intervals")?;
            ensure!(
                !code.iter().any(|&(a, b)| a <= rva && rva < b),
                "identity-only endpoint is inside another registry body: {name}"
            );
        }
        ensure!(
            !entries.original.contains_key(&original) && !entries.rebuilt.contains_key(&rebuilt),
            "identity-only endpoint conflicts with an existing classified identity: {name}"
        );
        let evidence =
            crate::register_imports::verified_documentary_evidence(project, &selected.evidence)
                .with_context(|| format!("identity-only endpoint witness {name}"))?;
        entries
            .original
            .insert(original, BTreeSet::from([name.clone()]));
        entries
            .rebuilt
            .insert(rebuilt, BTreeSet::from([name.clone()]));
        result.push(
            serde_json::json!({"name":name,"original":original,"rebuilt":rebuilt,
            "original_symbol":symbol,"evidence":evidence}),
        );
    }
    result.sort_by(|a, b| a["name"].as_str().cmp(&b["name"].as_str()));
    Ok(result)
}

fn registry_support_records(cfg: &Value, image: &Image) -> Result<Vec<Record>> {
    let base = num(&cfg["target"]["base"])?;
    let registry = cfg["function_registry"]
        .as_array()
        .context("registry support checks require function_registry")?;
    let mut result = Vec::new();
    for entry in registry {
        if ![1, 2, 3].contains(&num(&entry["definition"])?) {
            continue;
        }
        let name = entry["name"].as_str().context("registry support name")?;
        let ranges: Vec<(u32, u32)> = serde_json::from_value(entry["code"].clone())
            .context("registry support code intervals")?;
        if ranges.is_empty() {
            continue;
        }
        let convert = |(a, b): (u32, u32)| -> Result<(u32, u32)> {
            ensure!(a < b, "invalid registry support interval: {name}");
            Ok((
                base.checked_add(a)
                    .context("registry support address overflow")?,
                base.checked_add(b)
                    .context("registry support address overflow")?,
            ))
        };
        ensure!(
            ranges[0].0 == num(&entry["rva"])?,
            "registry support primary interval does not begin at entry: {name}"
        );
        let mut code = ranges
            .into_iter()
            .map(convert)
            .collect::<Result<Vec<_>>>()?;
        let (start, end) = code.remove(0);
        let data: Vec<(u32, u32)> = serde_json::from_value(entry["text_data"].clone())
            .context("registry support text_data intervals")?;
        let record = Record {
            name: name.into(),
            start,
            end,
            code_chunks: code,
            shared: data.into_iter().map(convert).collect::<Result<_>>()?,
            evidence: Value::Null,
            pending_extent: false,
        };
        ensure!(
            record
                .code_ranges()
                .all(|(a, b)| image.file_backed_code(a, b)),
            "registry support extent outside original file-backed code: {name}"
        );
        ensure!(
            record
                .shared
                .iter()
                .all(|&(a, b)| image.file_backed_code(a, b)),
            "registry support text data outside original file-backed code: {name}"
        );
        validate_chunk_ownership(image, std::slice::from_ref(&record))?;
        result.push(record);
    }
    Ok(result)
}

pub(crate) fn integer_bytes(value: u64, width: usize) -> Result<[u8; 8]> {
    ensure!(width <= 8, "unsupported CRT global width {width}");
    ensure!(
        width == 8 || value < (1u64 << (width * 8)),
        "CRT global value does not fit declared {width}-byte width"
    );
    Ok(value.to_le_bytes())
}

#[cfg(test)]
mod registry_tests {
    use super::*;
    use serde_json::json;
    #[test]
    fn original_runtime_driver_merge_coalesces_only_complete_equal_records() {
        let driver = Record {
            name: "crt".into(),
            start: 0x401000,
            end: 0x401004,
            shared: vec![(0x402000, 0x402004)],
            code_chunks: vec![(0x401010, 0x401014)],
            pending_extent: false,
            evidence: serde_json::json!({"witness":"qualified"}),
        };
        let mut original = vec![driver.clone()];
        append_original_drivers(&mut original, std::slice::from_ref(&driver));
        assert_eq!(original, vec![driver.clone()]);
        for kind in 0..6 {
            let mut conflicting = driver.clone();
            match kind {
                0 => conflicting.start += 1,
                1 => conflicting.end += 1,
                2 => conflicting.code_chunks[0].1 += 1,
                3 => conflicting.shared[0].1 += 1,
                4 => conflicting.pending_extent = true,
                _ => conflicting.evidence = serde_json::json!({"witness":"other"}),
            }
            let mut original = vec![conflicting.clone()];
            append_original_drivers(&mut original, std::slice::from_ref(&driver));
            assert_eq!(original, vec![conflicting, driver.clone()]);
            assert_eq!(original.iter().filter(|r| r.name == "crt").count(), 2);
        }
        let mut alias = driver.clone();
        alias.name = "different_logical_identity".into();
        let mut original = vec![alias.clone(), driver.clone()];
        append_original_drivers(&mut original, std::slice::from_ref(&driver));
        assert_eq!(original, vec![alias, driver]);
    }
    #[test]
    fn exact_rebuilt_end_checks_padding_and_all_bound_inputs() {
        let image = Image::Pe(crate::tests::image(&[0xc3, 0]));
        let project = Path::new(env!("CARGO_MANIFEST_DIR"));
        let evidence = json!({"path":"fixtures/register-preservation.json",
            "sha256":hash(include_bytes!("../fixtures/register-preservation.json"))});
        let map_sha = "a".repeat(64);
        let row = json!({"name":"owner","symbol":"actual","rebuilt_extent":{
            "end":{"rebuilt_symbol":"actual","offset":1},"padding_hex":"00",
            "image_sha256":hash(image.source_bytes()),"map_sha256":map_sha,
            "object":evidence,"evidence":evidence}});
        let run = |row: &Value| {
            checked_rebuilt_extent(&image, row, 0x401000, 0x401002, &map_sha, project)
        };
        let (end, proof) = run(&row).unwrap();
        assert_eq!(end, 0x401001);
        assert_eq!(proof.unwrap()["map_envelope_end"], 0x401002);
        assert_eq!(run(&json!({"name":"owner"})).unwrap(), (0x401002, None));
        for (key, value) in [
            ("padding_hex", json!("90")),
            ("padding_hex", json!("0000")),
            ("end", json!(0x401000)),
            ("end", json!(0x401002)),
            ("end", json!(0x401003)),
            ("end", json!({"rebuilt_symbol":"other","offset":1})),
            ("end", json!({"rebuilt_symbol":"actual","offset":"1"})),
            (
                "end",
                json!({"rebuilt_symbol":"actual","offset":1,"typo":true}),
            ),
            ("image_sha256", json!("0".repeat(64))),
            ("map_sha256", json!("0".repeat(64))),
            ("object", Value::Null),
            ("evidence", Value::Null),
        ] {
            let mut wrong = row.clone();
            wrong["rebuilt_extent"][key] = value;
            assert!(run(&wrong).is_err(), "{key}");
        }
        let mut wrong = row.clone();
        wrong["rebuilt_extent"]["typo"] = json!(true);
        assert!(run(&wrong).is_err());
        let changed = Image::Pe(crate::tests::image(&[0xc3, 0x90]));
        let mut wrong = row.clone();
        wrong["rebuilt_extent"]["image_sha256"] = json!(hash(changed.source_bytes()));
        assert!(
            checked_rebuilt_extent(&changed, &wrong, 0x401000, 0x401002, &map_sha, project)
                .is_err()
        );
    }

    #[test]
    fn unknown_endpoint_selection_is_evidenced_names_only_and_strict() {
        let witness = json!({"path":"fixtures/register-preservation.json",
            "sha256":hash(include_bytes!("../fixtures/register-preservation.json"))});
        let cfg = json!({"target":{"base":0x400000},"accepted":[],"driver":[],
            "identity_only_entries":[{"name":"external","evidence":witness}],
            "function_registry":[{"name":"external","rva":0x1000,"definition":0,
                "implementation":0,"code":[],"text_data":[],"native_symbol":null,"original_symbol":"actual"}]});
        let publics = BTreeMap::from([("actual".into(), 0x401000)]);
        let project = Path::new(env!("CARGO_MANIFEST_DIR"));
        let run =
            |cfg: &Value, publics: &BTreeMap<String, u32>, absolutes: &BTreeMap<String, u32>| {
                let mut entries = RegistryEntries::default();
                let report = identity_only_entries(cfg, publics, absolutes, &mut entries, project)?;
                Ok::<_, anyhow::Error>((report, entries))
            };
        let (report, entries) = run(&cfg, &publics, &BTreeMap::new()).unwrap();
        assert_eq!(report.len(), 1);
        assert!(entries.original[&0x401000].contains("external"));
        assert!(entries.rebuilt[&0x401000].contains("external"));
        assert!(entries.original_imports.is_empty() && entries.rebuilt_imports.is_empty());
        assert_eq!(cfg["function_registry"][0]["definition"], 0);
        assert!(
            run(&json!({}), &BTreeMap::new(), &BTreeMap::new())
                .unwrap()
                .0
                .is_empty()
        );
        for (key, value) in [
            ("definition", json!(3)),
            ("implementation", json!(1)),
            ("code", json!([[0x1000, 0x1001]])),
            ("text_data", json!([[0x1000, 0x1001]])),
            ("native_symbol", json!("native")),
            ("original_symbol", Value::Null),
            ("original_symbol", json!(7)),
            ("rva", json!(0xffff_ffffu32)),
        ] {
            let mut wrong = cfg.clone();
            wrong["function_registry"][0][key] = value;
            assert!(run(&wrong, &publics, &BTreeMap::new()).is_err(), "{key}");
        }
        for value in [
            json!([]),
            Value::Null,
            json!(["external"]),
            json!([{"name":"external","evidence":witness,"typo":true}]),
            json!([{"name":"missing","evidence":witness}]),
            json!([{"name":"external","evidence":null}]),
            json!([{"name":"external","evidence":{"path":"fixtures/register-preservation.json","sha256":"0".repeat(64)}}]),
        ] {
            let mut wrong = cfg.clone();
            wrong["identity_only_entries"] = value;
            assert!(run(&wrong, &publics, &BTreeMap::new()).is_err());
        }
        for key in ["identity_only_entries", "function_registry"] {
            let mut wrong = cfg.clone();
            let copy = wrong[key][0].clone();
            wrong[key].as_array_mut().unwrap().push(copy);
            assert!(run(&wrong, &publics, &BTreeMap::new()).is_err());
        }
        for key in ["accepted", "driver"] {
            let mut wrong = cfg.clone();
            wrong[key] = if key == "accepted" {
                json!(["external"])
            } else {
                json!([{"name":"external"}])
            };
            assert!(run(&wrong, &publics, &BTreeMap::new()).is_err());
        }
        let mut wrong = cfg.clone();
        wrong["function_registry"]
            .as_array_mut()
            .unwrap()
            .push(json!({"name":"other","rva":0x1000,"code":[],"original_symbol":"other"}));
        assert!(run(&wrong, &publics, &BTreeMap::new()).is_err());
        wrong["function_registry"][1]["rva"] = json!(0x1002);
        wrong["function_registry"][1]["code"] = json!([[0x1000, 0x1003]]);
        assert!(run(&wrong, &publics, &BTreeMap::new()).is_err());
        wrong["function_registry"][1]["code"] = json!([]);
        let aliased = BTreeMap::from([("actual".into(), 0x401000), ("other".into(), 0x401000)]);
        assert!(run(&wrong, &aliased, &BTreeMap::new()).is_err());
        assert!(run(&cfg, &BTreeMap::new(), &BTreeMap::new()).is_err());
        assert!(run(&cfg, &publics, &publics).is_err());
    }

    #[test]
    fn registry_support_uses_only_exact_known_support_intervals() {
        let image = Image::Pe(crate::tests::image(&[0x90, 0x90, 0x90, 0xc3]));
        let entry = serde_json::json!({"name":"support","definition":3,"rva":0x1000,"code":[[0x1000,0x1004]],"text_data":[]});
        let mut cfg = serde_json::json!({"target":{"base":0x400000},"function_registry":[entry]});
        assert_eq!(registry_support_records(&cfg, &image).unwrap().len(), 1);
        for definition in [0, 4] {
            cfg["function_registry"][0]["definition"] = serde_json::json!(definition);
            assert!(registry_support_records(&cfg, &image).unwrap().is_empty());
        }
        cfg["function_registry"][0]["definition"] = serde_json::json!(3);
        cfg["function_registry"][0]["code"] = serde_json::json!([]);
        assert!(registry_support_records(&cfg, &image).unwrap().is_empty());
        for code in [
            serde_json::json!(null),
            serde_json::json!([[0x1001, 0x1004]]),
            serde_json::json!([[0x1000, 0x1000]]),
            serde_json::json!([[0x1000, 0x2000]]),
            serde_json::json!([[0x1000, 0x1004], [0x1002, 0x1004]]),
        ] {
            cfg["function_registry"][0]["code"] = code;
            assert!(registry_support_records(&cfg, &image).is_err());
        }
    }
    #[test]
    fn entry_names_do_not_invent_bodies_and_import_classification_is_explicit() {
        let cfg = json!({"target":{"base":0x400000},"function_registry":[{"name":"pending","rva":0x1000,"definition":4,"original_symbol":"actual"}]});
        let publics = BTreeMap::from([("actual".into(), 0x501000)]);
        let entries =
            registry_entry_names(&cfg, &publics, &BTreeMap::new(), &[], &[], None, None).unwrap();
        assert!(entries.original[&0x401000].contains("pending"));
        assert!(entries.rebuilt[&0x501000].contains("pending"));
        assert!(entries.original_imports.is_empty());
        let image = Image::Pe(crate::tests::image(&[0xc3]));
        assert!(validate_registry_entry_bytes(&image, &entries.original, "original").is_ok());
        let invalid = BTreeMap::from([(0xfffffff0, BTreeSet::from(["bad".into()]))]);
        assert!(validate_registry_entry_bytes(&image, &invalid, "original").is_err());
        let mut data = crate::tests::image(&[0xc3]);
        data.sections[0].flags = 0xc0000040;
        assert!(
            validate_registry_entry_bytes(&Image::Pe(data), &entries.original, "rebuilt").is_err()
        );
        let mut import = cfg.clone();
        import["function_registry"][0]["definition"] = json!(1);
        assert!(
            registry_entry_names(
                &import,
                &publics,
                &BTreeMap::new(),
                &[],
                &[],
                Some(&crate::imports::Catalog::default()),
                None
            )
            .is_err()
        );
        let mut catalog = crate::imports::Catalog::default();
        catalog.thunks.insert(
            0x401000,
            crate::imports::Identity {
                dll: "msvcrt.dll".into(),
                symbol: crate::imports::Symbol::Name { name: "fn".into() },
            },
        );
        let entries = registry_entry_names(
            &import,
            &publics,
            &BTreeMap::new(),
            &[],
            &[],
            Some(&catalog),
            None,
        )
        .unwrap();
        assert!(entries.original_imports.contains(&0x401000));
        for (key, value) in [
            ("import_dll", json!(7)),
            ("import_name", json!([])),
            ("import_ordinal", json!("x")),
            ("original_symbol", json!(7)),
        ] {
            let mut malformed = import.clone();
            malformed["function_registry"][0][key] = value;
            assert!(
                registry_entry_names(
                    &malformed,
                    &publics,
                    &BTreeMap::new(),
                    &[],
                    &[],
                    Some(&catalog),
                    None
                )
                .is_err()
            );
        }
        let mut wrong_provider = import.clone();
        wrong_provider["function_registry"][0]["import_dll"] = json!("kernel32.dll");
        assert!(
            registry_entry_names(
                &wrong_provider,
                &publics,
                &BTreeMap::new(),
                &[],
                &[],
                Some(&catalog),
                None
            )
            .is_err()
        );
        let preserved = registry_entry_names(
            &cfg,
            &publics,
            &BTreeMap::new(),
            &[],
            &[],
            Some(&catalog),
            None,
        )
        .unwrap();
        assert!(preserved.original_imports.is_empty());
        let mut duplicate = cfg.clone();
        duplicate["function_registry"]
            .as_array_mut()
            .unwrap()
            .push(cfg["function_registry"][0].clone());
        assert!(
            registry_entry_names(&duplicate, &publics, &BTreeMap::new(), &[], &[], None, None)
                .is_err()
        );
        assert!(
            registry_entry_names(
                &cfg,
                &publics,
                &BTreeMap::from([("actual".into(), 0x501000)]),
                &[],
                &[],
                None,
                None
            )
            .is_err()
        );
    }
}
