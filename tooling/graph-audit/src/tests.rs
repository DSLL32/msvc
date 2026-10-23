use crate::{
    decode::Decoder,
    flow::{Options, walk},
    pe::Pe,
};
use std::collections::BTreeSet;
fn image(code: &[u8]) -> Pe {
    let mut b = vec![0; 0x600];
    b[..2].copy_from_slice(b"MZ");
    b[60..64].copy_from_slice(&0x80u32.to_le_bytes());
    b[0x80..0x84].copy_from_slice(b"PE\0\0");
    b[0x86..0x88].copy_from_slice(&1u16.to_le_bytes());
    b[0x94..0x96].copy_from_slice(&0xe0u16.to_le_bytes());
    b[0x98..0x9a].copy_from_slice(&0x10bu16.to_le_bytes());
    b[0xb4..0xb8].copy_from_slice(&0x400000u32.to_le_bytes());
    let s = 0x178;
    b[s..s + 5].copy_from_slice(b".text");
    for (off, value) in [
        (8, 0x400u32),
        (12, 0x1000),
        (16, 0x400),
        (20, 0x200),
        (36, 0x60000020),
    ] {
        b[s + off..s + off + 4].copy_from_slice(&value.to_le_bytes());
    }
    b[0x200..0x200 + code.len()].copy_from_slice(code);
    Pe::parse(b).unwrap()
}
#[test]
fn direct_call_and_return() {
    let p = image(&[0xe8, 0x0b, 0, 0, 0, 0xc3]);
    let g = walk(
        &Decoder::new().unwrap(),
        &p,
        0x401000,
        0x401006,
        &Options::default(),
    )
    .unwrap();
    assert_eq!(g.calls, BTreeSet::from([0x401010]));
    assert_eq!(g.returns, BTreeSet::from([0]));
}
#[test]
fn conditional_tail_is_an_edge() {
    let p = image(&[0x74, 0x0e, 0xc3]);
    let g = walk(
        &Decoder::new().unwrap(),
        &p,
        0x401000,
        0x401003,
        &Options::default(),
    )
    .unwrap();
    assert_eq!(g.calls, BTreeSet::from([0x401010]));
    assert_eq!(g.tails, g.calls);
}
#[test]
fn identified_fallthrough_is_an_edge() {
    let p = image(&[0x90]);
    let d = Decoder::new().unwrap();
    assert!(walk(&d, &p, 0x401000, 0x401001, &Options::default()).is_err());
    let o = Options {
        fallthrough_entry: Some(0x401001),
        ..Options::default()
    };
    let g = walk(&d, &p, 0x401000, 0x401001, &o).unwrap();
    assert_eq!(g.calls, BTreeSet::from([0x401001]));
}
#[test]
fn unresolved_indirect_tail_preserved() {
    let p = image(&[0xff, 0x25, 0x20, 0x10, 0x40, 0]);
    let d = Decoder::new().unwrap();
    let o = Options {
        infer_switches: true,
        ..Options::default()
    };
    let g = walk(&d, &p, 0x401000, 0x401006, &o).unwrap();
    assert!(g.indirect);
    let o = Options { strict: true, ..o };
    assert!(walk(&d, &p, 0x401000, 0x401006, &o).is_err());
}
#[test]
fn partial_ret_operand_rejected() {
    let p = image(&[0xc2, 4, 0]);
    assert!(
        walk(
            &Decoder::new().unwrap(),
            &p,
            0x401000,
            0x401001,
            &Options::default()
        )
        .is_err()
    );
}
#[test]
fn prefixed_return_recognized() {
    let p = image(&[0xf3, 0xc3]);
    let g = walk(
        &Decoder::new().unwrap(),
        &p,
        0x401000,
        0x401002,
        &Options::default(),
    )
    .unwrap();
    assert_eq!(g.returns, BTreeSet::from([0]));
}
#[test]
fn declared_switch_data_is_not_code() {
    let p = image(&[
        0xff, 0x24, 0x85, 0x08, 0x10, 0x40, 0, 0xcc, 0x10, 0x10, 0x40, 0, 0x11, 0x10, 0x40, 0,
        0xc3, 0xc3,
    ]);
    let o = Options {
        data: vec![(0x401008, 0x401010)],
        switches: [(0x401000, vec![0x401010, 0x401011])].into(),
        strict: true,
        ..Options::default()
    };
    let g = walk(&Decoder::new().unwrap(), &p, 0x401000, 0x401012, &o).unwrap();
    assert_eq!(g.instructions.len(), 3);
    assert_eq!(g.covered, 9);
}
#[test]
fn switch_recovery_is_shared() {
    let p = image(&[
        0xff, 0x24, 0x85, 0x08, 0x10, 0x40, 0, 0xcc, 0x10, 0x10, 0x40, 0, 0x11, 0x10, 0x40, 0,
        0xc3, 0xc3, 0, 0, 0, 0,
    ]);
    let o = Options {
        infer_switches: true,
        ..Options::default()
    };
    let g = walk(&Decoder::new().unwrap(), &p, 0x401000, 0x401012, &o).unwrap();
    assert_eq!(g.tables, vec![(0x401008, 0x401010)]);
    assert_eq!(g.instructions.len(), 3);
}
#[test]
fn overlapping_instruction_streams_are_valid_x86() {
    let p = image(&[0x74, 1, 0xb8, 0, 0, 0, 0, 0xc3]);
    let d = Decoder::new().unwrap();
    let graph = walk(&d, &p, 0x401000, 0x401008, &Options::default()).unwrap();
    assert!(graph.instructions.contains_key(&0x401002));
    assert!(graph.instructions.contains_key(&0x401003));
    assert_eq!(graph.covered, 8);
    let options = Options {
        forbid_overlapping_instructions: true,
        ..Options::default()
    };
    assert!(walk(&d, &p, 0x401000, 0x401008, &options).is_err());
}
#[test]
fn branch_into_declared_data_rejected() {
    let p = image(&[0xeb, 0, 0xc3]);
    let o = Options {
        data: vec![(0x401002, 0x401003)],
        ..Options::default()
    };
    assert!(walk(&Decoder::new().unwrap(), &p, 0x401000, 0x401003, &o).is_err());
}
#[test]
fn explicit_os_root_preserves_detached_callee() {
    let p = image(&[0xc3, 0xe8, 0x0a, 0, 0, 0, 0xc3]);
    let d = Decoder::new().unwrap();
    let o = Options {
        roots: vec![0x401001],
        ..Options::default()
    };
    let g = walk(&d, &p, 0x401000, 0x401007, &o).unwrap();
    assert_eq!(g.calls, BTreeSet::from([0x401010]));
}

#[test]
fn loop_branch_discovers_its_detached_callee() {
    // Capstone LOOP has branch_relative, not the generic JUMP group.
    let p = image(&[
        0xe2, 5, 0xc3, 0xcc, 0xcc, 0xcc, 0xcc, 0xe8, 4, 0, 0, 0, 0xc3,
    ]);
    let g = walk(
        &Decoder::new().unwrap(),
        &p,
        0x401000,
        0x40100d,
        &Options::default(),
    )
    .unwrap();
    assert_eq!(g.calls, BTreeSet::from([0x401010]));
}

#[test]
fn software_interrupt_can_return_to_following_call() {
    let p = image(&[0xcd, 0x21, 0xe8, 9, 0, 0, 0, 0xc3]);
    let d = Decoder::new().unwrap();
    let g = walk(&d, &p, 0x401000, 0x401008, &Options::default()).unwrap();
    assert_eq!(g.calls, BTreeSet::from([0x401010]));
    let legacy = Options {
        stop_on_software_interrupts: true,
        ..Options::default()
    };
    assert!(
        walk(&d, &p, 0x401000, 0x401008, &legacy)
            .unwrap()
            .calls
            .is_empty()
    );
}

#[test]
fn crt_literal_must_fit_its_declared_width() {
    assert!(crate::index::integer_bytes(256, 1).is_err());
    assert!(crate::index::integer_bytes(0x1_0000_0000, 4).is_err());
    assert_eq!(crate::index::integer_bytes(255, 1).unwrap()[0], 255);
    assert_eq!(crate::index::integer_bytes(u64::MAX, 8).unwrap(), [255; 8]);
    assert!(crate::index::integer_bytes(1, 0).is_err());
    assert!(crate::index::integer_bytes(0, 0).is_ok());
}

#[test]
fn extent_only_walk_allows_fragment_end_but_never_split_operand() {
    let d = Decoder::new().unwrap();
    let p = image(&[0x90, 0xc2, 4, 0]);
    let options = Options {
        allow_extent_end: true,
        ..Options::default()
    };
    assert!(walk(&d, &p, 0x401000, 0x401001, &options).is_ok());
    assert!(walk(&d, &p, 0x401000, 0x401002, &options).is_err());
}

#[test]
fn declared_cvpack_alignment_requires_exact_unreachable_disjoint_bytes() {
    let d = Decoder::new().unwrap();
    let code = [0xc3, 0x05, 0, 0, 0, 0];
    let pe = image(&code);
    let options = Options {
        strict: true,
        ..Options::default()
    };
    let range = (0x401001, 0x401006);
    assert!(crate::cvpack::audited(&d, &pe, 0x401000, 0x401006, &options, &[range]).is_ok());
    assert!(crate::cvpack::audited(&d, &pe, 0x401000, 0x401006, &options, &[]).is_err());
    assert!(
        crate::cvpack::audited(&d, &pe, 0x401000, 0x401006, &options, &[range, range]).is_err()
    );
    let bad = image(&[0xc3, 0x05, 1, 0, 0, 0]);
    assert!(crate::cvpack::audited(&d, &bad, 0x401000, 0x401006, &options, &[range]).is_err());
    let rooted = Options {
        roots: vec![range.0],
        ..options.clone()
    };
    // A terminal RET follows the declared ADD so its reachability, rather than
    // a missing terminal instruction, is the rejection reason.
    let pe = image(&[0xc3, 0x05, 0, 0, 0, 0, 0xc3]);
    let err = crate::cvpack::audited(&d, &pe, 0x401000, 0x401007, &rooted, &[range])
        .err()
        .unwrap();
    assert!(err.to_string().contains("CFG-reachable"));
    let data = Options {
        data: vec![range],
        ..options
    };
    assert!(crate::cvpack::audited(&d, &pe, 0x401000, 0x401007, &data, &[range]).is_err());
}

#[test]
fn split_function_traverses_owned_chunks_without_auto_rooting() {
    // Hot conditional enters cold+0x10; cold calls an external function and
    // branches back to the primary RET. An unreferenced cold+0x18 chunk is owned
    // but is NOT treated as another root.
    let mut code = vec![0xcc; 0x20];
    code[..3].copy_from_slice(&[0x74, 0x0e, 0xc3]);
    code[0x10..0x17].copy_from_slice(&[0xe8, 0x2b, 0, 0, 0, 0xeb, 0xeb]);
    code[0x18..0x1e].copy_from_slice(&[0xe8, 0x33, 0, 0, 0, 0xc3]);
    let pe = image(&code);
    let d = Decoder::new().unwrap();
    let options = Options {
        code_chunks: vec![(0x401010, 0x401017), (0x401018, 0x40101e)],
        ..Options::default()
    };
    let graph = walk(&d, &pe, 0x401000, 0x401003, &options).unwrap();
    assert_eq!(graph.calls, BTreeSet::from([0x401040]));
    assert_eq!(graph.returns, BTreeSet::from([0]));
    assert!(!graph.instructions.contains_key(&0x401018));
    assert!(!graph.instructions.contains_key(&0x401003));
}
#[test]
fn code_chunks_reject_overlap_file_bounds_and_split_operands() {
    let pe = image(&[0xe9, 0x0b, 0, 0, 0]);
    let d = Decoder::new().unwrap();
    for code_chunks in [
        vec![(0x401004, 0x401010)],
        vec![(0x401400, 0x401410)],
        vec![(0x401010, 0x401010)],
    ] {
        assert!(
            walk(
                &d,
                &pe,
                0x401000,
                0x401005,
                &Options {
                    code_chunks,
                    ..Options::default()
                }
            )
            .is_err()
        );
    }
    let pe = image(&[
        0x74, 0x0e, 0xc3, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
        0xcc, 0xc2, 4, 0,
    ]);
    let options = Options {
        code_chunks: vec![(0x401010, 0x401012)],
        ..Options::default()
    };
    assert!(
        walk(&d, &pe, 0x401000, 0x401003, &options)
            .err()
            .unwrap()
            .to_string()
            .contains("chunk boundary")
    );
}
#[test]
fn code_chunk_ownership_retains_exact_aliases_but_rejects_foreign_vertices() {
    let pe = image(&[0xc3]);
    let record = crate::index::Record {
        name: "one".into(),
        start: 0x401000,
        end: 0x401001,
        shared: vec![],
        code_chunks: vec![(0x401010, 0x401018)],
        evidence: serde_json::Value::Null,
        pending_extent: false,
    };
    let mut alias = record.clone();
    alias.name = "alias".into();
    assert!(crate::index::validate_chunk_ownership(&pe, &[record.clone(), alias]).is_ok());
    let mut other = record.clone();
    other.name = "foreign".into();
    other.start = 0x401012;
    other.end = 0x401014;
    other.code_chunks.clear();
    assert!(crate::index::validate_chunk_ownership(&pe, &[record, other]).is_err());
}

#[test]
fn real_pgo_functions_preserve_cross_chunk_calls_and_return_abis() {
    let fixtures: serde_json::Value =
        serde_json::from_str(include_str!("../fixtures/code-chunks.json")).unwrap();
    let d = Decoder::new().unwrap();
    for target in fixtures["targets"].as_array().unwrap() {
        let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../..")
            .join(target["path"].as_str().unwrap());
        let pe = Pe::open(&path).unwrap();
        assert_eq!(
            crate::index::hash(&pe.data),
            target["sha256"].as_str().unwrap()
        );
        for function in target["functions"].as_array().unwrap() {
            let start = crate::index::num(&function["start"]).unwrap();
            let end = crate::index::num(&function["end"]).unwrap();
            let chunks: Vec<(u32, u32)> =
                serde_json::from_value(function["code_chunks"].clone()).unwrap();
            let options = Options {
                code_chunks: chunks.clone(),
                roots: serde_json::from_value(
                    function
                        .get("roots")
                        .cloned()
                        .unwrap_or(serde_json::json!([])),
                )
                .unwrap(),
                infer_switches: true,
                infer_seh: true,
                ..Options::default()
            };
            let graph = walk(&d, &pe, start, end, &options)
                .unwrap_or_else(|error| panic!("{}: {error:#}", function["name"]));
            if function.get("expected_calls").is_some() {
                assert_eq!(
                    graph.calls,
                    serde_json::from_value::<BTreeSet<u32>>(function["expected_calls"].clone())
                        .unwrap(),
                    "{}",
                    function["name"]
                );
            }
            for call in function["required_calls"].as_array().into_iter().flatten() {
                assert!(
                    graph.calls.contains(&crate::index::num(call).unwrap()),
                    "cold call missing in {}",
                    function["name"]
                );
            }
            assert_eq!(
                graph.returns,
                serde_json::from_value::<BTreeSet<u32>>(function["expected_returns"].clone())
                    .unwrap()
            );
            assert_eq!(graph.indirect, function["indirect"].as_bool().unwrap());
            assert!(graph.instructions.values().all(|i| {
                std::iter::once((start, end))
                    .chain(chunks.iter().copied())
                    .any(|(a, b)| a <= i.a && i.next() <= b)
            }));
            assert!(
                graph
                    .calls
                    .iter()
                    .all(|target| !chunks.iter().any(|(a, b)| a <= target && target < b)),
                "internal cold branch leaked as callee"
            );
        }
    }
}
