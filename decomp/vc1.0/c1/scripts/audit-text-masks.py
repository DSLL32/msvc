#!/usr/bin/env python3
"""C1 mask contract for switch tables outside accepted function extents.

Call-graph/CFG/ABI/endpoint discovery is owned by tooling/graph-audit. This
narrow target check remains here because the common tool does not yet assert
that every original external switch-table cell is explicitly unmasked.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from binary import PE, u32

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--config', type=Path, default=BINARY/'functions.json')
args = parser.parse_args()
config = json.loads(args.config.read_text())
original = PE(ROOT/'toolchains/1.0-nt/bin/C13232.EXE')
assert hashlib.sha256(original.data).hexdigest() == config['target']['sha256']
code = [(original.base+s['rva'],original.base+s['rva']+s['virtual_size'])
        for s in original.sections if s['flags'] & 0x20]
cs = Cs(CS_ARCH_X86, CS_MODE_32)
checked = set()
for function in config['driver']:
    if function['name'] not in config['accepted']:
        continue
    start, end = function['start'], function['end']
    tables = {}
    while True:
        instructions = []
        cursor = start
        for left, right in sorted(tables.items()) + [(end,end)]:
            instructions.extend(cs.disasm(original.read(cursor,left-cursor),cursor))
            cursor = right
        discovered = dict(tables)
        for instruction in instructions:
            if instruction.mnemonic != 'jmp' or '[' not in instruction.op_str:
                continue
            literals = re.findall(r'0x[0-9a-f]+',instruction.op_str)
            if not literals:
                continue
            table = int(literals[-1],16)
            if not any(left <= table < right for left,right in code):
                continue
            cursor = table
            while start <= u32(original.read(cursor,4),0) < end:
                if not start <= table < end:
                    assert any(left <= cursor and cursor+4 <= right
                               for left,right in function.get('shared',[])), \
                        'external switch table is masked: %s at %x' % (function['name'],cursor)
                    checked.add((function['name'],cursor))
                cursor += 4
            if start <= table < end and cursor > table:
                discovered[table] = cursor
        if discovered == tables:
            break
        tables = discovered
print('%d external original switch-table cells have explicit mask coverage.' % len(checked))
