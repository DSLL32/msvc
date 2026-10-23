use crate::{
    Args,
    decode::Decoder,
    flow::{self, Graph, Options},
    index::{hash, json, map, num},
    pe::Pe,
};
use anyhow::{Context, Result, bail, ensure};
use serde_json::{Value, json};
use std::{
    collections::{BTreeMap, BTreeSet, HashMap},
    fs,
    path::Path,
};
fn list<'a>(v: &'a Value, key: &str) -> Result<&'a Vec<Value>> {
    v[key]
        .as_array()
        .with_context(|| format!("expected {key} array"))
}
fn intervals(v: &Value) -> Result<Vec<(u32, u32)>> {
    v.as_array()
        .into_iter()
        .flatten()
        .map(|p| Ok((num(&p[0])?, num(&p[1])?)))
        .collect()
}
fn overlap(a: (u32, u32), b: (u32, u32)) -> bool {
    a.0 < b.1 && b.0 < a.1
}
fn ownership(cfg: &Value, pe: &Pe, runtime: &Value, data: &Value, project: &Path) -> Result<()> {
    let code = |a: u32, b: u32| -> Result<()> {
        ensure!(
            a < b
                && pe
                    .sections
                    .iter()
                    .filter(|s| s.flags & 0x20 != 0
                        && s.start <= a
                        && b <= s.start + s.size.min(s.virtual_size)
                        && s.off + (b - s.start) as usize <= pe.data.len())
                    .count()
                    == 1,
            "range is not original file-backed code {a:x}..{b:x}"
        );
        Ok(())
    };
    let functions = list(cfg, "functions")?;
    let mut application = Vec::<(u32, u32, String, String)>::new();
    for r in functions {
        let start = num(&r["original"])?;
        code(start, start + 1)?;
        let name = r["name"].as_str().context("function name")?;
        if r["status"] == "stub" {
            ensure!(
                r["extent"].is_null() && r["text_data"].as_array().is_none_or(|v| v.is_empty()),
                "stub owns restored bytes"
            );
            continue;
        }
        let a = num(&r["extent"][0])?;
        let b = num(&r["extent"][1])?;
        ensure!(a == start, "extent does not start at entry");
        for (kind, (a, b)) in std::iter::once(("code", (a, b))).chain(
            intervals(&r["text_data"])?
                .into_iter()
                .map(|v| ("text-data", v)),
        ) {
            code(a, b)?;
            ensure!(
                application.iter().all(|p| !overlap((a, b), (p.0, p.1))),
                "overlapping application ownership"
            );
            for other in functions {
                if other["name"] == name && kind == "code" {
                    continue;
                }
                let entry = num(&other["original"])?;
                ensure!(
                    !(a <= entry && entry < b),
                    "application range contains another entry"
                );
            }
            application.push((a, b, name.into(), kind.into()));
        }
    }
    let known = list(runtime, "functions")?
        .iter()
        .chain(list(runtime, "imports")?)
        .collect::<Vec<_>>();
    let mut aliases = Vec::new();
    for alias in cfg["runtime_aliases"].as_array().into_iter().flatten() {
        let names = alias["names"]
            .as_array()
            .context("alias names")?
            .iter()
            .filter_map(Value::as_str)
            .collect::<BTreeSet<_>>();
        ensure!(
            names.len() >= 2
                && project
                    .join(alias["evidence"].as_str().context("alias evidence")?)
                    .is_file(),
            "unidentified runtime alias evidence"
        );
        for name in names {
            ensure!(
                known.iter().any(|r| r["name"] == name
                    && r["start"] == alias["start"]
                    && r["end"] == alias["end"]),
                "unidentified runtime alias"
            );
        }
        aliases.push(alias);
    }
    let mut support = Vec::<(u32, u32, String)>::new();
    for r in cfg["support_unmasked"].as_array().into_iter().flatten() {
        let a = num(&r["start"])?;
        let b = num(&r["end"])?;
        let name = r["name"].as_str().context("runtime name")?;
        code(a, b)?;
        ensure!(
            known
                .iter()
                .any(|k| k["name"] == name && k["start"] == a && k["end"] == b),
            "unidentified runtime range"
        );
        ensure!(
            application.iter().all(|p| !overlap((a, b), (p.0, p.1))),
            "runtime overlaps application ownership"
        );
        for f in functions {
            let entry = num(&f["original"])?;
            ensure!(
                !(a <= entry && entry < b),
                "runtime overlaps application entry"
            );
        }
        for item in data.as_array().context("data array")? {
            let start = num(&item["original"])?;
            ensure!(
                !overlap((a, b), (start, start + num(&item["size"])?)),
                "runtime overlaps contextual data"
            );
        }
        for p in &support {
            if !overlap((a, b), (p.0, p.1)) {
                continue;
            }
            ensure!(
                aliases.iter().any(|alias| alias["start"] == a
                    && alias["end"] == b
                    && p.0 == a
                    && p.1 == b
                    && p.2 != name
                    && alias["names"].as_array().is_some_and(
                        |v| v.iter().any(|v| v == name) && v.iter().any(|v| v == &p.2)
                    )),
                "overlapping runtime ownership without explicit aliases"
            );
        }
        support.push((a, b, name.into()));
    }
    for item in data.as_array().context("data array")? {
        let a = num(&item["original"])?;
        let b = a + num(&item["size"])?;
        let owners = application
            .iter()
            .filter(|p| overlap((a, b), (p.0, p.1)))
            .collect::<Vec<_>>();
        if !owners.is_empty() {
            ensure!(
                owners.len() == 1
                    && owners[0].3 == "text-data"
                    && owners[0].0 <= a
                    && a < b
                    && b <= owners[0].1,
                "contextual data overlaps code/ambiguous ownership"
            );
        }
    }
    Ok(())
}
fn identities(
    runtime: &Value,
    functions: &[Value],
    linked: Option<&BTreeMap<String, u32>>,
) -> Result<HashMap<u32, String>> {
    let mut names = BTreeMap::<u32, BTreeSet<String>>::new();
    for r in list(runtime, "functions")?
        .iter()
        .chain(list(runtime, "imports")?)
    {
        names
            .entry(num(&r["start"])?)
            .or_default()
            .insert(r["name"].as_str().context("runtime name")?.into());
    }
    let mut out = names
        .into_iter()
        .map(|(a, n)| (a, n.into_iter().collect::<Vec<_>>().join("|")))
        .collect::<HashMap<_, _>>();
    for r in functions {
        let a = if let Some(linked) = linked {
            *linked
                .get(r["symbol"].as_str().context("symbol")?)
                .context("missing app public")?
        } else {
            num(&r["original"])?
        };
        out.insert(a, r["name"].as_str().context("function name")?.into());
    }
    Ok(out)
}
fn named(g: &Graph, names: &HashMap<u32, String>) -> Result<BTreeSet<String>> {
    let mut out = BTreeSet::new();
    for a in &g.calls {
        out.insert(
            names
                .get(a)
                .with_context(|| format!("unresolved callee {a:x}"))?
                .clone(),
        );
    }
    if g.indirect {
        out.insert("indirect".into());
    }
    Ok(out)
}
pub(crate) fn audited(
    d: &Decoder,
    pe: &Pe,
    start: u32,
    end: u32,
    options: &Options,
    padding: &[(u32, u32)],
) -> Result<Graph> {
    for (index, &(a, b)) in padding.iter().enumerate() {
        ensure!(
            start <= a && a < b && b <= end && b - a == 5,
            "invalid declared alignment padding {a:x}..{b:x}"
        );
        ensure!(
            pe.read(a, 5)? == [0x05, 0, 0, 0, 0],
            "declared alignment padding bytes differ at {a:x}"
        );
        ensure!(
            padding[..index].iter().all(|r| !overlap((a, b), *r))
                && options.data.iter().all(|r| !overlap((a, b), *r)),
            "declared alignment padding overlaps another declaration"
        );
    }

    let mut g = flow::walk(d, pe, start, end, options)?;
    let covered = g
        .instructions
        .values()
        .flat_map(|i| i.a..i.next())
        .collect::<BTreeSet<_>>();
    let mut cursor = start;
    let mut previous: Option<crate::decode::Inst> = None;
    let mut extra = Vec::new();
    while cursor < end {
        if let Some((_, b)) = options
            .data
            .iter()
            .find(|(a, b)| *a <= cursor && cursor < *b)
        {
            cursor = *b;
            continue;
        }
        let i = d
            .one(pe, cursor, (end - cursor).min(15) as usize)?
            .context("invalid/truncated whole-function instruction")?;
        ensure!(i.next() <= end, "partial final instruction");
        if !covered.contains(&cursor) {
            if previous.as_ref().is_some_and(|p| {
                p.m == "jmp" && i.m == "jmp" && p.text == i.text && i.imm().is_some()
            }) {
                ensure!(
                    g.instructions.contains_key(&i.imm().unwrap()),
                    "duplicate jump target not rooted"
                );
                extra.push(cursor);
            } else {
                ensure!(
                    [
                        &[0x90][..],
                        &[0x8b, 0xff],
                        &[0x8d, 0x49, 0],
                        &[0x8d, 0x64, 0x24, 0],
                        &[0x8d, 0xa4, 0x24, 0, 0, 0, 0],
                        &[0x8d, 0x9b, 0, 0, 0, 0],
                        &[0xcc]
                    ]
                    .contains(&i.bytes.as_slice())
                        || padding.contains(&(cursor, i.next())),
                    "unrooted code {cursor:x}: {} {}",
                    i.m,
                    i.text
                );
            }
        }
        cursor = i.next();
        previous = Some(i);
    }
    if !extra.is_empty() {
        let mut options = options.clone();
        options.roots.extend(extra);
        g = flow::walk(d, pe, start, end, &options)?;
    }
    // Check the final graph, including compiler-duplicate-jump roots discovered
    // by the whole-body pass. A declaration cannot hide reachable instructions.
    for &(a, b) in padding {
        ensure!(
            g.instructions
                .values()
                .all(|i| !overlap((a, b), (i.a, i.next()))),
            "declared alignment padding is CFG-reachable at {a:x}"
        );
    }
    Ok(g)
}
fn seh(d: &Decoder, pe: &Pe, start: u32, end: u32, scaffolding: bool) -> Result<Vec<u32>> {
    let mut roots = Vec::new();
    for i in d.linear(pe, start, (start + 96).min(end))? {
        if i.m != "mov" || !i.text.starts_with("dword ptr [ebp - ") {
            continue;
        }
        let Some((crate::decode::Op::Imm(table), _)) = i.ops.last() else {
            continue;
        };
        if !pe
            .sections
            .iter()
            .any(|s| s.name == ".rdata" && s.start <= *table && *table < s.start + s.virtual_size)
        {
            continue;
        }
        for n in 0..16 {
            let p = *table + n * 12;
            let previous = pe.word(p)?;
            if previous != u32::MAX && previous >= n {
                break;
            }
            let mut valid = false;
            for a in [pe.word(p + 4)?, pe.word(p + 8)?] {
                if start <= a && a < end {
                    roots.push(a);
                    valid = true
                }
            }
            if !valid {
                break;
            }
        }
    }
    if scaffolding && !roots.is_empty() {
        let instructions = d.linear(pe, start, end)?;
        let by_address = instructions
            .iter()
            .map(|i| (i.a, i))
            .collect::<HashMap<_, _>>();
        for root in roots.clone() {
            for i in instructions.iter().filter(|i| i.a >= root) {
                if i.m == "ret" {
                    if let Some(next) = by_address.get(&i.next())
                        && next.m == "jmp"
                        && let Some(target) = next.imm()
                    {
                        ensure!(
                            start <= target && target < end,
                            "compiler SEH jump outside extent"
                        );
                        roots.push(next.a);
                    }
                    break;
                }
                if i.m == "jmp" {
                    if let Some(next) = by_address.get(&i.next())
                        && next.m == "mov"
                        && next.text.starts_with("dword ptr [ebp - 0x")
                        && next.text.ends_with("], 0xffffffff")
                    {
                        ensure!(
                            instructions
                                .iter()
                                .filter(|v| v.bytes == next.bytes)
                                .count()
                                >= 2,
                            "compiler scope reset differs"
                        );
                        roots.push(next.a);
                    }
                    break;
                }
            }
        }
    }
    roots.sort();
    roots.dedup();
    Ok(roots)
}
type SwitchLayout = (BTreeMap<u32, Vec<u32>>, Vec<(u32, u32)>);
fn switches(d: &Decoder, pe: &Pe, start: u32, end: u32, expected: &[u32]) -> Result<SwitchLayout> {
    if expected.is_empty() {
        return Ok((BTreeMap::new(), Vec::new()));
    }
    let mut out = BTreeMap::new();
    let mut data = Vec::<(u32, u32)>::new();
    let mut counts = Vec::new();
    let mut recent = Vec::<crate::decode::Inst>::new();
    let mut cursor = start;
    while cursor < end {
        if let Some((_, b)) = data.iter().find(|(a, b)| *a <= cursor && cursor < *b) {
            cursor = *b;
            continue;
        }
        let i = d
            .one(pe, cursor, (end - cursor).min(15) as usize)?
            .context("invalid compiler instruction")?;
        if i.m == "jmp"
            && let Some(mem) = i.mem()
        {
            let base = mem.disp;
            ensure!(
                start <= base && base < end,
                "indirect jump is not compiler text table"
            );
            let mut bound = None;
            for (n, p) in recent.iter().enumerate() {
                if p.m == "cmp"
                    && let Some((crate::decode::Op::Imm(v), _)) = p.ops.last()
                    && recent[n + 1..].iter().any(|b| b.m == "ja")
                {
                    bound = Some(v.wrapping_add(1));
                }
            }
            let bound = bound.context("no unsigned compiler table bound")?;
            ensure!(bound > 0 && bound <= 256, "invalid compiler table bound");
            ensure!(
                base + bound * 4 <= end,
                "compiler table exceeds function extent"
            );
            let targets = (0..bound)
                .map(|n| pe.word(base + n * 4))
                .collect::<Result<Vec<_>>>()?;
            ensure!(
                targets.iter().all(|t| start <= *t && *t < end),
                "compiler switch target outside function"
            );
            out.insert(i.a, targets);
            data.push((base, base + 4 * bound));
            counts.push(bound);
        }
        cursor = i.next();
        recent.push(i);
        if recent.len() > 8 {
            recent.remove(0);
        }
    }
    ensure!(counts == expected, "compiler/source switch bounds differ");
    Ok((out, data))
}
pub fn run(a: &Args) -> Result<Value> {
    ensure!(
        !a.include_evidenced_runtime && !a.check_accepted_extents,
        "CVPACK uses explicit application/support ownership metadata; indexed-runtime flags are for standalone profiles"
    );
    ensure!(
        a.metadata.is_none(),
        "CVPACK graph metadata comes from functions.json; --metadata is for standalone profiles"
    );
    let cfg = json(&a.config.clone().unwrap_or(a.project.join("functions.json")))?;
    ensure!(
        list(&cfg, "functions")?
            .iter()
            .all(
                |r| ["code_chunks", "rebuilt_code_chunks"].iter().all(|key| r
                    .get(*key)
                    .is_none_or(|v| v.as_array().is_some_and(Vec::is_empty)))
            ),
        "CVPACK profile does not support code_chunks; its explicit ownership policy must be extended first"
    );
    let original = Pe::open(
        &a.original.clone().unwrap_or(
            a.repo
                .join(cfg["target"]["path"].as_str().context("target path")?),
        ),
    )?;
    ensure!(
        hash(&original.data) == cfg["target"]["sha256"].as_str().context("target hash")?,
        "target changed"
    );
    ensure!(
        original.entry == num(&cfg["target"]["entry"])?,
        "target entry differs"
    );
    let rebuilt = Pe::open(
        &a.rebuilt
            .clone()
            .unwrap_or(a.project.join("build/CVPACK.EXE")),
    )?;
    let linked = map(
        &a.map.clone().unwrap_or(a.project.join("build/cvpack.map")),
        false,
    )?;
    let cache =
        json(a.rebuilt_runtime.as_ref().context(
            "CVPACK requires --rebuilt-runtime frozen graph-identity cache (see README)",
        )?)?;
    ensure!(cache["schema"] == 1, "unsupported identity cache schema");
    for (label, bytes) in [
        ("original", original.data.as_slice()),
        ("rebuilt", rebuilt.data.as_slice()),
    ] {
        ensure!(
            cache["provenance"][format!("{label}_sha256")] == hash(bytes),
            "{label} identity cache stale"
        );
    }
    ensure!(
        cache["provenance"]["archive_sha256"]
            == hash(&fs::read(a.repo.join("toolchains/1.0-nt/lib/LIBC.LIB"))?),
        "archive identity cache stale"
    );
    ensure!(
        cache["provenance"]["functions_sha256"]
            == hash(&fs::read(
                a.config.clone().unwrap_or(a.project.join("functions.json"))
            )?),
        "function identity cache stale"
    );
    ensure!(
        cache["provenance"]["map_sha256"]
            == hash(&fs::read(
                a.map.clone().unwrap_or(a.project.join("build/cvpack.map"))
            )?),
        "map identity cache stale"
    );
    ensure!(
        cache["provenance"]["runtime_anchors_sha256"]
            == hash(&fs::read(a.project.join("runtime-anchors.json"))?),
        "runtime anchor identity cache stale"
    );
    let runtime = json(&a.runtime.clone().unwrap_or(a.project.join("runtime.json")))?;
    ensure!(
        cache["original_runtime"] == runtime,
        "original runtime identity cache differs"
    );
    let functions = list(&cfg, "functions")?;
    let names = functions
        .iter()
        .map(|r| r["name"].as_str())
        .collect::<BTreeSet<_>>();
    let entries = functions
        .iter()
        .map(|r| num(&r["original"]))
        .collect::<Result<BTreeSet<_>>>()?;
    ensure!(
        names.len() == functions.len() && entries.len() == functions.len(),
        "duplicate function name/entry"
    );
    let data = json(&a.project.join("data.json"))?;
    if a.discovery == crate::Discovery::Legacy || a.check_ownership {
        ownership(&cfg, &original, &runtime, &data, &a.project)?;
    }
    let oi = identities(&runtime, functions, None)?;
    let ri = identities(&cache["rebuilt_runtime"], functions, Some(&linked))?;
    let addresses = linked
        .values()
        .copied()
        .chain([rebuilt.code_end()?])
        .collect::<BTreeSet<_>>();
    let d = Decoder::new()?;
    let mut report = BTreeMap::new();
    for r in functions {
        ensure!(
            ["stub", "candidate", "accepted"]
                .contains(&r["status"].as_str().context("function status")?),
            "invalid function status"
        );
        ensure!(r["kind"] == "application", "function kind differs");
        if r["status"] == "stub" {
            continue;
        }
        let start = num(&r["extent"][0])?;
        let end = num(&r["extent"][1])?;
        ensure!(start == num(&r["original"])?, "extent start differs");
        let mut options = Options {
            strict: true,
            stop_on_software_interrupts: a.discovery == crate::Discovery::Legacy,
            forbid_overlapping_instructions: a.discovery == crate::Discovery::Legacy
                || a.check_instruction_overlap,
            data: intervals(&r["text_data"])?,
            roots: r["roots"]
                .as_array()
                .into_iter()
                .flatten()
                .map(num)
                .collect::<Result<_>>()?,
            ..Options::default()
        };
        for (address, targets) in r["switches"].as_object().into_iter().flatten() {
            options.switches.insert(
                address.parse()?,
                targets
                    .as_array()
                    .context("switch target array")?
                    .iter()
                    .map(num)
                    .collect::<Result<_>>()?,
            );
        }
        for t in r["switch_tables"].as_array().into_iter().flatten() {
            for (n, target) in list(t, "targets")?.iter().enumerate() {
                ensure!(
                    original.word(num(&t["start"])? + 4 * n as u32)? == num(target)?,
                    "switch targets differ"
                );
            }
        }
        let l = if a.discovery == crate::Discovery::Legacy || a.check_extents {
            audited(
                &d,
                &original,
                start,
                end,
                &options,
                &intervals(&r["alignment_padding"])?,
            )?
        } else {
            flow::walk(&d, &original, start, end, &options)?
        };
        let rebuilt_start = *linked
            .get(r["symbol"].as_str().context("symbol")?)
            .context("missing rebuilt function")?;
        let rebuilt_end = *addresses
            .range((
                std::ops::Bound::Excluded(rebuilt_start),
                std::ops::Bound::Unbounded,
            ))
            .next()
            .context("no rebuilt end")?;
        let mut roots = seh(
            &d,
            &rebuilt,
            rebuilt_start,
            rebuilt_end,
            r["compiler_seh_scaffolding"].as_bool().unwrap_or(false),
        )?;
        for off in r["rebuilt_extra_roots"].as_array().into_iter().flatten() {
            roots.push(rebuilt_start + num(off)?);
        }
        let counts = r["compiler_switch_counts"]
            .as_array()
            .into_iter()
            .flatten()
            .map(num)
            .collect::<Result<Vec<_>>>()?;
        let (switches, data) = switches(&d, &rebuilt, rebuilt_start, rebuilt_end, &counts)?;
        let right_options = Options {
            strict: true,
            stop_on_software_interrupts: a.discovery == crate::Discovery::Legacy,
            forbid_overlapping_instructions: a.discovery == crate::Discovery::Legacy
                || a.check_instruction_overlap,
            roots,
            data,
            switches,
            ..Options::default()
        };
        let right = if a.discovery == crate::Discovery::Legacy || a.check_extents {
            audited(
                &d,
                &rebuilt,
                rebuilt_start,
                rebuilt_end,
                &right_options,
                &[],
            )?
        } else {
            flow::walk(&d, &rebuilt, rebuilt_start, rebuilt_end, &right_options)?
        };
        let left = named(&l, &oi)?;
        let right_names = named(&right, &ri)?;
        ensure!(
            left == right_names,
            "{} graph differs: {:?} / {:?}",
            r["name"],
            left,
            right_names
        );
        if a.discovery == crate::Discovery::Legacy || a.check_ret {
            let expected = BTreeSet::from([num(&r["stack_cleanup"])?]);
            ensure!(
                l.returns == right.returns && l.returns == expected,
                "{} ABI RET cleanup differs",
                r["name"]
            );
            ensure!(
                r["abi"].as_str().is_some_and(|s| !s.is_empty()),
                "missing ABI"
            );
        }
        report.insert(
            r["name"].as_str().context("name")?.to_owned(),
            json!({"edges":left,"returns":l.returns}),
        );
    }
    if a.check_callbacks || a.check_references {
        bail!(
            "CVPACK callback/reference checks have no declared comparison policy; use graph/RET/ownership checks"
        )
    }
    Ok(
        json!({"profile":"cvpack","scope":"graph subset with optional ownership/RET checks; runtime identity generation/source/data checks excluded","checked_functions":report.len(),"functions":report}),
    )
}
