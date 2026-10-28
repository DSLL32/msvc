//! Checked OMF32 records normalized to the runtime matcher's section/symbol model.
//! Unsupported code encodings are excluded, never treated as relocation-free bytes.
use super::*;

struct Cursor<'a> {
    data: &'a [u8],
    p: usize,
}
impl<'a> Cursor<'a> {
    fn take(&mut self, n: usize) -> Result<&'a [u8]> {
        let v = bytes(self.data, self.p, n)?;
        self.p += n;
        Ok(v)
    }
    fn byte(&mut self) -> Result<u8> {
        Ok(self.take(1)?[0])
    }
    fn index(&mut self) -> Result<usize> {
        let a = self.byte()?;
        Ok(if a & 128 == 0 {
            a as usize
        } else {
            ((a as usize & 127) << 8) | self.byte()? as usize
        })
    }
    fn number(&mut self, wide: bool) -> Result<u32> {
        let n = if wide { 4 } else { 2 };
        let v = self.take(n)?;
        Ok(if wide {
            u32::from_le_bytes(v.try_into()?)
        } else {
            u16::from_le_bytes(v.try_into()?) as u32
        })
    }
    fn name(&mut self) -> Result<String> {
        let n = self.byte()? as usize;
        Ok(latin1(self.take(n)?))
    }
    fn more(&self) -> bool {
        self.p < self.data.len()
    }
}
fn record(data: &[u8], p: usize) -> Result<(u8, &[u8], usize)> {
    let n = word(data, p + 1)? as usize;
    ensure!(n > 0, "empty OMF record");
    let all = bytes(data, p, n + 3)?;
    // CW32's compiler-name COMENT A3 records have stale nonzero checksums.
    // These documentary names never supply bytes, symbols or relocations.
    // Keep checksums mandatory for every semantic record (or explicit zero).
    let compiler_name =
        all[0] == 0x88 && n >= 4 && all[3..5] == [0, 0xa3] && all[5] as usize + 4 == n;
    ensure!(
        all[n + 2] == 0 || compiler_name || all.iter().fold(0u8, |a, b| a.wrapping_add(*b)) == 0,
        "invalid OMF checksum at {p:x}"
    );
    Ok((all[0], &all[3..n + 2], p + n + 3))
}
pub(super) fn archive(data: &[u8]) -> Result<Vec<(String, Vec<u8>)>> {
    let (tag, header, mut p) = record(data, 0)?;
    ensure!(
        tag == 0xf0 && header.len() >= 7,
        "invalid OMF library header"
    );
    let page = p;
    ensure!(
        page.is_power_of_two() && (16..=32768).contains(&page),
        "invalid OMF library page size"
    );
    let dictionary = dword(header, 0)? as usize;
    let blocks = word(header, 4)? as usize;
    ensure!(
        dictionary >= page && dictionary <= data.len(),
        "invalid OMF library dictionary offset"
    );
    bytes(
        data,
        dictionary,
        blocks.checked_mul(512).context("OMF dictionary overflow")?,
    )?;
    let mut out = vec![];
    let mut ended = false;
    while p < dictionary {
        let start = p;
        let (tag, head, next) = record(data, p)?;
        if tag == 0xf1 {
            ensure!(next <= dictionary, "OMF library end exceeds dictionary");
            ended = true;
            break;
        }
        ensure!(tag == 0x80, "OMF library member lacks THEADR");
        let mut c = Cursor { data: head, p: 0 };
        let name = c.name()?;
        ensure!(!name.is_empty() && !c.more(), "invalid OMF member name");
        p = next;
        loop {
            let (tag, _, next) = record(data, p)?;
            p = next;
            ensure!(p <= dictionary, "OMF member exceeds dictionary");
            if matches!(tag, 0x8a | 0x8b) {
                break;
            }
        }
        out.push((name, data[start..p].to_vec()));
        let aligned = p.checked_add(page - 1).context("OMF alignment overflow")? & !(page - 1);
        ensure!(
            bytes(data, p, aligned - p)?.iter().all(|v| *v == 0),
            "nonzero OMF member padding"
        );
        p = aligned;
    }
    ensure!(ended, "OMF library lacks LIBEND");
    Ok(out)
}
struct Segment {
    section: Section,
    covered: Vec<bool>,
    usable: bool,
}
#[derive(Clone, Copy)]
struct Datum {
    method: u8,
    index: usize,
}
fn datum(c: &mut Cursor<'_>, method: u8, frame: bool) -> Result<Datum> {
    let index = match method {
        0..=2 => c.index()?,
        3 => c.number(false)? as usize,
        4..=5 if frame => 0,
        _ => bail!("unsupported OMF fixup method {method}"),
    };
    Ok(Datum { method, index })
}
pub(super) fn parse(data: &[u8]) -> Result<Coff> {
    let mut names = vec![String::new()];
    let mut segments: Vec<Segment> = vec![];
    let mut symbols = vec![];
    let mut external = vec![String::new()];
    let mut groups = vec![String::new()];
    let mut virtual_count = 0usize;
    let mut last = None;
    let mut frames = [None; 4];
    let mut targets = [None; 4];
    let mut p = 0;
    let mut ended = false;
    while p < data.len() {
        let (tag, body, next) = record(data, p)?;
        let wide = tag & 1 != 0;
        let mut c = Cursor { data: body, p: 0 };
        p = next;
        match tag {
            0x80 => {
                ensure!(
                    symbols.is_empty() && segments.is_empty(),
                    "duplicate OMF module header"
                );
                let _ = c.name()?;
            }
            0x88 => {} // Compiler/debug/import comments do not define executable bytes.
            0x96 => {
                while c.more() {
                    names.push(c.name()?);
                }
            }
            0x98 | 0x99 => {
                let attr = c.byte()?;
                let align = attr >> 5;
                if align == 0 {
                    c.take(3)?;
                }
                let size = c.number(wide)? as usize;
                let name = c.index()?;
                let class = c.index()?;
                let overlay = c.index()?;
                ensure!(
                    name < names.len() && class < names.len() && overlay < names.len(),
                    "OMF segment name index out of range"
                );
                ensure!(
                    !c.more() && size <= data.len().saturating_mul(1024).max(65536),
                    "invalid OMF segment size/record"
                );
                let code = names[class].eq_ignore_ascii_case("CODE");
                let usable = attr & 1 != 0 && attr & 2 == 0 && align != 0;
                ensure!(
                    virtual_count == 0,
                    "SEGDEF after virtual contributions unsupported"
                );
                let index = i16::try_from(segments.len() + 1)?;
                segments.push(Segment {
                    section: Section {
                        data: vec![0; size],
                        relocs: vec![],
                        index,
                        flags: if code { 0x20 } else { 0 },
                    },
                    covered: vec![false; size],
                    usable,
                });
            }
            0x9a => {
                let name = c.index()?;
                ensure!(name < names.len(), "OMF group name index out of range");
                while c.more() {
                    ensure!(c.byte()? == 0xff, "unsupported OMF group component");
                    let s = c.index()?;
                    ensure!(
                        segments
                            .iter()
                            .any(|segment| segment.section.index as usize == s),
                        "invalid OMF group segment"
                    );
                }
                groups.push(names[name].clone());
            }
            0x8c | 0xb0 => {
                while c.more() {
                    let name = c.name()?;
                    c.index()?;
                    external.push(name.clone());
                    if tag == 0xb0 {
                        let kind = c.byte()?;
                        ensure!(
                            kind == 0x61 || kind == 0x62 || (1..=0x5f).contains(&kind),
                            "unsupported OMF COMDEF type"
                        );
                        let mut lengths = vec![];
                        for _ in 0..if kind == 0x61 { 2 } else { 1 } {
                            let n = c.byte()?;
                            let size = match n {
                                0..=128 => n as usize,
                                0x81 => c.number(false)? as usize,
                                0x84 => {
                                    let v = c.take(3)?;
                                    v[0] as usize | ((v[1] as usize) << 8) | ((v[2] as usize) << 16)
                                }
                                0x88 => c.number(true)? as usize,
                                _ => bail!("invalid OMF communal length"),
                            };
                            lengths.push(size);
                        }
                        if kind < 0x60 {
                            // Borland VIRDEF: template SEGDEF index, declared size,
                            // and a separate 0x4001-based segment index space.
                            let template = segments
                                .get(kind as usize - 1)
                                .context("invalid OMF virtual template")?;
                            ensure!(
                                template.section.index == kind as i16,
                                "virtual template is not an ordinary segment"
                            );
                            let flags = template.section.flags;
                            let usable = template.usable;
                            let size = lengths[0];
                            ensure!(
                                size <= data.len().saturating_mul(1024).max(65536),
                                "invalid OMF virtual size"
                            );
                            virtual_count += 1;
                            let index = i16::try_from(0x4000 + virtual_count)?;
                            symbols.push(Some(Symbol {
                                name,
                                value: 0,
                                section: index,
                                kind: 0x20,
                                storage: 2,
                            }));
                            segments.push(Segment {
                                section: Section {
                                    data: vec![0; size],
                                    relocs: vec![],
                                    index,
                                    flags,
                                },
                                covered: vec![false; size],
                                usable,
                            });
                        }
                    }
                }
            }
            0x90 | 0x91 => {
                let group = c.index()?;
                let segment = c.index()?;
                ensure!(
                    group < groups.len()
                        && (segment == 0
                            || segments.iter().any(|s| s.section.index as usize == segment)),
                    "invalid OMF public base"
                );
                if segment == 0 {
                    c.take(2)?;
                }
                while c.more() {
                    let name = c.name()?;
                    let value = c.number(wide)?;
                    c.index()?;
                    if segment > 0 {
                        ensure!(
                            value as usize
                                <= segments
                                    .iter()
                                    .find(|s| s.section.index as usize == segment)
                                    .unwrap()
                                    .section
                                    .data
                                    .len(),
                            "OMF public beyond segment"
                        );
                        symbols.push(Some(Symbol {
                            name,
                            value: value as i64,
                            section: segment.try_into()?,
                            kind: 0x20,
                            storage: 2,
                        }));
                    }
                }
            }
            0xa0 | 0xa1 | 0xa2 | 0xa3 => {
                let s = c.index()?;
                ensure!(
                    segments
                        .iter()
                        .any(|segment| segment.section.index as usize == s),
                    "invalid OMF data segment"
                );
                let offset = c.number(wide)? as usize;
                let seg = segments
                    .iter_mut()
                    .find(|seg| seg.section.index as usize == s)
                    .context("OMF data segment absent")?;
                if tag & 2 != 0 {
                    seg.usable = false;
                    last = Some((s, offset, usize::MAX));
                } else {
                    let v = c.take(body.len() - c.p)?;
                    let end = offset.checked_add(v.len()).context("OMF data overflow")?;
                    ensure!(end <= seg.section.data.len(), "OMF data exceeds segment");
                    ensure!(
                        seg.covered[offset..end].iter().all(|v| !*v),
                        "overlapping OMF data"
                    );
                    seg.section.data[offset..end].copy_from_slice(v);
                    seg.covered[offset..end].fill(true);
                    last = Some((s, offset, v.len()));
                }
            }
            0x9c | 0x9d => {
                while c.more() {
                    let a = c.byte()?;
                    if a & 128 == 0 {
                        ensure!(a & 32 == 0, "invalid OMF thread");
                        let frame = a & 64 != 0;
                        let d = datum(&mut c, (a >> 2) & 7, frame)?;
                        if frame {
                            frames[(a & 3) as usize] = Some(d)
                        } else {
                            targets[(a & 3) as usize] = Some(d)
                        }
                        continue;
                    }
                    let loc = ((a as usize & 3) << 8) | c.byte()? as usize;
                    let kind = (a >> 2) & 15;
                    let fix = c.byte()?;
                    let frame = if fix & 128 != 0 {
                        frames[((fix >> 4) & 3) as usize].context("undefined OMF frame thread")?
                    } else {
                        datum(&mut c, (fix >> 4) & 7, true)?
                    };
                    let target = if fix & 8 != 0 {
                        targets[(fix & 3) as usize].context("undefined OMF target thread")?
                    } else {
                        datum(&mut c, fix & 3, false)?
                    };
                    let displacement = if fix & 4 != 0 { 0 } else { c.number(wide)? };
                    let (s, base, len) =
                        last.context("OMF fixup without LEDATA (LIDATA fixups unsupported)")?;
                    let current = segments
                        .iter()
                        .position(|seg| seg.section.index as usize == s)
                        .context("OMF fixup segment absent")?;
                    let width = match kind {
                        0 => 1,
                        1 | 2 | 5 => 2,
                        3 | 9 | 13 => 4,
                        11 => 6,
                        _ => bail!("unsupported OMF location {kind}"),
                    };
                    ensure!(loc + width <= len, "OMF fixup exceeds data record");
                    if len == usize::MAX
                        || kind != 9
                        || !matches!(frame.method, 1 | 4 | 5)
                        || (frame.method == 1
                            && groups.get(frame.index).is_none_or(|name| name != "FLAT"))
                    {
                        segments[current].usable = false;
                        continue;
                    }
                    let symbol = match target.method {
                        0 => {
                            ensure!(
                                segments
                                    .iter()
                                    .any(|s| s.section.index as usize == target.index),
                                "invalid OMF target segment"
                            );
                            if target.index >= 0x4001 && target.index != s {
                                let public = symbols
                                    .iter()
                                    .flatten()
                                    .find(|p| p.section as usize == target.index && p.value == 0)
                                    .context("OMF virtual public absent")?;
                                Symbol {
                                    name: public.name.clone(),
                                    value: displacement as i64,
                                    section: 0,
                                    kind: 0,
                                    storage: 0xfe,
                                }
                            } else {
                                Symbol {
                                    name: String::new(),
                                    value: displacement as i64,
                                    section: target.index.try_into()?,
                                    kind: 0,
                                    storage: 0,
                                }
                            }
                        }
                        2 => {
                            let name = external
                                .get(target.index)
                                .filter(|_| target.index > 0)
                                .context("invalid OMF external index")?;
                            Symbol {
                                name: name.clone(),
                                value: displacement as i64,
                                section: 0,
                                kind: 0,
                                storage: 0xfe, // OMF public/import identity
                            }
                        }
                        _ => {
                            segments[current].usable = false;
                            continue;
                        }
                    };
                    let seg = &mut segments[current];
                    if a & 64 == 0 && symbol.section != 0 && symbol.section != seg.section.index {
                        seg.usable = false;
                        continue; // no inferred cross-segment code placement
                    }
                    // OMF stores the explicit target displacement outside the patched
                    // field. Fold it into the matcher's addend; exact bytes elsewhere
                    // remain unchanged. REL32 uses the same next-instruction base.
                    let at = base + loc;
                    ensure!(
                        seg.section
                            .relocs
                            .iter()
                            .all(|r| r.offset + 4 <= at || at + 4 <= r.offset),
                        "overlapping OMF fixups"
                    );
                    let old = dword(&seg.section.data, at)?;
                    let addend = old.wrapping_add(displacement);
                    seg.section.data[at..at + 4].copy_from_slice(&addend.to_le_bytes());
                    let mut symbol = symbol;
                    symbol.value -= displacement as i64;
                    seg.section.relocs.push(Relocation {
                        offset: at,
                        target: symbol,
                        kind: if a & 64 == 0 { 20 } else { 6 },
                    });
                }
            }
            0xc6 => {
                while c.more() {
                    c.name()?;
                    c.name()?;
                }
            } // aliases add no bytes/public boundaries
            0x8a | 0x8b => {
                ended = true;
                ensure!(p == data.len(), "trailing OMF module bytes");
                break;
            }
            _ => bail!("unsupported OMF record {tag:02x}"),
        }
    }
    ensure!(ended, "OMF module lacks MODEND");
    Ok(Coff {
        symbols,
        sections: segments
            .into_iter()
            .map(|mut s| {
                if !s.usable || s.covered.iter().any(|v| !*v) {
                    s.section.flags = 0;
                }
                s.section
            })
            .collect(),
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    fn rec(tag: u8, body: &[u8]) -> Vec<u8> {
        let mut v = vec![tag];
        v.extend_from_slice(&((body.len() + 1) as u16).to_le_bytes());
        v.extend_from_slice(body);
        v.push(0u8.wrapping_sub(v.iter().fold(0u8, |a, b| a.wrapping_add(*b))));
        v
    }
    fn object(fix: &[u8]) -> Vec<u8> {
        let code = [
            0x55, 0x89, 0xe5, 0x83, 0xec, 4, 0xe8, 0, 0, 0, 0, 0xc9, 0xc3,
        ];
        let mut out = vec![];
        for (t, b) in [
            (0x80, b"\x04test".as_slice()),
            (0x96, b"\x00\x05_TEXT\x04CODE\x04FLAT"),
            (0x98, &[0xa9, 13, 0, 2, 3, 1]),
            (0x9a, &[4]),
            (0x8c, b"\x07_callee\0"),
            (0x90, b"\x01\x01\x06_first\0\0\0"),
        ] {
            out.extend(rec(t, b));
        }
        let mut body = vec![1, 0, 0];
        body.extend(code);
        out.extend(rec(0xa0, &body));
        out.extend(rec(0x9d, fix));
        out.extend(rec(0x8a, &[0]));
        out
    }
    #[test]
    fn explicit_and_threaded_rel32_match_then_reject_wrong_callee() {
        for fix in [
            &[0xa4, 7, 0x16, 1, 1][..],
            &[0x44, 1, 8, 1, 0xa4, 7, 0x8c][..],
            &[0xa4, 7, 0x12, 1, 1, 4, 0, 0, 0][..],
        ] {
            let obj = parse(&object(fix)).unwrap();
            let section = &obj.sections[0];
            assert_eq!(section.flags, 0x20);
            assert_eq!(section.relocs[0].kind, 20);
            let mut code = section.data.clone();
            let addend = dword(&code, 7).unwrap();
            code[7..11].copy_from_slice(&(0x1100u32 + addend - 0x100b).to_le_bytes());
            assert_eq!(matches(&code, section).unwrap(), vec![0]);
            let mut data = vec![0u8; 200 + 256];
            data[200..213].copy_from_slice(&code);
            let pe = Pe {
                data,
                base: 0x1000,
                optional: 4,
                sections: vec![PeSection {
                    name: "CODE".into(),
                    rva: 0,
                    raw_size: 256,
                    offset: 200,
                    virtual_size: 256,
                    flags: 0x20000000,
                }],
            };
            let make = || Candidate {
                section: section.clone(),
                start: 0x1000,
                records: vec![Record {
                    name: "_first".into(),
                    start: 0x1000,
                    end: 0x100d,
                    object: "test.obj".into(),
                    library: "test".into(),
                    shared: vec![],
                }],
            };
            let mut candidates = vec![make()];
            prune_with_external_anchors(
                &pe,
                &mut candidates,
                &HashMap::new(),
                &HashMap::from([("_callee".into(), HashSet::from([0x1100]))]),
            )
            .unwrap();
            assert_eq!(candidates.len(), 1);
            let mut candidates = vec![make()];
            prune_with_external_anchors(
                &pe,
                &mut candidates,
                &HashMap::new(),
                &HashMap::from([("_callee".into(), HashSet::from([0x1200]))]),
            )
            .unwrap();
            assert!(candidates.is_empty());
        }
    }
    #[test]
    fn corrupt_records_indices_and_fixup_bounds_reject() {
        let original = object(&[0xa4, 7, 0x16, 1, 1]);
        let mut checksum = original.clone();
        checksum[8] ^= 1;
        assert!(parse(&checksum).is_err());
        assert!(parse(&original[..original.len() - 1]).is_err());
        assert!(parse(&object(&[0xa4, 12, 0x16, 1, 1])).is_err());
        assert!(parse(&object(&[0xa4, 7, 0x16, 1, 2])).is_err());
        assert!(parse(&object(&[0xa4, 7, 0x8c])).is_err());
        let unsupported = parse(&object(&[0x84, 7, 0x16, 1, 1])).unwrap();
        assert_eq!(unsupported.sections[0].flags, 0);
        let mut tail = original;
        tail.push(0);
        assert!(parse(&tail).is_err());
    }
    #[test]
    fn shipped_archive_and_startup_are_bounded_and_parsed() {
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../toolchains/bc4.0/BC4/LIB");
        let data = fs::read(root.join("CW32.LIB")).unwrap();
        let members = archive(&data).unwrap();
        assert_eq!(members.len(), 724);
        for (name, data) in members {
            parse(&data).unwrap_or_else(|e| panic!("{name}: {e:#}"));
        }
        let obj = parse(&fs::read(root.join("C0X32.OBJ")).unwrap()).unwrap();
        let text = &obj.sections[0];
        assert_eq!(
            (text.flags, text.data.len(), text.relocs.len()),
            (0x20, 79, 9)
        );
        assert!(
            obj.symbols
                .iter()
                .flatten()
                .any(|s| s.name == "__acrtused" && s.value == 0 && s.section == 1)
        );
        let mut damaged = data.clone();
        damaged[3..7].copy_from_slice(&u32::MAX.to_le_bytes());
        assert!(archive(&damaged).is_err());
        assert!(archive(&data[..data.len() / 2]).is_err());
    }
}
