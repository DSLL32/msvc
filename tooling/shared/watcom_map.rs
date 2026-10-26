//! Shared Watcom 10 PE MAP coordinates. Callers supply sections from their
//! existing PE parser; group/public validation is identical for export and audit.
use anyhow::{Context, Result, ensure};
use regex::Regex;
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::BTreeMap;

pub struct Section<'a> {
    pub name: &'a str,
    pub start: u32,
    pub size: u32,
    pub virtual_size: u32,
    pub off: usize,
    pub flags: u32,
}
pub struct Image<'a> {
    pub data: &'a [u8],
    pub sections: Vec<Section<'a>>,
}
impl Image<'_> {
    fn file_backed_code(&self, start: u32, end: u32) -> bool {
        start < end
            && self.sections.iter().any(|s| {
                s.flags & 0x20 != 0
                    && s.off != 0
                    && s.start <= start
                    && u64::from(end) <= u64::from(s.start) + u64::from(s.virtual_size.min(s.size))
                    && (s.off as u64) + u64::from(end - s.start) <= self.data.len() as u64
            })
    }
}
fn hash(data: &[u8]) -> String {
    format!("{:x}", Sha256::digest(data))
}
#[derive(Clone)]
struct Group {
    name: String,
    segment: u16,
    offset: u32,
    size: u32,
}
#[derive(Clone)]
struct Segment {
    name: String,
    class: String,
    group: String,
    segment: u16,
    offset: u32,
    size: u32,
}
pub struct Publics {
    pub addresses: BTreeMap<String, u32>,
    pub absolutes: BTreeMap<String, u32>,
    pub provenance: Value,
}
fn add_anchor(anchors: &mut BTreeMap<u16, u32>, segment: u16, base: u32) -> Result<()> {
    ensure!(
        segment != 0,
        "Watcom absolute segment cannot anchor a PE region"
    );
    if let Some(old) = anchors.insert(segment, base) {
        ensure!(old == base, "ambiguous Watcom MAP segment/group anchor");
    }
    Ok(())
}
fn mapped(pe: &Image<'_>, address: u32) -> bool {
    pe.sections.iter().any(|s| {
        s.start <= address && u64::from(address) < u64::from(s.start) + u64::from(s.virtual_size)
    })
}
pub fn parse_text(text: &str, pe: &Image<'_>) -> Result<Publics> {
    ensure!(
        text.starts_with("WATCOM Linker Version 10."),
        "not a Watcom 10 linker MAP"
    );
    ensure!(
        text.lines().any(|v| v.trim() == "creating a PE executable"),
        "Watcom MAP is not for a PE executable"
    );
    let group_re =
        Regex::new(r"^([^\s]+)\s+([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})\s*$")?;
    let symbol_re = Regex::new(r"^([0-9a-fA-F]{4}):([0-9a-fA-F]{8})([+*]?)\s+(\S+)\s*$")?;
    let address_re = Regex::new(r"^([0-9a-fA-F]{4}):([0-9a-fA-F]{8})$")?;
    let hex32 = |s: &str| u32::from_str_radix(s, 16).map_err(anyhow::Error::from);
    let mut groups = BTreeMap::new();
    let mut segments = Vec::new();
    let mut raw_symbols = Vec::new();
    let mut mode = "";
    let mut columns = None;
    for raw in text.lines() {
        let line = raw.trim_end();
        if line.trim() == "|   Groups   |" {
            mode = "groups";
            continue;
        }
        if line.trim() == "|   Segments   |" {
            mode = "segments";
            continue;
        }
        if line.trim() == "|   Memory Map   |" {
            mode = "symbols";
            continue;
        }
        if mode == "symbols" && line.contains("|   ") {
            mode = "done";
        }
        match mode {
            "groups" => {
                if let Some(c) = group_re.captures(line) {
                    let g = Group {
                        name: c[1].into(),
                        segment: u16::from_str_radix(&c[2], 16)?,
                        offset: hex32(&c[3])?,
                        size: hex32(&c[4])?,
                    };
                    ensure!(
                        g.offset.checked_add(g.size).is_some(),
                        "Watcom MAP group range overflow"
                    );
                    ensure!(
                        groups.insert(g.name.clone(), g).is_none(),
                        "duplicate Watcom MAP group"
                    );
                }
            }
            "segments" => {
                if line.starts_with("Segment ") {
                    columns = Some((
                        line.find("Class").context("Watcom MAP Class column")?,
                        line.find("Group").context("Watcom MAP Group column")?,
                        line.find("Address").context("Watcom MAP Address column")?,
                        line.find("Size").context("Watcom MAP Size column")?,
                    ));
                    continue;
                }
                if let Some((c, g, a, n)) = columns
                    && let (Some(name), Some(class), Some(group), Some(address), Some(size)) = (
                        line.get(..c),
                        line.get(c..g),
                        line.get(g..a),
                        line.get(a..n),
                        line.get(n..),
                    )
                    && let Some(cap) = address_re.captures(address.trim())
                {
                    let v = Segment {
                        name: name.trim().into(),
                        class: class.trim().into(),
                        group: group.trim().into(),
                        segment: u16::from_str_radix(&cap[1], 16)?,
                        offset: hex32(&cap[2])?,
                        size: hex32(size.trim())?,
                    };
                    ensure!(
                        v.offset.checked_add(v.size).is_some(),
                        "Watcom MAP segment range overflow"
                    );
                    segments.push(v);
                }
            }
            "symbols" => {
                if let Some(c) = symbol_re.captures(line) {
                    raw_symbols.push((
                        u16::from_str_radix(&c[1], 16)?,
                        hex32(&c[2])?,
                        c[4].to_string(),
                    ));
                }
            }
            _ => {}
        }
    }
    ensure!(
        !segments.is_empty() && !raw_symbols.is_empty(),
        "Watcom MAP has no parsed segments/publics"
    );
    let mut anchors = BTreeMap::new();
    for group in groups.values() {
        for section in pe.sections.iter().filter(|s| s.name == group.name) {
            add_anchor(
                &mut anchors,
                group.segment,
                section
                    .start
                    .checked_sub(group.offset)
                    .context("Watcom MAP group base underflow")?,
            )?;
        }
    }
    for segment in &segments {
        if !segment.group.is_empty() {
            let group = groups
                .get(&segment.group)
                .context("Watcom MAP segment has unknown group")?;
            ensure!(
                group.segment == segment.segment
                    && group.offset <= segment.offset
                    && u64::from(segment.offset) + u64::from(segment.size)
                        <= u64::from(group.offset) + u64::from(group.size),
                "Watcom MAP segment/group address disagreement"
            );
        }
        for section in pe.sections.iter().filter(|s| s.name == segment.name) {
            add_anchor(
                &mut anchors,
                segment.segment,
                section
                    .start
                    .checked_sub(segment.offset)
                    .context("Watcom MAP segment base underflow")?,
            )?;
        }
    }
    let mut addresses = BTreeMap::new();
    let mut absolutes = BTreeMap::new();
    let mut boundaries = BTreeMap::new();
    let mut seen = BTreeMap::new();
    for (segment, offset, name) in raw_symbols {
        if let Some(old) = seen.insert(name.clone(), (segment, offset)) {
            ensure!(
                old == (segment, offset),
                "duplicate/ambiguous Watcom MAP public name: {name}"
            );
            continue;
        }
        if segment == 0 {
            absolutes.insert(name, offset);
            continue;
        }
        let base = *anchors
            .get(&segment)
            .context("Watcom MAP public has no verified PE segment/group anchor")?;
        let address = base
            .checked_add(offset)
            .context("Watcom MAP public address overflow")?;
        if !mapped(pe, address) {
            ensure!(
                segments.iter().any(|s| s.segment == segment
                    && s.offset == offset
                    && s.size == 0
                    && s.class == "DATA")
                    && pe
                        .sections
                        .iter()
                        .any(|s| u64::from(s.start) + u64::from(s.virtual_size)
                            == u64::from(address)),
                "Watcom MAP public outside actual PE mapped region: {name}"
            );
            // An empty DATA table can name its one-past end. Preserve that
            // boundary as metadata, never as a callable/function public.
            boundaries.insert(name, address);
            continue;
        }
        let owners = segments
            .iter()
            .filter(|s| {
                s.segment == segment
                    && s.offset <= offset
                    && u64::from(offset) < u64::from(s.offset) + u64::from(s.size)
            })
            .collect::<Vec<_>>();
        ensure!(
            !owners.is_empty()
                || segments
                    .iter()
                    .any(|s| s.segment == segment && s.size == 0 && s.offset == offset),
            "Watcom MAP public outside declared group/segment extent: {name}"
        );
        if owners.iter().any(|s| s.class == "CODE") {
            ensure!(
                address
                    .checked_add(1)
                    .is_some_and(|end| pe.file_backed_code(address, end)),
                "Watcom MAP code public is not actual file-backed code: {name}"
            );
        }
        addresses.insert(name, address);
    }
    let mut provenance = json!({"format":"watcom10-pe","map_sha256":hash(text.as_bytes()),"rebuilt_pe_sha256":hash(&pe.data),"segment_group_bases":anchors,"absolute_symbols":absolutes});
    if !boundaries.is_empty() {
        provenance["boundary_symbols"] = json!(boundaries);
    }
    Ok(Publics {
        provenance,
        addresses,
        absolutes,
    })
}
