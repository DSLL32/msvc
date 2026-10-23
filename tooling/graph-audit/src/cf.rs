//! Explicit Watcom CF initialized-image view. CF is not a PE and has no section
//! table, IAT, executable-region map or inferred function ownership.
use crate::{
    index,
    pe::{u16_at, u32_at},
};
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use std::{
    collections::BTreeSet,
    fs,
    path::{Path, PathBuf},
};

#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct FileBinding {
    path: PathBuf,
    sha256: String,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Definition {
    format: String,
    coordinates: String,
    analysis_base: u32,
    initialized_sha256: String,
    relocation_sha256: String,
    loader: FileBinding,
    layout_evidence: FileBinding,
}
fn bound_file(base: &Path, binding: &FileBinding, label: &str) -> Result<()> {
    ensure!(
        !binding.path.is_absolute(),
        "CF {label} must be relative to its base"
    );
    let base = base
        .canonicalize()
        .with_context(|| format!("CF {label} base"))?;
    let path = base
        .join(&binding.path)
        .canonicalize()
        .with_context(|| format!("CF {label} file"))?;
    ensure!(
        path.starts_with(&base) && path.is_file(),
        "CF {label} escapes base or is not a file"
    );
    ensure!(
        index::hash(&fs::read(path)?) == binding.sha256,
        "CF {label} hash differs"
    );
    Ok(())
}
#[derive(Clone, Debug, Serialize)]
pub struct Header {
    pub header_file_offset: u32,
    pub image_file_offset: u32,
    pub initialized_size: u32,
    pub relocation_offset: u32,
    pub allocation_size: u32,
    pub entry_offset: u32,
}
#[derive(Clone, Debug, Serialize)]
pub struct Group {
    count: u16,
    first: u32,
    last: u32,
}
pub struct Cf {
    source: Vec<u8>,
    pub header: Header,
    pub fixups: Vec<u32>,
    groups: Vec<Group>,
    initialized_sha256: String,
    relocation_sha256: String,
    loader: FileBinding,
    layout_evidence: FileBinding,
    code_ranges: Vec<(u32, u32)>,
}
impl Cf {
    pub fn open(path: &Path, metadata: &Value, project: &Path, repo: &Path) -> Result<Self> {
        let definition: Definition =
            serde_json::from_value(metadata.clone()).context("CF image definition")?;
        ensure!(definition.format == "watcom-cf", "unsupported image format");
        ensure!(
            definition.coordinates == "image-offset" && definition.analysis_base == 0,
            "CF requires image-offset coordinates and analysis_base 0"
        );
        bound_file(repo, &definition.loader, "loader")?;
        bound_file(project, &definition.layout_evidence, "layout evidence")?;
        let source = fs::read(path).with_context(|| format!("read CF {}", path.display()))?;
        Self::parse(source, definition)
    }
    fn parse(source: Vec<u8>, definition: Definition) -> Result<Self> {
        ensure!(
            source.get(..2) == Some(b"MZ"),
            "CF requires original MZ container"
        );
        let h = u32::from(u16_at(&source, 8)?)
            .checked_mul(16)
            .context("CF header offset overflow")?;
        let at = h as usize;
        ensure!(
            u32_at(&source, at)? == 0x4643,
            "not an uncompressed Watcom CF image"
        );
        let header = Header {
            header_file_offset: h,
            image_file_offset: u32_at(&source, at + 4)?,
            initialized_size: u32_at(&source, at + 8)?,
            relocation_offset: u32_at(&source, at + 12)?,
            allocation_size: u32_at(&source, at + 16)?,
            entry_offset: u32_at(&source, at + 20)?,
        };
        let file_end = header
            .image_file_offset
            .checked_add(header.initialized_size)
            .context("CF image file range overflow")?;
        ensure!(
            header.image_file_offset >= h.checked_add(24).context("CF header range overflow")?
                && file_end as usize <= source.len(),
            "CF initialized image outside source file"
        );
        ensure!(
            header.entry_offset < header.relocation_offset
                && header.relocation_offset < header.initialized_size
                && header.initialized_size <= header.allocation_size,
            "invalid CF entry/relocation/allocation bounds"
        );
        let image = &source[header.image_file_offset as usize..file_end as usize];
        let initialized_sha256 = index::hash(image);
        let relocation_sha256 = index::hash(&image[header.relocation_offset as usize..]);
        ensure!(
            initialized_sha256 == definition.initialized_sha256,
            "CF initialized image hash differs"
        );
        ensure!(
            relocation_sha256 == definition.relocation_sha256,
            "CF relocation stream hash differs"
        );
        let mut cursor = header.relocation_offset as usize;
        let mut next_word = || -> Result<u16> {
            let v = u16_at(image, cursor).context("truncated CF relocation stream")?;
            cursor += 2;
            Ok(v)
        };
        let mut fixups = Vec::new();
        let mut groups = Vec::new();
        loop {
            let count = next_word()?;
            if count == 0 {
                break;
            }
            let mut address = u32::from(next_word()?) << 16;
            address |= u32::from(next_word()?);
            let first = address;
            for n in 0..count {
                if n != 0 {
                    address = address
                        .checked_add(u32::from(next_word()?))
                        .context("CF relocation offset overflow")?;
                }
                ensure!(
                    address
                        .checked_add(4)
                        .is_some_and(|end| end <= header.relocation_offset),
                    "CF relocation write outside initialized pre-stream image"
                );
                // CONTENTS may be negative indexed displacements (real WPP
                // 11a6a contains ffffd8be). Only WRITE locations are bounded.
                fixups.push(address);
            }
            groups.push(Group {
                count,
                first,
                last: address,
            });
        }
        ensure!(
            cursor == image.len(),
            "CF relocation terminator is not exact initialized-image end"
        );
        let mut ordered = fixups.clone();
        ordered.sort();
        ensure!(
            ordered
                .windows(2)
                .all(|v| v[0].checked_add(4).is_some_and(|end| end <= v[1])),
            "duplicate/overlapping CF relocation writes"
        );
        Ok(Self {
            source,
            header,
            fixups,
            groups,
            initialized_sha256,
            relocation_sha256,
            loader: definition.loader,
            layout_evidence: definition.layout_evidence,
            code_ranges: Vec::new(),
        })
    }
    pub fn source(&self) -> &[u8] {
        &self.source
    }
    pub fn payload(&self) -> &[u8] {
        let a = self.header.image_file_offset as usize;
        &self.source[a..a + self.header.initialized_size as usize]
    }
    pub fn bind_code(&mut self, ranges: impl Iterator<Item = (u32, u32)>) -> Result<()> {
        let mut unique = BTreeSet::new();
        for (a, b) in ranges {
            ensure!(
                a < b && b <= self.header.relocation_offset,
                "CF declared code outside initialized pre-stream bytes: {a:x}..{b:x}"
            );
            unique.insert((a, b));
        }
        self.code_ranges = unique.into_iter().collect();
        Ok(())
    }
    pub fn file_backed_code(&self, a: u32, b: u32) -> bool {
        a < b
            && b <= self.header.relocation_offset
            && self.code_ranges.iter().any(|&(s, e)| s <= a && b <= e)
    }
    pub fn text_data_backed(&self, a: u32, b: u32) -> bool {
        a < b && b <= self.header.relocation_offset
    }
    pub fn read(&self, a: u32, n: usize) -> Result<&[u8]> {
        let a = a as usize;
        let end = a.checked_add(n).context("CF read range overflow")?;
        self.payload()
            .get(a..end)
            .context("CF read outside initialized image; allocation tail contents are not inferred")
    }
    pub fn string(&self, a: u32) -> Result<String> {
        let bytes = self
            .payload()
            .get(a as usize..)
            .context("CF string outside initialized image")?;
        let n = bytes
            .iter()
            .position(|&v| v == 0)
            .context("unterminated CF string")?;
        Ok(bytes[..n].iter().map(|&v| char::from(v)).collect())
    }
    pub fn provenance(&self) -> Value {
        json!({"format":"watcom-cf","coordinates":"image-offset","analysis_base":0,"original_file_sha256":index::hash(&self.source),"header":self.header,"initialized_sha256":self.initialized_sha256,"relocation_sha256":self.relocation_sha256,"relocation_count":self.fixups.len(),"relocation_groups":self.groups,"loader":self.loader,"layout_evidence":self.layout_evidence,"declared_code_ranges":self.code_ranges,"allocation_tail_contents":"not inferred","ownership_basis":"explicit index records only; no CF section table or whole-image executable claim"})
    }
    /// Loader arithmetic is DWORD wrapping addition, not a pointer-range test.
    #[cfg(test)]
    pub fn relocated_image(&self, base: u32) -> Vec<u8> {
        let mut image = self.payload().to_vec();
        for &a in &self.fixups {
            let at = a as usize;
            let value = u32::from_le_bytes(
                image[at..at + 4]
                    .try_into()
                    .expect("verified CF fixup span"),
            );
            image[at..at + 4].copy_from_slice(&value.wrapping_add(base).to_le_bytes());
        }
        image
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture(stream: &[u16]) -> (Vec<u8>, Definition) {
        let size = 0x40 + 2 * stream.len();
        let mut b = vec![0; 0x100 + size];
        b[..2].copy_from_slice(b"MZ");
        b[8..10].copy_from_slice(&5u16.to_le_bytes());
        for (n, v) in [0x4643, 0x100, size as u32, 0x40, 0x100, 0]
            .into_iter()
            .enumerate()
        {
            b[0x50 + 4 * n..0x54 + 4 * n].copy_from_slice(&v.to_le_bytes());
        }
        b[0x100..0x106].copy_from_slice(&[0xb8, 0xf0, 0xff, 0xff, 0xff, 0xc3]);
        for (n, v) in stream.iter().enumerate() {
            b[0x140 + 2 * n..0x142 + 2 * n].copy_from_slice(&v.to_le_bytes());
        }
        let d = Definition {
            format: "watcom-cf".into(),
            coordinates: "image-offset".into(),
            analysis_base: 0,
            initialized_sha256: index::hash(&b[0x100..]),
            relocation_sha256: index::hash(&b[0x140..]),
            loader: FileBinding {
                path: "loader".into(),
                sha256: String::new(),
            },
            layout_evidence: FileBinding {
                path: "evidence".into(),
                sha256: String::new(),
            },
        };
        (b, d)
    }
    #[test]
    fn raw_view_needs_explicit_code_and_never_infers_zero_tail() {
        let (b, d) = fixture(&[1, 0, 1, 0]);
        let mut cf = Cf::parse(b, d).unwrap();
        assert!(!cf.file_backed_code(0, 6));
        cf.bind_code([(0, 6)].into_iter()).unwrap();
        assert!(cf.file_backed_code(0, 6));
        assert!(!cf.file_backed_code(6, 7));
        assert!(cf.read(0x48, 1).is_err());
        assert!(cf.text_data_backed(0x20, 0x24));
        assert!(cf.bind_code([(0x3f, 0x41)].into_iter()).is_err());
    }
    #[test]
    fn loader_addition_wraps_without_rejecting_negative_content() {
        let (b, d) = fixture(&[1, 0, 1, 0]);
        let cf = Cf::parse(b, d).unwrap();
        assert_eq!(u32_at(&cf.relocated_image(0x30), 1).unwrap(), 0x20);
        assert_eq!(cf.relocated_image(0), cf.payload());
    }
    #[test]
    fn bounded_relocations_reject_duplicates_overlaps_and_stream_damage() {
        for stream in [
            &[2, 0, 1, 0, 0][..],
            &[2, 0, 1, 1, 0][..],
            &[1, 0, 0x3f, 0][..],
            &[2, 0, 1][..],
            &[0, 0][..],
        ] {
            let (b, d) = fixture(stream);
            assert!(Cf::parse(b, d).is_err());
        }
    }
    #[test]
    fn wrong_payload_and_relocation_hashes_reject() {
        let (b, mut d) = fixture(&[1, 0, 1, 0]);
        d.initialized_sha256 = "0".repeat(64);
        assert!(Cf::parse(b, d).is_err());
        let (b, mut d) = fixture(&[1, 0, 1, 0]);
        d.relocation_sha256 = "0".repeat(64);
        assert!(Cf::parse(b, d).is_err());
    }
    #[test]
    fn malformed_header_bounds_reject() {
        for (offset, value) in [
            (0x50, 0x00434346),
            (0x54, u32::MAX),
            (0x58, u32::MAX),
            (0x5c, 0x48),
            (0x60, 0x20),
            (0x64, 0x40),
        ] {
            let (mut b, d) = fixture(&[1, 0, 1, 0]);
            b[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
            assert!(Cf::parse(b, d).is_err());
        }
    }
    #[test]
    fn cf_code_boundaries_still_use_same_decoder_and_cfg() {
        use crate::{
            decode::Decoder,
            flow::{Options, walk},
            image::{Image, ImageView},
        };
        let (b, d) = fixture(&[1, 0, 1, 0]);
        let mut cf = Cf::parse(b, d).unwrap();
        cf.bind_code([(0, 4)].into_iter()).unwrap();
        let view = Image::Cf(cf);
        assert!(walk(&Decoder::new().unwrap(), &view, 0, 4, &Options::default()).is_err());
        assert!(view.file_backed_code(0, 4));
    }
    #[test]
    fn real_three_cf_layouts_and_wpp_wrapping_addend() {
        for (name, count, entry) in [
            ("WCC386", 11801, 0x2fe00),
            ("WPP386", 15085, 0x664f4),
            ("WASM", 2915, 0x1da16),
        ] {
            let path = Path::new(env!("CARGO_MANIFEST_DIR"))
                .join("../../toolchains/watcom10.0a/binb")
                .join(format!("{name}.EXE"));
            let b = fs::read(path).unwrap();
            let h = usize::from(u16_at(&b, 8).unwrap()) * 16;
            let load = u32_at(&b, h + 4).unwrap() as usize;
            let size = u32_at(&b, h + 8).unwrap() as usize;
            let reloc = u32_at(&b, h + 12).unwrap() as usize;
            let (_, mut d) = fixture(&[1, 0, 1, 0]);
            d.initialized_sha256 = index::hash(&b[load..load + size]);
            d.relocation_sha256 = index::hash(&b[load + reloc..load + size]);
            let cf = Cf::parse(b, d).unwrap();
            assert_eq!(cf.fixups.len(), count);
            assert_eq!(cf.header.entry_offset, entry);
            if name == "WPP386" {
                assert!(cf.fixups.contains(&0x11a6a));
                assert_eq!(u32_at(cf.payload(), 0x11a6a).unwrap(), 0xffffd8be);
                assert_eq!(
                    u32_at(&cf.relocated_image(0x008e0020), 0x11a6a).unwrap(),
                    0x008dd8de
                );
            }
        }
    }
    #[test]
    fn real_trademark_cfg_matches_at_zero_and_observed_unaligned_base() {
        use crate::{
            decode::Decoder,
            flow::{Options, walk},
            image::ImageView,
        };
        struct Loaded {
            bytes: Vec<u8>,
            base: u32,
        }
        impl ImageView for Loaded {
            fn read(&self, a: u32, n: usize) -> Result<&[u8]> {
                let a = a.checked_sub(self.base).context("below loaded base")? as usize;
                self.bytes.get(a..a + n).context("outside loaded bytes")
            }
            fn file_backed_code(&self, a: u32, b: u32) -> bool {
                self.base + 0xcb74 <= a && a < b && b <= self.base + 0xcba9
            }
        }
        let path = Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../../toolchains/watcom10.0a/binb/WASM.EXE");
        let b = fs::read(path).unwrap();
        let (_, mut d) = fixture(&[1, 0, 1, 0]);
        d.initialized_sha256 = index::hash(&b[0x2200..0x2200 + 0x28282]);
        d.relocation_sha256 = index::hash(&b[0x2200 + 0x26bae..0x2200 + 0x28282]);
        let cf = Cf::parse(b, d).unwrap();
        for base in [0, 0x008e0020, 0x10001003] {
            let loaded = Loaded {
                bytes: cf.relocated_image(base),
                base,
            };
            let g = walk(
                &Decoder::new().unwrap(),
                &loaded,
                base + 0xcb74,
                base + 0xcba9,
                &Options {
                    strict: true,
                    ..Options::default()
                },
            )
            .unwrap();
            assert_eq!(
                g.calls.into_iter().map(|v| v - base).collect::<Vec<_>>(),
                [0x1d9ad]
            );
            assert_eq!(g.returns.into_iter().collect::<Vec<_>>(), [0]);
            assert!(!g.indirect);
            assert_eq!(u32_at(&loaded.bytes, 0xcb79).unwrap(), base + 0x262f2);
        }
    }
    #[test]
    fn flat_seh_requires_both_explicit_owned_roots() {
        use crate::{
            decode::Decoder,
            flow::{Options, walk},
            image::Image,
        };
        let (mut b, mut d) = fixture(&[1, 0, 1, 0]);
        b[0x100..0x106].copy_from_slice(&[0x68, 0x30, 0, 0, 0, 0xc3]);
        b[0x110] = 0xc3;
        b[0x120] = 0xc3;
        for (at, v) in [(0x130, u32::MAX), (0x134, 0x10), (0x138, 0x20)] {
            b[at..at + 4].copy_from_slice(&v.to_le_bytes());
        }
        d.initialized_sha256 = index::hash(&b[0x100..]);
        let mut cf = Cf::parse(b, d).unwrap();
        cf.bind_code([(0, 6), (0x10, 0x11), (0x20, 0x21)].into_iter())
            .unwrap();
        let view = Image::Cf(cf);
        let decoder = Decoder::new().unwrap();
        let mut options = Options {
            require_explicit_seh_roots: true,
            code_chunks: vec![(0x10, 0x11), (0x20, 0x21)],
            ..Options::default()
        };
        assert!(
            walk(&decoder, &view, 0, 6, &options)
                .err()
                .unwrap()
                .to_string()
                .contains("requires explicit metadata roots")
        );
        options.roots = vec![0x10];
        assert!(walk(&decoder, &view, 0, 6, &options).is_err());
        options.roots.push(0x20);
        let graph = walk(&decoder, &view, 0, 6, &options).unwrap();
        assert_eq!(graph.instructions.len(), 4);
        options.roots.push(0x18);
        assert!(walk(&decoder, &view, 0, 6, &options).is_err());
    }
    #[test]
    fn inferred_flat_switch_needs_exact_shared_ownership_and_disjoint_code() {
        use crate::{
            decode::Decoder,
            flow::{Options, walk},
            image::Image,
            index::Record,
        };
        let (mut b, mut d) = fixture(&[1, 0, 3, 0]);
        b[0x100..0x107].copy_from_slice(&[0xff, 0x24, 0x85, 0x30, 0, 0, 0]);
        b[0x110] = 0xc3;
        b[0x120] = 0xc3;
        for (at, v) in [(0x130, 0x10u32), (0x134, 0x20), (0x138, 0x100)] {
            b[at..at + 4].copy_from_slice(&v.to_le_bytes());
        }
        d.initialized_sha256 = index::hash(&b[0x100..]);
        let mut cf = Cf::parse(b, d).unwrap();
        cf.bind_code([(0, 7), (0x10, 0x11), (0x20, 0x21)].into_iter())
            .unwrap();
        let view = Image::Cf(cf);
        let decoder = Decoder::new().unwrap();
        let options = Options {
            infer_switches: true,
            code_chunks: vec![(0x10, 0x11), (0x20, 0x21)],
            ..Options::default()
        };
        let mut graph = walk(&decoder, &view, 0, 7, &options).unwrap();
        assert_eq!(graph.tables, [(0x30, 0x38)]);
        let mut record = Record {
            name: "switch".into(),
            start: 0,
            end: 7,
            shared: vec![],
            code_chunks: options.code_chunks,
            evidence: Value::Null,
            pending_extent: false,
        };
        assert!(
            crate::audit::validate_inferred_data(&view, &record, &graph, &[record.clone()])
                .is_err()
        );
        record.shared.push((0x30, 0x34));
        assert!(
            crate::audit::validate_inferred_data(&view, &record, &graph, &[record.clone()])
                .is_err()
        );
        record.shared[0] = (0x30, 0x38);
        crate::audit::validate_inferred_data(&view, &record, &graph, &[record.clone()]).unwrap();
        graph.tables = vec![(0, 1)];
        assert!(
            crate::audit::validate_inferred_data(&view, &record, &graph, &[record.clone()])
                .unwrap_err()
                .to_string()
                .contains("intersects executable")
        );
    }
}
