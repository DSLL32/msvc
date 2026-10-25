//! PE loader-owned bytes, independently of reconstruction function ownership.
use crate::original_image::Image;
use anyhow::{Context, Result, ensure};
#[derive(Debug)]
pub struct Range {
    pub start: u32,
    pub end: u32,
    pub kind: &'static str,
}
pub struct Metadata {
    pub ranges: Vec<Range>,
    pub imports: Vec<String>,
}
struct Reader<'a> {
    image: &'a Image,
    bytes: &'a [u8],
    ranges: Vec<Range>,
}
impl Reader<'_> {
    fn add(&mut self, rva: u32, size: usize, kind: &'static str) -> Result<usize> {
        ensure!(size > 0, "empty {kind}");
        let end = rva
            .checked_add(u32::try_from(size)?)
            .context("loader RVA overflow")?;
        let offset = self
            .image
            .file_offset(rva, size)
            .with_context(|| format!("{kind} {rva:x}..{end:x}"))?;
        self.ranges.push(Range {
            start: rva,
            end,
            kind,
        });
        Ok(offset)
    }
    fn word(&self, rva: u32) -> Result<u32> {
        let at = self.image.file_offset(rva, 4)?;
        Ok(u32::from_le_bytes(self.bytes[at..at + 4].try_into()?))
    }
    fn string(&mut self, rva: u32, prefix: usize, kind: &'static str) -> Result<Vec<u8>> {
        let text = rva
            .checked_add(u32::try_from(prefix)?)
            .context("loader string overflow")?;
        for n in 0..4096u32 {
            let at = self
                .image
                .file_offset(text.checked_add(n).context("loader string overflow")?, 1)?;
            if self.bytes[at] == 0 {
                let at = self.add(rva, prefix + n as usize + 1, kind)?;
                return Ok(self.bytes[at + prefix..at + prefix + n as usize].to_vec());
            }
        }
        anyhow::bail!("unterminated {kind}")
    }
    fn directory(&self, index: usize) -> Result<Option<(u32, u32)>> {
        let pe = u32::from_le_bytes(
            self.bytes
                .get(0x3c..0x40)
                .context("truncated PE offset")?
                .try_into()?,
        ) as usize;
        ensure!(
            pe.checked_add(120)
                .is_some_and(|end| end <= self.bytes.len()),
            "truncated optional header"
        );
        let optional = u16::from_le_bytes(self.bytes[pe + 20..pe + 22].try_into()?) as usize;
        ensure!(
            optional >= 96
                && pe
                    .checked_add(24 + optional)
                    .is_some_and(|end| end <= self.bytes.len()),
            "short PE32 optional header"
        );
        let count = u32::from_le_bytes(self.bytes[pe + 116..pe + 120].try_into()?) as usize;
        ensure!(
            count <= (optional - 96) / 8,
            "PE directories exceed optional header"
        );
        if index >= count {
            return Ok(None);
        }
        let at = pe + 120 + index * 8;
        let rva = u32::from_le_bytes(self.bytes[at..at + 4].try_into()?);
        let size = u32::from_le_bytes(self.bytes[at + 4..at + 8].try_into()?);
        if rva == 0 {
            ensure!(size == 0, "directory size without RVA");
            return Ok(None);
        }
        if size != 0 {
            self.image.file_offset(rva, size as usize)?;
        }
        Ok(Some((rva, size)))
    }
}
pub fn parse(image: &Image, bytes: &[u8]) -> Result<Metadata> {
    read(image, bytes, false)
}
pub fn imports(image: &Image, bytes: &[u8]) -> Result<Vec<String>> {
    Ok(read(image, bytes, true)?.imports)
}
fn read(image: &Image, bytes: &[u8], only_imports: bool) -> Result<Metadata> {
    let mut r = Reader {
        image,
        bytes,
        ranges: vec![],
    };
    let mut imports = vec![];
    if let Some((directory, size)) = r.directory(1)? {
        let limit = if size == 0 { 65536 } else { size / 20 };
        let mut terminated = false;
        for n in 0..limit {
            let descriptor = directory
                .checked_add(n.checked_mul(20).context("import overflow")?)
                .context("import overflow")?;
            let at = r.add(descriptor, 20, "import descriptor")?;
            let words = bytes[at..at + 20]
                .chunks_exact(4)
                .map(|b| u32::from_le_bytes(b.try_into().unwrap()))
                .collect::<Vec<_>>();
            if words.iter().all(|v| *v == 0) {
                terminated = true;
                break;
            }
            ensure!(
                words[3] != 0 && words[4] != 0,
                "incomplete import descriptor"
            );
            let name = r.string(words[3], 0, "import DLL name")?;
            ensure!(
                !name.is_empty() && name.is_ascii(),
                "non-ASCII/empty import DLL name"
            );
            imports.push(String::from_utf8(name)?);
            let lookup = if words[0] == 0 { words[4] } else { words[0] };
            let mut ended = false;
            for n in 0..65536u32 {
                let delta = n * 4;
                let slot = lookup.checked_add(delta).context("import thunk overflow")?;
                let value = r.word(slot)?;
                r.add(slot, 4, "import lookup thunk")?;
                r.add(
                    words[4].checked_add(delta).context("IAT overflow")?,
                    4,
                    "import address thunk",
                )?;
                if value == 0 {
                    ended = true;
                    break;
                }
                if value & 0x80000000 == 0 {
                    r.string(value, 2, "import hint/name")?;
                }
            }
            ensure!(ended, "unterminated import thunk table");
        }
        ensure!(terminated, "unterminated import directory");
    }
    if only_imports {
        return Ok(Metadata {
            ranges: r.ranges,
            imports,
        });
    }
    // The advertised IAT directory bounds the same loader-written thunk slots;
    // restore only slots reached through import descriptors, never arbitrary bytes.
    if let Some((_, size)) = r.directory(12)? {
        ensure!(size % 4 == 0, "unaligned IAT directory");
    }
    if let Some((directory, size)) = r.directory(0)? {
        ensure!(size >= 40, "short export directory");
        let at = r.add(directory, 40, "export header")?;
        let word = |n| u32::from_le_bytes(bytes[at + n..at + n + 4].try_into().unwrap());
        let functions = word(20);
        let names = word(24);
        ensure!(
            functions <= 1_000_000 && names <= 1_000_000,
            "oversized export tables"
        );
        r.string(word(12), 0, "export DLL name")?;
        let eat = word(28);
        let name_table = word(32);
        let ordinals = word(36);
        if functions != 0 {
            r.add(eat, functions as usize * 4, "export address table")?;
        }
        if names != 0 {
            r.add(name_table, names as usize * 4, "export name table")?;
            r.add(ordinals, names as usize * 2, "export ordinal table")?;
        }
        let end = directory.checked_add(size).context("export overflow")?;
        for n in 0..functions {
            let address = r.word(eat + n * 4)?;
            if address >= directory && address < end {
                r.string(address, 0, "export forwarder")?;
            }
        }
        for n in 0..names {
            let name = r.word(name_table + n * 4)?;
            r.string(name, 0, "export name")?;
            let at = image.file_offset(ordinals + n * 2, 2)?;
            ensure!(
                u32::from(u16::from_le_bytes(bytes[at..at + 2].try_into()?)) < functions,
                "export ordinal outside EAT"
            );
        }
    }
    if let Some((directory, size)) = r.directory(6)? {
        ensure!(size != 0 && size % 28 == 0, "invalid debug directory size");
        let at = r.add(directory, size as usize, "debug directory")?;
        for n in 0..size as usize / 28 {
            let at = at + n * 28;
            let size = u32::from_le_bytes(bytes[at + 16..at + 20].try_into()?) as usize;
            let address = u32::from_le_bytes(bytes[at + 20..at + 24].try_into()?);
            let raw = u32::from_le_bytes(bytes[at + 24..at + 28].try_into()?) as usize;
            if size == 0 {
                continue;
            }
            ensure!(
                raw <= bytes.len() && size <= bytes.len() - raw,
                "debug payload beyond file"
            );
            if address != 0 {
                ensure!(
                    image.file_offset(address, size)? == raw,
                    "debug RVA/file pointer mismatch"
                );
                r.add(address, size, "debug payload")?;
            } else {
                if let Some(address) = image.rva_for_file(raw, size)? {
                    r.add(address, size, "debug payload")?;
                }
            }
        }
    }
    Ok(Metadata {
        ranges: r.ranges,
        imports,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    fn original() -> Vec<u8> {
        let repo = std::env::var_os("RECON_TOOL_TEST_REPO")
            .map(std::path::PathBuf::from)
            .unwrap_or_else(|| std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../.."));
        std::fs::read(repo.join("toolchains/5.10/bin/RCDLL.DLL")).unwrap()
    }
    fn info() -> crate::abi::Info {
        crate::abi::Info {
            binary: "test".into(),
            original_path: "".into(),
            original_sha256: "".into(),
            original_entry_rva: 0,
            case: "test".into(),
            description: "".into(),
            execution: 0,
            terminal: 0,
            frontier_rva: 0,
            first_entry_order: vec![],
            first_entry_seen: vec![],
            functions: vec![],
            tests: vec![],
            module: "".into(),
        }
    }
    #[test]
    fn executable_rcdll_loader_data_is_preserved_but_function_code_is_not() {
        let bytes = original();
        let image = Image::parse(&bytes).unwrap();
        let metadata = parse(&image, &bytes).unwrap();
        let allowed = [
            (0x1000, 0x1124),
            (0x1130, 0x114c),
            (0x12e2c, 0x1331d),
            (0x13320, 0x13371),
        ];
        assert!(
            metadata
                .ranges
                .iter()
                .all(|r| allowed.iter().any(|&(a, b)| a <= r.start && r.end <= b)),
            "{:?}",
            metadata.ranges
        );
        let masked = image.mask(&bytes, &info()).unwrap();
        for r in &metadata.ranges {
            let at = image
                .file_offset(r.start, (r.end - r.start) as usize)
                .unwrap();
            assert_eq!(
                &masked[at..at + (r.end - r.start) as usize],
                &bytes[at..at + (r.end - r.start) as usize]
            );
        }
        for rva in [0x2690, 0xbe41, 0x12d20] {
            assert_eq!(masked[image.file_offset(rva, 1).unwrap()], 0xcc);
        }
        // Alignment bytes do not acquire invented loader ownership.
        assert_eq!(masked[image.file_offset(0x1124, 1).unwrap()], 0xcc);
        let mut conflict = info();
        conflict.functions.push(crate::abi::FunctionInfo {
            name: "conflict".into(),
            rva: 0x1000,
            native_symbol: Some("conflict".into()),
            original_symbol: None,
            import_dll: None,
            import_name: None,
            import_ordinal: 0,
            definition: 4,
            implementation: 0,
            code: vec![],
            text_data: vec![],
        });
        assert!(
            image
                .mask(&bytes, &conflict)
                .unwrap_err()
                .to_string()
                .contains("overlaps application")
        );
        conflict.functions[0].rva = 0x2000;
        conflict.functions[0].code = vec![[0x1001, 0x1002]];
        assert!(image.mask(&bytes, &conflict).is_err());
    }
    #[test]
    fn malformed_loader_pointers_and_tables_fail_closed() {
        let base = original();
        let pe = u32::from_le_bytes(base[0x3c..0x40].try_into().unwrap()) as usize;
        for (at, value) in [
            (pe + 120 + 12 * 8 + 4, 3),
            (0x12e2c + 12, u32::MAX),
            (0x13320 + 20, u32::MAX),
            (0x1130 + 24, u32::MAX),
        ] {
            let mut bytes = base.clone();
            bytes[at..at + 4].copy_from_slice(&value.to_le_bytes());
            let image = Image::parse(&bytes).unwrap();
            assert!(parse(&image, &bytes).is_err(), "offset {at:x}");
        }
    }
}
