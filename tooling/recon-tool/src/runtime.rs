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
    /// Optional compiled-image MAP qualification; never adds runtime candidates.
    #[arg(long, conflicts_with = "bindings")]
    pub map: Option<PathBuf>,
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
// Old SDK import archives also carry machine-neutral, code-free weak aliases.
// Qualify their complete supported COFF shape; never match them as executable code.
fn weak_import_alias(data: &[u8]) -> Result<bool> {
    if data.get(..2) != Some(&[0, 0]) || data.get(2..4) == Some(&[0xff, 0xff]) {
        return Ok(false);
    }
    ensure!(
        word(data, 2)? == 1 && word(data, 16)? == 0 && word(data, 18)? == 0,
        "unsupported machine-neutral COFF header"
    );
    ensure!(
        dword(data, 12)? == 3,
        "machine-neutral import alias must have three symbol records"
    );
    let section = bytes(data, 20, 40)?;
    ensure!(
        dword(section, 8)? == 0
            && dword(section, 12)? == 0
            && dword(section, 16)? == 0
            && dword(section, 20)? == 0
            && dword(section, 24)? == 0
            && dword(section, 28)? == 0
            && word(section, 32)? == 0
            && word(section, 34)? == 0,
        "machine-neutral import alias contains section contents/relocations"
    );
    let symbol_offset = dword(data, 8)? as usize;
    ensure!(
        symbol_offset == 60,
        "machine-neutral import alias has unexpected data before symbols"
    );
    let mut host = data.to_vec();
    host[..2].copy_from_slice(&0x14cu16.to_le_bytes());
    let object = Coff::parse(&host)?;
    let public = object.symbols[0].as_ref().context("alias public missing")?;
    let weak = object.symbols[1].as_ref().context("weak alias missing")?;
    ensure!(
        public.value == 0
            && public.section == 0
            && public.kind == 0
            && public.storage == 2
            && !public.name.is_empty(),
        "invalid import alias fallback public"
    );
    ensure!(
        weak.value == 0
            && weak.section == 0
            && weak.kind == 0
            && weak.storage == 105
            && !weak.name.is_empty()
            && weak.name != public.name
            && object.symbols[2].is_none(),
        "invalid weak import alias symbol"
    );
    ensure!(
        bytes(data, symbol_offset + 17, 1)? == [0] && bytes(data, symbol_offset + 35, 1)? == [1],
        "invalid weak alias auxiliary count"
    );
    let aux = bytes(data, symbol_offset + 36, 18)?;
    ensure!(
        dword(aux, 0)? == 0 && dword(aux, 4)? == 3 && aux[8..].iter().all(|b| *b == 0),
        "unsupported weak import alias fallback/search mode"
    );
    let strings = symbol_offset + 54;
    ensure!(
        strings.checked_add(dword(data, strings)? as usize) == Some(data.len()),
        "weak import alias string table size differs"
    );
    Ok(true)
}

// Microsoft short import objects describe linker imports, never CRT bodies.
fn short_import(data: &[u8]) -> Result<bool> {
    if data.get(..4) != Some(&[0, 0, 0xff, 0xff]) {
        return Ok(false);
    }
    ensure!(
        word(data, 4)? == 0 && word(data, 6)? == 0x14c,
        "unsupported short import version/machine"
    );
    let size = dword(data, 12)? as usize;
    ensure!(
        20usize.checked_add(size) == Some(data.len()),
        "short import size differs from member"
    );
    let flags = word(data, 18)?;
    let kind = flags & 3;
    let naming = (flags >> 2) & 7;
    ensure!(
        flags >> 5 == 0 && kind <= 2 && naming <= 4,
        "unsupported short import type/name mode"
    );
    let strings = &data[20..];
    let names: Vec<_> = strings.split(|b| *b == 0).collect();
    let count = if naming == 4 { 3 } else { 2 };
    ensure!(
        names.len() == count + 1
            && names.last() == Some(&&[][..])
            && names[..count].iter().all(|v| !v.is_empty()),
        "invalid short import names"
    );
    Ok(true)
}

// Multiple COFF names can denote the same entry. They all own the same
// proven interval through the next distinct symbol, not a zero-byte body.
fn distinct_function_end<'a>(
    symbols: impl Iterator<Item = &'a Symbol>,
    begin: i64,
    section_len: usize,
) -> Result<usize> {
    symbols
        .filter(|symbol| symbol.value > begin)
        .map(|symbol| symbol.value)
        .min()
        .map(usize::try_from)
        .transpose()
        .map(|end| end.unwrap_or(section_len))
        .map_err(Into::into)
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
    if weak_import_alias(data)
        .with_context(|| format!("read {library} weak import alias {member}"))?
    {
        return Ok(());
    }
    if short_import(data).with_context(|| format!("read {library} import member {member}"))? {
        return Ok(());
    }
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
            for aliases in funcs.chunk_by(|a, b| a.value == b.value) {
                let sym = &aliases[0];
                let begin = usize::try_from(sym.value)?;
                let end = distinct_function_end(funcs.iter(), sym.value, text.data.len())?;
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
                // Preserve the same-value COFF alias witness in each fragment.
                // Equal code bytes from a different member are not aliases.
                let symbols = aliases
                    .iter()
                    .cloned()
                    .map(|mut sym| {
                        sym.value = 0;
                        sym
                    })
                    .collect();
                let found = matches(code, &fragment)?;
                pieces.push((fragment, symbols, found));
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
                for sym in &symbols {
                    let end = distinct_function_end(symbols.iter(), sym.value, fragment.data.len())?
                        as i64;
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
    fn symbol_owned(&self, name: &str, object: &str) -> Result<&MapSymbol> {
        let entries = self
            .symbols
            .get(name)
            .with_context(|| format!("override MAP lacks symbol {name}"))?;
        let mut matches = entries.iter().filter(|entry| {
            !entry.object.contains(':') && entry.object.eq_ignore_ascii_case(object)
        });
        let owner = matches
            .next()
            .with_context(|| format!("override MAP lacks {name} in selected object {object}"))?;
        ensure!(
            matches.next().is_none(),
            "ambiguous override MAP symbol {name} in {object}"
        );
        Ok(owner)
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
/// Restrict already matched records; MAP names never seed a candidate or body.
fn qualify_candidates(map: &LinkMap, candidates: &mut Vec<Candidate>) {
    for candidate in candidates.iter_mut() {
        let qualified: HashSet<_> = candidate
            .records
            .iter()
            .filter(|record| {
                map.symbols.get(&record.name).is_some_and(|entries| {
                    entries.iter().any(|symbol| symbol.address == record.start)
                })
            })
            .map(|record| (record.start, record.end))
            .collect();
        candidate
            .records
            .retain(|record| match map.symbols.get(&record.name) {
                Some(entries) => entries.iter().any(|symbol| symbol.address == record.start),
                // Same-entry, same-end records in this candidate originate from
                // same-value symbols in this exact COFF member/fragment.
                None => qualified.contains(&(record.start, record.end)),
            });
    }
    candidates.retain(|candidate| !candidate.records.is_empty());
}

/// Validate explicit rebuilt ownership through the existing PE reader only.
pub(crate) fn validate_code_ranges(image: &Path, ranges: &[(u32, u32)]) -> Result<()> {
    let pe = Pe::parse(fs::read(image)?)?;
    for &(start, end) in ranges {
        ensure!(start < end, "invalid rebuilt code interval");
        let rva = start
            .checked_sub(pe.base)
            .context("rebuilt code below PE base")?;
        ensure!(
            pe.sections
                .iter()
                .filter(|section| {
                    section.flags & 0x20000020 != 0
                        && rva >= section.rva
                        && u64::from(rva) + u64::from(end - start)
                            <= u64::from(section.rva) + section.raw_size as u64
                })
                .count()
                == 1,
            "rebuilt code interval is not uniquely executable/file-backed: {start:x}..{end:x}"
        );
        pe.read(start, (end - start) as usize)?;
    }
    Ok(())
}

// The exporter shares graph-audit's Watcom coordinates and checks. Runtime
// body identification/override qualification below remains MSVC/COFF-only.
#[path = "../../shared/watcom_map.rs"]
#[allow(dead_code)] // Export consumes addresses; graph-audit also consumes provenance.
mod watcom_map;
fn watcom_addresses(text: &str, pe: &Pe) -> Result<HashMap<String, Vec<u32>>> {
    let sections = pe
        .sections
        .iter()
        .map(|s| {
            Ok(watcom_map::Section {
                name: &s.name,
                start: pe
                    .base
                    .checked_add(s.rva)
                    .context("Watcom PE section address overflow")?,
                size: u32::try_from(s.raw_size)?,
                virtual_size: u32::try_from(if s.virtual_size == 0 {
                    s.raw_size
                } else {
                    s.virtual_size
                })?,
                off: s.offset,
                flags: s.flags,
            })
        })
        .collect::<Result<Vec<_>>>()?;
    let image = watcom_map::Image {
        data: &pe.data,
        sections,
    };
    Ok(watcom_map::parse_text(text, &image)?
        .addresses
        .into_iter()
        .map(|(name, address)| (name, vec![address]))
        .collect())
}
/// Resolve MAP publics against the PE using the detected MSVC or Watcom format.
pub fn map_addresses(image: &Path, map: &Path) -> Result<HashMap<String, Vec<u32>>> {
    let pe = Pe::parse(fs::read(image)?)?;
    let text = fs::read_to_string(map)?;
    public_addresses(&text, &pe)
}
fn public_addresses(text: &str, pe: &Pe) -> Result<HashMap<String, Vec<u32>>> {
    if text.starts_with("WATCOM Linker Version ") {
        return watcom_addresses(text, pe);
    }
    let map = LinkMap::parse(text, pe)?;
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
    start: u32,
) -> Result<u32> {
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
fn override_names(registry: &serde_json::Value) -> Result<HashSet<String>> {
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
    if let Some(rows) = registry.get("function_registry") {
        for row in rows
            .as_array()
            .context("override function_registry must be an array")?
        {
            if row.get("definition").and_then(serde_json::Value::as_u64) != Some(3) {
                continue;
            }
            let name = row
                .get("original_symbol")
                .and_then(serde_json::Value::as_str)
                .or_else(|| row.get("name").and_then(serde_json::Value::as_str))
                .context("classified CRT anchor lacks symbol")?;
            ensure!(!name.is_empty(), "empty classified CRT anchor");
            eligible.insert(name.to_owned());
        }
    }
    Ok(eligible)
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
    let eligible = override_names(&registry)?;
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
            for symbol in &functions {
                if symbol.storage != 2 {
                    continue;
                }
                ensure!(
                    eligible.contains(&symbol.name),
                    "selected override {} is not an explicit runtime registry entry",
                    symbol.name
                );
                let owner = map.symbol_owned(&symbol.name, name)?;
                ensure!(
                    owner.object.eq_ignore_ascii_case(name) && !owner.object.contains(':'),
                    "override MAP owner differs for {}: {}",
                    symbol.name,
                    owner.object
                );
                let end = distinct_function_end(
                    functions.iter().copied(),
                    symbol.value,
                    section.data.len(),
                )?;
                let address = validate_override(pe, &map, section, symbol, end, owner.address)?;
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
    prune_with_external_anchors(pe, candidates, overrides, &HashMap::new())
}

fn prune_with_external_anchors(
    pe: &Pe,
    candidates: &mut Vec<Candidate>,
    overrides: &HashMap<String, HashSet<u32>>,
    external_anchors: &HashMap<String, HashSet<u32>>,
) -> Result<()> {
    let imports = pe.imports()?;
    loop {
        let mut known: HashMap<&str, HashSet<u32>> = external_anchors
            .iter()
            .map(|(name, addresses)| (name.as_str(), addresses.clone()))
            .collect();
        for (name, addresses) in overrides {
            known.entry(name).or_default().extend(addresses);
        }
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
    if let Some(path) = &options.map {
        let map =
            LinkMap::parse(&fs::read_to_string(path)?, &pe).context("runtime MAP qualification")?;
        // A removed identity must not become a permissive unknown on the
        // second fixed point. MAP publics anchor external identities only;
        // they never relax same-member relocations or add matched bodies.
        let mut anchors: HashMap<String, HashSet<u32>> = candidates
            .iter()
            .flat_map(|candidate| &candidate.records)
            .map(|record| (record.name.clone(), HashSet::new()))
            .collect();
        for (name, symbols) in &map.symbols {
            anchors
                .entry(name.clone())
                .or_default()
                .extend(symbols.iter().map(|s| s.address));
        }
        qualify_candidates(&map, &mut candidates);
        prune_with_external_anchors(&pe, &mut candidates, &overrides, &anchors)?;
    }
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
    fn whole_and_fragment_matches_keep_every_same_entry_alias() {
        let body = [
            0x31, 0xc0, 0x90, 0x90, 0x90, 0x90, 0x90, 0xc3, 0x33, 0xc0, 0x40, 0x40, 0x40, 0x40,
            0x40, 0xc3,
        ];
        let symbol_offset = 60 + body.len();
        let mut object = vec![0u8; symbol_offset + 3 * 18 + 4];
        object[0..2].copy_from_slice(&0x14cu16.to_le_bytes());
        object[2..4].copy_from_slice(&1u16.to_le_bytes());
        object[8..12].copy_from_slice(&(symbol_offset as u32).to_le_bytes());
        object[12..16].copy_from_slice(&3u32.to_le_bytes());
        object[20..25].copy_from_slice(b".text");
        object[36..40].copy_from_slice(&(body.len() as u32).to_le_bytes());
        object[40..44].copy_from_slice(&60u32.to_le_bytes());
        object[56..60].copy_from_slice(&0x60000020u32.to_le_bytes());
        object[60..symbol_offset].copy_from_slice(&body);
        for (i, (name, value)) in [("_first", 0u32), ("_alias", 0), ("_next", 8)]
            .iter()
            .enumerate()
        {
            let p = symbol_offset + i * 18;
            object[p..p + name.len()].copy_from_slice(name.as_bytes());
            object[p + 8..p + 12].copy_from_slice(&value.to_le_bytes());
            object[p + 12..p + 14].copy_from_slice(&1i16.to_le_bytes());
            object[p + 14..p + 16].copy_from_slice(&0x20u16.to_le_bytes());
            object[p + 16] = 2;
        }
        object[symbol_offset + 54..].copy_from_slice(&4u32.to_le_bytes());
        let pe = Pe {
            data: vec![],
            base: 0x1000,
            optional: 0,
            sections: vec![],
        };
        for change_neighbor in [false, true] {
            let mut code = body;
            if change_neighbor {
                code[8] = 0x32;
            }
            let mut candidates = Vec::new();
            collect(
                &pe,
                "test.lib",
                "aliases.obj",
                &object,
                &code,
                0,
                &mut candidates,
            )
            .unwrap();
            let records: Vec<_> = candidates.iter().flat_map(|c| &c.records).collect();
            for name in ["_first", "_alias"] {
                let matching: Vec<_> = records.iter().filter(|r| r.name == name).collect();
                assert_eq!(matching.len(), 1);
                assert_eq!((matching[0].start, matching[0].end), (0x1000, 0x1008));
            }
            assert!(candidates.iter().any(|candidate| {
                candidate.records.iter().any(|r| r.name == "_first")
                    && candidate.records.iter().any(|r| r.name == "_alias")
            }));
            assert!(records.iter().all(|r| r.start < r.end));
            assert_eq!(records.iter().any(|r| r.name == "_next"), !change_neighbor);
        }
    }
    #[test]
    fn removed_map_identity_cannot_become_an_unknown_callee() {
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
        let make = || Candidate {
            section: Section {
                data: vec![0xe8, 0, 0, 0, 0],
                relocs: vec![Relocation {
                    offset: 1,
                    target: symbol("_false_callee", 0),
                    kind: 20,
                }],
                index: 1,
                flags: 0x20,
            },
            start: 0x1000,
            records: vec![record("_real_caller", 0x1000)],
        };
        // Losing all candidates for a formerly known name must remain a
        // rejection on later fixed-point iterations, not become unknown.
        let denied = HashMap::from([("_false_callee".into(), HashSet::new())]);
        let mut candidates = vec![make()];
        prune_with_external_anchors(&pe, &mut candidates, &HashMap::new(), &denied).unwrap();
        assert!(candidates.is_empty());
        // A bound MAP public is an identity witness without a body record.
        let bound = HashMap::from([("_false_callee".into(), HashSet::from([0x1030]))]);
        candidates = vec![make()];
        prune_with_external_anchors(&pe, &mut candidates, &HashMap::new(), &bound).unwrap();
        assert_eq!(candidates.len(), 1);
        assert_eq!(candidates[0].records.len(), 1);
        let wrong = HashMap::from([("_false_callee".into(), HashSet::from([0x1040]))]);
        candidates = vec![make()];
        prune_with_external_anchors(&pe, &mut candidates, &HashMap::new(), &wrong).unwrap();
        assert!(candidates.is_empty());
    }

    #[test]
    fn map_qualification_preserves_only_witnessed_aliases() {
        let make = |names: &[&str]| Candidate {
            section: Section {
                data: vec![0xc3; 5],
                relocs: vec![],
                index: 1,
                flags: 0x20,
            },
            start: 0x1000,
            records: names.iter().map(|name| record(name, 0x1000)).collect(),
        };
        let map = LinkMap {
            symbols: HashMap::from([
                (
                    "_real".into(),
                    vec![MapSymbol {
                        address: 0x1000,
                        object: "real.obj".into(),
                    }],
                ),
                (
                    "_elsewhere".into(),
                    vec![MapSymbol {
                        address: 0x2000,
                        object: "other.obj".into(),
                    }],
                ),
            ]),
        };
        let mut candidates = vec![
            make(&["_real", "_same_value_alias", "_elsewhere"]),
            // Identical bytes from another member do not prove an alias.
            make(&["_unrelated"]),
        ];
        qualify_candidates(&map, &mut candidates);
        assert_eq!(candidates.len(), 1);
        let names: Vec<_> = candidates[0]
            .records
            .iter()
            .map(|r| r.name.as_str())
            .collect();
        assert_eq!(names, ["_real", "_same_value_alias"]);
    }

    #[test]
    fn same_entry_aliases_share_next_distinct_symbol_boundary() {
        let mut first = symbol("__alloca_probe", 1);
        first.value = 0;
        let mut alias = first.clone();
        alias.name = "__chkstk".into();
        let mut next = first.clone();
        next.name = "next".into();
        next.value = 45;
        let symbols = [first, alias, next];
        assert_eq!(distinct_function_end(symbols.iter(), 0, 80).unwrap(), 45);
        assert_eq!(distinct_function_end(symbols.iter(), 45, 80).unwrap(), 80);
        assert_eq!(
            distinct_function_end(symbols[..2].iter(), 0, 45).unwrap(),
            45
        );
    }
    #[test]
    fn code_free_machine_neutral_aliases_are_not_runtime_bodies() {
        let mut member = vec![0u8; 60 + 54 + 4 + 4];
        member[2..4].copy_from_slice(&1u16.to_le_bytes());
        member[8..12].copy_from_slice(&60u32.to_le_bytes());
        member[12..16].copy_from_slice(&3u32.to_le_bytes());
        member[60..62].copy_from_slice(b"_a");
        member[76] = 2;
        member[78..80].copy_from_slice(b"_b");
        member[94] = 105;
        member[95] = 1;
        member[100..104].copy_from_slice(&3u32.to_le_bytes());
        member[114..118].copy_from_slice(&8u32.to_le_bytes());
        assert!(weak_import_alias(&member).unwrap());
        let mut bad = member.clone();
        bad[36] = 1;
        assert!(weak_import_alias(&bad).is_err());
        let mut bad = member.clone();
        bad[96] = 2;
        assert!(weak_import_alias(&bad).is_err());
        let mut bad = member.clone();
        bad[94] = 2;
        assert!(weak_import_alias(&bad).is_err());
    }
    #[test]
    fn short_import_members_have_no_reconstructable_code() {
        let mut member = vec![0u8; 20];
        member[2..4].copy_from_slice(&0xffffu16.to_le_bytes());
        member[6..8].copy_from_slice(&0x14cu16.to_le_bytes());
        member[18..20].copy_from_slice(&4u16.to_le_bytes());
        member.extend_from_slice(b"_puts\0MSVCRT.dll\0");
        let size = (member.len() - 20) as u32;
        member[12..16].copy_from_slice(&size.to_le_bytes());
        assert!(short_import(&member).unwrap());
        let mut bad = member.clone();
        bad[18] = 3;
        assert!(short_import(&bad).is_err());
        let mut bad = member.clone();
        bad.pop();
        assert!(short_import(&bad).is_err());
        let mut bad = member.clone();
        bad[6] = 0;
        assert!(short_import(&bad).is_err());
        assert!(!short_import(&[0x4c, 1, 0, 0]).unwrap());
    }
    #[test]
    fn exporter_watcom_coordinates_share_audit_validation() {
        let mut pe = Pe {
            data: vec![0; 512],
            base: 0x400000,
            optional: 16,
            sections: vec![PeSection {
                name: "BEGTEXT".into(),
                rva: 0x1000,
                raw_size: 128,
                offset: 128,
                virtual_size: 128,
                flags: 0x60000020,
            }],
        };
        let mut map = String::from(
            "WATCOM Linker Version 10.0\ncreating a PE executable\n|   Segments   |\nSegment                Class          Group          Address         Size\n",
        );
        map.push_str(&format!(
            "{:<23}{:<15}{:<15}{:<16}{}\n",
            "BEGTEXT", "CODE", "", "0001:00000000", "00000080"
        ));
        map.push_str("|   Memory Map   |\n0001:00000010+ directive_\n0001:00000010* Alias_\n0000:00001234 __DOSseg__\n");
        let addresses = public_addresses(&map, &pe).unwrap();
        assert_eq!(addresses["directive_"], [0x401010]);
        assert_eq!(addresses["Alias_"], [0x401010]);
        assert!(!addresses.contains_key("__DOSseg__"));
        for invalid in [
            map.clone() + "0001:00000011 directive_\n",
            map.replace("0001:00000010+", "0002:00000010+"),
            map.replace("0001:00000010+", "0001:00000080+"),
            map.replace("Version 10.0", "Version 11.0"),
        ] {
            assert!(public_addresses(&invalid, &pe).is_err());
        }
        pe.sections[0].name = "wrong-anchor".into();
        assert!(public_addresses(&map, &pe).is_err());
        pe.sections[0].name = "BEGTEXT".into();
        pe.sections[0].flags = 0xc0000040;
        assert!(public_addresses(&map, &pe).is_err());
        pe.sections[0].flags = 0x60000020;
        pe.base = u32::MAX;
        assert!(public_addresses(&map, &pe).is_err());
        // MSVC timestamp/base qualification is unchanged by format detection.
        pe.base = 0x400000;
        let msvc = "Timestamp is 00000000\nPreferred load address is 00400000\nAddress Publics by Value Rva+Base Lib:Object\n0001:00000010 _x 00401010 f x.obj\n";
        assert_eq!(public_addresses(msvc, &pe).unwrap()["_x"], [0x401010]);
        assert!(public_addresses(&msvc.replace("00400000", "00500000"), &pe).is_err());
        assert!(
            public_addresses(
                &msvc.replace("Timestamp is 00000000", "Timestamp is 00000001"),
                &pe
            )
            .is_err()
        );
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

#[cfg(test)]
mod override_identity_tests {
    use super::*;
    #[test]
    fn selected_object_disambiguates_source_from_library_only() {
        let map = LinkMap {
            symbols: HashMap::from([(
                "_flsall".into(),
                vec![
                    MapSymbol {
                        address: 0x1000,
                        object: "flush-all.c.obj".into(),
                    },
                    MapSymbol {
                        address: 0x2000,
                        object: "LIBC:FFLUSH.OBJ".into(),
                    },
                ],
            )]),
        };
        assert!(map.symbol("_flsall").is_err());
        assert_eq!(
            map.symbol_owned("_flsall", "FLUSH-ALL.C.OBJ")
                .unwrap()
                .address,
            0x1000
        );
        assert!(map.symbol_owned("_flsall", "other.obj").is_err());
        let mut bad = map;
        bad.symbols.get_mut("_flsall").unwrap().push(MapSymbol {
            address: 0x3000,
            object: "flush-all.c.obj".into(),
        });
        assert!(bad.symbol_owned("_flsall", "flush-all.c.obj").is_err());
    }
    #[test]
    fn pending_classified_crt_is_identity_only_not_app_eligibility() {
        let registry = serde_json::json!({"driver":[{"name":"_source","category":"runtime"}],"function_registry":[{"name":"__commit","definition":3,"implementation":0,"code":[]},{"name":"pending-app","definition":4,"implementation":0},{"name":"import-name","definition":1},{"name":"logical-crt","original_symbol":"_actual_crt","definition":3,"implementation":0}]});
        let names = override_names(&registry).unwrap();
        assert_eq!(
            names,
            HashSet::from(["_source".into(), "__commit".into(), "_actual_crt".into()])
        );
        assert!(
            registry["function_registry"][0]["code"]
                .as_array()
                .unwrap()
                .is_empty()
        );
    }
}
