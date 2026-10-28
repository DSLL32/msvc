//! Detailed TLINK32 PE MAP coordinates. No symbol, function or library identity
//! is inferred beyond the exact displayed public spelling and declared ranges.
use anyhow::{Context, Result, ensure};
use regex::Regex;
use serde::Serialize;
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
#[derive(Clone, Serialize)]
struct Segment {
    segment: u16,
    offset: u32,
    size: u32,
    name: String,
    class: String,
}
#[derive(Clone, Serialize)]
pub struct Contribution {
    pub segment: u16,
    pub offset: u32,
    pub size: u32,
    pub name: String,
    pub class: String,
    pub group: String,
    pub module: String,
    pub start: u32,
    pub end: u32,
}
pub struct Publics {
    pub addresses: BTreeMap<String, u32>,
    // Runtime qualification consumes these; the graph adapter uses provenance.
    #[allow(dead_code)]
    pub contributions: Vec<Contribution>,
    pub provenance: Value,
}
fn hash(bytes: &[u8]) -> String {
    format!("{:x}", Sha256::digest(bytes))
}
fn hex(text: &str) -> Result<u32> {
    u32::from_str_radix(text, 16).context("TLINK32 MAP uint32 hexadecimal field")
}
fn within(offset: u32, size: u32, lo: u32, length: u32) -> bool {
    lo <= offset && u64::from(offset) + u64::from(size) <= u64::from(lo) + u64::from(length)
}
fn code(section: &Section<'_>) -> bool {
    section.flags & 0x20000020 != 0
}
fn backed(image: &Image<'_>, section: &Section<'_>, offset: u32, size: u32) -> bool {
    section.off != 0
        && section.flags & 0x80 == 0
        && within(offset, size, 0, section.size.min(section.virtual_size))
        && section.off as u64 + u64::from(offset) + u64::from(size) <= image.data.len() as u64
}

/// Supports the actual BC4 TLINK32 `/m /s /Tpe` summary, detailed contributions,
/// and both public tables. Compact/16-bit maps and unknown anchors fail closed.
pub fn parse_text(text: &str, image: &Image<'_>) -> Result<Publics> {
    let summary_re = Regex::new(
        r"^\s*([0-9A-Fa-f]{4}):([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{1,9})H\s+(\S+)\s+(\S+)\s*$",
    )?;
    let detail_re = Regex::new(
        r"^\s*([0-9A-Fa-f]{4}):([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})\s+C=(\S+)\s+S=(\S+)\s+G=(\S+)\s+M=(.+?)\s+ACBP=([0-9A-Fa-f]{2})\s*$",
    )?;
    let public_re = Regex::new(r"^\s*([0-9A-Fa-f]{4}):([0-9A-Fa-f]{8})( {6}|  Idle) (.+?)\s*$")?;
    let mut mode = 0;
    let mut summaries = Vec::<Segment>::new();
    let mut contributions = Vec::<Contribution>::new();
    let mut tables: [BTreeMap<String, (u16, u32)>; 2] = [BTreeMap::new(), BTreeMap::new()];
    for raw in text.lines() {
        let line = raw.trim_end();
        if line.trim().is_empty() {
            continue;
        }
        let words: Vec<_> = line.split_whitespace().collect();
        if words == ["Start", "Length", "Name", "Class"] {
            ensure!(
                mode == 0,
                "duplicate/out-of-order TLINK32 MAP segment summary"
            );
            mode = 1;
            continue;
        }
        if line.trim() == "Detailed map of segments" {
            ensure!(
                mode == 1 && !summaries.is_empty(),
                "TLINK32 MAP detailed segments need a preceding summary"
            );
            mode = 2;
            continue;
        }
        if words == ["Address", "Publics", "by", "Name"] {
            ensure!(
                mode == 2 && !contributions.is_empty(),
                "TLINK32 MAP publics need detailed segments (/m /s)"
            );
            mode = 3;
            continue;
        }
        if words == ["Address", "Publics", "by", "Value"] {
            ensure!(
                mode == 3 && !tables[0].is_empty(),
                "TLINK32 MAP value publics need name publics first"
            );
            mode = 4;
            continue;
        }
        match mode {
            1 => {
                let c = summary_re
                    .captures(line)
                    .context("unsupported/malformed TLINK32 PE segment summary row")?;
                let segment = Segment {
                    segment: u16::from_str_radix(&c[1], 16)?,
                    offset: hex(&c[2])?,
                    size: hex(&c[3])?,
                    name: c[4].into(),
                    class: c[5].into(),
                };
                ensure!(
                    segment.segment != 0 && segment.offset.checked_add(segment.size).is_some(),
                    "TLINK32 MAP absolute/overflowing segment is unsupported"
                );
                ensure!(
                    !summaries
                        .iter()
                        .any(|s| s.segment == segment.segment && s.name == segment.name),
                    "duplicate TLINK32 MAP summary segment"
                );
                summaries.push(segment);
            }
            2 => {
                let c = detail_re
                    .captures(line)
                    .context("unsupported/malformed TLINK32 detailed contribution row")?;
                contributions.push(Contribution {
                    segment: u16::from_str_radix(&c[1], 16)?,
                    offset: hex(&c[2])?,
                    size: hex(&c[3])?,
                    class: c[4].into(),
                    name: c[5].into(),
                    group: c[6].into(),
                    module: c[7].into(),
                    start: 0,
                    end: 0,
                });
            }
            3 | 4 => {
                let c = public_re.captures(line).context("unsupported/malformed TLINK32 public row (requires 32-bit coordinates and blank/Idle status)")?;
                let coordinate = (u16::from_str_radix(&c[1], 16)?, hex(&c[2])?);
                ensure!(
                    coordinate.0 != 0,
                    "TLINK32 MAP absolute publics are unsupported"
                );
                if let Some(previous) = tables[mode - 3].insert(c[4].into(), coordinate) {
                    ensure!(
                        previous == coordinate,
                        "conflicting TLINK32 MAP public spelling: {}",
                        &c[4]
                    );
                }
            }
            _ => anyhow::bail!("not a supported detailed TLINK32 PE MAP"),
        }
    }
    ensure!(
        mode == 4 && !tables[1].is_empty() && tables[0] == tables[1],
        "TLINK32 MAP name/value public tables are incomplete or disagree"
    );
    let mut anchors = BTreeMap::<u16, usize>::new();
    for summary in &summaries {
        if summary.offset != 0 || summary.size == 0 {
            continue;
        }
        let sections: Vec<_> = image
            .sections
            .iter()
            .enumerate()
            .filter(|(_, s)| s.name == summary.class)
            .collect();
        if sections.is_empty() {
            continue;
        }
        ensure!(
            sections.len() == 1,
            "ambiguous TLINK32 MAP class/PE section anchor"
        );
        let (index, section) = sections[0];
        ensure!(
            (summary.class == "CODE" && code(section))
                || (summary.class == "DATA" && !code(section) && section.flags & 0xc0 != 0),
            "unsupported TLINK32 MAP class/PE section flags"
        );
        if let Some(old) = anchors.insert(summary.segment, index) {
            ensure!(
                old == index,
                "conflicting TLINK32 MAP logical segment anchor"
            );
        }
    }
    ensure!(
        !anchors.is_empty(),
        "TLINK32 MAP has no exact class/PE section anchors"
    );
    let mut anchored_sections = std::collections::BTreeSet::new();
    ensure!(
        anchors
            .values()
            .all(|index| anchored_sections.insert(*index)),
        "ambiguous TLINK32 logical segments share a PE section"
    );
    let mut summary_ranges: Vec<_> = summaries.iter().filter(|s| s.size != 0).collect();
    summary_ranges.sort_by_key(|s| (s.segment, s.offset));
    ensure!(
        summary_ranges
            .windows(2)
            .all(|w| w[0].segment != w[1].segment
                || u64::from(w[0].offset) + u64::from(w[0].size) <= u64::from(w[1].offset)),
        "overlapping TLINK32 segment summaries are unsupported"
    );
    for summary in &summaries {
        let section = &image.sections[*anchors
            .get(&summary.segment)
            .context("unanchored TLINK32 MAP logical segment")?];
        ensure!(
            within(summary.offset, summary.size, 0, section.virtual_size),
            "TLINK32 MAP segment exceeds actual PE mapped region"
        );
        ensure!(
            (summary.class == "CODE") == code(section),
            "TLINK32 MAP code/data class disagrees with PE section"
        );
        if code(section) {
            ensure!(
                backed(image, section, summary.offset, summary.size),
                "TLINK32 MAP code segment is not initialized file-backed code"
            );
        }
    }
    for contribution in &mut contributions {
        let section = &image.sections[*anchors
            .get(&contribution.segment)
            .context("unanchored TLINK32 contribution")?];
        ensure!(
            summaries.iter().any(|s| s.segment == contribution.segment
                && s.name == contribution.name
                && s.class == contribution.class
                && within(contribution.offset, contribution.size, s.offset, s.size)),
            "TLINK32 contribution is outside its matching segment summary"
        );
        contribution.start = section
            .start
            .checked_add(contribution.offset)
            .context("TLINK32 contribution address overflow")?;
        contribution.end = contribution
            .start
            .checked_add(contribution.size)
            .context("TLINK32 contribution extent overflow")?;
        if code(section) {
            ensure!(
                backed(image, section, contribution.offset, contribution.size),
                "TLINK32 contribution is not initialized file-backed code"
            );
        }
    }
    let mut ranges: Vec<_> = contributions.iter().filter(|c| c.size != 0).collect();
    ranges.sort_by_key(|c| (c.segment, c.offset));
    ensure!(
        ranges
            .windows(2)
            .all(|w| w[0].segment != w[1].segment || w[0].end <= w[1].start),
        "overlapping TLINK32 detailed contributions are unsupported"
    );
    let mut addresses = BTreeMap::new();
    for (name, (segment, offset)) in &tables[0] {
        let section = &image.sections[*anchors
            .get(segment)
            .context("TLINK32 public has unknown logical segment")?];
        ensure!(
            contributions.iter().any(|c| c.segment == *segment
                && c.offset <= *offset
                && u64::from(*offset) < u64::from(c.offset) + u64::from(c.size)),
            "TLINK32 public outside declared nonempty contribution: {name}"
        );
        if code(section) {
            ensure!(
                backed(image, section, *offset, 1),
                "TLINK32 code public is not initialized executable code: {name}"
            );
        }
        addresses.insert(
            name.clone(),
            section
                .start
                .checked_add(*offset)
                .context("TLINK32 public address overflow")?,
        );
    }
    let provenance = json!({"schema":1,"format":"tlink32-pe","map_sha256":hash(text.as_bytes()),"image_sha256":hash(image.data),
        "logical_segment_anchors":anchors.iter().map(|(segment,index)|json!({"segment":segment,"pe_section":image.sections[*index].name,"base":image.sections[*index].start})).collect::<Vec<_>>(),
        "segments":summaries,"contributions":contributions,"public_count":addresses.len(),"public_names":"exact MAP display spelling; no demangling or function classification"});
    Ok(Publics {
        addresses,
        contributions,
        provenance,
    })
}
