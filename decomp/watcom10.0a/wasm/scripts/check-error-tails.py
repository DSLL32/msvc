#!/usr/bin/env python3
"""Finite RET residual for the cold InternalError and pure usage-exit paths.

The shared audit still checks all68 App graphs/extents/references and65 ordinary
RET contracts. Pending InternalError remains masked and is never reconstructed.
"""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys

project = Path(__file__).resolve().parent.parent
repo = project.parents[2]
profile = json.loads((project / 'harness/cf-profile.json').read_text())
original = repo / profile['target']['path']
raw = original.read_bytes()
assert hashlib.sha256(raw).hexdigest() == profile['target']['sha256']
payload = profile['target']['header']['image_file_offset']
rebuilt = project / 'build/WASM.EXE'


def instructions(path, start, end, raw_offset=0):
    args = ['objdump', '-D', '-Mintel', '--no-show-raw-insn']
    if raw_offset:
        args += ['-b', 'binary', '-m', 'i386']
    text = subprocess.check_output(args + ['--start-address=' + hex(start + raw_offset),
        '--stop-address=' + hex(end + raw_offset), str(path)], text=True)
    return [(int(a, 16) - raw_offset, m, operand.strip()) for a, m, operand in
        re.findall(r'^[ \t]*([0-9a-f]+):[ \t]+(\S+)[ \t]*(.*)$', text, re.M)]


def external_jumps(code, start, end, raw_offset=0):
    return [(a, m, int(operand, 16) - raw_offset) for a, m, operand in code
        if m.startswith('j') and re.fullmatch(r'0x[0-9a-f]+', operand)
        and not start <= int(operand, 16) - raw_offset < end]


def pe_bytes(address, size):
    data = rebuilt.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count, optional = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    base = struct.unpack_from('<I', data, pe + 52)[0]
    for k in range(count):
        at = pe + 24 + optional + 40 * k
        rva, length, offset = struct.unpack_from('<III', data, at + 12)
        if base + rva <= address and address + size <= base + rva + length:
            return data[offset + address - base - rva:offset + address - base - rva + size]
    raise AssertionError('unmapped rebuilt ABI bytes')


symbols = {name: 0x410000 + int(offset, 16) for offset, name in re.findall(
    r'^0001:([0-9a-fA-F]{8})[ *+]\s+(\S+)', (project / 'build/wasm.map').read_text(), re.M)}
owners = [
    ('dir_insert', 0xfa3c, 0xfce6, '2c4a71433d309f2841f5c0f4133f58208025f160e00134a81e3f37b12e7d534f',
     [0xfb2d, 0xfbbd, 0xfc51, 0xfc64, 0xfc73, 0xfcc1, 0xfce5], [0xfad2]),
    ('fix_final', 0x15e08, 0x15f55, '0da2e6cfb45e02dabbbd2a2916c67246b9528ecf38fa39847d485425aeece9d1',
     [0x15f54], [0x15e67, 0x15eec]),
]
report = []
for name, start, end, digest, returns, tails in owners:
    assert hashlib.sha256(raw[payload + start:payload + end]).hexdigest() == digest
    code = instructions(original, start, end, payload)
    assert [(a, operand) for a, m, operand in code if m.startswith('ret')] == [(a, '') for a in returns]
    assert external_jumps(code, start, end, payload) == [(a, 'jmp', 0xf6f8) for a in tails]
    begin = symbols[name + '_']
    limit = min(a for a in symbols.values() if a > begin)
    code = instructions(rebuilt, begin, limit)
    rebuilt_returns = [(a, operand) for a, m, operand in code if m.startswith('ret')]
    assert rebuilt_returns and all(not operand for _, operand in rebuilt_returns)
    jumps = external_jumps(code, begin, limit)
    assert len(jumps) == len(tails) and all(m == 'jmp' and target == symbols['InternalError_'] for _, m, target in jumps)
    report.append({'name': name, 'original_sha256': digest, 'original_ret0': returns,
        'original_error_tails': tails, 'rebuilt_extent': [begin, limit], 'rebuilt_ret0': rebuilt_returns,
        'rebuilt_error_tails': jumps, 'rebuilt_sha256': hashlib.sha256(pe_bytes(begin, limit - begin)).hexdigest()})

# Actual assertion helper has no RET and terminates through standard exit(255).
error = raw[payload + 0xf6f8:payload + 0xf724]
assert hashlib.sha256(error).hexdigest() == '2cabf02bd1a565689928a4133792fa341fc1c40c92e4c52649ab295bb7c2098f'
code = instructions(original, 0xf6f8, 0xf724, payload)
assert not any(m.startswith('ret') for _, m, _ in code)
assert external_jumps(code, 0xf6f8, 0xf724, payload) == [(0xf71f, 'jmp', 0x1cb31)]
assert raw[payload + 0xf71a:payload + 0xf71f] == bytes.fromhex('b8ff000000')
assert '#pragma aux InternalError aborts;' in (project / 'src/asminit.h').read_text()
registry = (project / 'binary.c').read_text().split('.name = "InternalError"', 1)[1].split('},', 1)[0]
assert 'RECON_UNIMPLEMENTED' in registry and '.code =' not in registry

# Pure help termination owns no ordinary RET contract. The rebuilt CALL exit(1)
# cannot return, even if this compiler emits an unreachable fallback epilogue.
assert hashlib.sha256(raw[payload + 0xcb60:payload + 0xcb74]).hexdigest() == '7edf180b312e91c27027d4ee9838f4ee251a361a77d90af5746251d72e18ce02'
assert raw[payload + 0xcb6a:payload + 0xcb6f] == bytes.fromhex('b801000000')
code = instructions(original, 0xcb60, 0xcb74, payload)
assert not any(m.startswith('ret') for _, m, _ in code)
assert external_jumps(code, 0xcb60, 0xcb74, payload) == [(0xcb6f, 'jmp', 0x1cb31)]
begin = symbols['usage_msg_']
limit = min(a for a in symbols.values() if a > begin)
code = instructions(rebuilt, begin, limit)
calls = [(a, operand) for a, m, operand in code if m == 'call']
assert [int(operand, 16) for _, operand in calls] == [symbols['PrintfUsage_'], symbols['exit_']]
assert pe_bytes(calls[-1][0] - 5, 5) == bytes.fromhex('b801000000')
report.append({'name': 'usage_msg', 'rebuilt_exit_call': calls[-1][0], 'status': 1,
    'rebuilt_sha256': hashlib.sha256(pe_bytes(begin, limit - begin)).hexdigest()})
if len(sys.argv) == 2:
    output = Path(sys.argv[1])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
print('PASS dir_insert/fix_final normal RET0 and cold InternalError→exit(255); usage_msg→exit(1), pending code stays masked')
