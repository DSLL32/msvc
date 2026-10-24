use anyhow::{Context, Result, bail};
use libloading::Library;
use object::{Object, ObjectSymbol};
use serde::{Deserialize, Serialize};
use std::{
    ffi::{CStr, c_char, c_void},
    path::{Path, PathBuf},
    slice,
};
#[repr(C)]
pub struct Extent {
    pub start: u32,
    pub end: u32,
}
#[repr(C)]
pub struct Function {
    pub name: *const c_char,
    pub entry_rva: u32,
    pub code: *const Extent,
    pub code_count: u32,
    pub text_data: *const Extent,
    pub text_data_count: u32,
    pub definition: u32,
    pub implementation: u32,
    pub native_symbol: *const c_char,
    pub import_dll: *const c_char,
    pub import_name: *const c_char,
    pub import_ordinal: u32,
}
#[repr(C)]
pub struct Export {
    pub name: *const c_char,
    pub ordinal: u32,
    pub entry_rva: u32,
}
#[repr(C)]
pub struct Binary {
    pub abi_version: u32,
    pub struct_size: u32,
    pub name: *const c_char,
    pub original_path: *const c_char,
    pub original_sha256: *const c_char,
    pub original_entry_rva: u32,
    pub functions: *const Function,
    pub function_count: u32,
    pub exports: *const Export,
    pub export_count: u32,
}
pub type Check = unsafe extern "C" fn() -> i32;
pub type Io = unsafe extern "C" fn(i32, *mut *mut c_char) -> i32;
#[repr(C)]
pub struct Test {
    pub name: *const c_char,
    pub fork: u32,
    pub exec: Option<Check>,
}
#[repr(C)]
pub struct Case {
    pub abi_version: u32,
    pub struct_size: u32,
    pub name: *const c_char,
    pub description: *const c_char,
    pub first_entry_order: *const u32,
    pub first_entry_count: u32,
    pub first_entry_seen: *const u32,
    pub first_entry_seen_count: u32,
    pub execution: u32,
    pub terminal: u32,
    pub frontier_rva: u32,
    pub setup: Option<Check>,
    pub io: Option<Io>,
    pub direct: Option<Check>,
    pub tests: *const Test,
    pub test_count: u32,
}
pub unsafe fn string(p: *const c_char) -> Result<String> {
    if p.is_null() {
        bail!("required descriptor string is NULL")
    }
    Ok(unsafe { CStr::from_ptr(p) }.to_str()?.to_owned())
}
pub unsafe fn items<'a, T>(p: *const T, n: u32) -> Result<&'a [T]> {
    if n > 1000000 || n > 0 && p.is_null() {
        bail!("invalid descriptor array")
    }
    if n == 0 {
        Ok(&[])
    } else {
        Ok(unsafe { slice::from_raw_parts(p, n as usize) })
    }
}
#[derive(Serialize, Deserialize, Clone)]
pub struct FunctionInfo {
    pub name: String,
    pub rva: u32,
    pub native_symbol: Option<String>,
    pub definition: u32,
    pub implementation: u32,
    pub code: Vec<[u32; 2]>,
    pub text_data: Vec<[u32; 2]>,
}
#[derive(Serialize, Deserialize, Clone)]
pub struct Info {
    pub binary: String,
    pub original_path: String,
    pub original_sha256: String,
    pub original_entry_rva: u32,
    pub case: String,
    pub description: String,
    pub execution: u32,
    pub terminal: u32,
    pub frontier_rva: u32,
    pub first_entry_order: Vec<u32>,
    pub first_entry_seen: Vec<u32>,
    pub functions: Vec<FunctionInfo>,
    pub tests: Vec<String>,
    pub module: PathBuf,
}
pub struct Loaded {
    pub library: Library,
    pub case: &'static Case,
    pub info: Info,
    pub base: usize,
}
impl Loaded {
    pub unsafe fn open(path: &Path) -> Result<Self> {
        let library =
            unsafe { Library::new(path) }.with_context(|| format!("load {}", path.display()))?;
        let binary = unsafe { *library.get::<*const Binary>(b"binary_descriptor\0")? };
        let case = unsafe { *library.get::<*const Case>(b"case_descriptor\0")? };
        let binary = unsafe { binary.as_ref() }.context("NULL binary_descriptor")?;
        let case = unsafe { case.as_ref() }.context("NULL case_descriptor")?;
        if binary.abi_version != 1
            || binary.struct_size as usize != size_of::<Binary>()
            || case.abi_version != 1
            || case.struct_size as usize != size_of::<Case>()
        {
            bail!("descriptor ABI/version/size mismatch")
        }
        if case.execution > 1 || case.terminal > 1 {
            bail!("invalid case execution/terminal")
        }
        if case.execution == 0 && case.io.is_none() || case.execution == 1 && case.direct.is_none()
        {
            bail!("case missing execution callback")
        }
        let mut dl: libc::Dl_info = unsafe { std::mem::zeroed() };
        if unsafe { libc::dladdr(binary as *const _ as *const c_void, &mut dl) } == 0 {
            bail!("cannot resolve binary module")
        }
        let module = PathBuf::from(unsafe { string(dl.dli_fname) }?).canonicalize()?;
        let mut functions = vec![];
        let mut rvas = std::collections::BTreeSet::new();
        let mut native_names = std::collections::BTreeSet::new();
        for f in unsafe { items(binary.functions, binary.function_count) }? {
            if f.definition > 4 || f.implementation > 1 || !rvas.insert(f.entry_rva) {
                bail!("invalid/duplicate function RVA {:x}", f.entry_rva)
            }
            let native_symbol = if f.native_symbol.is_null() {
                None
            } else {
                Some(unsafe { string(f.native_symbol) }?)
            };
            if f.definition == 4 {
                let s = native_symbol
                    .as_ref()
                    .context("App function missing native_symbol")?;
                if !native_names.insert(s.clone()) {
                    bail!("duplicate native symbol: {s}")
                }
            }
            let ranges = |p, n| -> Result<Vec<[u32; 2]>> {
                unsafe { items(p, n) }?
                    .iter()
                    .map(|r: &Extent| {
                        if r.start >= r.end {
                            bail!("empty/inverted extent")
                        }
                        Ok([r.start, r.end])
                    })
                    .collect()
            };
            functions.push(FunctionInfo {
                name: unsafe { string(f.name) }?,
                rva: f.entry_rva,
                native_symbol,
                definition: f.definition,
                implementation: f.implementation,
                code: ranges(f.code, f.code_count)?,
                text_data: ranges(f.text_data, f.text_data_count)?,
            });
        }
        let order = unsafe { items(case.first_entry_order, case.first_entry_count) }?.to_vec();
        let mut seen = std::collections::BTreeSet::new();
        for rva in &order {
            if !seen.insert(rva) || !functions.iter().any(|f| f.rva == *rva && f.definition == 4) {
                bail!("invalid/duplicate first-entry RVA {rva:x}")
            }
        }
        let first_entry_seen =
            unsafe { items(case.first_entry_seen, case.first_entry_seen_count) }?.to_vec();
        for rva in &first_entry_seen {
            if !seen.insert(rva) || !functions.iter().any(|f| f.rva == *rva && f.definition == 4) {
                bail!("invalid/duplicate seeded App RVA {rva:x}")
            }
        }
        if case.terminal == 1
            && !functions
                .iter()
                .any(|f| f.rva == case.frontier_rva && f.definition == 4 && f.implementation == 0)
        {
            bail!("frontier must name an unimplemented App function")
        }
        let mut tests = vec![];
        for t in unsafe { items(case.tests, case.test_count) }? {
            if t.exec.is_none() || t.fork > 1 {
                bail!("invalid unit test")
            };
            tests.push(unsafe { string(t.name) }?)
        }
        let info = Info {
            binary: unsafe { string(binary.name) }?,
            original_path: unsafe { string(binary.original_path) }?,
            original_sha256: unsafe { string(binary.original_sha256) }?,
            original_entry_rva: binary.original_entry_rva,
            case: unsafe { string(case.name) }?,
            description: unsafe { string(case.description) }?,
            execution: case.execution,
            terminal: case.terminal,
            frontier_rva: case.frontier_rva,
            first_entry_order: order,
            first_entry_seen,
            functions,
            tests,
            module,
        };
        Ok(Self {
            library,
            case,
            info,
            base: dl.dli_fbase as usize,
        })
    }
    pub fn symbols(&self) -> Result<Vec<(FunctionInfo, usize)>> {
        let bytes = std::fs::read(&self.info.module)?;
        let elf = object::File::parse(bytes.as_slice())?;
        if elf.architecture() != object::Architecture::I386 {
            bail!("binary module is not i386")
        }
        let mut index = std::collections::HashMap::<&str, Vec<u64>>::new();
        for symbol in elf.symbols() {
            if symbol.is_definition() && symbol.kind() == object::SymbolKind::Text {
                index
                    .entry(symbol.name()?)
                    .or_default()
                    .push(symbol.address());
            }
        }
        let mut out = vec![];
        for f in self.info.functions.iter().filter(|f| f.definition == 4) {
            let name = f.native_symbol.as_ref().unwrap();
            let matches = index.get(name.as_str()).map(Vec::as_slice).unwrap_or(&[]);
            if matches.len() != 1 {
                bail!("native symbol {name:?} has {} definitions", matches.len())
            }
            out.push((f.clone(), self.base + matches[0] as usize));
        }
        Ok(out)
    }
}
