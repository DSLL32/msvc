//! Exact shipped-code identification, with relocation-aware fixed-point pruning.
//! This recognizes runtime boundaries; it does not reconstruct or credit code.
use anyhow::{Context, Result, bail, ensure};
use clap::Args;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::{HashMap, HashSet},
    fs,
    path::{Path, PathBuf},
};

#[derive(Args)]
pub struct Options {
    pub toolchain: String,
    pub image: PathBuf,
    #[arg(long)]
    pub output: PathBuf,
    #[arg(long)]
    pub repo: Option<PathBuf>,
    /// COFF archives; explicit inputs replace the toolchain defaults.
    #[arg(long = "library")]
    pub libraries: Vec<PathBuf>,
    /// Standalone COFF objects; explicit inputs replace the toolchain defaults.
    #[arg(long = "object")]
    pub objects: Vec<PathBuf>,
    /// Reviewed original-only bindings, checked against all declared hashes.
    #[arg(long)]
    pub bindings: Option<PathBuf>,
    /// Projected registry; only explicit historical category=runtime entries qualify.
    #[arg(long, requires = "override_objects")]
    pub override_registry: Option<PathBuf>,
    /// Rebuilt MSVC MAP identifying selected object ownership.
    #[arg(long, requires = "override_objects")]
    pub override_map: Option<PathBuf>,
    /// Compiled historical CRT overrides, used only to resolve callee identities.
    #[arg(long = "override-object", requires_all = ["override_registry", "override_map"])]
    pub override_objects: Vec<PathBuf>,
}

fn bytes(data: &[u8], offset: usize, count: usize) -> Result<&[u8]> {
    data.get(offset..offset.checked_add(count).context("file offset overflow")?)
        .context("truncated PE/COFF/archive data")
}
fn word(data: &[u8], offset: usize) -> Result<u16> {
    Ok(u16::from_le_bytes(bytes(data, offset, 2)?.try_into()?))
}
fn dword(data: &[u8], offset: usize) -> Result<u32> {
    Ok(u32::from_le_bytes(bytes(data, offset, 4)?.try_into()?))
}
fn latin1(data: &[u8]) -> String {
    data.iter().map(|&b| char::from(b)).collect()
}
fn cstring(data: &[u8], offset: usize) -> Result<String> {
    let data = data.get(offset..).context("string offset beyond file")?;
    Ok(latin1(
        &data[..data
            .iter()
            .position(|&b| b == 0)
            .context("unterminated string")?],
    ))
}
fn short_name(data: &[u8]) -> String {
    latin1(&data[..data.iter().rposition(|&b| b != 0).map_or(0, |n| n + 1)])
}

struct PeSection {
    name: String,
    rva: u32,
    raw_size: usize,
    offset: usize,
    virtual_size: usize,
    flags: u32,
}
struct Pe {
    data: Vec<u8>,
    base: u32,
    optional: usize,
    sections: Vec<PeSection>,
}
impl Pe {
    fn parse(data: Vec<u8>) -> Result<Self> {
        ensure!(data.get(..2) == Some(b"MZ"), "image is not PE");
        let pe = dword(&data, 0x3c)? as usize;
        ensure!(
            bytes(&data, pe, 4)? == b"PE\0\0" && word(&data, pe + 4)? == 0x14c,
            "image must be i386 PE"
        );
        let optional = pe + 24;
        ensure!(word(&data, optional)? == 0x10b, "image must be PE32");
        let base = dword(&data, optional + 28)?;
        let table = optional + word(&data, pe + 20)? as usize;
        let mut sections = vec![];
        for i in 0..word(&data, pe + 6)? as usize {
            let p = table + 40 * i;
            sections.push(PeSection {
                name: short_name(bytes(&data, p, 8)?),
                virtual_size: dword(&data, p + 8)? as usize,
                rva: dword(&data, p + 12)?,
                raw_size: dword(&data, p + 16)? as usize,
                offset: dword(&data, p + 20)? as usize,
                flags: dword(&data, p + 36)?,
            });
        }
        Ok(Self {
            data,
            base,
            optional,
            sections,
        })
    }
    fn offset(&self, va: u32) -> Result<usize> {
        let rva = va
            .checked_sub(self.base)
            .context("address below image base")?;
        for s in &self.sections {
            if rva >= s.rva && (rva - s.rva) < s.raw_size as u32 {
                return s
                    .offset
                    .checked_add((rva - s.rva) as usize)
                    .context("PE offset overflow");
            }
        }
        bail!("unbacked address {va:08x}")
    }
    fn read(&self, va: u32, count: usize) -> Result<&[u8]> {
        bytes(&self.data, self.offset(va)?, count)
    }
    fn imports(&self) -> Result<HashMap<String, u32>> {
        let mut targets = HashMap::new();
        let import_rva = dword(&self.data, self.optional + 104)?;
        if import_rva == 0 {
            return Ok(targets);
        }
        let mut p = self.offset(
            self.base
                .checked_add(import_rva)
                .context("import RVA overflow")?,
        )?;
        let mut lookup = HashMap::new();
        while dword(&self.data, p + 12)? != 0 {
            let names = dword(&self.data, p)?;
            let iat = dword(&self.data, p + 16)?;
            for n in 0..10000u32 {
                let table = self
                    .base
                    .checked_add(names)
                    .and_then(|v| v.checked_add(4 * n))
                    .context("import lookup overflow")?;
                let v = dword(self.read(table, 4)?, 0)?;
                if v == 0 {
                    break;
                }
                let name = if v & 0x80000000 != 0 {
                    format!("ordinal_{}", v & 65535)
                } else {
                    cstring(
                        &self.data,
                        self.offset(
                            self.base
                                .checked_add(v)
                                .and_then(|v| v.checked_add(2))
                                .context("import name overflow")?,
                        )?,
                    )?
                };
                let address = self
                    .base
                    .checked_add(iat)
                    .and_then(|v| v.checked_add(4 * n))
                    .context("IAT overflow")?;
                lookup.insert(address, name);
            }
            p += 20;
        }
        for section in &self.sections {
            if section.flags & 0x20 == 0 {
                continue;
            }
            let code = bytes(&self.data, section.offset, section.virtual_size)?;
            for i in 0..code.len().saturating_sub(5) {
                if code[i..i + 2] == [0xff, 0x25] {
                    if let Some(name) = lookup.get(&dword(code, i + 2)?) {
                        let start = self
                            .base
                            .checked_add(section.rva)
                            .and_then(|v| v.checked_add(i as u32))
                            .context("import thunk overflow")?;
                        targets.insert(name.clone(), start);
                    }
                }
            }
        }
        Ok(targets)
    }
}

#[derive(Clone)]
struct Symbol {
    name: String,
    value: i64,
    section: i16,
    kind: u16,
    storage: u8,
}
#[derive(Clone)]
struct Relocation {
    offset: usize,
    target: Symbol,
    kind: u16,
}
#[derive(Clone)]
struct Section {
    data: Vec<u8>,
    relocs: Vec<Relocation>,
    index: i16,
    flags: u32,
}
struct Coff {
    symbols: Vec<Option<Symbol>>,
    sections: Vec<Section>,
}
impl Coff {
    fn parse(data: &[u8]) -> Result<Self> {
        ensure!(
            word(data, 0)? == 0x14c,
            "runtime member must be i386 COFF (OMF/import objects unsupported)"
        );
        let symoff = dword(data, 8)? as usize;
        let count = dword(data, 12)? as usize;
        let strings = symoff
            .checked_add(count.checked_mul(18).context("symbol size overflow")?)
            .context("symbol offset overflow")?;
        // Bound allocation using the file itself before trusting the symbol count.
        bytes(
            data,
            symoff,
            count.checked_mul(18).context("symbol size overflow")?,
        )?;
        let mut symbols = vec![None; count];
        let mut i = 0;
        while i < count {
            let p = symoff + 18 * i;
            let name = if bytes(data, p, 4)? == [0; 4] {
                cstring(
                    data,
                    strings
                        .checked_add(dword(data, p + 4)? as usize)
                        .context("symbol name overflow")?,
                )?
            } else {
                short_name(bytes(data, p, 8)?)
            };
            symbols[i] = Some(Symbol {
                name,
                value: dword(data, p + 8)? as i64,
                section: word(data, p + 12)? as i16,
                kind: word(data, p + 14)?,
                storage: bytes(data, p + 16, 1)?[0],
            });
            i += 1 + bytes(data, p + 17, 1)?[0] as usize;
        }
        let mut sections = vec![];
        for i in 0..word(data, 2)? as usize {
            let p = 20 + word(data, 16)? as usize + 40 * i;
            let size = dword(data, p + 16)? as usize;
            let offset = dword(data, p + 20)? as usize;
            let reloc = dword(data, p + 24)? as usize;
            let mut relocs = vec![];
            for j in 0..word(data, p + 32)? as usize {
                let q = reloc + 10 * j;
                let symbol = dword(data, q + 4)? as usize;
                relocs.push(Relocation {
                    offset: dword(data, q)? as usize,
                    target: symbols
                        .get(symbol)
                        .and_then(Option::as_ref)
                        .context("relocation references missing/aux COFF symbol")?
                        .clone(),
                    kind: word(data, q + 8)?,
                });
            }
            sections.push(Section {
                data: if offset == 0 {
                    vec![]
                } else {
                    bytes(data, offset, size)?.to_vec()
                },
                relocs,
                index: (i + 1).try_into().context("too many COFF sections")?,
                flags: dword(data, p + 36)?,
            });
        }
        Ok(Self { symbols, sections })
    }
}
struct Library {
    name: String,
    data: Vec<u8>,
    members: Vec<(String, Vec<u8>)>,
}
fn archive(data: &[u8]) -> Result<Vec<(String, Vec<u8>)>> {
    ensure!(
        data.get(..8) == Some(b"!<arch>\n"),
        "runtime library must be a COFF ar archive"
    );
    let mut members = vec![];
    let mut p = 8;
    let mut names = &[][..];
    while p < data.len() {
        let header = bytes(data, p, 60)?;
        ensure!(&header[58..60] == b"`\n", "invalid archive header");
        let size = std::str::from_utf8(&header[48..58])?
            .trim_end_matches(['\0', ' '])
            .parse::<usize>()?;
        let name = std::str::from_utf8(&header[..16])?.trim();
        let payload = bytes(data, p + 60, size)?;
        if name == "//" {
            names = payload;
        } else if let Some(offset) = name
            .strip_prefix('/')
            .filter(|v| !v.is_empty() && v.bytes().all(|b| b.is_ascii_digit()))
        {
            let start = offset.parse::<usize>()?;
            let rest = names
                .get(start..)
                .context("archive long-name offset beyond table")?;
            let end = rest
                .iter()
                .position(|&b| b == 0)
                .or_else(|| rest.iter().position(|&b| b == b'\n'))
                .context("unterminated archive member name")?;
            members.push((
                latin1(&rest[..end]).trim_end_matches('/').to_owned(),
                payload.to_vec(),
            ));
        } else if name != "/" && name != "//" {
            members.push((name.trim_end_matches('/').to_owned(), payload.to_vec()));
        }
        p = p
            .checked_add(60)
            .and_then(|v| v.checked_add(size))
            .and_then(|v| v.checked_add(size & 1))
            .context("archive offset overflow")?;
    }
    Ok(members)
}

#[derive(Clone, Serialize, Deserialize)]
struct Record {
    name: String,
    start: u32,
    end: u32,
    object: String,
    library: String,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    shared: Vec<[u32; 2]>,
}
struct Candidate {
    section: Section,
    start: u32,
    records: Vec<Record>,
}
fn matches(code: &[u8], section: &Section) -> Result<Vec<usize>> {
    if section.data.is_empty() {
        return Ok(vec![]);
    }
    let mut mask = vec![true; section.data.len()];
    for reloc in &section.relocs {
        if !matches!(reloc.kind, 6 | 20) {
            return Ok(vec![]);
        }
        let end = reloc.offset.checked_add(4).context("relocation overflow")?;
        mask.get_mut(reloc.offset..end)
            .context("relocation exceeds code section")?
            .fill(false);
    }
    let mut runs = vec![];
    let mut p = 0;
    while p < mask.len() {
        if !mask[p] {
            p += 1;
            continue;
        }
        let start = p;
        while p < mask.len() && mask[p] {
            p += 1;
        }
        runs.push((start, p));
    }
    // The Python matcher takes the first longest run on equal lengths.
    let Some(&(a, b)) = runs
        .iter()
        .max_by_key(|&&(a, b)| (b - a, std::cmp::Reverse(a)))
    else {
        return Ok(vec![]);
    };
    if b - a < 5 {
        return Ok(vec![]);
    }
    let anchor = &section.data[a..b];
    let mut result = vec![];
    if anchor.len() > code.len() {
        return Ok(result);
    }
    let finder = memchr::memmem::Finder::new(anchor);
    let mut position = 0;
    while let Some(found) = finder.find(&code[position..]) {
        let q = position + found;
        position = q + 1; // Preserve overlapping matches, as bytes.find did.
        let Some(start) = q.checked_sub(a) else {
            continue;
        };
        if runs
            .iter()
            .all(|&(x, y)| code.get(start + x..start + y) == Some(&section.data[x..y]))
        {
            result.push(start);
        }
    }
    Ok(result)
}
fn collect(
    pe: &Pe,
    library: &str,
    member: &str,
    data: &[u8],
    code: &[u8],
    text_rva: u32,
    candidates: &mut Vec<Candidate>,
) -> Result<()> {
    let obj = Coff::parse(data).with_context(|| format!("read {library} member {member}"))?;
    for text in &obj.sections {
        if text.flags & 0x20 == 0 {
            continue;
        }
        let mut funcs: Vec<_> = obj
            .symbols
            .iter()
            .flatten()
            .filter(|s| {
                s.section == text.index
                    && (s.kind == 0x20 || s.storage == 2)
                    && !s.name.starts_with('.')
            })
            .cloned()
            .collect();
        funcs.sort_by_key(|s| s.value);
        let whole = matches(code, text)?;
        let mut pieces = vec![];
        if !whole.is_empty() {
            pieces.push((text.clone(), funcs, whole));
        } else {
            for (i, sym) in funcs.iter().enumerate() {
                let begin = usize::try_from(sym.value)?;
                let end = if i + 1 < funcs.len() {
                    usize::try_from(funcs[i + 1].value)?
                } else {
                    text.data.len()
                };
                if begin == end {
                    continue;
                }
                let mut fragment = text.clone();
                fragment.data = bytes(
                    &text.data,
                    begin,
                    end.checked_sub(begin).context("function extent reversed")?,
                )?
                .to_vec();
                fragment.relocs.clear();
                for reloc in &text.relocs {
                    if begin <= reloc.offset && reloc.offset < end {
                        let mut reloc = reloc.clone();
                        reloc.offset -= begin;
                        if reloc.target.section == text.index {
                            reloc.target.value -= begin as i64;
                        }
                        fragment.relocs.push(reloc);
                    }
                }
                let mut sym = sym.clone();
                sym.value = 0;
                let found = matches(code, &fragment)?;
                pieces.push((fragment, vec![sym], found));
            }
        }
        for (fragment, symbols, found) in pieces {
            for offset in found {
                let start = pe
                    .base
                    .checked_add(text_rva)
                    .and_then(|v| v.checked_add(offset as u32))
                    .context("matched section address overflow")?;
                let mut records = vec![];
                for (i, sym) in symbols.iter().enumerate() {
                    let end = if i + 1 < symbols.len() {
                        symbols[i + 1].value
                    } else {
                        fragment.data.len() as i64
                    };
                    let address =
                        |value: i64| -> Result<u32> { Ok(u32::try_from(start as i64 + value)?) };
                    records.push(Record {
                        name: sym.name.clone(),
                        start: address(sym.value)?,
                        end: address(end)?,
                        object: member.to_owned(),
                        library: library.to_owned(),
                        shared: vec![],
                    });
                }
                candidates.push(Candidate {
                    section: fragment.clone(),
                    start,
                    records,
                });
            }
        }
    }
    Ok(())
}
// MAP ownership and exact compiled-object bytes qualify ONLY explicit historical
// CRT identities. They never add source overrides to the shipped runtime index.
#[derive(Clone)]
struct MapSymbol {
    address: u32,
    object: String,
}
struct LinkMap {
    symbols: HashMap<String, Vec<MapSymbol>>,
}
impl LinkMap {
    fn parse(text: &str, pe: &Pe) -> Result<Self> {
        let mut base = None;
        let mut timestamp = None;
        let mut publics = false;
        let mut symbols: HashMap<String, Vec<MapSymbol>> = HashMap::new();
        for line in text.lines().map(str::trim) {
            if let Some(value) = line.strip_prefix("Timestamp is ") {
                ensure!(timestamp.is_none(), "duplicate MAP timestamp");
                timestamp = Some(u32::from_str_radix(
                    value
                        .split_whitespace()
                        .next()
                        .context("MAP timestamp missing")?,
                    16,
                )?);
            }
            if let Some(value) = line.strip_prefix("Preferred load address is ") {
                ensure!(base.is_none(), "duplicate MAP image base");
                base = Some(u32::from_str_radix(value.trim(), 16)?);
            }
            if line.contains("Publics by Value") {
                publics = true;
                continue;
            }
            if !publics {
                continue;
            }
            let fields: Vec<_> = line.split_whitespace().collect();
            if fields.len() >= 4
                && fields[0].split_once(':').is_some_and(|(section, offset)| {
                    !section.is_empty()
                        && !offset.is_empty()
                        && section
                            .bytes()
                            .chain(offset.bytes())
                            .all(|b| b.is_ascii_hexdigit())
                })
            {
                let address =
                    u32::from_str_radix(fields[2], 16).context("invalid MAP public address")?;
                let owner = fields
                    .get(if fields[3] == "f" { 4 } else { 3 })
                    .context("MAP function lacks object owner")?;
                symbols
                    .entry(fields[1].to_owned())
                    .or_default()
                    .push(MapSymbol {
                        address,
                        object: (*owner).to_owned(),
                    });
            }
        }
        ensure!(base == Some(pe.base), "override MAP image base differs");
        ensure!(
            timestamp == Some(dword(&pe.data, pe.optional - 16)?),
            "override MAP timestamp differs from image"
        );
        Ok(Self { symbols })
    }
    fn symbol(&self, name: &str) -> Result<&MapSymbol> {
        let entries = self
            .symbols
            .get(name)
            .with_context(|| format!("override MAP lacks symbol {name}"))?;
        ensure!(entries.len() == 1, "ambiguous override MAP symbol {name}");
        Ok(&entries[0])
    }
}
/// Resolve public MAP addresses only after checking its identity against the PE.
pub fn map_addresses(image: &Path, map: &Path) -> Result<HashMap<String, Vec<u32>>> {
    let pe = Pe::parse(fs::read(image)?)?;
    let map = LinkMap::parse(&fs::read_to_string(map)?, &pe)?;
    Ok(map
        .symbols
        .into_iter()
        .map(|(name, values)| (name, values.into_iter().map(|v| v.address).collect()))
        .collect())
}
fn validate_override(
    pe: &Pe,
    map: &LinkMap,
    section: &Section,
    symbol: &Symbol,
    end: usize,
) -> Result<u32> {
    let start = map.symbol(&symbol.name)?.address;
    let begin = usize::try_from(symbol.value)?;
    ensure!(end > begin, "empty override function {}", symbol.name);
    let mut fragment = section.clone();
    fragment.data = bytes(&section.data, begin, end - begin)?.to_vec();
    fragment.relocs = section
        .relocs
        .iter()
        .filter(|r| begin <= r.offset && r.offset < end)
        .map(|r| {
            let mut r = r.clone();
            r.offset -= begin;
            r
        })
        .collect();
    let rva = start
        .checked_sub(pe.base)
        .context("override below image base")?;
    ensure!(
        pe.sections.iter().any(|s| s.flags & 0x20 != 0
            && rva >= s.rva
            && (rva as u64 + fragment.data.len() as u64)
                <= s.rva as u64 + s.raw_size.min(s.virtual_size) as u64),
        "override {} is outside initialized code",
        symbol.name
    );
    let body = pe.read(start, fragment.data.len())?;
    ensure!(
        matches(body, &fragment)? == [0],
        "compiled override body differs for {}",
        symbol.name
    );
    let imports = pe.imports()?;
    for reloc in &fragment.relocs {
        if reloc.kind != 20 {
            continue;
        }
        let displacement = i32::from_le_bytes(bytes(body, reloc.offset, 4)?.try_into()?) as i64;
        let addend = i32::from_le_bytes(bytes(&fragment.data, reloc.offset, 4)?.try_into()?) as i64;
        let actual = start as i64 + reloc.offset as i64 + 4 + displacement - addend;
        let target = &reloc.target;
        let expected = if target.section == section.index {
            start as i64 - symbol.value + target.value
        } else if target.section == 0 && target.name.starts_with('_') && target.name.contains('@') {
            let name = target.name[1..].rsplit_once('@').unwrap().0;
            *imports
                .get(name)
                .with_context(|| format!("override import {name} missing"))? as i64
        } else {
            map.symbol(&target.name)?.address as i64
        };
        ensure!(
            actual == expected,
            "compiled override REL32 differs for {} -> {}",
            symbol.name,
            target.name
        );
    }
    Ok(start)
}
fn override_targets(pe: &Pe, options: &Options) -> Result<HashMap<String, HashSet<u32>>> {
    if options.override_objects.is_empty() {
        return Ok(HashMap::new());
    }
    let registry: serde_json::Value = serde_json::from_slice(&fs::read(
        options
            .override_registry
            .as_ref()
            .context("override registry required")?,
    )?)?;
    let rows = registry
        .get("driver")
        .and_then(serde_json::Value::as_array)
        .context("override registry lacks driver array")?;
    let mut eligible = HashSet::new();
    for row in rows {
        if row.get("category").and_then(serde_json::Value::as_str) == Some("runtime") {
            let name = row
                .get("name")
                .and_then(serde_json::Value::as_str)
                .context("runtime registry entry lacks name")?;
            ensure!(
                eligible.insert(name.to_owned()),
                "duplicate runtime override registry name {name}"
            );
        }
    }
    let map = LinkMap::parse(
        &fs::read_to_string(
            options
                .override_map
                .as_ref()
                .context("override MAP required")?,
        )?,
        pe,
    )?;
    let mut selected = HashSet::new();
    let mut targets = HashMap::new();
    for path in &options.override_objects {
        let name = path
            .file_name()
            .and_then(|v| v.to_str())
            .context("override object filename is not UTF8")?;
        ensure!(
            selected.insert(name.to_ascii_lowercase()),
            "ambiguous selected override object name {name}"
        );
        let object = Coff::parse(&fs::read(path)?)
            .with_context(|| format!("override object {}", path.display()))?;
        let mut count = 0;
        for section in object.sections.iter().filter(|s| s.flags & 0x20 != 0) {
            let mut functions: Vec<_> = object
                .symbols
                .iter()
                .flatten()
                .filter(|s| s.section == section.index && s.kind == 0x20)
                .collect();
            functions.sort_by_key(|s| s.value);
            for (i, symbol) in functions.iter().enumerate() {
                if symbol.storage != 2 {
                    continue;
                }
                ensure!(
                    eligible.contains(&symbol.name),
                    "selected override {} is not an explicit runtime registry entry",
                    symbol.name
                );
                let owner = map.symbol(&symbol.name)?;
                ensure!(
                    owner.object.eq_ignore_ascii_case(name) && !owner.object.contains(':'),
                    "override MAP owner differs for {}: {}",
                    symbol.name,
                    owner.object
                );
                let end = if i + 1 < functions.len() {
                    usize::try_from(functions[i + 1].value)?
                } else {
                    section.data.len()
                };
                let address = validate_override(pe, &map, section, symbol, end)?;
                ensure!(
                    targets
                        .insert(symbol.name.clone(), HashSet::from([address]))
                        .is_none(),
                    "duplicate selected override symbol {}",
                    symbol.name
                );
                count += 1;
            }
        }
        ensure!(
            count > 0,
            "selected object {name} has no historical runtime overrides"
        );
    }
    Ok(targets)
}

fn prune(
    pe: &Pe,
    candidates: &mut Vec<Candidate>,
    overrides: &HashMap<String, HashSet<u32>>,
) -> Result<()> {
    let imports = pe.imports()?;
    loop {
        let mut known: HashMap<&str, HashSet<u32>> = overrides
            .iter()
            .map(|(name, addresses)| (name.as_str(), addresses.clone()))
            .collect();
        for c in candidates.iter() {
            for r in &c.records {
                known.entry(&r.name).or_default().insert(r.start);
            }
        }
        let mut keep = vec![];
        for c in candidates.iter() {
            let mut valid = true;
            for reloc in &c.section.relocs {
                if reloc.kind != 20 {
                    continue;
                }
                let location = c
                    .start
                    .checked_add(reloc.offset as u32)
                    .context("relocation address overflow")?;
                let displacement = i32::from_le_bytes(pe.read(location, 4)?.try_into()?);
                let actual = location as i64 + 4 + displacement as i64;
                let addend =
                    i32::from_le_bytes(bytes(&c.section.data, reloc.offset, 4)?.try_into()?) as i64;
                let expected = actual - addend;
                let name = &reloc.target.name;
                if reloc.target.section == c.section.index {
                    valid = expected == c.start as i64 + reloc.target.value
                        || reloc.target.storage == 2
                            && overrides.get(name).is_some_and(|targets| {
                                targets.iter().any(|&target| expected == target as i64)
                            });
                } else if reloc.target.section == 0 {
                    if name.starts_with('_') && name.contains('@') {
                        let name = name[1..].rsplit_once('@').unwrap().0;
                        valid = imports
                            .get(name)
                            .is_some_and(|&target| expected == target as i64);
                    } else if let Some(targets) = known.get(name.as_str()) {
                        valid = targets.iter().any(|&target| expected == target as i64);
                    }
                }
                if !valid {
                    break;
                }
            }
            keep.push(valid);
        }
        if keep.iter().all(|v| *v) {
            break;
        }
        let mut i = 0;
        candidates.retain(|_| {
            let valid = keep[i];
            i += 1;
            valid
        });
    }
    Ok(())
}
#[derive(Deserialize)]
struct Binding {
    record: Record,
    target_sha256: String,
    original_body_sha256: String,
    library_sha256: String,
    member_sha256: String,
    source_credit: u32,
    application_credit: u32,
}
fn digest(data: &[u8]) -> String {
    crate::fixture::hex(&Sha256::digest(data))
}
fn bindings(path: &Path, pe: &Pe, libraries: &[Library]) -> Result<Vec<Record>> {
    let value: serde_json::Value = serde_json::from_slice(&fs::read(path)?)?;
    let bindings: Vec<Binding> = if value.is_array() {
        serde_json::from_value(value)?
    } else {
        vec![serde_json::from_value(value)?]
    };
    let target = digest(&pe.data);
    let mut records = vec![];
    for binding in bindings {
        ensure!(
            binding.source_credit == 0 && binding.application_credit == 0,
            "runtime binding must award zero reconstruction credit"
        );
        ensure!(
            target == binding.target_sha256,
            "runtime binding target SHA256 differs"
        );
        let r = binding.record;
        let length = r
            .end
            .checked_sub(r.start)
            .filter(|n| *n > 0)
            .context("invalid runtime binding extent")?;
        ensure!(
            digest(pe.read(r.start, length as usize)?) == binding.original_body_sha256,
            "runtime binding original body SHA256 differs"
        );
        let libs: Vec<_> = libraries.iter().filter(|l| l.name == r.library).collect();
        ensure!(
            libs.len() == 1,
            "runtime binding library {} must identify one selected archive",
            r.library
        );
        let library = libs[0];
        ensure!(
            digest(&library.data) == binding.library_sha256,
            "runtime binding library SHA256 differs"
        );
        let members: Vec<_> = library
            .members
            .iter()
            .filter(|(name, _)| *name == r.object)
            .collect();
        ensure!(
            members.len() == 1,
            "runtime binding member {} must identify one archive member",
            r.object
        );
        ensure!(
            digest(&members[0].1) == binding.member_sha256,
            "runtime binding member SHA256 differs"
        );
        records.push(r);
    }
    Ok(records)
}
fn input_path(path: &Path, toolchain: &Path) -> Result<PathBuf> {
    if path.is_absolute() || path.exists() {
        return path
            .canonicalize()
            .with_context(|| format!("runtime input {}", path.display()));
    }
    toolchain
        .join("lib")
        .join(path)
        .canonicalize()
        .with_context(|| format!("runtime input {}", path.display()))
}
pub fn identify(options: Options) -> Result<()> {
    ensure!(
        !options.toolchain.is_empty()
            && !options.toolchain.contains(['/', '\\'])
            && options.toolchain != "."
            && options.toolchain != "..",
        "toolchain must be a directory name"
    );
    let repo = crate::toolchain::repository(options.repo.as_deref())?;
    let toolchain = repo.join("toolchains").join(&options.toolchain);
    ensure!(
        toolchain.is_dir(),
        "toolchain {} does not exist",
        options.toolchain
    );
    let mut library_paths = options.libraries.clone();
    let mut object_paths = options.objects.clone();
    if library_paths.is_empty() && object_paths.is_empty() {
        ensure!(
            options.toolchain == "1.0-nt",
            "no default runtime profile for {}; supply --library/--object (i386 COFF/ar only)",
            options.toolchain
        );
        library_paths.push(PathBuf::from("LIBC.LIB"));
        object_paths.push(PathBuf::from("SETARGV.OBJ"));
    }
    let pe = Pe::parse(
        fs::read(&options.image)
            .with_context(|| format!("read image {}", options.image.display()))?,
    )?;
    let section = pe
        .sections
        .iter()
        .find(|s| s.name == ".text")
        .context("image has no .text section")?;
    let code = bytes(&pe.data, section.offset, section.virtual_size)?;
    let mut libraries = vec![];
    let mut candidates = vec![];
    for path in library_paths {
        let path = input_path(&path, &toolchain)?;
        let name = path
            .file_name()
            .and_then(|s| s.to_str())
            .context("library filename is not UTF8")?
            .to_owned();
        let data = fs::read(&path)?;
        let members =
            archive(&data).with_context(|| format!("read runtime archive {}", path.display()))?;
        for (member, data) in &members {
            collect(&pe, &name, member, data, code, section.rva, &mut candidates)?;
        }
        libraries.push(Library {
            name,
            data,
            members,
        });
    }
    for path in object_paths {
        let path = input_path(&path, &toolchain)?;
        let name = path
            .file_name()
            .and_then(|s| s.to_str())
            .context("object filename is not UTF8")?
            .to_owned();
        collect(
            &pe,
            &name,
            &name,
            &fs::read(&path)?,
            code,
            section.rva,
            &mut candidates,
        )?;
    }
    let overrides = override_targets(&pe, &options)?;
    prune(&pe, &mut candidates, &overrides)?;
    let mut records: Vec<_> = candidates.into_iter().flat_map(|c| c.records).collect();
    for i in 0..records.len() {
        if records[i].name == "_strcat" {
            let shared = records
                .iter()
                .find(|r| r.name == "_strcpy" && r.start == records[i].start + 4)
                .context("matched strcat lacks its shared strcpy extent")?;
            records[i].shared = vec![[records[i].start + 8, shared.end]];
        }
    }
    if let Some(path) = options.bindings {
        records.extend(bindings(&path, &pe, &libraries)?);
    }
    records.sort_by_key(|r| r.start);
    fs::write(
        &options.output,
        serde_json::to_string_pretty(&records)? + "\n",
    )?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn symbol(name: &str, section: i16) -> Symbol {
        Symbol {
            name: name.into(),
            value: 0,
            section,
            kind: 0x20,
            storage: 2,
        }
    }
    fn record(name: &str, start: u32) -> Record {
        Record {
            name: name.into(),
            start,
            end: start + 5,
            object: "test.obj".into(),
            library: "test.lib".into(),
            shared: vec![],
        }
    }
    #[test]
    fn malformed_and_ambiguous_map_owners_are_rejected() {
        let pe = Pe {
            data: vec![0; 128],
            base: 0x1000,
            optional: 16,
            sections: vec![],
        };
        let header = "Timestamp is 00000000\nPreferred load address is 00001000\nAddress Publics by Value Rva+Base Lib:Object\n";
        assert!(LinkMap::parse(&format!("{header}0001:00000000 _x 00001000 f\n"), &pe).is_err());
        let row = "0001:00000000 _x 00001000 f runtime.obj\n";
        let map = LinkMap::parse(&format!("{header}{row}{row}"), &pe).unwrap();
        assert!(map.symbol("_x").is_err());
    }
    #[test]
    fn reviewed_override_preempts_only_a_named_public_definition() {
        let mut data = vec![0; 256];
        data[129..133].copy_from_slice(&43i32.to_le_bytes());
        let pe = Pe {
            data,
            base: 0x1000,
            optional: 0,
            sections: vec![PeSection {
                name: ".text".into(),
                rva: 0,
                raw_size: 128,
                offset: 128,
                virtual_size: 128,
                flags: 0x20,
            }],
        };
        let mut callee = symbol("__sopen", 1);
        callee.value = 0x20;
        let make = |callee| Candidate {
            section: Section {
                data: vec![0xe8, 0, 0, 0, 0],
                relocs: vec![Relocation {
                    offset: 1,
                    target: callee,
                    kind: 20,
                }],
                index: 1,
                flags: 0x20,
            },
            start: 0x1000,
            records: vec![record("__open", 0x1000)],
        };
        let overrides = HashMap::from([("__sopen".into(), HashSet::from([0x1030]))]);
        let mut candidates = vec![make(callee.clone())];
        prune(&pe, &mut candidates, &HashMap::new()).unwrap();
        assert!(candidates.is_empty());
        candidates = vec![make(callee.clone())];
        prune(&pe, &mut candidates, &overrides).unwrap();
        assert_eq!(candidates.len(), 1);
        callee.storage = 3;
        candidates = vec![make(callee)];
        prune(&pe, &mut candidates, &overrides).unwrap();
        assert!(candidates.is_empty());
    }
    #[test]
    fn masked_matching_preserves_overlaps_and_non_relocation_bytes() {
        let mut section = Section {
            data: b"abcde0000fghij".to_vec(),
            relocs: vec![Relocation {
                offset: 5,
                target: symbol("_callee", 0),
                kind: 20,
            }],
            index: 1,
            flags: 0x20,
        };
        assert_eq!(matches(b"xabcdeABCDfghij", &section).unwrap(), [1]);
        assert!(matches(b"xabcdeABCDfghik", &section).unwrap().is_empty());
        section.relocs[0].kind = 99;
        assert!(matches(b"xabcdeABCDfghij", &section).unwrap().is_empty());
        section.data = b"aaaaa".to_vec();
        section.relocs.clear();
        assert_eq!(matches(b"aaaaaaa", &section).unwrap(), [0, 1, 2]);
    }
    #[test]
    fn rel32_validation_reaches_fixed_point() {
        let mut data = vec![0; 256];
        // Caller initially matches either callee candidate. The first callee
        // fails its Win32 import constraint, then the caller fails next round.
        data[129..133].copy_from_slice(&11i32.to_le_bytes());
        let pe = Pe {
            data,
            base: 0x1000,
            optional: 0,
            sections: vec![PeSection {
                name: ".text".into(),
                rva: 0,
                raw_size: 128,
                offset: 128,
                virtual_size: 128,
                flags: 0x20,
            }],
        };
        let candidate = |name, start, callee: Option<&str>| Candidate {
            section: Section {
                data: vec![0xe8, 0, 0, 0, 0],
                relocs: callee
                    .map(|name| Relocation {
                        offset: 1,
                        target: symbol(name, 0),
                        kind: 20,
                    })
                    .into_iter()
                    .collect(),
                index: 1,
                flags: 0x20,
            },
            start,
            records: vec![record(name, start)],
        };
        let mut candidates = vec![
            candidate("_caller", 0x1000, Some("_callee")),
            candidate("_callee", 0x1010, Some("_Missing@0")),
            candidate("_callee", 0x1020, None),
        ];
        prune(&pe, &mut candidates, &HashMap::new()).unwrap();
        assert_eq!(candidates.len(), 1);
        assert_eq!(candidates[0].start, 0x1020);
    }
}
