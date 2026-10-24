//! Shared instruction-level CFG discovery. Legacy comparison policies live in
//! audit.rs, not here; roots/switches/data are explicit input metadata.
use crate::{
    decode::{Decoder, Inst, Op},
    image::ImageView,
};
use anyhow::{Context, Result, ensure};
use std::collections::{BTreeMap, BTreeSet, HashMap};
#[derive(Clone, Default)]
pub struct Options {
    pub roots: Vec<u32>,
    pub record_dataflow: bool,
    pub imports: Option<std::sync::Arc<crate::imports::Catalog>>,
    pub code_chunks: Vec<(u32, u32)>,
    pub fallthrough_entries: Vec<u32>,
    pub data: Vec<(u32, u32)>,
    pub switches: BTreeMap<u32, Vec<u32>>,
    pub infer_switches: bool,
    pub infer_seh: bool,
    pub require_explicit_seh_roots: bool,
    pub strict: bool,
    pub forbid_overlapping_instructions: bool,
    pub stop_on_software_interrupts: bool,
    pub allow_extent_end: bool,
    pub fallthrough_entry: Option<u32>,
}
#[derive(Default)]
pub struct Graph {
    pub calls: BTreeSet<u32>,
    pub imports: BTreeSet<crate::imports::Identity>,
    pub references: BTreeSet<u32>,
    pub returns: BTreeSet<u32>,
    pub indirect: bool,
    pub unresolved_tail: bool,
    pub tails: BTreeSet<u32>,
    pub instructions: BTreeMap<u32, Inst>,
    pub roots: BTreeSet<u32>,
    pub successors: BTreeMap<u32, BTreeSet<u32>>,
    pub indirect_calls: BTreeSet<u32>,
    pub import_calls: BTreeMap<u32, crate::imports::Identity>,
    pub covered: usize,
    pub tables: Vec<(u32, u32)>,
}
pub fn walk(
    d: &Decoder,
    pe: &impl ImageView,
    start: u32,
    end: u32,
    options: &Options,
) -> Result<Graph> {
    ensure!(start < end, "invalid function extent");
    let mut ranges = vec![(start, end)];
    ranges.extend(&options.code_chunks);
    ranges.sort();
    ensure!(
        ranges.iter().all(|&(a, b)| pe.file_backed_code(a, b)),
        "invalid/non-file-backed code chunk"
    );
    ensure!(
        ranges.windows(2).all(|w| w[0].1 <= w[1].0),
        "overlapping code chunks"
    );
    let inside = |a| ranges.iter().any(|&(s, e)| s <= a && a < e);
    let containing_end = |a| {
        ranges
            .iter()
            .find(|&&(s, e)| s <= a && a < e)
            .map(|&(_, e)| e)
    };
    let is_data = |a| options.data.iter().any(|(s, e)| *s <= a && a < *e);
    let mut pending = vec![start];
    pending.extend(&options.roots);
    let mut owners = HashMap::new();
    let mut out = Graph::default();
    if options.record_dataflow {
        out.roots.extend(pending.iter().copied());
    }
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
        let chunk_end = containing_end(a).context("instruction outside code chunks")?;
        let i = d
            .one(
                pe,
                a,
                if options.strict {
                    (chunk_end - a).min(15) as usize
                } else {
                    15
                },
            )?
            .with_context(|| format!("invalid/truncated instruction at {a:x}"))?;
        ensure!(
            i.next() <= chunk_end,
            "instruction crosses code chunk boundary at {a:x}"
        );
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
                // TEST's immediate is a bitmask, even if it equals a function VA.
                if i.m != "test" {
                    out.references.insert(*v);
                }
                if (options.infer_seh || options.require_explicit_seh_roots)
                    && let Ok(b) = pe.read(*v, 12)
                    && b.len() == 12
                    && crate::pe::u32_at(b, 0)? == u32::MAX
                {
                    for off in [4, 8] {
                        let target = crate::pe::u32_at(b, off)?;
                        if inside(target) {
                            ensure!(
                                !options.require_explicit_seh_roots
                                    || target == start
                                    || options.roots.contains(&target),
                                "CF SEH target {target:x} from scope {v:x} requires explicit metadata roots"
                            );
                            if !options.infer_seh {
                                continue;
                            }
                            pending.push(target);
                            if options.record_dataflow {
                                out.roots.insert(target);
                            }
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
                ensure!(!is_data(t), "call enters declared text data at {t:x}");
                if let Some(identity) = options.imports.as_ref().and_then(|c| c.thunks.get(&t)) {
                    out.imports.insert(identity.clone());
                    if options.record_dataflow {
                        out.import_calls.insert(a, identity.clone());
                    }
                } else {
                    out.calls.insert(t);
                }
            } else if let Some(identity) = options
                .imports
                .as_ref()
                .and_then(|c| c.absolute_transfer(&i))
            {
                out.imports.insert(identity.clone());
                if options.record_dataflow {
                    out.import_calls.insert(a, identity.clone());
                }
            } else {
                if options.record_dataflow {
                    out.indirect_calls.insert(a);
                }
                out.indirect = true
            }
        }
        if operation == "jmp" {
            if let Some(t) = i.imm() {
                ensure!(!is_data(t), "branch enters declared text data at {t:x}");
                if inside(t) {
                    pending.push(t);
                    if options.record_dataflow {
                        out.successors.entry(a).or_default().insert(t);
                    }
                } else {
                    if let Some(identity) = options.imports.as_ref().and_then(|c| c.thunks.get(&t))
                    {
                        out.imports.insert(identity.clone());
                    } else {
                        out.calls.insert(t);
                        out.tails.insert(t);
                    }
                }
            } else if let Some(identity) = options
                .imports
                .as_ref()
                .and_then(|c| c.absolute_transfer(&i))
            {
                out.imports.insert(identity.clone());
            } else if let Some(targets) = options.switches.get(&a) {
                pending.extend(targets);
                if options.record_dataflow {
                    out.successors.entry(a).or_default().extend(targets);
                }
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
                    pending.extend(&targets);
                    if options.record_dataflow {
                        out.successors.entry(a).or_default().extend(targets);
                    }
                }
            }
            out.instructions.insert(a, i);
            continue;
        }
        if i.jump || i.m.starts_with("loop") {
            let t = i.imm().context("unknown conditional transfer")?;
            ensure!(
                !is_data(t),
                "conditional branch enters declared text data at {t:x}"
            );
            if inside(t) {
                pending.push(t);
                if options.record_dataflow {
                    out.successors.entry(a).or_default().insert(t);
                }
            } else {
                ensure!(!options.strict, "conditional target outside extent");
                if let Some(identity) = options.imports.as_ref().and_then(|c| c.thunks.get(&t)) {
                    out.imports.insert(identity.clone());
                } else {
                    out.calls.insert(t);
                    out.tails.insert(t);
                }
            }
        }
        if i.next() == chunk_end && options.allow_extent_end {
            // Extent-only passes check instruction integrity, not whether a
            // library fragment/alias has a separately identified successor.
        } else if i.next() == chunk_end && inside(i.next()) {
            pending.push(i.next());
            if options.record_dataflow {
                out.successors.entry(a).or_default().insert(i.next());
            }
        } else if i.next() == chunk_end
            && (options.fallthrough_entry == Some(chunk_end)
                || options.fallthrough_entries.contains(&chunk_end))
        {
            out.calls.insert(chunk_end);
            out.tails.insert(chunk_end);
        } else {
            ensure!(
                i.next() < chunk_end,
                "extent ends before terminal instruction at {a:x}"
            );
            pending.push(i.next());
            if options.record_dataflow {
                out.successors.entry(a).or_default().insert(i.next());
            }
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
