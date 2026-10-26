//! Finite typed virtual-call normal-return preservation; never target/value discovery.
use crate::{
    decode::{Inst, Op},
    flow::Graph,
    imports::Catalog,
    index::Record,
    pe::Pe,
    register_imports::Side,
};
use anyhow::{Context, Result, ensure};
use capstone::arch::x86::X86Reg::X86_REG_ECX;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::{BTreeMap, BTreeSet};

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Contract {
    pub owner: String,
    pub original: Location,
    pub rebuilt: Location,
    abi: String,
    stack_argument_bytes: u32,
    preserves: Vec<String>,
    evidence: Value,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Location {
    pub site: u32,
    slot: u32,
    receiver_global: u32,
    receiver_load_site: u32,
    this_load_site: u32,
    vptr_load_site: u32,
    vtables: Vec<Table>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Table {
    address: u32,
    target: String,
}
#[derive(Serialize)]
struct Binding {
    address: u32,
    slot_cell: u32,
    target: String,
    target_address: u32,
}
#[derive(Serialize)]
pub struct Report {
    slot: u32,
    receiver_global: u32,
    receiver_load_site: u32,
    this_load_site: u32,
    vptr_load_site: u32,
    abi: String,
    preserves: Vec<String>,
    vtables: Vec<Binding>,
    evidence: Value,
    meaning: &'static str,
}
pub struct Qualified<'a> {
    pub names: &'a BTreeMap<String, u32>,
    pub masks: &'a BTreeMap<u32, [bool; 4]>,
}
fn readable(pe: &Pe, start: u32, size: u32) -> bool {
    start.checked_add(size).is_some_and(|end| {
        pe.sections.iter().any(|s| {
            s.flags & 0x40000000 != 0
                && s.start <= start
                && s.start
                    .checked_add(s.virtual_size)
                    .is_some_and(|limit| end <= limit)
        })
    })
}
fn absolute_load(i: &Inst, global: u32) -> Option<u16> {
    if i.m != "mov" || i.address_size != 4 {
        return None;
    }
    match i.ops.as_slice() {
        [(Op::Reg(reg), 4), (Op::Mem(m), 4)]
            if m.segment == 0 && m.base == 0 && m.index == 0 && m.disp == global =>
        {
            Some(*reg)
        }
        _ => None,
    }
}
fn instruction<'a>(owner: &Record, g: &'a Graph, site: u32) -> Result<&'a Inst> {
    ensure!(
        owner.contains_code(site),
        "virtual preservation site outside owner"
    );
    g.instructions
        .get(&site)
        .context("virtual preservation site absent from owner CFG")
}
impl Contract {
    pub fn verify(&self, project: &std::path::Path) -> Result<()> {
        ensure!(
            self.abi == "thiscall" && self.stack_argument_bytes == 0 && self.preserves == ["edi"],
            "virtual preservation supports only explicit thiscall/no-stack-arguments EDI normal-return ABI"
        );
        crate::register_imports::verified_evidence(project, &self.evidence)?;
        let targets = |loc: &Location| -> Result<BTreeSet<String>> {
            ensure!(
                loc.slot.is_multiple_of(4)
                    && loc.receiver_global.is_multiple_of(4)
                    && !loc.vtables.is_empty(),
                "virtual preservation requires aligned slot/receiver and finite vtables"
            );
            let mut tables = BTreeSet::new();
            let mut names = BTreeSet::new();
            for t in &loc.vtables {
                ensure!(
                    t.address.is_multiple_of(4)
                        && tables.insert(t.address)
                        && !t.target.is_empty()
                        && names.insert(t.target.clone()),
                    "duplicate/invalid virtual preservation vtable or target"
                );
            }
            Ok(names)
        };
        ensure!(
            targets(&self.original)? == targets(&self.rebuilt)?,
            "virtual preservation original/rebuilt target sets differ"
        );
        Ok(())
    }
    pub fn report(
        &self,
        owner: &Record,
        g: &Graph,
        pe: &Pe,
        side: Side,
        qualified: &Qualified<'_>,
        catalog: &Catalog,
    ) -> Result<Report> {
        let loc = match side {
            Side::Original => &self.original,
            Side::Rebuilt => &self.rebuilt,
        };
        ensure!(
            readable(pe, loc.receiver_global, 4),
            "virtual receiver global is not mapped readable memory"
        );
        ensure!(
            !catalog
                .slots
                .keys()
                .any(|a| u64::from(*a) < u64::from(loc.receiver_global) + 4
                    && u64::from(loc.receiver_global) < u64::from(*a) + 4),
            "virtual receiver global overlaps IAT"
        );
        let receiver = absolute_load(
            instruction(owner, g, loc.receiver_load_site)?,
            loc.receiver_global,
        )
        .context("virtual receiver load differs from declared global")?;
        ensure!(
            absolute_load(
                instruction(owner, g, loc.this_load_site)?,
                loc.receiver_global
            ) == Some(X86_REG_ECX as u16),
            "virtual thiscall ECX load differs from receiver global"
        );
        let vptr = instruction(owner, g, loc.vptr_load_site)?;
        let table_register = match vptr.ops.as_slice() {
            [(Op::Reg(reg), 4), (Op::Mem(m), 4)]
                if vptr.m == "mov"
                    && vptr.address_size == 4
                    && m.segment == 0
                    && m.base == receiver
                    && m.index == 0
                    && m.disp == 0 =>
            {
                *reg
            }
            _ => anyhow::bail!("virtual vptr load differs from decoded receiver"),
        };
        ensure!(
            table_register != X86_REG_ECX as u16
                && loc.receiver_load_site < loc.vptr_load_site
                && loc.vptr_load_site < loc.site
                && loc.this_load_site < loc.site,
            "virtual receiver/vptr/this sequence is not ordered"
        );
        let call = instruction(owner, g, loc.site)?;
        ensure!(
            g.indirect_calls.contains(&loc.site)
                && call.m == "call"
                && call.bytes.first() == Some(&0xff)
                && call.address_size == 4,
            "virtual preservation requires discovered opaque unprefixed memory CALL"
        );
        ensure!(
            matches!(call.ops.as_slice(),[(Op::Mem(m),4)] if m.segment==0 && m.base==table_register && m.index==0 && m.disp==loc.slot),
            "virtual call table register/slot differs"
        );
        // Prove the actual finite receiver chain, rather than assume a receiver
        // register survives a join or unknown instruction. Other intervening
        // instructions may only store to disjoint absolute globals.
        let mut pc = loc.receiver_load_site.min(loc.this_load_site);
        let mut previous = None;
        let declared = BTreeSet::from([
            loc.receiver_load_site,
            loc.this_load_site,
            loc.vptr_load_site,
        ]);
        let mut consumed = BTreeSet::new();
        loop {
            let i = instruction(owner, g, pc)?;
            if let Some(prev) = previous {
                let predecessors: BTreeSet<_> = g
                    .successors
                    .iter()
                    .filter(|(_, to)| to.contains(&pc))
                    .map(|(from, _)| *from)
                    .collect();
                ensure!(
                    !g.roots.contains(&pc) && predecessors == BTreeSet::from([prev]),
                    "virtual receiver chain has a bypass/join"
                );
            }
            if pc == loc.site {
                break;
            }
            if declared.contains(&pc) {
                consumed.insert(pc);
            } else {
                let safe_store = match i.ops.as_slice() {
                    [(Op::Mem(m), size), _]
                        if i.m == "mov"
                            && i.address_size == 4
                            && m.segment == 0
                            && m.base == 0
                            && m.index == 0 =>
                    {
                        m.disp.checked_add(u32::from(*size)).is_some_and(|end| {
                            *size > 0
                                && (end <= loc.receiver_global || m.disp >= loc.receiver_global + 4)
                                && pe.sections.iter().any(|s| {
                                    s.flags & 0x80000000 != 0
                                        && s.start <= m.disp
                                        && s.start
                                            .checked_add(s.virtual_size)
                                            .is_some_and(|limit| end <= limit)
                                })
                                && catalog.slots.keys().all(|a| {
                                    u64::from(*a) >= u64::from(end)
                                        || u64::from(m.disp) >= u64::from(*a) + 4
                                })
                                && loc.vtables.iter().all(|t| {
                                    t.address.checked_add(loc.slot).is_some_and(|cell| {
                                        u64::from(cell) >= u64::from(end)
                                            || u64::from(m.disp) >= u64::from(cell) + 4
                                    })
                                })
                        })
                    }
                    _ => false,
                };
                ensure!(
                    safe_store,
                    "virtual receiver chain contains an unqualified clobber/store"
                );
            }
            ensure!(
                !i.control_transfer
                    && i.next() <= loc.site
                    && g.successors.get(&pc) == Some(&BTreeSet::from([i.next()])),
                "virtual receiver chain does not fall through directly to CALL"
            );
            previous = Some(pc);
            pc = i.next();
        }
        ensure!(
            consumed == declared,
            "virtual receiver chain declarations not consumed"
        );
        let mut bindings = Vec::new();
        for table in &loc.vtables {
            let address = *qualified
                .names
                .get(&table.target)
                .context("virtual target lacks explicit named preservation contract")?;
            ensure!(
                qualified.masks.get(&address).is_some_and(|mask| mask[2]),
                "virtual target lacks qualified EDI preservation"
            );
            let cell = table
                .address
                .checked_add(loc.slot)
                .context("virtual slot overflow")?;
            ensure!(
                readable(pe, cell, 4)
                    && !catalog
                        .slots
                        .keys()
                        .any(|a| u64::from(*a) < u64::from(cell) + 4
                            && u64::from(cell) < u64::from(*a) + 4),
                "virtual slot is unreadable or overlaps IAT"
            );
            ensure!(
                pe.word(cell)? == address,
                "virtual slot target differs from qualified function"
            );
            bindings.push(Binding {
                address: table.address,
                slot_cell: cell,
                target: table.target.clone(),
                target_address: address,
            });
        }
        Ok(Report {
            slot: loc.slot,
            receiver_global: loc.receiver_global,
            receiver_load_site: loc.receiver_load_site,
            this_load_site: loc.this_load_site,
            vptr_load_site: loc.vptr_load_site,
            abi: self.abi.clone(),
            preserves: self.preserves.clone(),
            vtables: bindings,
            evidence: self.evidence.clone(),
            meaning: "retain only existing EDI must-value on normal return; virtual call stays opaque; no target/value seed, body availability or permission",
        })
    }
}
