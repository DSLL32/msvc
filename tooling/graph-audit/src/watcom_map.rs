//! Watcom 10 PE MAP parsing. The original MAP is immutable; group-relative
//! offsets are anchored to actual PE sections, not rewritten as MSVC text.
use crate::pe::Pe;
use anyhow::{Context, Result, ensure};
use regex::Regex;
use serde_json::{Value, json};
use std::{collections::BTreeMap, fs, path::Path};
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
fn mapped(pe: &Pe, address: u32) -> bool {
    pe.sections.iter().any(|s| {
        s.start <= address && u64::from(address) < u64::from(s.start) + u64::from(s.virtual_size)
    })
}
pub fn parse(path: &Path, pe: &Pe) -> Result<Publics> {
    let text = fs::read_to_string(path).context("read Watcom MAP")?;
    parse_text(&text, pe)
}
fn parse_text(text: &str, pe: &Pe) -> Result<Publics> {
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
        ensure!(
            mapped(pe, address),
            "Watcom MAP public outside actual PE mapped region: {name}"
        );
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
    Ok(Publics {
        provenance: json!({"format":"watcom10-pe","map_sha256":crate::index::hash(text.as_bytes()),"rebuilt_pe_sha256":crate::index::hash(&pe.data),"segment_group_bases":anchors,"absolute_symbols":absolutes}),
        addresses,
        absolutes,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> (Pe, String) {
        let mut b = vec![0u8; 0x700];
        b[..2].copy_from_slice(b"MZ");
        b[60..64].copy_from_slice(&0x80u32.to_le_bytes());
        b[0x80..0x84].copy_from_slice(b"PE\0\0");
        b[0x86..0x88].copy_from_slice(&3u16.to_le_bytes());
        b[0x94..0x96].copy_from_slice(&224u16.to_le_bytes());
        b[0x98..0x9a].copy_from_slice(&0x10bu16.to_le_bytes());
        b[0xb4..0xb8].copy_from_slice(&0x400000u32.to_le_bytes());
        for (n, name, rva, size, off, flags) in [
            (0, "BEGTEXT", 0x10000u32, 0x100u32, 0x400u32, 0x60000020u32),
            (1, "DGROUP", 0x20000, 0x40, 0x500, 0xc0000040),
            (2, ".bss", 0x30000, 0x40, 0, 0xc0000080),
        ] {
            let p = 0x178 + n * 40;
            b[p..p + name.len()].copy_from_slice(name.as_bytes());
            for (o, v) in [(12, rva), (16, size), (20, off), (36, flags)] {
                b[p + o..p + o + 4].copy_from_slice(&v.to_le_bytes());
            }
        }
        let mut map = String::from(
            "WATCOM Linker Version 10.0\ncreating a PE executable\n|   Groups   |\nDGROUP 0002:00000000 00010040\n|   Segments   |\nSegment                Class          Group          Address         Size\n",
        );
        for (name, class, group, address, size) in [
            ("BEGTEXT", "CODE", "", "0001:00000000", "00000040"),
            ("_DATA", "DATA", "DGROUP", "0002:00000000", "00000040"),
            ("_BSS", "BSS", "DGROUP", "0002:00010000", "00000040"),
        ] {
            map.push_str(&format!(
                "{name:<23}{class:<15}{group:<15}{address:<16}{size}\n"
            ));
        }
        map.push_str("|   Memory Map   |\n0001:00000010+ Foo_\n0001:00000010* Alias_\n0002:00010004  _bss_public\n0000:00001234  __DOSseg__\n");
        (Pe::parse(b).unwrap(), map)
    }
    #[test]
    fn actual_pe_group_coordinates_reach_bss_and_keep_aliases() {
        let (pe, map) = fixture();
        let p = parse_text(&map, &pe).unwrap();
        assert_eq!(p.addresses["Foo_"], 0x410010);
        assert_eq!(p.addresses["Alias_"], 0x410010);
        assert_eq!(p.addresses["_bss_public"], 0x430004);
        assert_eq!(p.absolutes["__DOSseg__"], 0x1234);
        assert!(!p.addresses.contains_key("__DOSseg__"));
        let map = map + "0001:00000010  Foo_\n";
        assert!(parse_text(&map, &pe).is_ok());
    }
    #[test]
    fn map_conflicts_unmapped_and_unowned_publics_reject() {
        let (pe, map) = fixture();
        for changed in [
            map.clone() + "0001:00000011 Foo_\n",
            map.replace("0002:00010004  _bss_public", "0002:00020004  _bss_public"),
            map.replace("0001:00000010+ Foo_", "0001:00000050+ Foo_"),
            map.replace(
                "DGROUP 0002:00000000 00010040",
                "DGROUP 0003:00000000 00010040",
            ),
            map.replace("_BSS                   BSS", "_BSS                   CODE"),
            map.replace("Version 10.0", "Version 11.0"),
        ] {
            assert!(parse_text(&changed, &pe).is_err());
        }
    }
}
