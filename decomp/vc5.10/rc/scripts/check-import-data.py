#!/usr/bin/env python3
"""RC-only _adjust_fdiv data-import witness, beyond Rust call qualification.

Read binutils' import/disassembly reports; never discover functions or build a
local graph. The common registry/MAP owns the function boundaries. This covers
only the known IAT-pointer load followed by its scalar dereference.
"""
import json
from pathlib import Path
import re
import subprocess

registry = json.loads(Path('.build/functions.json').read_text())
startup = next(r for r in registry['driver'] if r['name'] == '_mainCRTStartup')
publics = {m[1]: int(m[2], 16) for m in re.finditer(
    r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', Path('build/rc.map').read_text(), re.M)}

def check(image, start, end):
    table = subprocess.check_output(['objdump', '-p', str(image)], text=True)
    base = int(re.search(r'^ImageBase\s+([0-9a-f]+)', table, re.M)[1], 16)
    thunk = None
    library = None
    slots = {}
    index = 0
    for line in table.splitlines():
        descriptor = re.match(r'^ ([0-9a-f]{8})\s+([0-9a-f]{8})\s+([0-9a-f]{8})\s+([0-9a-f]{8})\s+([0-9a-f]{8})\s+([0-9a-f]{8})\s*$', line)
        if descriptor:
            thunk = int(descriptor[6], 16)
        name = re.search(r'DLL Name: (\S+)', line)
        if name:
            assert thunk is not None
            library = name[1].lower()
            index = 0
        member = re.match(r'^\s*([0-9a-f]+)\s+(?:<none>\s+)?([0-9a-f]+)\s+(\S+)', line)
        if library and member:
            slots[base + thunk + index * 4] = (library, member[3])
            index += 1
    slot = [s for s, name in slots.items() if name == ('msvcrt.dll', '_adjust_fdiv')]
    assert len(slot) == 1, 'Missing/ambiguous actual _adjust_fdiv IAT'
    text = subprocess.check_output(['objdump', '-d', '-Mintel', '--start-address=' + hex(start),
                                    '--stop-address=' + hex(end), str(image)], text=True)
    instructions = []
    for line in text.splitlines():
        match = re.match(r'^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+\s*([a-z][a-z0-9]*)\s*(.*)', line)
        if match:
            instructions.append((int(match[1], 16), match[2], match[3].strip()))
    uses = [i for i, (_, _, operand) in enumerate(instructions)
            if hex(slot[0]) in re.findall(r'0x[0-9a-f]+', operand)]
    assert len(uses) == 1, 'Unexpected _adjust_fdiv data-reference count'
    i = uses[0]
    assert instructions[i][1:] == ('mov', 'eax,ds:' + hex(slot[0])), '_adjust_fdiv pointer load differs'
    assert instructions[i+1][1] == 'mov', '_adjust_fdiv scalar load differs'
    scalar = re.fullmatch(r'(eax|ecx|edx),DWORD PTR \[eax\]', instructions[i+1][2])
    assert scalar, '_adjust_fdiv scalar dereference differs'
    assert instructions[i+2][1] == 'mov' and instructions[i+2][2].endswith(',' + scalar[1]), '_adjust_fdiv state store differs'

check(Path('../../../') / registry['target']['path'], startup['start'], startup['end'])
begin = publics['_mainCRTStartup']
end = min(v for v in publics.values() if v > begin)
check(Path('build/rc.exe'), begin, end)
