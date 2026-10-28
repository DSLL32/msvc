use anyhow::{Context, Result, bail, ensure};
use serde::Serialize;
use std::{fs, path::Path};

pub fn u16_at(b: &[u8], o: usize) -> Result<u16> {
    Ok(u16::from_le_bytes(
        b.get(o..o + 2).context("short u16")?.try_into()?,
    ))
}
pub fn u32_at(b: &[u8], o: usize) -> Result<u32> {
    Ok(u32::from_le_bytes(
        b.get(o..o + 4).context("short u32")?.try_into()?,
    ))
}
#[derive(Clone, Debug, Serialize)]
pub struct Section {
    pub name: String,
    pub start: u32,
    pub size: u32,
    pub virtual_size: u32,
    pub off: usize,
    pub flags: u32,
}
impl Section {
    pub fn is_code(&self) -> bool {
        // TLINK32 marks CODE executable without IMAGE_SCN_CNT_CODE.
        self.flags & 0x20000020 != 0
    }
}
#[derive(Clone)]
pub struct Pe {
    pub data: Vec<u8>,
    pub base: u32,
    pub entry: u32,
    pub optional: usize,
    pub sections: Vec<Section>,
    zeros: Vec<u8>,
}
impl Pe {
    pub fn open(path: &Path) -> Result<Self> {
        Self::parse(fs::read(path).with_context(|| format!("read {}", path.display()))?)
    }
    pub fn parse(data: Vec<u8>) -> Result<Self> {
        ensure!(data.get(..2) == Some(b"MZ"), "not MZ");
        let p = u32_at(&data, 60)? as usize;
        ensure!(data.get(p..p + 4) == Some(b"PE\0\0"), "not PE");
        let optional = p + 24;
        ensure!(u16_at(&data, optional)? == 0x10b, "requires PE32");
        let base = u32_at(&data, optional + 28)?;
        let entry = base
            .checked_add(u32_at(&data, optional + 16)?)
            .context("entry overflow")?;
        let first = optional + u16_at(&data, p + 20)? as usize;
        let mut sections = Vec::new();
        for n in 0..u16_at(&data, p + 6)? as usize {
            let q = first + 40 * n;
            let name = data.get(q..q + 8).context("short section")?;
            let size = u32_at(&data, q + 16)?;
            let vs = u32_at(&data, q + 8)?;
            sections.push(Section {
                name: String::from_utf8_lossy(name).trim_end_matches('\0').into(),
                start: base
                    .checked_add(u32_at(&data, q + 12)?)
                    .context("section address overflow")?,
                size,
                virtual_size: if vs == 0 { size } else { vs },
                off: u32_at(&data, q + 20)? as usize,
                flags: u32_at(&data, q + 36)?,
            });
        }
        let zeros = vec![
            0;
            sections
                .iter()
                .filter(|s| s.off == 0 || s.flags & 0x80 != 0)
                .map(|s| s.virtual_size as usize)
                .max()
                .unwrap_or(0)
        ];
        Ok(Self {
            zeros,
            data,
            base,
            entry,
            optional,
            sections,
        })
    }
    pub fn file_backed_code(&self, start: u32, end: u32) -> bool {
        start < end
            && self.sections.iter().any(|s| {
                s.is_code()
                    && s.off != 0
                    && s.start <= start
                    && u64::from(end) <= u64::from(s.start) + u64::from(s.virtual_size.min(s.size))
                    && (s.off as u64) + u64::from(end - s.start) <= self.data.len() as u64
            })
    }
    pub fn read(&self, a: u32, n: usize) -> Result<&[u8]> {
        for s in &self.sections {
            if (s.off == 0 || s.flags & 0x80 != 0)
                && a >= s.start
                && a as u64 + n as u64 <= s.start as u64 + s.virtual_size as u64
            {
                return self
                    .zeros
                    .get(..n)
                    .context("zero section read exceeds backing buffer");
            }
        }
        // Reads crossing raw section boundaries match the Python file slice reader.
        for s in &self.sections {
            if s.off != 0
                && s.flags & 0x80 == 0
                && a >= s.start
                && (a as u64) < s.start as u64 + s.size as u64
            {
                let p = s.off + (a - s.start) as usize;
                return self
                    .data
                    .get(p..(p + n).min(self.data.len()))
                    .context("unbacked PE file range");
            }
        }
        bail!("unbacked address {a:08x}")
    }
    pub fn word(&self, a: u32) -> Result<u32> {
        u32_at(self.read(a, 4)?, 0)
    }
    pub fn string(&self, a: u32) -> Result<String> {
        let section = self
            .sections
            .iter()
            .find(|s| {
                s.off != 0
                    && s.flags & 0x80 == 0
                    && s.start <= a
                    && (a as u64) < s.start as u64 + s.size as u64
            })
            .context("unbacked PE string")?;
        let p = section.off + (a - section.start) as usize;
        let tail = self.data.get(p..).context("PE string offset beyond file")?;
        let n = tail
            .iter()
            .position(|v| *v == 0)
            .context("unterminated PE string")?;
        Ok(tail[..n].iter().map(|v| char::from(*v)).collect())
    }
    pub fn code_end(&self) -> Result<u32> {
        self.sections
            .iter()
            .filter(|s| s.is_code())
            .map(|s| s.start + s.virtual_size)
            .max()
            .context("no code")
    }
}
