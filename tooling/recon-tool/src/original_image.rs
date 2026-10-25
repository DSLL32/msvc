use crate::abi::Info;
use anyhow::{Context, Result, bail};
pub struct Image {
    pub entry: u32,
    pub dll: bool,
    sections: Vec<(u32, usize, usize, bool)>,
}
fn u16_at(b: &[u8], o: usize) -> Result<u16> {
    Ok(u16::from_le_bytes(
        b.get(o..o + 2).context("truncated PE")?.try_into()?,
    ))
}
fn u32_at(b: &[u8], o: usize) -> Result<u32> {
    Ok(u32::from_le_bytes(
        b.get(o..o + 4).context("truncated PE")?.try_into()?,
    ))
}
impl Image {
    pub fn parse(b: &[u8]) -> Result<Self> {
        anyhow::ensure!(b.get(..2) == Some(b"MZ"), "original is not a PE image");
        let pe = u32_at(b, 0x3c)? as usize;
        anyhow::ensure!(
            b.get(pe..pe + 4) == Some(b"PE\0\0")
                && u16_at(b, pe + 4)? == 0x14c
                && u16_at(b, pe + 24)? == 0x10b,
            "original must be i386 PE32"
        );
        let count = u16_at(b, pe + 6)? as usize;
        let optional = u16_at(b, pe + 20)? as usize;
        let table = pe + 24 + optional;
        let mut sections = vec![];
        for i in 0..count {
            let s = table + i * 40;
            let rva = u32_at(b, s + 12)?;
            let size = u32_at(b, s + 16)? as usize;
            let offset = u32_at(b, s + 20)? as usize;
            let flags = u32_at(b, s + 36)?;
            anyhow::ensure!(
                offset <= b.len() && size <= b.len() - offset,
                "PE section beyond file"
            );
            sections.push((rva, offset, size, flags & 0x20000020 != 0));
        }
        Ok(Self {
            entry: u32_at(b, pe + 24 + 16)?,
            dll: u16_at(b, pe + 22)? & 0x2000 != 0,
            sections,
        })
    }
    pub fn file_offset(&self, rva: u32, size: usize) -> Result<usize> {
        let matches: Vec<_> = self
            .sections
            .iter()
            .filter(|(start, _, len, _)| {
                rva >= *start && (rva as u64) + size as u64 <= *start as u64 + *len as u64
            })
            .collect();
        anyhow::ensure!(
            matches.len() == 1,
            "RVA must belong to one initialized section"
        );
        let (start, offset, _, _) = matches[0];
        Ok(offset + (rva - start) as usize)
    }
    /// Export RVAs for an explicitly supplied typed helper DLL.
    pub fn exports(&self, b: &[u8]) -> Result<std::collections::HashMap<String, u32>> {
        let pe = u32_at(b, 0x3c)? as usize;
        let directory = u32_at(b, pe + 24 + 96)?;
        let directory_size = u32_at(b, pe + 24 + 100)?;
        anyhow::ensure!(
            self.dll && directory != 0,
            "typed helper must be a DLL with exports"
        );
        let at = |rva: u32, size: usize| -> Result<usize> {
            let offset = self.file_offset(rva, size)?;
            anyhow::ensure!(
                offset.checked_add(size).is_some_and(|end| end <= b.len()),
                "helper export overflow"
            );
            Ok(offset)
        };
        let table = at(directory, 40)?;
        let functions = u32_at(b, table + 20)?;
        let names = u32_at(b, table + 24)?;
        anyhow::ensure!(
            functions <= 100000 && names <= functions,
            "invalid helper export counts"
        );
        let function_table = at(u32_at(b, table + 28)?, functions as usize * 4)?;
        let name_table = at(u32_at(b, table + 32)?, names as usize * 4)?;
        let ordinals = at(u32_at(b, table + 36)?, names as usize * 2)?;
        let mut exports = std::collections::HashMap::new();
        for i in 0..names as usize {
            let name_rva = u32_at(b, name_table + 4 * i)?;
            let mut name = vec![];
            for n in 0..4096u32 {
                let byte = b[at(name_rva.checked_add(n).context("export name overflow")?, 1)?];
                if byte == 0 {
                    break;
                }
                name.push(byte);
            }
            anyhow::ensure!(!name.is_empty() && name.len() < 4096, "invalid export name");
            let ordinal = u16_at(b, ordinals + 2 * i)? as u32;
            anyhow::ensure!(
                ordinal < functions,
                "helper export ordinal outside functions"
            );
            let rva = u32_at(b, function_table + 4 * ordinal as usize)?;
            anyhow::ensure!(
                !(directory <= rva && (rva as u64) < directory as u64 + directory_size as u64),
                "forwarded helper exports unsupported"
            );
            let _ = at(rva, 1)?;
            anyhow::ensure!(
                exports.insert(String::from_utf8(name)?, rva).is_none(),
                "duplicate helper export"
            );
        }
        Ok(exports)
    }
    pub fn mask(&self, original: &[u8], info: &Info) -> Result<Vec<u8>> {
        let mut b = original.to_vec();
        for &(_, off, len, code) in &self.sections {
            if code {
                b[off..off + len].fill(0xcc);
            }
        }
        for f in &info.functions {
            if !(f.definition == 4 && f.implementation == 1 || matches!(f.definition, 1..=3)) {
                continue;
            }
            for &[start, end] in f.code.iter().chain(&f.text_data) {
                let (rva, off, len, _) = self
                    .sections
                    .iter()
                    .find(|(r, _, len, _)| start >= *r && (end as u64) <= *r as u64 + *len as u64)
                    .context("unmask extent outside initialized PE section")?;
                let offset = off + (start - rva) as usize;
                let n = (end - start) as usize;
                if offset + n > off + len {
                    bail!("unmask overflow")
                }
                b[offset..offset + n].copy_from_slice(&original[offset..offset + n]);
            }
        }
        Ok(b)
    }
}
