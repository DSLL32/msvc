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
#[derive(Clone, Debug, Deserialize, Serialize)]
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
    pub o: Image,
    pub r: Image,
    pub oi: Vec<Record>,
    pub ri: Vec<Record>,
    pub drivers: Vec<Record>,
    pub registry_support: Vec<Record>,
    pub original_support: Vec<Record>,
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
                        && o.pe_sections()
                            .into_iter()
                            .flatten()
                            .any(|s| s.flags & 0x20 != 0
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
        let registry_entries = registry_entry_names(
            &cfg,
            &raw_publics,
            &absolute_publics,
            &oi,
            &ri,
            original_imports.as_ref(),
            rebuilt_imports.as_ref(),
        )?;
        validate_registry_entry_bytes(&o, &registry_entries.original, "original")?;
        validate_registry_entry_bytes(&r, &registry_entries.rebuilt, "rebuilt")?;
        if cfg.get("function_registry").is_some() {
            ensure!(
                a.discovery != crate::Discovery::Legacy,
                "function_registry identity mappings require cfg discovery"
            );
        }
        o.bind_code(&oi)?;
        r.bind_code(&ri)?;
        for (side, pe, index) in [("original", &o, &oi), ("rebuilt", &r, &ri)] {
            if pe.is_cf() || index.iter().any(|record| !record.code_chunks.is_empty()) {
                ensure!(
                    a.discovery != crate::Discovery::Legacy,
                    "code_chunks require cfg discovery; legacy mode cannot audit fragmented functions"
                );
                validate_chunk_ownership(pe, index)
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
            original_support,
            registry_support,
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
        data.sections[0].flags &= !0x20;
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
