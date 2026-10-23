//! Shared byte-view interface. Format-specific provenance stays explicit;
//! neither CF bytes nor their owner declarations are synthesized PE sections.
use crate::{
    cf::Cf,
    index::Record,
    pe::{Pe, Section, u32_at},
};
use anyhow::{Context, Result};
use serde_json::{Value, json};
use std::path::Path;
pub trait ImageView {
    fn read(&self, a: u32, n: usize) -> Result<&[u8]>;
    fn file_backed_code(&self, a: u32, b: u32) -> bool;
    fn text_data_backed(&self, a: u32, b: u32) -> bool {
        self.file_backed_code(a, b)
    }
    fn word(&self, a: u32) -> Result<u32> {
        u32_at(self.read(a, 4)?, 0)
    }
    fn pe_sections(&self) -> Option<&[Section]> {
        None
    }
    fn explicit_flat_ownership(&self) -> bool {
        false
    }
}
impl ImageView for Pe {
    fn read(&self, a: u32, n: usize) -> Result<&[u8]> {
        Pe::read(self, a, n)
    }
    fn file_backed_code(&self, a: u32, b: u32) -> bool {
        Pe::file_backed_code(self, a, b)
    }
    fn pe_sections(&self) -> Option<&[Section]> {
        Some(&self.sections)
    }
}
pub enum Image {
    Pe(Pe),
    Cf(Cf),
}
impl Image {
    pub fn open_original(
        path: &Path,
        definition: &Value,
        project: &Path,
        repo: &Path,
    ) -> Result<Self> {
        if definition.is_null() {
            Ok(Self::Pe(Pe::open(path)?))
        } else {
            Ok(Self::Cf(Cf::open(path, definition, project, repo)?))
        }
    }
    pub fn open_pe(path: &Path) -> Result<Self> {
        Ok(Self::Pe(Pe::open(path)?))
    }
    pub fn as_pe(&self) -> Option<&Pe> {
        match self {
            Self::Pe(v) => Some(v),
            Self::Cf(_) => None,
        }
    }
    pub fn require_pe(&self) -> Result<&Pe> {
        self.as_pe().context("operation requires a real PE image")
    }
    pub fn is_cf(&self) -> bool {
        matches!(self, Self::Cf(_))
    }
    pub fn source_bytes(&self) -> &[u8] {
        match self {
            Self::Pe(v) => &v.data,
            Self::Cf(v) => v.source(),
        }
    }
    pub fn bind_code(&mut self, records: &[Record]) -> Result<()> {
        if let Self::Cf(v) = self {
            v.bind_code(records.iter().flat_map(Record::code_ranges))?;
        }
        Ok(())
    }
    pub fn code_end(&self) -> Result<u32> {
        self.require_pe()?.code_end()
    }
    pub fn string(&self, a: u32) -> Result<String> {
        match self {
            Self::Pe(v) => v.string(a),
            Self::Cf(v) => v.string(a),
        }
    }
    pub fn provenance(&self) -> Value {
        match self {
            Self::Pe(v) => {
                json!({"format":"pe32","coordinates":"preferred-va","preferred_base":v.base,"original_file_sha256":crate::index::hash(&v.data)})
            }
            Self::Cf(v) => v.provenance(),
        }
    }
}
impl ImageView for Image {
    fn read(&self, a: u32, n: usize) -> Result<&[u8]> {
        match self {
            Self::Pe(v) => v.read(a, n),
            Self::Cf(v) => v.read(a, n),
        }
    }
    fn file_backed_code(&self, a: u32, b: u32) -> bool {
        match self {
            Self::Pe(v) => v.file_backed_code(a, b),
            Self::Cf(v) => v.file_backed_code(a, b),
        }
    }
    fn text_data_backed(&self, a: u32, b: u32) -> bool {
        match self {
            Self::Pe(v) => v.file_backed_code(a, b),
            Self::Cf(v) => v.text_data_backed(a, b),
        }
    }
    fn pe_sections(&self) -> Option<&[Section]> {
        self.as_pe().map(|v| v.sections.as_slice())
    }
    fn explicit_flat_ownership(&self) -> bool {
        self.is_cf()
    }
}
