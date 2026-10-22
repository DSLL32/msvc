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
        inst = next(cs.disasm(pe.read(address, min(15, function['end']-address)),address,count=1), None)
        assert inst is not None, (function['name'], 'invalid instruction',hex(address))
        visited[address] = inst
        after = address + inst.size
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

report = []
failures = 0
for function in cfg['functions']:
    if function['name'] not in cfg['accepted']:
        continue
    rec_function = next(f for f in ri if f['name'] == function['name'])
    a, original_flow = edges(original, oi, function)
    b, reconstructed_flow = edges(reconstructed, ri, rec_function)
    report.append(dict(name=function['name'],original=original_flow,reconstructed=reconstructed_flow,
                       original_calls=sorted(a),reconstructed_calls=sorted(b)))
    if a != b:
        print(function['name'],'original only',a-b,'reconstruction only',b-a)
        failures += 1
Path('build/graph.json').write_text(json.dumps(report,indent=2)+'\n')
if failures:
    raise SystemExit(1)
print('%d accepted non-import functions have matching calls over their complete CFGs.' % len(report))
