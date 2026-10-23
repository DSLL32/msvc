//! Optional standard PE32 import identity discovery. Delay imports, forwarded
//! exports and arbitrary function pointers are not guessed or loader-resolved.
use crate::{
    decode::Inst,
    pe::{Pe, u32_at},
};
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
#[serde(untagged, deny_unknown_fields)]
pub enum Symbol {
    Name { name: String },
    Ordinal { ordinal: u16 },
}
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
pub struct Identity {
    pub dll: String,
    #[serde(flatten)]
    pub symbol: Symbol,
}
impl Identity {
    pub fn legacy_name(&self) -> String {
        match &self.symbol {
            Symbol::Name { name } => name.clone(),
            Symbol::Ordinal { ordinal } => format!("ordinal_{ordinal}"),
        }
    }
}
#[derive(Clone, Default)]
pub struct Catalog {
    pub slots: BTreeMap<u32, Identity>,
    pub thunks: BTreeMap<u32, Identity>,
}
struct Reader<'a>(&'a Pe);
impl Reader<'_> {
    fn bytes(&self, address: u32, size: usize) -> Result<&[u8]> {
        let section = self
            .0
            .sections
            .iter()
            .find(|s| {
                s.off != 0
                    && s.flags & 0x80 == 0
                    && s.start <= address
                    && u64::from(address) + size as u64
                        <= u64::from(s.start) + u64::from(s.size.min(s.virtual_size))
            })
            .context("import range is not file-backed")?;
        let offset = section
            .off
            .checked_add((address - section.start) as usize)
            .context("import file offset overflow")?;
        self.0
            .data
            .get(offset..offset.checked_add(size).context("import range overflow")?)
            .context("truncated import file range")
    }
    fn va(&self, rva: u32) -> Result<u32> {
        self.0.base.checked_add(rva).context("import RVA overflow")
    }
    fn word(&self, address: u32) -> Result<u32> {
        u32_at(self.bytes(address, 4)?, 0)
    }
    fn string(&self, address: u32) -> Result<String> {
        let section = self
            .0
            .sections
            .iter()
            .find(|s| {
                s.off != 0
                    && s.flags & 0x80 == 0
                    && s.start <= address
                    && u64::from(address)
                        < u64::from(s.start) + u64::from(s.size.min(s.virtual_size))
            })
            .context("import string outside file-backed section")?;
        let count = (u64::from(section.start) + u64::from(section.size.min(section.virtual_size))
            - u64::from(address)) as usize;
        let bytes = self.bytes(address, count)?;
        let length = bytes
            .iter()
            .position(|b| *b == 0)
            .context("unterminated import string")?;
        ensure!(
            length > 0 && bytes[..length].is_ascii(),
            "empty/non-ASCII import string"
        );
        Ok(std::str::from_utf8(&bytes[..length])?.into())
    }
}
impl Catalog {
    pub fn load(pe: &Pe) -> Result<Self> {
        let read = Reader(pe);
        let optional_size = crate::pe::u16_at(&pe.data, pe.optional - 4)? as usize;
        ensure!(
            optional_size >= 96,
            "PE optional header lacks data directory count"
        );
        if u32_at(&pe.data, pe.optional + 92)? < 2 {
            return Ok(Self::default());
        }
        ensure!(
            optional_size >= 112,
            "PE optional header truncates import directory"
        );
        let rva = u32_at(&pe.data, pe.optional + 104)?;
        let size = u32_at(&pe.data, pe.optional + 108)?;
        if rva == 0 {
            ensure!(size == 0, "import size without directory");
            return Ok(Self::default());
        }
        ensure!(size >= 20, "truncated import directory");
        let begin = read.va(rva)?;
        let end = begin
            .checked_add(size)
            .context("import directory overflow")?;
        read.bytes(begin, size as usize)?;
        let mut slots = BTreeMap::new();
        let mut descriptor = begin;
        let mut terminated = false;
        while u64::from(descriptor) + 20 <= u64::from(end) {
            let data = read.bytes(descriptor, 20)?;
            if data.iter().all(|b| *b == 0) {
                terminated = true;
                break;
            }
            let lookup = u32_at(data, 0)?;
            let dll_rva = u32_at(data, 12)?;
            let iat_rva = u32_at(data, 16)?;
            ensure!(dll_rva != 0 && iat_rva != 0, "incomplete import descriptor");
            let provider = read.string(read.va(dll_rva)?)?;
            let dll = provider
                .rsplit(['\\', '/'])
                .next()
                .context("DLL basename")?
                .to_ascii_lowercase();
            ensure!(!dll.is_empty(), "empty DLL basename");
            let mut lookup = read.va(if lookup == 0 { iat_rva } else { lookup })?;
            let mut slot = read.va(iat_rva)?;
            loop {
                let value = read.word(lookup)?;
                read.bytes(slot, 4)?;
                if value == 0 {
                    ensure!(
                        read.word(slot)? == 0,
                        "IAT terminator differs from lookup terminator"
                    );
                    break;
                }
                let symbol = if value & 0x80000000 != 0 {
                    ensure!(value & 0x7fff0000 == 0, "reserved ordinal bits are set");
                    Symbol::Ordinal {
                        ordinal: value as u16,
                    }
                } else {
                    let hint = read.va(value)?;
                    read.bytes(hint, 2)?;
                    Symbol::Name {
                        name: read.string(hint.checked_add(2).context("hint/name overflow")?)?,
                    }
                };
                ensure!(
                    slots
                        .insert(
                            slot,
                            Identity {
                                dll: dll.clone(),
                                symbol
                            }
                        )
                        .is_none(),
                    "overlapping import address tables"
                );
                lookup = lookup.checked_add(4).context("lookup address overflow")?;
                slot = slot.checked_add(4).context("IAT address overflow")?;
            }
            descriptor = descriptor
                .checked_add(20)
                .context("import descriptor overflow")?;
        }
        ensure!(terminated, "unterminated import directory");
        let mut thunks = BTreeMap::new();
        for section in pe
            .sections
            .iter()
            .filter(|s| s.flags & 0x20 != 0 && s.off != 0)
        {
            let length = section.size.min(section.virtual_size) as usize;
            let code = read.bytes(section.start, length)?;
            for (offset, bytes) in code.windows(6).enumerate() {
                if bytes[..2] == [0xff, 0x25]
                    && let Some(identity) = slots.get(&u32_at(bytes, 2)?)
                {
                    thunks.insert(
                        section
                            .start
                            .checked_add(offset as u32)
                            .context("thunk address overflow")?,
                        identity.clone(),
                    );
                }
            }
        }
        Ok(Self { slots, thunks })
    }
    pub fn absolute_transfer(&self, instruction: &Inst) -> Option<&Identity> {
        // Prefixes can change the address space (notably FS); never interpret
        // such a transfer as an absolute image IAT access by byte resemblance.
        if instruction.bytes.len() != 6
            || ![[0xff, 0x15], [0xff, 0x25]].contains(&[instruction.bytes[0], instruction.bytes[1]])
        {
            return None;
        }
        let memory = instruction.mem()?;
        if memory.base != 0 || memory.index != 0 {
            return None;
        }
        self.slots.get(&memory.disp)
    }
}
