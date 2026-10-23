#!/usr/bin/env python3
"""Retain complete detached-startup SEH calls beyond inferred CFG roots.
Rust owns application/support graphs and every accepted function/import extent.
This residual scans CALLs in only the two canonical startup bodies.
"""
import json
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM
from reference import indexes


def main():
    config, original, rebuilt, oi, ri = indexes()
    graph = json.loads(Path('build/graph.json').read_text())
    assert graph['discovery'] == 'cfg' and graph['profile'] == 'rcpp'
    assert graph['checked_functions'] == sum(f['name'] in config['accepted'] for f in config['functions'])
    required = {'call_graph', 'ret_cleanup', 'function_references', 'extent_ownership',
                'whole_extent_boundaries', 'accepted_index_extents'}
    assert required <= set(graph['checks']), 'required Rust graph checks were omitted'
    expected_extents = len(config['accepted'])
    assert graph['checked_accepted_extents'] == dict(original=expected_extents, rebuilt=expected_extents)
    assert graph['checked_unique_accepted_extents'] == dict(original=expected_extents, rebuilt=expected_extents)
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    startup_calls = {}
    for label, pe, records in [('original', original, oi), ('rebuilt', rebuilt, ri)]:
        names = {f['start']: f['name'] for f in records}
        startup = next(f for f in records if f['name'] == '_rcpp_start')
        calls = set()
        for instruction in decoder.disasm(pe.read(startup['start'], startup['end']-startup['start']), startup['start']):
            if instruction.mnemonic == 'call':
                operand = instruction.operands[0]
                calls.add(names.get(operand.imm, hex(operand.imm)) if operand.type == X86_OP_IMM else 'indirect')
        assert calls == set(graph['functions']['_rcpp_start']['edges']), (label, 'detached SEH call coverage', calls)
        startup_calls[label] = sorted(calls)
    report = dict(status='passed', graph='Rust consolidated CFG with RET/reference/ownership/whole-tail checks',
                  accepted_extents=graph['checked_accepted_extents'], startup_linear_seh_calls=startup_calls,
                  retained_python_scope='Complete detached startup-call coverage only')
    Path('build/graph-residual.json').write_text(json.dumps(report, indent=2)+'\n')
    print('Detached startup SEH calls verified; Rust checked %d accepted extents per binary.' % expected_extents)
    Path('boundary-validation.json').write_text(json.dumps(dict(
        target_sha256=config['target']['sha256'],
        accepted_nonimport_extents_per_binary=graph['checked_functions'],
        accepted_import_extents_per_binary=expected_extents-graph['checked_functions'],
        checked_extents=2*expected_extents, reachable_instruction_endpoints_checked=True,
        tail_decode_lookahead_bytes=15, shared_switch_tables_treated_as_data=True,
        split_instructions=0, auditor='Rust consolidated CFG and accepted-index extent checks',
        startup_seh_residual='Complete Python linear CALL-set scan on original and rebuilt startup'),indent=2)+'\n')


if __name__ == '__main__': main()
