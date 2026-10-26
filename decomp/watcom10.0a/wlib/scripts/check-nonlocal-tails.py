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
# These actual compiler-shared chunks retain their normal return and cold
# nonlocal branch. The independent full graph/ownership audit covers all edges.
OWNERS = [
    ('AllocMemory', [(0x411fe0, 0x412021)], 0x412020, 0x412007),
    ('CheckBreak', [(0x412a43, 0x412a5c)], 0x412a5b, 0x412a56),
    ('InsertSymbol', [(0x413d52, 0x413e76), (0x413e76, 0x413e96)], 0x413e95, 0x413e71),
    ('CheckLibrarySize', [(0x413ef7, 0x413f37), (0x413ec3, 0x413ec6), (0x413e6a, 0x413e76)], 0x413ec5, 0x413e71),
    ('ReadBufferedByteStrict', [(0x41231e, 0x41233e), (0x412308, 0x41231e), (0x412471, 0x412487)], 0x41233d, 0x412482),
    ('WriteFileBlock', [(0x412446, 0x412487), (0x412487, 0x4124a1)], 0x4124a0, 0x412482),
]
CHUNK_HASHES = {
    (0x411fe0, 0x412021): 'bc22fd67e4b715bf0cf6193c550a5bb1fd09ced78035b64cb7c4cea01997c03d',
    (0x412a43, 0x412a5c): '008e627cd4cb4bae91a40222564a548cff8ab37512e00a40b4215d56c61e0f02',
    (0x413d52, 0x413e76): '7623657d708ae44f86f264c877e7c14764a326c9952704205d0cdf062ff5efda',
    (0x413e76, 0x413e96): '9b06049bb3b269df415140379ce4c69ee4e3172c66792f5978367762946c38de',
    (0x413ef7, 0x413f37): 'b25683f9332e09982040db15c1b3e1c1c937446cbd29f93f83300072f2148809',
    (0x413ec3, 0x413ec6): 'b39b98e5fec26b89a4d95ff11dcc8e9a60b6efbea74809eded6f2e897705d575',
    (0x413e6a, 0x413e76): 'b9d9bf1de941ac8924e56dd95b3acaf7b18db0427b5dd10d25d854d91adf9252',
    (0x41231e, 0x41233e): '80512e2e54e3f67ba0929473bde44b5f4956b8b711363fc4a2a21c88171aa9c1',
    (0x412308, 0x41231e): 'a82db1778a1863e7822f37a548ba6acbfa76bd47e6cadd36d32a9c54da926803',
    (0x412471, 0x412487): '5bd5c193648f29ecbcc9ef7264b1964436f628c7282e3fa25abf5dc96b536ccc',
    (0x412446, 0x412487): '41640223ffd6ecdb49b36ea90ba2e02705b82b5dd14fd35b83de368ec66cec7d',
    (0x412487, 0x4124a1): '8cd74521e4a451c8f5c5a093d4e160c9d9398813886be0747a35831692e59f3c',
}
for name, chunks, original_ret, original_tail in OWNERS:
    original_code = []
    for begin, end in chunks:
        assert hashlib.sha256(read_va(original, begin, end - begin)).hexdigest() == CHUNK_HASHES[(begin, end)]
        original_code.extend(instructions(original, begin, end))
    original_code = sorted(set(original_code))
    assert [(a, operands) for a, m, operands in original_code if m.startswith('ret')] == [(original_ret, '')]
    original_tails = [(a, m, int(operands.split()[0], 16)) for a, m, operands in original_code
                      if m.startswith('j') and not any(begin <= int(operands.split()[0], 16) < end for begin, end in chunks)]
    assert original_tails == [(original_tail, 'jmp', 0x412d76)]
    start = symbols[name + '_']
    end = min(a for a in symbols.values() if a > start)
    code = instructions(rebuilt, start, end)
    returns = [(a, operands) for a, m, operands in code if m.startswith('ret')]
    assert len(returns) == 1 and returns[0][1] == '', (name, returns)
    tails = [(a, operands) for a, m, operands in code if m == 'jmp' and not (start <= int(operands.split()[0], 16) < end)]
    # The period compiler duplicates WriteFileBlock's two error exits rather
    # than sharing the original single terminal sequence; both remain real
    # AbortCommand edges, and the full graph audit compares that identity.
    assert len(tails) == (2 if name == 'WriteFileBlock' else 1), (name, tails)
    assert all(int(operand.split()[0], 16) == symbols['AbortCommand_'] for _, operand in tails), (name, tails)
assert '#pragma aux AbortCommand aborts;' in (project / 'src/toolstate.h').read_text()
print('PASS six normal RET0 owners and exact exceptional AbortCommand→longjmp nonlocal tails (pending code remains masked)')
