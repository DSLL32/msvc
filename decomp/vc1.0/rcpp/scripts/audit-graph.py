#!/usr/bin/env python3
"""Compare calls over complete CFGs, following switch tables as data."""
import json
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_JUMP
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from binary import u32
from compare import indexes

cfg, original, reconstructed, oi, ri = indexes()
cs = Cs(CS_ARCH_X86, CS_MODE_32)
cs.detail = True

def at(address, records):
    matches = [f for f in records if f['start'] == address]
    return matches[0]['name'] if matches else hex(address)

def edges(pe, records, function):
    calls = set()
    pending = [function['start']]
    visited = {}
    tables = {}
    # Startup contains detached SEH filter and handler blocks. Linear decoding
    # is valid there (no switch data); retain their calls as well as the entry.
    if function['name'] == '_rcpp_start':
        for inst in cs.disasm(pe.read(function['start'],function['end']-function['start']),function['start']):
            if inst.mnemonic == 'call':
                op = inst.operands[0]
                calls.add(at(op.imm, records) if op.type == X86_OP_IMM else 'indirect')
        return calls, {'mode': 'linear SEH blocks; no inline table data'}
    while pending:
        address = pending.pop()
        if address in visited:
            continue
        assert function['start'] <= address < function['end'], (function['name'], 'branch exceeds extent', hex(address))
        inst = next(cs.disasm(pe.read(address,15),address,count=1),None)
        assert inst is not None, (function['name'], 'invalid instruction',hex(address))
        visited[address] = inst
        after = address + inst.size
        assert after <= function['end'], (function['name'],'extent truncates instruction',hex(address),inst.mnemonic,hex(function['end']))
        if inst.mnemonic.startswith('ret'):
            continue
        if inst.mnemonic == 'call':
            op = inst.operands[0]
            calls.add(at(op.imm, records) if op.type == X86_OP_IMM else 'indirect')
        if inst.mnemonic == 'jmp':
            op = inst.operands[0]
            if op.type == X86_OP_IMM:
                if function['start'] <= op.imm < function['end']:
                    pending.append(op.imm)
                else:
                    calls.add(at(op.imm, records))
            elif op.type == X86_OP_MEM and ((op.mem.index and op.mem.scale == 4) or (op.mem.base and op.mem.disp and op.mem.scale == 1)):
                table = op.mem.disp & 0xffffffff
                targets = []
                for n in range(256):
                    destination = u32(pe.read(table+4*n,4),0)
                    if not function['start'] <= destination < function['end']:
                        break
                    targets.append(destination)
                assert targets, (function['name'], 'empty/unresolved switch',hex(table))
                assert len(targets) < 256, (function['name'], 'unbounded switch',hex(table))
                tables[hex(table)] = len(targets)
                pending.extend(targets)
            else:
                raise AssertionError((function['name'],'unresolved indirect jump',hex(address),inst.op_str))
            continue
        if inst.group(CS_GRP_JUMP):
            op = inst.operands[0]
            assert op.type == X86_OP_IMM
            pending.append(op.imm)
        pending.append(after)
    return calls, dict(instructions=len(visited), switches=tables,
                       last_instruction_end=max(a+i.size for a,i in visited.items()))

def boundary(pe, function, flow):
    """Decode beyond the endpoint to detect split RET operands/padding too.

    CFG decoding above checks every reached instruction. This tail check also
    catches a split instruction in detached or alignment code. Switch tables
    are data: start after the final known table within this function's extent.
    The original tables are separately owned .text data beyond their functions;
    reconstructed /Od tables can precede the next public-symbol endpoint.
    """
    start,end=function['start'],function['end']
    table_ends=[int(address,16)+4*count for address,count in flow.get('switches',{}).items()
                if start <= int(address,16) < end]
    tail=max([start]+table_ends)
    assert tail <= end,(function['name'],'table exceeds accepted extent')
    cursor=tail
    for instruction in cs.disasm(pe.read(tail,end-tail+15),tail):
        if instruction.address >= end: break
        following=instruction.address+instruction.size
        assert following <= end,(function['name'],'extent truncates instruction',hex(instruction.address),instruction.mnemonic,hex(end))
        cursor=following
    assert cursor==end,(function['name'],'undecodable extent tail',hex(cursor),hex(end))

report = []
boundaries = []
failures = 0
for function in cfg['functions']:
    if function['name'] not in cfg['accepted']:
        continue
    rec_function = next(f for f in ri if f['name'] == function['name'])
    a, original_flow = edges(original, oi, function)
    b, reconstructed_flow = edges(reconstructed, ri, rec_function)
    boundary(original,function,original_flow)
    boundary(reconstructed,rec_function,reconstructed_flow)
    boundaries.append(function['name'])
    report.append(dict(name=function['name'],original=original_flow,reconstructed=reconstructed_flow,
                       original_calls=sorted(a),reconstructed_calls=sorted(b)))
    if a != b:
        print(function['name'],'original only',a-b,'reconstruction only',b-a)
        failures += 1
Path('build/graph.json').write_text(json.dumps(report,indent=2)+'\n')
if failures:
    raise SystemExit(1)
print('%d accepted non-import functions have matching calls over their complete CFGs.' % len(report))

for pe,records in [(original,oi),(reconstructed,ri)]:
    for function in records:
        if function['name'] in cfg['accepted'] and function['name'].startswith('import:'):
            boundary(pe,function,{})
imports_count=sum(name.startswith('import:') for name in cfg['accepted'])
Path('boundary-validation.json').write_text(json.dumps(dict(
    target_sha256=cfg['target']['sha256'],
    accepted_nonimport_extents_per_binary=len(boundaries),
    accepted_import_extents_per_binary=imports_count,
    checked_extents=2*(len(boundaries)+imports_count),
    reachable_instruction_endpoints_checked=True,
    tail_decode_lookahead_bytes=15,
    shared_switch_tables_treated_as_data=True,
    split_instructions=0),indent=2)+'\n')
print('%d accepted original/reconstruction extents end on complete instruction or owned switch-data boundaries.'%(2*(len(boundaries)+imports_count)))
