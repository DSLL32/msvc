use crate::image::ImageView;
use anyhow::{Context, Result};
use capstone::{arch::x86::X86OperandType, prelude::*};
#[derive(Clone, Debug)]
pub struct Mem {
    pub segment: u16,
    pub base: u16,
    pub index: u16,
    pub scale: i32,
    pub disp: u32,
}
#[derive(Clone, Debug)]
pub enum Op {
    Imm(u32),
    Mem(Mem),
    Reg(u16),
    Other,
}
#[derive(Clone, Debug)]
pub struct Inst {
    pub a: u32,
    pub size: u32,
    pub m: String,
    pub text: String,
    pub ops: Vec<(Op, u8)>,
    pub jump: bool,
    pub ret: bool,
    pub bytes: Vec<u8>,
    pub writes: Vec<u16>,
    pub address_size: u8,
}
impl Inst {
    pub fn next(&self) -> u32 {
        self.a + self.size
    }
    pub fn imm(&self) -> Option<u32> {
        if let Some((Op::Imm(v), _)) = self.ops.first() {
            Some(*v)
        } else {
            None
        }
    }
    pub fn mem(&self) -> Option<&Mem> {
        if let Some((Op::Mem(v), _)) = self.ops.first() {
            Some(v)
        } else {
            None
        }
    }
}
pub struct Decoder(Capstone);
impl Decoder {
    pub fn new() -> Result<Self> {
        Ok(Self(
            Capstone::new()
                .x86()
                .mode(arch::x86::ArchMode::Mode32)
                .detail(true)
                .build()?,
        ))
    }
    pub fn one(&self, pe: &impl ImageView, a: u32, limit: usize) -> Result<Option<Inst>> {
        let bytes = pe.read(a, limit)?;
        let insns = self.0.disasm_count(bytes, a as u64, 1)?;
        let Some(i) = insns.first() else {
            return Ok(None);
        };
        let detail = self.0.insn_detail(i)?;
        let arch = detail.arch_detail();
        let x = arch.x86().context("not x86")?;
        let ops = x
            .operands()
            .map(|v| {
                (
                    match v.op_type {
                        X86OperandType::Imm(n) => Op::Imm(n as u32),
                        X86OperandType::Mem(v) => Op::Mem(Mem {
                            segment: v.segment().0,
                            base: v.base().0,
                            index: v.index().0,
                            scale: v.scale(),
                            disp: v.disp() as u32,
                        }),
                        X86OperandType::Reg(r) => Op::Reg(r.0),
                        _ => Op::Other,
                    },
                    v.size,
                )
            })
            .collect();
        Ok(Some(Inst {
            a,
            size: i.bytes().len() as u32,
            m: i.mnemonic().unwrap_or("").into(),
            text: i.op_str().unwrap_or("").into(),
            ops,
            jump: detail.groups().iter().any(|g| g.0 == 1),
            ret: detail.groups().iter().any(|g| g.0 == 3),
            bytes: i.bytes().to_vec(),
            writes: detail.regs_write().iter().map(|r| r.0).collect(),
            address_size: x.addr_size(),
        }))
    }
    pub fn linear(&self, pe: &impl ImageView, start: u32, end: u32) -> Result<Vec<Inst>> {
        let mut out = Vec::new();
        let mut a = start;
        while a < end {
            let Some(i) = self.one(pe, a, (end - a) as usize)? else {
                break;
            };
            a = i.next();
            out.push(i);
        }
        Ok(out)
    }
}
