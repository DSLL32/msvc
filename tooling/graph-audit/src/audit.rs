use crate::{
    Args, Profile,
    decode::{Decoder, Inst, Op},
    image::{Image, ImageView},
    index::{Inputs, Record, first_names, identity},
};
use anyhow::{Context, Result, bail, ensure};
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};
type Edges = BTreeSet<String>;
fn inside(r: &Record, a: u32) -> bool {
    r.contains_code(a)
}
fn immediate_text(i: &Inst) -> Option<u32> {
    i.text
        .strip_prefix("0x")
        .filter(|s| {
            !s.is_empty()
                && s.bytes()
                    .all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase())
        })
        .and_then(|s| u32::from_str_radix(s, 16).ok())
}
fn add_call(i: &Inst, names: &HashMap<u32, String>, out: &mut Edges) {
    out.insert(
        i.imm()
            .map(|t| identity(names, t))
            .unwrap_or_else(|| "indirect".into()),
    );
}
fn boundary(
    d: &Decoder,
    pe: &impl ImageView,
    r: &Record,
    start: u32,
    complete: bool,
) -> Result<()> {
    let mut a = start;
    while a < r.end {
        let Some(i) = d.one(pe, a, 15)? else { break };
        ensure!(
            i.next() <= r.end,
            "{} extent truncates {} at {:x} (end {:x})",
            r.name,
            i.m,
            a,
            r.end
        );
        a = i.next();
    }
    ensure!(
        !complete || a == r.end,
        "{} undecodable extent tail {:x} != {:x}",
        r.name,
        a,
        r.end
    );
    Ok(())
}
fn linear_tables(
    d: &Decoder,
    pe: &impl ImageView,
    r: &Record,
    profile: Profile,
    is_original: bool,
    names: &HashMap<u32, String>,
) -> Result<Edges> {
    let mut tables = BTreeMap::<u32, u32>::new();
    loop {
        let mut instructions = Vec::new();
        let mut cursor = r.start;
        for (&a, &b) in tables.iter().chain(std::iter::once((&r.end, &r.end))) {
            instructions.extend(d.linear(pe, cursor, a)?);
            cursor = b;
        }
        let mut discovered = tables.clone();
        for i in &instructions {
            if i.m != "jmp" || !i.text.contains('[') {
                continue;
            }
            let Some(mem) = i.mem() else { continue };
            let table = mem.disp;
            if !inside(r, table) {
                if profile == Profile::C1
                    && is_original
                    && pe.pe_sections().into_iter().flatten().any(|s| {
                        s.flags & 0x20 != 0 && s.start <= table && table < s.start + s.virtual_size
                    })
                {
                    let mut cursor = table;
                    while inside(r, pe.word(cursor)?) {
                        ensure!(
                            r.shared
                                .iter()
                                .any(|(a, b)| *a <= cursor && cursor + 4 <= *b),
                            "external switch table is masked: {} at {:x}",
                            r.name,
                            cursor
                        );
                        cursor += 4;
                    }
                }
                continue;
            }
            let mut count = 0;
            while table + count * 4 < r.end && inside(r, pe.word(table + count * 4)?) {
                count += 1;
            }
            if count > 0 {
                discovered.insert(table, table + count * 4);
            }
        }
        if discovered == tables {
            boundary(
                d,
                pe,
                r,
                tables.values().copied().max().unwrap_or(r.start),
                false,
            )?;
            let mut out = Edges::new();
            for i in &instructions {
                if i.m == "call" {
                    add_call(i, names, &mut out)
                } else if i.m == "jmp"
                    && let Some(t) = immediate_text(i)
                    && !inside(r, t)
                {
                    out.insert(identity(names, t));
                }
            }
            return Ok(out);
        }
        tables = discovered;
    }
}
fn link_edges(
    d: &Decoder,
    pe: &impl ImageView,
    r: &Record,
    names: &HashMap<u32, String>,
) -> Result<Edges> {
    let mut a = r.start;
    let mut tables = Vec::<(u32, u32)>::new();
    let mut out = Edges::new();
    while a < r.end {
        if let Some((_, end)) = tables.iter().find(|(s, e)| *s <= a && a < *e) {
            a = *end;
            continue;
        }
        let Some(i) = d.one(pe, a, 15)? else {
            a += 1;
            continue;
        };
        ensure!(i.next() <= r.end, "{} truncated instruction", r.name);
        for (op, _) in &i.ops {
            if let Op::Mem(m) = op {
                if !(m.index != 0 && m.scale == 4 || i.m == "jmp") {
                    continue;
                }
                let mut count = 0;
                while count < 4096 {
                    let Ok(t) = pe.word(m.disp + count * 4) else {
                        break;
                    };
                    if !inside(r, t) {
                        break;
                    }
                    count += 1;
                }
                if count >= 2 {
                    tables.push((m.disp, m.disp + count * 4));
                }
            }
        }
        if i.m == "call" {
            add_call(&i, names, &mut out)
        } else if i.m == "jmp"
            && let Some(t) = immediate_text(&i)
            && !inside(r, t)
            && names.contains_key(&t)
        {
            out.insert(identity(names, t));
        }
        a = i.next();
    }
    Ok(out)
}
fn c2_edges(
    d: &Decoder,
    pe: &impl ImageView,
    r: &Record,
    names: &HashMap<u32, String>,
) -> Result<Edges> {
    let mut pending = vec![r.start];
    for i in d.linear(pe, r.start, (r.start + 256).min(r.end))? {
        for (op, _) in i.ops {
            if let Op::Imm(table) = op
                && let Ok(bytes) = pe.read(table, 12)
                && bytes.len() == 12
            {
                let previous = crate::pe::u32_at(bytes, 0)?;
                let filter = crate::pe::u32_at(bytes, 4)?;
                let handler = crate::pe::u32_at(bytes, 8)?;
                if previous == u32::MAX && inside(r, handler) && (filter == 0 || inside(r, filter))
                {
                    pending.push(handler);
                    if filter != 0 {
                        pending.push(filter);
                    }
                }
            }
        }
    }
    let mut seen = HashSet::new();
    let mut out = Edges::new();
    while let Some(mut a) = pending.pop() {
        while inside(r, a) && seen.insert(a) {
            let Some(i) = d.one(pe, a, 15)? else { break };
            ensure!(
                i.next() <= r.end,
                "{} extent ends inside {} at {:x}",
                r.name,
                i.m,
                a
            );
            if i.m == "call" {
                add_call(&i, names, &mut out)
            } else if i.jump {
                if let Some(t) = i.imm() {
                    if inside(r, t) {
                        pending.push(t)
                    } else {
                        out.insert(identity(names, t));
                    }
                } else {
                    let mut targets = Vec::new();
                    if let Some(m) = i.mem()
                        && (m.index != 0 || m.base != 0)
                        && inside(r, m.disp)
                    {
                        for n in 0..256 {
                            let Ok(t) = pe.word(m.disp + n * 4) else {
                                break;
                            };
                            if !inside(r, t) {
                                break;
                            }
                            targets.push(t);
                        }
                    }
                    if targets.is_empty() {
                        out.insert("indirect".into());
                    } else {
                        pending.extend(targets)
                    }
                }
                if i.m == "jmp" {
                    break;
                }
            } else if i.ret || i.m == "int3" {
                break;
            }
            a = i.next();
        }
    }
    Ok(out)
}
fn rcpp_edges(
    d: &Decoder,
    pe: &impl ImageView,
    r: &Record,
    names: &HashMap<u32, String>,
) -> Result<Edges> {
    if r.name == "_rcpp_start" {
        let mut out = Edges::new();
        for i in d.linear(pe, r.start, r.end)? {
            if i.m == "call" {
                add_call(&i, names, &mut out)
            }
        }
        boundary(d, pe, r, r.start, true)?;
        return Ok(out);
    }
    let mut pending = vec![r.start];
    let mut seen = HashSet::new();
    let mut tables = BTreeMap::new();
    let mut out = Edges::new();
    while let Some(a) = pending.pop() {
        if !seen.insert(a) {
            continue;
        }
        ensure!(inside(r, a), "{} branch exceeds extent {:x}", r.name, a);
        let i = d.one(pe, a, 15)?.context("invalid instruction")?;
        ensure!(i.next() <= r.end, "{} extent truncates instruction", r.name);
        if i.m.starts_with("ret") {
            continue;
        }
        if i.m == "call" {
            add_call(&i, names, &mut out)
        }
        if i.m == "jmp" {
            if let Some(t) = i.imm() {
                if inside(r, t) {
                    pending.push(t)
                } else {
                    out.insert(identity(names, t));
                }
            } else if let Some(m) = i.mem().filter(|m| {
                m.index != 0 && m.scale == 4 || m.base != 0 && m.disp != 0 && m.scale == 1
            }) {
                let mut targets = Vec::new();
                for n in 0..256 {
                    let t = pe.word(m.disp + 4 * n)?;
                    if !inside(r, t) {
                        break;
                    }
                    targets.push(t);
                }
                ensure!(
                    !targets.is_empty() && targets.len() < 256,
                    "{} empty/unbounded switch",
                    r.name
                );
                tables.insert(m.disp, targets.len() as u32);
                pending.extend(targets);
            } else {
                bail!("{} unresolved indirect jump", r.name)
            }
            continue;
        }
        if i.jump {
            pending.push(i.imm().context("unknown conditional transfer")?)
        }
        pending.push(i.next());
    }
    let tail = tables
        .iter()
        .filter(|(a, _)| inside(r, **a))
        .map(|(a, n)| a + 4 * n)
        .max()
        .unwrap_or(r.start);
    ensure!(tail <= r.end, "table exceeds accepted extent");
    boundary(d, pe, r, tail, true)?;
    Ok(out)
}
#[derive(Clone, Default)]
struct Description {
    calls: BTreeSet<u32>,
    references: BTreeSet<u32>,
    returns: BTreeSet<u32>,
    tails: BTreeSet<u32>,
    indirect: bool,
}
fn describe(d: &Decoder, pe: &impl ImageView, r: &Record) -> Result<Description> {
    let mut out = Description::default();
    let mut queue = vec![r.start];
    let mut seen = HashSet::new();
    while let Some(mut a) = queue.pop() {
        while inside(r, a) && seen.insert(a) {
            let i = d.one(pe, a, 15)?.context("invalid instruction")?;
            ensure!(
                i.next() <= r.end,
                "{} extent truncates instruction {:x}",
                r.name,
                a
            );
            for (operand, _) in &i.ops {
                if let Op::Imm(v) = operand
                    && i.m != "call"
                    && i.m != "jmp"
                    && !i.jump
                {
                    // Logical immediates are bitmasks, even if equal to a function VA.
                    if !matches!(i.m.as_str(), "test" | "and" | "or" | "xor") {
                        out.references.insert(*v);
                    }
                    if let Ok(bytes) = pe.read(*v, 12)
                        && bytes.len() == 12
                        && crate::pe::u32_at(bytes, 0)? == u32::MAX
                    {
                        for off in [4, 8] {
                            let t = crate::pe::u32_at(bytes, off)?;
                            if inside(r, t) {
                                queue.push(t)
                            }
                        }
                    }
                }
            }
            if i.m == "call" {
                if let Some(t) = i.imm() {
                    out.calls.insert(t);
                } else {
                    out.indirect = true
                }
            }
            if i.m.starts_with("ret") {
                out.returns.insert(i.imm().unwrap_or(0));
                break;
            }
            if i.jump {
                if let Some(t) = i.imm() {
                    if inside(r, t) {
                        queue.push(t)
                    } else if i.m == "jmp" {
                        out.calls.insert(t);
                        out.tails.insert(t);
                    }
                } else if let Some(m) = i
                    .mem()
                    .filter(|m| m.disp != 0 && (m.base != 0 || m.index != 0))
                {
                    for n in 0..512 {
                        let Ok(t) = pe.word(m.disp + n * 4) else {
                            break;
                        };
                        if !inside(r, t) {
                            break;
                        }
                        queue.push(t)
                    }
                }
                if i.m == "jmp" {
                    break;
                }
            }
            if i.m == "int3" {
                break;
            }
            a = i.next();
        }
    }
    out.references = out.references.difference(&out.calls).copied().collect();
    Ok(out)
}
fn aliases(index: &[Record]) -> BTreeMap<u32, BTreeSet<String>> {
    let mut out = BTreeMap::<_, BTreeSet<_>>::new();
    for r in index {
        out.entry(r.start).or_default().insert(r.name.clone());
    }
    out
}
pub(crate) fn equivalent(
    left: &BTreeSet<u32>,
    right: &BTreeSet<u32>,
    oa: &BTreeMap<u32, BTreeSet<String>>,
    ra: &BTreeMap<u32, BTreeSet<String>>,
) -> bool {
    if left.len() != right.len() {
        return false;
    }
    let candidates: BTreeMap<u32, Vec<u32>> = right
        .iter()
        .map(|r| {
            (
                *r,
                left.iter()
                    .filter(|l| {
                        oa.get(l)
                            .is_some_and(|n| ra.get(r).is_some_and(|m| !n.is_disjoint(m)))
                    })
                    .copied()
                    .collect(),
            )
        })
        .collect();
    fn matches(
        r: u32,
        c: &BTreeMap<u32, Vec<u32>>,
        paired: &mut HashMap<u32, u32>,
        seen: &mut HashSet<u32>,
    ) -> bool {
        for l in &c[&r] {
            if !seen.insert(*l) {
                continue;
            }
            if !paired.contains_key(l) || matches(paired[l], c, paired, seen) {
                paired.insert(*l, r);
                return true;
            }
        }
        false
    }
    let mut paired = HashMap::new();
    right
        .iter()
        .all(|r| matches(*r, &candidates, &mut paired, &mut HashSet::new()))
}
fn cleanup(
    d: &Decoder,
    pe: &impl ImageView,
    r: &Record,
    index: &[Record],
    visiting: &mut HashSet<u32>,
    cache: &mut HashMap<(u32, u32), Description>,
) -> Result<BTreeSet<u32>> {
    ensure!(visiting.insert(r.start), "tail-call cycle {}", r.name);
    let desc = cached(d, pe, r, cache)?;
    let mut out = desc.returns;
    for a in desc.tails {
        let target = index
            .iter()
            .rev()
            .find(|r| r.start == a && !r.pending_extent)
            .context("unidentified tail-call ABI")?;
        let inherited = cleanup(d, pe, target, index, visiting, cache)?;
        ensure!(
            !inherited.is_empty(),
            "tail callee has no determined RET convention"
        );
        out.extend(inherited);
    }
    visiting.remove(&r.start);
    Ok(out)
}
fn cached(
    d: &Decoder,
    pe: &impl ImageView,
    r: &Record,
    cache: &mut HashMap<(u32, u32), Description>,
) -> Result<Description> {
    let key = (r.start, r.end);
    if let Some(v) = cache.get(&key) {
        return Ok(v.clone());
    }
    let v = describe(d, pe, r)?;
    cache.insert(key, v.clone());
    Ok(v)
}
fn rc_runtime(
    d: &Decoder,
    x: &Inputs,
    oa: &BTreeMap<u32, BTreeSet<String>>,
    ra: &BTreeMap<u32, BTreeSet<String>>,
) -> Result<()> {
    let names = &x.publics;
    let addr = |n: &str| {
        names
            .get(n)
            .copied()
            .with_context(|| format!("missing public {n}"))
    };
    if x.accepted.contains("_doexit") && x.accepted.contains("__endstdio") {
        for (label, start, end) in [
            ("xc", 0x101f000, 0x101f010),
            ("xi", 0x101f020, 0x101f030),
            ("xp", 0x101f040, 0x101f060),
            ("xt", 0x101f070, 0x101f080),
        ] {
            let left = (start..end)
                .step_by(4)
                .map(|a| x.o.word(a))
                .collect::<Result<Vec<_>>>()?
                .into_iter()
                .filter(|a| *a != 0)
                .collect::<Vec<_>>();
            let right = (addr(&format!("___{label}_a"))?..addr(&format!("___{label}_z"))?)
                .step_by(4)
                .map(|a| x.r.word(a))
                .collect::<Result<Vec<_>>>()?
                .into_iter()
                .filter(|a| *a != 0)
                .collect::<Vec<_>>();
            ensure!(left.len() == right.len(), "CRT callback count differs");
            for (a, b) in left.iter().zip(right) {
                ensure!(
                    oa.get(a).is_some_and(|n| ra
                        .get(&b)
                        .is_some_and(|m| n.intersection(m).any(|v| x.accepted.contains(v)))),
                    "unaccepted/different CRT callback"
                );
            }
        }
    }
    let oiob = 0x101d3b0;
    let riob = addr("__iob")?;
    ensure!(
        x.o.word(0x101d630)? - oiob == 19 * 32 && x.r.word(addr("__lastiob")?)? - riob == 19 * 32,
        "FILE array boundary differs"
    );
    for n in 0..20 {
        for off in [12, 16] {
            ensure!(
                x.o.read(oiob + n * 32 + off, 4)? == x.r.read(riob + n * 32 + off, 4)?,
                "FILE flags/descriptor differs"
            );
        }
    }
    let a = x.o.word(0x101d710)?;
    let b = x.r.word(addr("__pctype")?)?;
    ensure!(
        x.o.read(a - 2, 514)? == x.r.read(b - 2, 514)?,
        "C locale classes differ"
    );
    ensure!(
        x.o.read(0x101d3a0, 4)? == x.r.read(addr("___mb_cur_max")?, 4)?,
        "multibyte max differs"
    );
    ensure!(
        x.o.string(x.o.word(0x101d3a8)?)? == "."
            && x.r.string(x.r.word(addr("__decimal_point")?)?)? == ".",
        "decimal point differs"
    );
    for n in 0..6 {
        let a = x.o.word(0x101dbf0 + 4 * n)?;
        let b = x.r.word(addr("__cfltcvt_tab")? + 4 * n)?;
        ensure!(
            oa.get(&a)
                .is_some_and(|n| ra.get(&b).is_some_and(|m| !n.is_disjoint(m))),
            "floating callback differs"
        );
    }
    if x.accepted.contains("_is_kanji_lead@4") {
        for (pe, index, base) in [
            (&x.o, &x.oi, 0x1018558),
            (&x.r, &x.ri, addr("_kanji_ranges")?),
        ] {
            let r = index
                .iter()
                .find(|r| r.name == "_is_kanji_lead@4")
                .context("missing kanji function")?;
            let mut fields = BTreeSet::new();
            for i in d.linear(pe, r.start, r.end)? {
                for (op, size) in i.ops {
                    if let Op::Mem(m) = op
                        && m.base == 0
                        && m.index == 0
                    {
                        let off = m.disp.wrapping_sub(base);
                        ensure!(
                            [0, 4, 8, 12].contains(&off) && size == 2 && i.m == "movsx",
                            "KANJI bounds access differs"
                        );
                        fields.insert(off);
                    }
                }
            }
            ensure!(
                fields == [0, 4, 8, 12].into() && pe.read(base, 16)? == [0; 16],
                "KANJI bounds differ"
            );
        }
    }
    Ok(())
}
pub fn run(a: &Args) -> Result<Value> {
    let x = Inputs::load(a)?;
    if a.discovery == crate::Discovery::Cfg {
        return consolidated(a, &x);
    };
    let d = Decoder::new()?;
    let on = first_names(&x.oi);
    let rn = first_names(&x.ri);
    let mut checked = 0;
    let mut boundaries = 0;
    let mut report = BTreeMap::<String, Value>::new();
    if a.profile == Profile::Rc {
        let oa = aliases(&x.oi);
        let ra = aliases(&x.ri);
        let mut oc = HashMap::new();
        let mut rc = HashMap::new();
        for (pe, idx, cache) in [(&x.o, &x.oi, &mut oc), (&x.r, &x.ri, &mut rc)] {
            let mut unique = BTreeSet::new();
            for r in idx {
                if x.accepted.contains(&r.name) {
                    cached(&d, pe, r, cache)?;
                    unique.insert((&r.name, r.start, r.end));
                }
            }
            boundaries += unique.len();
        }
        for f in &x.drivers {
            if !x.accepted.contains(&f.name) {
                continue;
            }
            let other =
                x.ri.iter()
                    .find(|r| r.name == f.name)
                    .context("missing reconstructed function")?;
            let l = cached(&d, &x.o, f, &mut oc)?;
            let r = cached(&d, &x.r, other, &mut rc)?;
            ensure!(
                equivalent(&l.calls, &r.calls, &oa, &ra),
                "{} call graph differs",
                f.name
            );
            let lr = l
                .references
                .iter()
                .filter(|v| oa.contains_key(v) && !l.calls.contains(v))
                .copied()
                .collect();
            let rr = r
                .references
                .iter()
                .filter(|v| ra.contains_key(v) && !r.calls.contains(v))
                .copied()
                .collect();
            ensure!(
                equivalent(&lr, &rr, &oa, &ra),
                "{} function references differ",
                f.name
            );
            let lret = cleanup(&d, &x.o, f, &x.oi, &mut HashSet::new(), &mut oc)?;
            let rret = cleanup(&d, &x.r, other, &x.ri, &mut HashSet::new(), &mut rc)?;
            ensure!(lret == rret, "{} return stack cleanup differs", f.name);
            ensure!(
                l.indirect == r.indirect,
                "{} indirect call topology differs",
                f.name
            );
            let vertices = |addresses: &BTreeSet<u32>| {
                addresses.iter().map(|address|json!({"address":address,"aliases":oa.get(address).cloned().unwrap_or_default()})).collect::<Vec<_>>()
            };
            report.insert(f.name.clone(),json!({"calls":l.calls.len(),"references":lr.len(),"call_vertices":vertices(&l.calls),"reference_vertices":vertices(&lr),"returns":lret,"indirect":l.indirect}));
            checked += 1;
        }
        rc_runtime(&d, &x, &oa, &ra)?;
    } else {
        if a.profile == Profile::C2 {
            let drivers = x
                .drivers
                .iter()
                .map(|r| r.name.as_str())
                .collect::<HashSet<_>>();
            for (pe, idx, names) in [(&x.o, &x.oi, &on), (&x.r, &x.ri, &rn)] {
                for r in idx {
                    if x.accepted.contains(&r.name) && !drivers.contains(r.name.as_str()) {
                        c2_edges(&d, pe, r, names)?;
                        boundaries += 1;
                    }
                }
            }
        }
        let mut progress = x.drivers.clone();
        if a.profile == Profile::Link {
            for r in &x.oi {
                if !r.evidence.is_null()
                    && r.evidence != Value::Bool(false)
                    && !progress.iter().any(|v| v.name == r.name)
                {
                    progress.push(r.clone());
                }
            }
        }
        for f in &progress {
            if !x.accepted.contains(&f.name) {
                continue;
            }
            let other =
                x.ri.iter()
                    .find(|r| r.name == f.name)
                    .with_context(|| format!("missing reconstructed function {}", f.name))?;
            let edges = |pe: &Image,
                         r: &Record,
                         n: &HashMap<u32, String>,
                         original: bool|
             -> Result<Edges> {
                match a.profile {
                    Profile::Cl => {
                        let mut out = Edges::new();
                        for i in d.linear(pe, r.start, r.end)? {
                            if i.m == "call" {
                                out.insert(
                                    immediate_text(&i)
                                        .map(|v| identity(n, v))
                                        .unwrap_or_else(|| "indirect".into()),
                                );
                            }
                        }
                        Ok(out)
                    }
                    Profile::C1 | Profile::C1xx => linear_tables(&d, pe, r, a.profile, original, n),
                    Profile::C2 => c2_edges(&d, pe, r, n),
                    Profile::Link => link_edges(&d, pe, r, n),
                    Profile::Rcpp => rcpp_edges(&d, pe, r, n),
                    _ => unreachable!(),
                }
            };
            let l = edges(&x.o, f, &on, true)?;
            let r = edges(&x.r, other, &rn, false)?;
            ensure!(
                l == r,
                "{} graph differs: original only {:?}; reconstruction only {:?}",
                f.name,
                l.difference(&r).collect::<Vec<_>>(),
                r.difference(&l).collect::<Vec<_>>()
            );
            report.insert(f.name.clone(), json!({"edges":l}));
            checked += 1;
        }
        if a.profile == Profile::Link {
            for r in &x.oi {
                if x.accepted.contains(&r.name) {
                    link_edges(&d, &x.o, r, &on)?;
                    boundaries += 1;
                }
            }
        }
        if a.profile == Profile::Rcpp {
            for (pe, idx) in [(&x.o, &x.oi), (&x.r, &x.ri)] {
                for r in idx {
                    if x.accepted.contains(&r.name) && r.name.starts_with("import:") {
                        boundary(&d, pe, r, r.start, true)?;
                        boundaries += 1;
                    }
                }
            }
            boundaries += 2 * checked;
        }
    }
    Ok(
        json!({"profile":a.profile,"checked_functions":checked,"checked_support_extents":boundaries,"functions":report}),
    )
}

fn consolidated(a: &Args, x: &Inputs) -> Result<Value> {
    use crate::flow::{self, Options};
    let d = Decoder::new()?;
    let mut oa = aliases(&x.oi);
    let mut ra = aliases(&x.ri);
    for (target, extra) in [
        (&mut oa, &x.original_entry_names),
        (&mut ra, &x.rebuilt_entry_names),
    ] {
        for (address, names) in extra {
            target
                .entry(*address)
                .or_default()
                .extend(names.iter().cloned());
        }
    }
    let mut on = first_names(&x.oi);
    let mut rn = first_names(&x.ri);
    for (target, extra) in [
        (&mut on, &x.original_entry_names),
        (&mut rn, &x.rebuilt_entry_names),
    ] {
        for (address, names) in extra {
            if let Some(name) = names.first() {
                target.entry(*address).or_insert_with(|| name.clone());
            }
        }
    }
    if a.check_imports {
        for (index, names) in [(&x.oi, &mut on), (&x.ri, &mut rn)] {
            for record in index.iter().filter(|r| !r.name.starts_with("import:")) {
                names.insert(record.start, record.name.clone());
            }
        }
    }
    if a.check_runtime_data {
        rc_runtime(&d, x, &oa, &ra)?;
    }
    let mut report = BTreeMap::new();
    let metadata = if let Some(path) = &a.metadata {
        crate::index::json(path)?
    } else if a.profile == Profile::Rc {
        serde_json::from_str(include_str!("../profiles/rc.json"))?
    } else {
        json!({"schema":1,"functions":{},"callback_tables":[]})
    };
    ensure!(metadata["schema"] == 1, "unsupported graph metadata schema");
    let mut progress = x.drivers.clone();
    if a.include_evidenced_runtime {
        for record in &x.oi {
            if declared_evidence(&record.evidence)
                && !progress.iter().any(|r| r.name == record.name)
            {
                progress.push(record.clone());
            }
        }
    }
    let import_catalog = |catalog: &Option<crate::imports::Catalog>,
                          index: &[Record],
                          classified_imports: &BTreeSet<u32>| {
        catalog.clone().map(|mut catalog| {
            for record in index.iter().filter(|r| {
                !r.name.starts_with("import:") && !classified_imports.contains(&r.start)
            }) {
                catalog.thunks.remove(&record.start);
            }
            std::sync::Arc::new(catalog)
        })
    };
    let original_imports = import_catalog(&x.original_imports, &x.oi, &x.original_import_entries);
    let rebuilt_imports = import_catalog(&x.rebuilt_imports, &x.ri, &x.rebuilt_import_entries);
    let enabled_owners = crate::register_imports::enabled_owners(&metadata)?;
    let selected: BTreeSet<_> = progress
        .iter()
        .filter(|f| x.accepted.contains(&f.name))
        .map(|f| f.name.clone())
        .collect();
    let ret_checks = a
        .ret_checks
        .as_ref()
        .map(|path| crate::ret_checks::Checks::load(path, &progress, &selected, &x.oi, &x.ri))
        .transpose()?;
    let mut ret_report = BTreeMap::new();
    for owner in &enabled_owners {
        ensure!(
            selected.contains(owner),
            "register import enabled owner is not selected/accepted: {owner}"
        );
        ensure!(
            x.oi.iter().any(|r| &r.name == owner) && x.ri.iter().any(|r| &r.name == owner),
            "register import enabled owner missing from paired indexes: {owner}"
        );
    }
    let expected_site_owners = crate::register_imports::expected_site_owners(&metadata)?;
    for owner in &expected_site_owners {
        ensure!(
            selected.contains(owner),
            "expected import sites owner is not selected/accepted: {owner}"
        );
        ensure!(
            a.resolve_register_imports || enabled_owners.contains(owner),
            "expected import sites owner requires register import resolution: {owner}"
        );
        ensure!(
            x.oi.iter().filter(|r| &r.name == owner).count() == 1
                && x.ri.iter().filter(|r| &r.name == owner).count() == 1,
            "expected import sites owner missing or ambiguous in paired indexes: {owner}"
        );
    }
    let use_register_imports = a.resolve_register_imports || !enabled_owners.is_empty();
    ensure!(
        !use_register_imports || a.check_imports,
        "register import resolution requires --check-imports"
    );
    let resolved_owners = if a.resolve_register_imports {
        selected
    } else {
        enabled_owners.clone()
    };
    let register_contracts = if use_register_imports {
        Some((
            crate::register_imports::Contracts::parse(
                &metadata,
                &x.oi,
                &x.original_entry_names,
                original_imports
                    .as_ref()
                    .context("missing original import catalog")?,
                &a.project,
            )?,
            crate::register_imports::Contracts::parse(
                &metadata,
                &x.ri,
                &x.rebuilt_entry_names,
                rebuilt_imports
                    .as_ref()
                    .context("missing rebuilt import catalog")?,
                &a.project,
            )?,
        ))
    } else {
        None
    };
    if let Some((left, right)) = &register_contracts {
        left.validate_callback_owners(&resolved_owners)?;
        right.validate_callback_owners(&resolved_owners)?;
    }
    let mut instruction_exports = BTreeMap::new();
    for f in &progress {
        if !x.accepted.contains(&f.name) {
            continue;
        }
        let other = paired_record(&x.ri, &x.rebuilt_entry_names, &f.name)?;
        let resolve_owner = a.resolve_register_imports || enabled_owners.contains(&f.name);
        let options = Options {
            infer_switches: true,
            infer_seh: !x.o.explicit_flat_ownership(),
            require_explicit_seh_roots: x.o.explicit_flat_ownership(),
            imports: original_imports.clone(),
            record_dataflow: resolve_owner || a.export_instructions,
            code_chunks: f.code_chunks.clone(),
            fallthrough_entries: f
                .code_ranges()
                .map(|(_, b)| b)
                .filter(|b| on.contains_key(b))
                .collect(),
            forbid_overlapping_instructions: a.check_instruction_overlap,
            fallthrough_entry: on.contains_key(&f.end).then_some(f.end),
            ..Options::default()
        };
        let mut options = options;
        apply_metadata(&mut options, &metadata["functions"][&f.name]["original"])?;
        validate_text_data(&x.o, f, &options.data, &x.oi)?;
        let mut l = flow::walk(&d, &x.o, f.start, f.end, &options)
            .with_context(|| format!("original {}", f.name))?;
        validate_inferred_data(&x.o, f, &l, &x.oi)?;
        let mut rebuilt_options = Options {
            infer_switches: true,
            infer_seh: true,
            imports: rebuilt_imports.clone(),
            record_dataflow: resolve_owner || a.export_instructions,
            code_chunks: other.code_chunks.clone(),
            fallthrough_entries: other
                .code_ranges()
                .map(|(_, b)| b)
                .filter(|b| rn.contains_key(b))
                .collect(),
            forbid_overlapping_instructions: a.check_instruction_overlap,
            fallthrough_entry: rn.contains_key(&other.end).then_some(other.end),
            ..Options::default()
        };
        apply_metadata(
            &mut rebuilt_options,
            &metadata["functions"][&f.name]["rebuilt"],
        )?;
        validate_text_data(&x.r, other, &rebuilt_options.data, &x.ri)?;
        let mut r = flow::walk(&d, &x.r, other.start, other.end, &rebuilt_options)
            .with_context(|| format!("reconstructed {}", f.name))?;
        for check in &x.rebuilt_extent_checks {
            let begin = crate::index::num(&check["end"])?;
            let end = crate::index::num(&check["map_envelope_end"])?;
            ensure!(
                r.calls
                    .iter()
                    .chain(&r.tails)
                    .all(|target| *target < begin || end <= *target)
                    && r.instructions
                        .values()
                        .all(|i| i.next() <= begin || end <= i.a),
                "{} control flow enters verified rebuilt alignment padding",
                f.name
            );
        }
        let register_report = if resolve_owner {
            let strict = crate::register_imports::strict_policy(
                &metadata["functions"][&f.name]["register_imports"],
            )?;
            let oc = original_imports
                .as_ref()
                .context("missing original import catalog")?;
            let rc = rebuilt_imports
                .as_ref()
                .context("missing rebuilt import catalog")?;
            let (left_contracts, right_contracts) = register_contracts
                .as_ref()
                .context("missing preservation contracts")?;
            let left = crate::register_imports::resolve(
                &mut l,
                oc,
                left_contracts,
                strict,
                f,
                x.o.require_pe()?,
                crate::register_imports::Side::Original,
            )
            .with_context(|| format!("original {} register imports", f.name))?;
            let right = crate::register_imports::resolve(
                &mut r,
                rc,
                right_contracts,
                strict,
                other,
                x.r.require_pe()?,
                crate::register_imports::Side::Rebuilt,
            )
            .with_context(|| format!("rebuilt {} register imports", f.name))?;
            let sites_checked = crate::register_imports::check_expected_sites(
                &metadata["functions"][&f.name]["register_imports"],
                &left,
                &right,
                &f.name,
            )?;
            let mut report = json!({"original":left,"rebuilt":right,"require_all_register_calls_resolved":strict});
            if sites_checked {
                report["expected_sites_checked"] = json!(true);
            }
            Some(report)
        } else {
            None
        };
        ensure!(
            equivalent(&l.calls, &r.calls, &oa, &ra),
            "{} call graph differs: original {:?}; reconstruction {:?}",
            f.name,
            l.calls
                .iter()
                .map(|a| identity(&on, *a))
                .collect::<BTreeSet<_>>(),
            r.calls
                .iter()
                .map(|a| identity(&rn, *a))
                .collect::<BTreeSet<_>>()
        );
        ensure!(
            l.imports == r.imports,
            "{} import graph differs: original {:?}; reconstruction {:?}",
            f.name,
            l.imports,
            r.imports
        );
        ensure!(
            l.indirect == r.indirect,
            "{} indirect call topology differs",
            f.name
        );
        if a.check_references {
            let left = l
                .references
                .iter()
                .filter(|a| oa.contains_key(a))
                .copied()
                .collect();
            let right = r
                .references
                .iter()
                .filter(|a| ra.contains_key(a))
                .copied()
                .collect();
            ensure!(
                equivalent(&left, &right, &oa, &ra),
                "{} function references differ",
                f.name
            );
        }
        let mut checked_returns = None;
        let expected_ret = ret_checks
            .as_ref()
            .and_then(|checks| checks.expected(&f.name));
        if a.check_ret || expected_ret.is_some() {
            let left = cfg_cleanup(
                &d,
                &x.o,
                f,
                &x.oi,
                &metadata,
                "original",
                &mut HashSet::new(),
            )?;
            let right = cfg_cleanup(
                &d,
                &x.r,
                other,
                &x.ri,
                &metadata,
                "rebuilt",
                &mut HashSet::new(),
            )?;
            ensure!(left == right, "{} RET cleanup differs", f.name);
            if let Some(expected) = expected_ret {
                ensure!(
                    &left == expected && &right == expected,
                    "{} RET cleanup differs from expected {:?}: original {:?}, rebuilt {:?}",
                    f.name,
                    expected,
                    left,
                    right
                );
                ret_report.insert(
                    f.name.clone(),
                    json!({"expected_cleanup":expected,"original":left,"rebuilt":right}),
                );
            }
            if a.export_instructions {
                checked_returns = Some((left, right));
            }
        }
        if a.check_extents {
            cfg_boundaries(&d, &x.o, f, &l, &options.data, true)?;
            cfg_boundaries(&d, &x.r, other, &r, &rebuilt_options.data, true)?;
        }
        if a.export_instructions {
            let original = crate::instruction_export::side(
                &x.o,
                f,
                &l,
                &options,
                checked_returns.as_ref().map(|p| &p.0),
            )?;
            let rebuilt = crate::instruction_export::side(
                &x.r,
                other,
                &r,
                &rebuilt_options,
                checked_returns.as_ref().map(|p| &p.1),
            )?;
            ensure!(instruction_exports.insert(f.name.clone(),json!({"selection_source":"normal checked selection from this audit input; may be a prospective trial configuration","original":original,"rebuilt":rebuilt})).is_none(), "duplicate instruction export owner {}", f.name);
        }
        let mut edges = l
            .calls
            .iter()
            .map(|v| identity(&on, *v))
            .collect::<BTreeSet<_>>();
        if l.indirect {
            edges.insert("indirect".into());
        }
        let mut function_report = json!({"edges":edges});
        if a.check_imports {
            function_report["imports"] = json!(l.imports);
        }
        if let Some(register_report) = register_report {
            function_report["register_imports"] = register_report;
        }
        report.insert(f.name.clone(), function_report);
    }
    if a.check_callbacks {
        callback_tables(x, &metadata, &oa, &ra)?;
    }
    if a.check_ownership {
        let mut ordered = x.drivers.iter().collect::<Vec<_>>();
        ordered.sort_by_key(|r| r.start);
        for r in &ordered {
            ensure!(
                r.code_ranges().all(|(a, b)| x.o.file_backed_code(a, b)),
                "extent outside original file-backed code"
            );
        }
        for pair in ordered.windows(2) {
            ensure!(
                pair[0].end <= pair[1].start,
                "overlapping application extents"
            );
        }
    }
    let accepted = if a.check_accepted_extents {
        Some(accepted_extents(
            &d,
            x,
            &metadata,
            a.check_instruction_overlap,
        )?)
    } else {
        None
    };
    let decoded_instructions = a
        .export_instructions
        .then(|| crate::instruction_export::finish(x, &metadata, instruction_exports, &report))
        .transpose()?;
    let mut result = json!({"profile":a.profile,"discovery":"cfg","checked_functions":report.len(),"checked_accepted_extents":accepted.as_ref().map(|v|&v.0),"checked_unique_accepted_extents":accepted.as_ref().map(|v|&v.1),"original_accepted_graphs":accepted.as_ref().map(|v|&v.2),"functions":report});
    if !x.rebuilt_extent_checks.is_empty() {
        result["rebuilt_extent_checks"] = json!({"schema":1,
            "semantics":"exact object-bound rebuilt body end; separately verified linker alignment, no body or ABI waiver",
            "entries":x.rebuilt_extent_checks});
    }
    if !x.identity_only_entries.is_empty() {
        result["identity_only_entries"] = json!({"schema":1,
            "semantics":"endpoint correlation names only; no body, ABI, classification, acceptance or execution permission",
            "images":{"original":x.o.provenance(),"rebuilt":x.r.provenance()},
            "map":x.map_provenance,"entries":x.identity_only_entries});
    }
    if !x.original_support.is_empty() {
        let mut functions = BTreeMap::new();
        for record in &x.original_support {
            let (graph, options) = structural_facts(
                &d,
                &x.o,
                record,
                &metadata,
                "original",
                &x.oi,
                crate::flow::Options {
                    record_dataflow: true,
                    forbid_overlapping_instructions: a.check_instruction_overlap,
                    ..Default::default()
                },
            )
            .with_context(|| format!("original support facts {}", record.name))?;
            functions.insert(
                record.name.clone(),
                crate::instruction_export::support_side(&x.o, record, &graph, &options)?,
            );
        }
        result["decoded_support_instructions"] =
            crate::instruction_export::finish_support(x, &metadata, functions)?;
    }
    if a.check_registry_support_extents {
        result["registry_support_extents"] =
            registry_support_extents(&d, x, &metadata, a.check_instruction_overlap)?;
    }
    if let Some(checks) = &ret_checks {
        ensure!(
            ret_report.len() == checks.len(),
            "RET checks selection was not fully audited"
        );
        result["ret_cleanup_checks"] =
            json!({"input_sha256":checks.input_sha256,"functions":ret_report});
    }
    if let Some(export) = decoded_instructions {
        result["decoded_instructions"] = export;
    }
    if a.check_imports {
        result["import_catalogs"] = json!({"original":{"slots":original_imports.as_ref().map(|c|&c.slots),"normalizable_thunks":original_imports.as_ref().map(|c|c.thunks.len())},"rebuilt":{"slots":rebuilt_imports.as_ref().map(|c|&c.slots),"normalizable_thunks":rebuilt_imports.as_ref().map(|c|c.thunks.len())}});
    }
    if use_register_imports {
        result["register_import_inputs"] = json!({"original_sha256":crate::index::hash(x.o.source_bytes()),"rebuilt_sha256":crate::index::hash(x.r.source_bytes()),"metadata_sha256":crate::index::hash(&serde_json::to_vec(&metadata)?),"preservation_contracts":metadata["register_imports"]["preservation_contracts"]});
    }
    if use_register_imports
        && metadata["register_imports"]["callback_preservation_contracts"]
            .as_array()
            .is_some_and(|v| !v.is_empty())
    {
        result["register_import_inputs"]["callback_preservation_contracts"] =
            metadata["register_imports"]["callback_preservation_contracts"].clone();
    }
    if !enabled_owners.is_empty() {
        result["register_import_inputs"]["enabled_functions"] = json!(resolved_owners);
    }
    if x.oi.iter().chain(&x.ri).any(|r| !r.code_chunks.is_empty()) {
        let mut domains = BTreeMap::new();
        for (side, index) in [("original", &x.oi), ("rebuilt", &x.ri)] {
            let ranges = index
                .iter()
                .filter(|r| x.accepted.contains(&r.name))
                .map(|r| (r.name.clone(), r.code_ranges().collect::<Vec<_>>()))
                .collect::<BTreeMap<_, _>>();
            let bytes = ranges
                .values()
                .flatten()
                .map(|(a, b)| u64::from(b - a))
                .sum::<u64>();
            let intervals = ranges.values().map(Vec::len).sum::<usize>();
            let mut physical = ranges.values().flatten().copied().collect::<Vec<_>>();
            physical.sort();
            let mut physical_bytes = 0u64;
            let mut previous_end = 0u32;
            for (a, b) in physical {
                physical_bytes += u64::from(b.saturating_sub(a.max(previous_end)));
                previous_end = previous_end.max(b);
            }
            domains.insert(side,json!({"functions":ranges,"logical_function_count":ranges.len(),"interval_count":intervals,"declared_code_bytes":bytes,"unique_code_bytes":physical_bytes}));
        }
        result["owned_code_chunks"] = json!(domains);
    }
    if x.o.is_cf() || x.map_provenance.is_some() {
        result["image_inputs"] =
            json!({"original":x.o.provenance(),"rebuilt":x.r.provenance(),"map":x.map_provenance});
    }
    Ok(result)
}

pub(crate) fn validate_inferred_data(
    pe: &impl ImageView,
    record: &Record,
    graph: &crate::flow::Graph,
    index: &[Record],
) -> Result<()> {
    if !pe.explicit_flat_ownership() {
        return Ok(());
    }
    validate_text_data(pe, record, &graph.tables, index)?;
    for &(a, b) in &graph.tables {
        ensure!(
            graph
                .instructions
                .values()
                .all(|i| i.next() <= a || b <= i.a),
            "{} inferred CF table intersects executable instructions: {a:x}..{b:x}",
            record.name
        );
    }
    Ok(())
}

pub(crate) fn validate_text_data(
    pe: &impl ImageView,
    record: &Record,
    data: &[(u32, u32)],
    index: &[Record],
) -> Result<()> {
    let overlaps = |(a, b): (u32, u32), (c, d): (u32, u32)| a < d && c < b;
    let mut ordered = data.to_vec();
    ordered.sort();
    ensure!(
        ordered.windows(2).all(|w| w[0].1 <= w[1].0),
        "{} overlapping text-data declarations",
        record.name
    );
    for &(a, b) in data {
        ensure!(
            pe.text_data_backed(a, b),
            "{} text data is malformed/non-file-backed: {a:x}..{b:x}",
            record.name
        );
        let touches_code = record.code_ranges().any(|r| overlaps((a, b), r));
        if touches_code {
            ensure!(
                record.code_ranges().any(|(c, d)| c <= a && b <= d),
                "{} text data crosses code chunk ownership boundary",
                record.name
            );
        } else {
            ensure!(
                record
                    .shared
                    .iter()
                    .any(|&(c, d)| c <= a && b <= d && pe.text_data_backed(c, d)),
                "{} external text data lacks explicit shared ownership: {a:x}..{b:x}",
                record.name
            );
        }
        for other in index {
            if record.start == other.start
                && record.code_ranges().collect::<BTreeSet<_>>()
                    == other.code_ranges().collect::<BTreeSet<_>>()
            {
                continue;
            }
            ensure!(
                other.code_ranges().all(|r| !overlaps((a, b), r)),
                "{} text data intersects another function {}",
                record.name,
                other.name
            );
        }
    }
    Ok(())
}
pub(crate) fn cfg_boundaries(
    d: &Decoder,
    pe: &impl ImageView,
    record: &Record,
    graph: &crate::flow::Graph,
    declared_data: &[(u32, u32)],
    complete: bool,
) -> Result<()> {
    let mut excluded = graph
        .tables
        .iter()
        .chain(declared_data)
        .copied()
        .collect::<Vec<_>>();
    excluded.sort();
    let mut merged = Vec::<(u32, u32)>::new();
    for (a, b) in excluded {
        ensure!(a < b, "{} invalid extent data range", record.name);
        ensure!(
            graph.calls.iter().all(|target| *target < a || b <= *target),
            "{} control transfer enters extent data",
            record.name
        );
        ensure!(
            graph
                .instructions
                .values()
                .all(|i| i.next() <= a || b <= i.a),
            "{} extent data overlaps executable instruction",
            record.name
        );
        if let Some(last) = merged.last_mut()
            && a < last.1
        {
            last.1 = last.1.max(b);
        } else {
            merged.push((a, b));
        }
    }
    for (a, b) in record.code_ranges() {
        let mut cursor = a;
        for &(start, end) in merged.iter().filter(|&&(c, d)| c < b && a < d) {
            ensure!(
                a <= start && end <= b,
                "{} extent data crosses code chunk boundary",
                record.name
            );
            if cursor < start {
                let mut code = record.clone();
                code.start = cursor;
                code.end = start;
                code.code_chunks.clear();
                boundary(d, pe, &code, cursor, complete)?;
            }
            cursor = end;
        }
        if cursor < b {
            let mut code = record.clone();
            code.start = cursor;
            code.end = b;
            code.code_chunks.clear();
            boundary(d, pe, &code, cursor, complete)?;
        }
    }
    Ok(())
}

fn apply_metadata(options: &mut crate::flow::Options, value: &Value) -> Result<()> {
    for root in value["roots"].as_array().into_iter().flatten() {
        options.roots.push(crate::index::num(root)?);
    }
    for interval in value["text_data"].as_array().into_iter().flatten() {
        options.data.push(
            serde_json::from_value(interval.clone())
                .context("text_data must contain exact uint32 interval pairs")?,
        );
    }
    for (address, targets) in value["switches"].as_object().into_iter().flatten() {
        options.switches.insert(
            address.parse()?,
            targets
                .as_array()
                .context("switch targets array")?
                .iter()
                .map(crate::index::num)
                .collect::<Result<_>>()?,
        );
    }
    Ok(())
}
fn cfg_cleanup(
    d: &Decoder,
    pe: &impl ImageView,
    r: &Record,
    index: &[Record],
    metadata: &Value,
    side: &str,
    visiting: &mut HashSet<u32>,
) -> Result<BTreeSet<u32>> {
    ensure!(visiting.insert(r.start), "tail-call cycle {}", r.name);
    let mut options = crate::flow::Options {
        infer_switches: true,
        infer_seh: !pe.explicit_flat_ownership(),
        require_explicit_seh_roots: pe.explicit_flat_ownership(),
        code_chunks: r.code_chunks.clone(),
        fallthrough_entries: r
            .code_ranges()
            .map(|(_, b)| b)
            .filter(|b| index.iter().any(|v| v.start == *b))
            .collect(),
        fallthrough_entry: index.iter().any(|v| v.start == r.end).then_some(r.end),
        ..Default::default()
    };
    apply_metadata(&mut options, &metadata["functions"][&r.name][side])?;
    validate_text_data(pe, r, &options.data, index)?;
    let graph = crate::flow::walk(d, pe, r.start, r.end, &options)?;
    validate_inferred_data(pe, r, &graph, index)?;
    ensure!(
        !graph.unresolved_tail,
        "unresolved indirect tail ABI in {}",
        r.name
    );
    let mut returns = graph.returns;
    for target in graph.tails {
        let target = index
            .iter()
            .rev()
            .find(|v| v.start == target && !v.pending_extent)
            .with_context(|| {
                format!(
                    "{side} {} source [{:x}..{:x}] unidentified tail-call ABI at target {target:x}",
                    r.name, r.start, r.end
                )
            })?;
        let inherited = cfg_cleanup(d, pe, target, index, metadata, side, visiting)?;
        ensure!(
            !inherited.is_empty(),
            "tail callee has no determined RET convention"
        );
        returns.extend(inherited);
    }
    visiting.remove(&r.start);
    Ok(returns)
}
fn callback_values(
    pe: &impl ImageView,
    begin: u32,
    end: u32,
    stride: u32,
    offset: u32,
) -> Result<Vec<u32>> {
    ensure!(
        stride >= 4 && offset.checked_add(4).is_some_and(|n| n <= stride),
        "invalid callback stride/pointer offset"
    );
    ensure!(
        begin <= end && (end - begin).is_multiple_of(stride),
        "invalid callback range"
    );
    (0..(end - begin) / stride)
        .map(|row| {
            let pointer = row
                .checked_mul(stride)
                .and_then(|n| begin.checked_add(n))
                .and_then(|n| n.checked_add(offset))
                .context("callback pointer overflow")?;
            pe.word(pointer)
        })
        .collect()
}
fn callback_tables(
    x: &Inputs,
    metadata: &Value,
    oa: &BTreeMap<u32, BTreeSet<String>>,
    ra: &BTreeMap<u32, BTreeSet<String>>,
) -> Result<()> {
    let tables = metadata["callback_tables"]
        .as_array()
        .context("callback metadata missing; pass --metadata")?;
    ensure!(
        !tables.is_empty(),
        "callback metadata has no declared tables"
    );
    for table in tables {
        if !table["when_accepted"]
            .as_array()
            .into_iter()
            .flatten()
            .all(|name| name.as_str().is_some_and(|n| x.accepted.contains(n)))
        {
            continue;
        }
        let begin = crate::index::num(&table["original"][0])?;
        let end = crate::index::num(&table["original"][1])?;
        let rebuilt_begin = *x
            .publics
            .get(
                table["rebuilt_begin"]
                    .as_str()
                    .context("rebuilt callback symbol")?,
            )
            .context("missing callback table public")?;
        let stride = table
            .get("stride")
            .map(crate::index::num)
            .transpose()?
            .unwrap_or(4);
        let pointer_offset = table
            .get("pointer_offset")
            .map(crate::index::num)
            .transpose()?
            .unwrap_or(0);
        let rebuilt_end = if let Some(symbol) = table["rebuilt_end"].as_str() {
            *x.publics
                .get(symbol)
                .context("missing callback end public")?
        } else {
            rebuilt_begin
                .checked_add(
                    stride
                        .checked_mul(crate::index::num(&table["count"])?)
                        .context("callback size overflow")?,
                )
                .context("callback endpoint overflow")?
        };
        let mut left = callback_values(&x.o, begin, end, stride, pointer_offset)?;
        let mut right = callback_values(&x.r, rebuilt_begin, rebuilt_end, stride, pointer_offset)?;
        if let Some(expected) = table.get("expected_functions") {
            let expected = expected
                .as_array()
                .context("expected callback functions must be an array")?;
            ensure!(
                left.len() == expected.len() && right.len() == expected.len(),
                "expected callback row count differs"
            );
            for ((a, b), name) in left.iter().zip(&right).zip(expected) {
                if name.is_null() {
                    ensure!(*a == 0 && *b == 0, "expected null callback row differs");
                } else {
                    let name = name
                        .as_str()
                        .filter(|n| !n.is_empty())
                        .context("expected callback function must be a logical name or null")?;
                    ensure!(
                        oa.get(a).is_some_and(|names| names.contains(name))
                            && ra.get(b).is_some_and(|names| names.contains(name))
                            && (table["require_accepted"] != true || x.accepted.contains(name)),
                        "expected callback function identity or acceptance differs: {name}"
                    );
                }
            }
        } else {
            // Preserve the historical contiguous-table comparison policy.
            left.retain(|v| *v != 0);
            right.retain(|v| *v != 0);
        }
        ensure!(left.len() == right.len(), "callback count differs");
        for (a, b) in left.into_iter().zip(right) {
            if a == 0 && b == 0 {
                continue;
            }
            ensure!(
                oa.get(&a)
                    .is_some_and(|names| ra.get(&b).is_some_and(|other| names
                        .intersection(other)
                        .any(|n| table["require_accepted"] != true || x.accepted.contains(n)))),
                "different/unaccepted callback identity or order"
            );
        }
    }
    Ok(())
}

fn declared_evidence(value: &Value) -> bool {
    match value {
        Value::Null => false,
        Value::Bool(v) => *v,
        Value::String(v) => !v.is_empty(),
        Value::Array(v) => !v.is_empty(),
        Value::Object(v) => !v.is_empty(),
        Value::Number(v) => v.as_f64().is_some_and(|v| v != 0.0),
    }
}
fn structural_record(
    d: &Decoder,
    pe: &impl ImageView,
    record: &Record,
    metadata: &Value,
    side: &str,
    index: &[Record],
    forbid_overlap: bool,
) -> Result<crate::flow::Graph> {
    Ok(structural_facts(
        d,
        pe,
        record,
        metadata,
        side,
        index,
        crate::flow::Options {
            forbid_overlapping_instructions: forbid_overlap,
            ..Default::default()
        },
    )?
    .0)
}

fn structural_facts(
    d: &Decoder,
    pe: &impl ImageView,
    record: &Record,
    metadata: &Value,
    side: &str,
    index: &[Record],
    capture_options: crate::flow::Options,
) -> Result<(crate::flow::Graph, crate::flow::Options)> {
    let mut options = crate::flow::Options {
        infer_switches: true,
        infer_seh: !pe.explicit_flat_ownership(),
        require_explicit_seh_roots: pe.explicit_flat_ownership(),
        code_chunks: record.code_chunks.clone(),
        allow_extent_end: true,
        ..capture_options
    };
    apply_metadata(&mut options, &metadata["functions"][&record.name][side])?;
    validate_text_data(pe, record, &options.data, index)?;
    let graph =
        crate::flow::walk(d, pe, record.start, record.end, &options).with_context(|| {
            format!(
                "{side} accepted extent {} {:x}..{:x}",
                record.name, record.start, record.end
            )
        })?;
    validate_inferred_data(pe, record, &graph, index)?;
    cfg_boundaries(d, pe, record, &graph, &options.data, false)
        .with_context(|| format!("{side} accepted extent tail {}", record.name))?;
    Ok((graph, options))
}

fn registry_support_extents(
    d: &Decoder,
    x: &Inputs,
    metadata: &Value,
    forbid_overlap: bool,
) -> Result<Value> {
    let mut original = 0usize;
    let mut rebuilt = 0usize;
    let mut unavailable = Vec::new();
    let mut original_ranges = BTreeSet::new();
    let mut rebuilt_ranges = BTreeSet::new();
    for record in &x.registry_support {
        let graph =
            structural_record(d, &x.o, record, metadata, "original", &x.oi, forbid_overlap)?;
        ensure!(
            record.shared.iter().all(|&(a, b)| graph
                .instructions
                .values()
                .all(|i| i.next() <= a || b <= i.a)),
            "registry support shared data overlaps original instruction: {}",
            record.name
        );
        original += 1;
        original_ranges.extend(record.code_ranges());
        let mut addresses: BTreeSet<_> = x
            .rebuilt_entry_names
            .iter()
            .filter(|(_, names)| names.contains(&record.name))
            .map(|(address, _)| *address)
            .collect();
        addresses.extend(
            x.ri.iter()
                .filter(|r| r.name == record.name)
                .map(|r| r.start),
        );
        ensure!(
            addresses.len() <= 1,
            "ambiguous rebuilt registry support entry {}",
            record.name
        );
        let found: Vec<_> =
            x.ri.iter()
                .filter(|r| addresses.contains(&r.start) && r.start < r.end)
                .collect();
        if found.is_empty() {
            unavailable.push(json!({"name":record.name,"original_start":record.start,"reason":"no proven rebuilt structural extent"}));
            continue;
        }
        let bounds: BTreeSet<_> = found
            .iter()
            .map(|r| (r.start, r.end, &r.code_chunks))
            .collect();
        ensure!(
            bounds.len() == 1,
            "ambiguous rebuilt registry support extent {}",
            record.name
        );
        let mut other = found[0].clone();
        other.name = record.name.clone();
        ensure!(
            other.code_ranges().all(|(a, b)| x.r.file_backed_code(a, b)),
            "registry support extent outside rebuilt file-backed code: {}",
            record.name
        );
        structural_record(d, &x.r, &other, metadata, "rebuilt", &x.ri, forbid_overlap)?;
        rebuilt += 1;
        rebuilt_ranges.extend(other.code_ranges());
    }
    Ok(
        json!({"checked_records":{"original":original,"rebuilt":rebuilt},"checked_unique_intervals":{"original":original_ranges.len(),"rebuilt":rebuilt_ranges.len()},"structurally_unavailable_rebuilt_count":unavailable.len(),"structurally_unavailable_rebuilt":unavailable}),
    )
}

fn paired_record<'a>(
    index: &'a [Record],
    entries: &crate::index::EntryNames,
    name: &str,
) -> Result<&'a Record> {
    if let Some(record) = index.iter().find(|r| r.name == name) {
        return Ok(record);
    }
    let addresses: BTreeSet<_> = entries
        .iter()
        .filter(|(_, names)| names.contains(name))
        .map(|(address, _)| *address)
        .collect();
    ensure!(
        addresses.len() == 1,
        "missing or ambiguous reconstructed endpoint {name}"
    );
    let records: Vec<_> = index
        .iter()
        .filter(|r| addresses.contains(&r.start))
        .collect();
    let bounds: BTreeSet<_> = records
        .iter()
        .map(|r| (r.start, r.end, &r.code_chunks))
        .collect();
    ensure!(
        bounds.len() == 1,
        "missing or ambiguous reconstructed body extent {name}"
    );
    Ok(records[0])
}

fn accepted_extents(
    d: &Decoder,
    x: &Inputs,
    metadata: &Value,
    forbid_overlap: bool,
) -> Result<(Value, Value, Value)> {
    let mut raw_original = BTreeMap::<String, (BTreeSet<u32>, BTreeSet<u32>)>::new();
    let mut counts = BTreeMap::new();
    let mut unique_counts = BTreeMap::new();
    for (side, pe, index) in [("original", &x.o, &x.oi), ("rebuilt", &x.r, &x.ri)] {
        let mut checked = BTreeSet::new();
        let mut records = 0;
        for record in index {
            if !x.accepted.contains(&record.name) {
                continue;
            }
            records += 1;
            if !checked.insert((
                record.name.clone(),
                record.start,
                record.end,
                record.code_chunks.clone(),
            )) {
                continue;
            }
            let graph = structural_record(d, pe, record, metadata, side, index, forbid_overlap)?;
            if side == "original" {
                let raw = raw_original.entry(record.name.clone()).or_default();
                raw.0.extend(&graph.calls);
                raw.1.extend(&graph.references);
            }
        }
        counts.insert(side, records);
        unique_counts.insert(side, checked.len());
    }
    Ok((
        json!(counts),
        json!(unique_counts),
        json!(
            raw_original
                .into_iter()
                .map(|(name, (calls, references))| (
                    name,
                    json!({"calls":calls,"references":references})
                ))
                .collect::<BTreeMap<_, _>>()
        ),
    ))
}

#[cfg(test)]
mod alias_tests {
    use super::*;
    #[test]
    fn paired_runtime_lookup_requires_existing_unambiguous_body() {
        let record = |end| Record {
            name: "import:callee".into(),
            start: 10,
            end,
            shared: vec![],
            code_chunks: vec![],
            evidence: Value::Null,
            pending_extent: false,
        };
        let entries = BTreeMap::from([(10, BTreeSet::from(["logical".into()]))]);
        let index = vec![record(16)];
        assert_eq!(paired_record(&index, &entries, "logical").unwrap().end, 16);
        assert!(paired_record(&[], &entries, "logical").is_err());
        assert!(paired_record(&[record(16), record(17)], &entries, "logical").is_err());
        let ambiguous = BTreeMap::from([
            (10, BTreeSet::from(["logical".into()])),
            (20, BTreeSet::from(["logical".into()])),
        ]);
        assert!(paired_record(&index, &ambiguous, "logical").is_err());
    }
    #[test]
    fn shared_alias_cannot_collapse_two_vertices_into_one() {
        let left = BTreeSet::from([1, 2]);
        let right = BTreeSet::from([3]);
        let oa = BTreeMap::from([
            (1, BTreeSet::from(["shared".into()])),
            (2, BTreeSet::from(["shared".into()])),
        ]);
        let ra = BTreeMap::from([(3, BTreeSet::from(["shared".into()]))]);
        assert!(!equivalent(&left, &right, &oa, &ra));
    }
    #[test]
    fn alias_matching_reassigns_to_preserve_bijection() {
        let left = BTreeSet::from([1, 2]);
        let right = BTreeSet::from([3, 4]);
        let oa = BTreeMap::from([
            (1, BTreeSet::from(["a".into(), "b".into()])),
            (2, BTreeSet::from(["a".into()])),
        ]);
        let ra = BTreeMap::from([
            (3, BTreeSet::from(["a".into()])),
            (4, BTreeSet::from(["b".into()])),
        ]);
        assert!(equivalent(&left, &right, &oa, &ra));
    }
    #[test]
    fn callback_stride_reads_only_pointer_fields() {
        let mut bytes = vec![0; 24];
        for (offset, value) in [
            (0, 0xdeadbeefu32),
            (4, 0x401020),
            (8, 99),
            (12, 0),
            (16, 7),
            (20, 0x401030),
        ] {
            bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
        }
        let image = crate::tests::image(&bytes);
        assert_eq!(
            callback_values(&image, 0x401000, 0x401018, 8, 4).unwrap(),
            vec![0x401020, 0, 0x401030]
        );
        for (end, stride, offset) in [
            (0x401018, 0, 0),
            (0x401018, 8, 5),
            (0x401017, 8, 4),
            (0x401018, 8, u32::MAX),
        ] {
            assert!(callback_values(&image, 0x401000, end, stride, offset).is_err());
        }
    }
    #[test]
    fn logical_immediate_masks_are_not_function_references() {
        let target = 0x401080;
        // A real indexed RET function resides at target. Equal-valued logical
        // masks are not address-taking; MOV/PUSH/CMP/ADD retain their identity.
        for (instruction, is_reference) in [
            (
                vec![0xf7, 0x05, 0xf0, 0x10, 0x40, 0, 0x80, 0x10, 0x40, 0],
                false,
            ),
            (vec![0x25, 0x80, 0x10, 0x40, 0], false), // AND EAX, mask
            (vec![0x0d, 0x80, 0x10, 0x40, 0], false), // OR EAX, mask
            (vec![0x35, 0x80, 0x10, 0x40, 0], false), // XOR EAX, mask
            (vec![0x05, 0x80, 0x10, 0x40, 0], true),  // ADD EAX, address
            (vec![0xb8, 0x80, 0x10, 0x40, 0], true),
            (vec![0x68, 0x80, 0x10, 0x40, 0], true),
            (vec![0x3d, 0x80, 0x10, 0x40, 0], true),
        ] {
            let mut code = vec![0; 0x81];
            code[..instruction.len()].copy_from_slice(&instruction);
            code[instruction.len()] = 0xc3;
            code[0x80] = 0xc3;
            let image = crate::tests::image(&code);
            let record = Record {
                name: "caller".into(),
                start: 0x401000,
                end: 0x401001 + instruction.len() as u32,
                shared: vec![],
                code_chunks: vec![],
                evidence: Value::Null,
                pending_extent: false,
            };
            let mut callback = record.clone();
            callback.name = "callback".into();
            callback.start = target;
            callback.end = target + 1;
            let known = aliases(&[record.clone(), callback]);
            let decoder = Decoder::new().unwrap();
            let legacy = describe(&decoder, &image, &record).unwrap();
            let cfg = crate::flow::walk(
                &decoder,
                &image,
                record.start,
                record.end,
                &crate::flow::Options::default(),
            )
            .unwrap();
            for (references, calls, returns) in [
                (&legacy.references, &legacy.calls, &legacy.returns),
                (&cfg.references, &cfg.calls, &cfg.returns),
            ] {
                let identified: BTreeSet<_> = references
                    .iter()
                    .filter(|a| known.contains_key(a))
                    .copied()
                    .collect();
                assert_eq!(
                    identified,
                    if is_reference {
                        BTreeSet::from([target])
                    } else {
                        BTreeSet::new()
                    },
                    "{instruction:x?}"
                );
                assert!(calls.is_empty());
                assert_eq!(returns, &BTreeSet::from([0]));
            }
        }
    }
}
