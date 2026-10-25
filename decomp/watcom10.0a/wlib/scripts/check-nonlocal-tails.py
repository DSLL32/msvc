#!/usr/bin/env python3
"""Finite exceptional-tail residual: normal RET0 plus a nonlocal error tail.
The shared graph/ownership/extent audit still checks every accepted App body.
No AbortCommand/longjmp code is restored or accepted by this check.
"""
from pathlib import Path
import hashlib
import re
import struct
import subprocess
import sys

project = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
original = project / '../../../toolchains/watcom10.0a/.patched/WLIB.EXE'
rebuilt = project / 'build/WLIB.EXE'

def read_va(path, address, size):
    data = path.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count, optional = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    base = struct.unpack_from('<I', data, pe + 52)[0]
    for k in range(count):
        at = pe + 24 + optional + 40 * k
        rva, raw_size, raw_at = struct.unpack_from('<III', data, at + 12)
        if base + rva <= address and address + size <= base + rva + raw_size:
            return data[raw_at + address - base - rva:raw_at + address - base - rva + size]
    raise AssertionError('unmapped ABI bytes')

def instructions(path, start, end):
    text = subprocess.check_output(['objdump', '-D', '-Mintel', '--no-show-raw-insn',
                                   '--start-address=' + hex(start), '--stop-address=' + hex(end), str(path)], text=True)
    return [(int(a, 16), m, operands.strip()) for a, m, operands in
            re.findall(r'^\s*([0-9a-f]+):\s+(\S+)\s*(.*?)$', text, re.M)]

def jump(path, at, target):
    code = read_va(path, at, 5)
    assert code[0] == 0xe9 and at + 5 + struct.unpack('<i', code[1:])[0] == target

# Frozen original byte identities delimit these actual old bodies, not an
# implementation/ABI exemption. longjmp switches to Env's saved stack/EIP.
EXPECTED = {
    (0x412a43, 0x412a5c): '008e627cd4cb4bae91a40222564a548cff8ab37512e00a40b4215d56c61e0f02',
    (0x411fe0, 0x412021): 'bc22fd67e4b715bf0cf6193c550a5bb1fd09ced78035b64cb7c4cea01997c03d',
    (0x412d76, 0x412e06): 'b7013e2a67703656e1c8c43361e9a1b84d3146ac7856607394510ad9e75eb73c',
    (0x418621, 0x418690): '126183286fc6a828f9306465a5cb427169766cdb0a1182f1145071b82db3f831',
}
for (start, end), digest in EXPECTED.items():
    assert hashlib.sha256(read_va(original, start, end - start)).hexdigest() == digest
assert read_va(original, 0x412020, 1) == b'\xc3'
original_returns = [(a, operands) for a, m, operands in instructions(original, 0x411fe0, 0x412021) if m.startswith('ret')]
assert original_returns == [(0x412020, '')]
jump(original, 0x412007, 0x412d76)
jump(original, 0x412e01, 0x418621)
assert read_va(original, 0x418647, 6) == bytes.fromhex('8b601cff7018')  # restore ESP; push saved EIP
abort_code = instructions(original, 0x412d76, 0x412e06)
assert not any(m.startswith('ret') for _, m, _ in abort_code)
abort_external_jumps = [(a, m, int(operands.split()[0], 16)) for a, m, operands in abort_code
                        if m.startswith('j') and not (0x412d76 <= int(operands.split()[0], 16) < 0x412e06)]
assert abort_external_jumps == [(0x412e01, 'jmp', 0x418621)]

symbols = {}
for offset, name in re.findall(r'^0001:([0-9a-fA-F]{8})[ *+]\s+(\S+)', (project / 'build/wlib.map').read_text(), re.M):
    symbols[name] = 0x410000 + int(offset, 16)
for name, original_start, original_end, original_ret, original_tail in [
    ('AllocMemory', 0x411fe0, 0x412021, 0x412020, 0x412007),
    ('CheckBreak', 0x412a43, 0x412a5c, 0x412a5b, 0x412a56),
]:
    original_code = instructions(original, original_start, original_end)
    assert [(a, operands) for a, m, operands in original_code if m.startswith('ret')] == [(original_ret, '')]
    original_tails = [(a, m, int(operands.split()[0], 16)) for a, m, operands in original_code
                      if m.startswith('j') and not (original_start <= int(operands.split()[0], 16) < original_end)]
    assert original_tails == [(original_tail, 'jmp', 0x412d76)]
    start = symbols[name + '_']
    end = min(a for a in symbols.values() if a > start)
    code = instructions(rebuilt, start, end)
    returns = [(a, operands) for a, m, operands in code if m.startswith('ret')]
    assert len(returns) == 1 and returns[0][1] == ''
    tails = [(a, operands) for a, m, operands in code if m == 'jmp' and not (start <= int(operands.split()[0], 16) < end)]
    assert len(tails) == 1 and int(tails[0][1].split()[0], 16) == symbols['AbortCommand_']
assert '#pragma aux AbortCommand aborts;' in (project / 'src/toolstate.h').read_text()
print('PASS AllocMemory/CheckBreak normal RET0 and exact exceptional AbortCommand→longjmp nonlocal tails (pending code remains masked)')
