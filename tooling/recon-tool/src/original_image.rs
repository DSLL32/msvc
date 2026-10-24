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
