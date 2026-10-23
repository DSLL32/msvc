//! Shared instruction-level CFG discovery. Legacy comparison policies live in
//! audit.rs, not here; roots/switches/data are explicit input metadata.
use crate::{
    decode::{Decoder, Inst, Op},
    pe::Pe,
};
use anyhow::{Context, Result, ensure};
use std::collections::{BTreeMap, BTreeSet, HashMap};
#[derive(Clone, Default)]
pub struct Options {
    pub roots: Vec<u32>,
    pub data: Vec<(u32, u32)>,
    pub switches: BTreeMap<u32, Vec<u32>>,
    pub infer_switches: bool,
    pub infer_seh: bool,
    pub strict: bool,
    pub forbid_overlapping_instructions: bool,
    pub stop_on_software_interrupts: bool,
    pub allow_extent_end: bool,
    pub fallthrough_entry: Option<u32>,
}
#[derive(Default)]
pub struct Graph {
    pub calls: BTreeSet<u32>,
    pub references: BTreeSet<u32>,
    pub returns: BTreeSet<u32>,
    pub indirect: bool,
    pub unresolved_tail: bool,
    pub tails: BTreeSet<u32>,
    pub instructions: BTreeMap<u32, Inst>,
    pub covered: usize,
    pub tables: Vec<(u32, u32)>,
}
pub fn walk(d: &Decoder, pe: &Pe, start: u32, end: u32, options: &Options) -> Result<Graph> {
    ensure!(start < end, "invalid function extent");
    let inside = |a| start <= a && a < end;
    let is_data = |a| options.data.iter().any(|(s, e)| *s <= a && a < *e);
    let mut pending = vec![start];
    pending.extend(&options.roots);
    let mut owners = HashMap::new();
    let mut out = Graph::default();
    while let Some(a) = pending.pop() {
        if out.instructions.contains_key(&a) {
            continue;
        }
        ensure!(inside(a), "CFG target outside extent: {a:x}");
        ensure!(!is_data(a), "CFG enters text data");
        ensure!(
            !options.forbid_overlapping_instructions || !owners.contains_key(&a),
            "CFG enters operand at {a:x}"
        );
        let i = d
            .one(
                pe,
                a,
                if options.strict {
                    (end - a).min(15) as usize
                } else {
                    15
                },
            )?
            .with_context(|| format!("invalid/truncated instruction at {a:x}"))?;
        ensure!(i.next() <= end, "truncated instruction {a:x}");
        for b in a..i.next() {
            ensure!(
                !options.forbid_overlapping_instructions || !owners.contains_key(&b),
                "overlapping instructions at {a:x}/{b:x}"
            );
            ensure!(!is_data(b), "instruction intersects text data");
            owners.entry(b).or_insert(a);
        }
        for (op, _) in &i.ops {
            if let Op::Imm(v) = op
                && i.m != "call"
                && i.m != "jmp"
                && !i.jump
            {
                out.references.insert(*v);
                if options.infer_seh
                    && let Ok(b) = pe.read(*v, 12)
                    && b.len() == 12
                    && crate::pe::u32_at(b, 0)? == u32::MAX
                {
                    for off in [4, 8] {
                        let target = crate::pe::u32_at(b, off)?;
                        if inside(target) {
                            pending.push(target)
                        }
                    }
                }
            }
        }
        let operation = i.m.split_whitespace().last().unwrap_or("");
        if i.ret || operation.starts_with("ret") {
            out.returns.insert(i.imm().unwrap_or(0));
            out.instructions.insert(a, i);
            continue;
        }
        if ["int3", "ud2", "hlt"].contains(&i.m.as_str())
            || (options.stop_on_software_interrupts && i.m == "int")
        {
            out.instructions.insert(a, i);
            continue;
        }
        if operation == "call" || operation == "lcall" {
            if let Some(t) = i.imm() {
                out.calls.insert(t);
            } else {
                out.indirect = true
            }
        }
        if operation == "jmp" {
            if let Some(t) = i.imm() {
                if inside(t) {
                    pending.push(t)
                } else {
                    out.calls.insert(t);
                    out.tails.insert(t);
                }
            } else if let Some(targets) = options.switches.get(&a) {
                pending.extend(targets);
            } else {
                ensure!(options.infer_switches, "undeclared indirect jump at {a:x}");
                let mut targets = Vec::new();
                if let Some(m) = i.mem().filter(|m| m.base != 0 || m.index != 0) {
                    for n in 0..4096 {
                        let Ok(t) = pe.word(m.disp + n * 4) else {
                            break;
                        };
                        if !inside(t) {
                            break;
                        }
                        targets.push(t);
                    }
                }
                ensure!(targets.len() < 4096, "unbounded switch at {a:x}");
                if targets.is_empty() {
                    ensure!(!options.strict, "unresolved indirect jump at {a:x}");
                    // An unresolved indirect tail transfer is a call-graph
                    // edge, not a guessed switch or a discarded instruction.
                    out.indirect = true;
                    out.unresolved_tail = true;
                } else {
                    if let Some(m) = i.mem() {
                        out.tables.push((m.disp, m.disp + 4 * targets.len() as u32));
                    }
                    pending.extend(targets);
                }
            }
            out.instructions.insert(a, i);
            continue;
        }
        if i.jump || i.m.starts_with("loop") {
            let t = i.imm().context("unknown conditional transfer")?;
            if inside(t) {
                pending.push(t)
            } else {
                ensure!(!options.strict, "conditional target outside extent");
                out.calls.insert(t);
                out.tails.insert(t);
            }
        }
        if i.next() == end && options.allow_extent_end {
            // Extent-only passes check instruction integrity, not whether a
            // library fragment/alias has a separately identified successor.
        } else if i.next() == end && options.fallthrough_entry == Some(end) {
            out.calls.insert(end);
            out.tails.insert(end);
        } else {
            ensure!(
                i.next() < end,
                "extent ends before terminal instruction at {a:x}"
            );
            pending.push(i.next());
        }
        out.instructions.insert(a, i);
    }
    for (a, b) in &options.data {
        ensure!(
            a < b && (*a..*b).all(|v| !owners.contains_key(&v)),
            "invalid/overlapping text data"
        );
    }
    out.references = out.references.difference(&out.calls).copied().collect();
    out.covered = owners.len();
    Ok(out)
}
