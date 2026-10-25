#!/usr/bin/env python3
"""Per-role ABI residual for this one SEH owner; Rust supplies CFG facts."""
import argparse
import hashlib
import json
import re
import struct
import subprocess


def scope_bytes(image, address, count):
    # Bounded contextual data only; objdump owns PE translation, not this check.
    raw = subprocess.check_output([
        'objdump', '-s', '--start-address='+hex(address),
        '--stop-address='+hex(address+count), str(image)], text=True)
    data = bytearray()
    for line in raw.splitlines():
        match = re.match(r'^\s*[0-9a-f]+\s+((?:[0-9a-f]{2,8}\s+){1,4})', line)
        if match:
            data.extend(bytes.fromhex(match[1]))
    assert len(data) == count, 'contextual scope/thunk read must be exact'
    return bytes(data)


def verify(report, policy, images):
    facts = report['decoded_instructions']
    roles = policy['seh_roles']
    for side, image in images.items():
        identity = facts['inputs']['images'][side]
        assert identity['original_file_sha256'] == hashlib.sha256(image.read_bytes()).hexdigest()
        owner = facts['functions'][roles['owner']][side]
        start = owner['record']['start']
        role = roles[side]
        filter_at, handler_at = start+role['filter'], start+role['handler']
        assert set(owner['effective_roots']) == {start, filter_at, handler_at}
        assert owner['checked_ret_cleanup_set'] == [0, 8]
        instructions = {row['address']: bytes.fromhex(row['bytes_hex']) for row in owner['instructions']}
        memory = {}
        for address, data in instructions.items():
            for offset, byte in enumerate(data):
                assert address+offset not in memory
                memory[address+offset] = byte
        def code(address, count):
            return bytes(memory[address+i] for i in range(count))
        prologue = code(start, 29)
        assert prologue[:6] == bytes.fromhex('558bec6aff68') and prologue[10] == 0x68
        assert prologue[15:] == bytes.fromhex('64a1000000005064892500000000')
        scope, exception = struct.unpack_from('<I', prologue, 6)[0], struct.unpack_from('<I', prologue, 11)[0]
        assert struct.unpack('<3I', scope_bytes(image, scope, 12)) == (roles['scope_level'], filter_at, handler_at)
        assert code(filter_at, handler_at-filter_at).hex() == role['filter_bytes']
        assert role['filter_bytes'] in ('6a0158c3', 'b801000000c3')
        assert roles['filter_cleanup'] == 0 and roles['filter_result'] == 1
        assert instructions[start+role['end']-3] == bytes.fromhex('c20800')
        tails = [bytes.fromhex(value) for value in (
            '8b4df064890d000000005f5e5bc9c20800',
            '8b4df064890d000000005f5e5b8be55dc20800')]
        assert any(all(memory.get(start+role['end']-len(tail)+i)==byte
                       for i,byte in enumerate(tail)) for tail in tails)
        assert roles['parent_cleanup'] == 8
        assert code(handler_at, 3) == bytes.fromhex('8b65e8')
        assert bytes.fromhex('8965e8') in [value for address,value in instructions.items() if start<=address<filter_at]
        cleanup = [value for address,value in instructions.items() if handler_at<=address<start+role['end']]
        assert bytes.fromhex('834dfcff') in cleanup or bytes.fromhex('c745fcffffffff') in cleanup
        thunk = scope_bytes(image, exception, 6)
        assert thunk[:2] == bytes.fromhex('ff25')
        slot = struct.unpack_from('<I', thunk, 2)[0]
        # readobj owns import parsing. Bind this exact thunk slot to the ordered
        # entries of the named DLL's IAT, rather than accepting a bare name.
        imports = subprocess.check_output(['llvm-readobj','--coff-imports',str(image)], text=True)
        found = []
        for block in re.findall(r'Import \{(.*?)\n\}', imports, re.S):
            module = re.search(r'Name: (\S+)', block).group(1)
            iat = int(re.search(r'ImportAddressTableRVA: (0x[0-9A-Fa-f]+)', block).group(1),16)
            names = re.findall(r'^  Symbol: (\S+) \(', block,re.M)
            for index,name in enumerate(names):
                if identity['preferred_base']+iat+4*index == slot:
                    found.append((module.lower(), name))
        assert found == [(roles['exception_provider'].lower(),roles['exception_name'])]


if __name__ == '__main__':
    from pathlib import Path
    parser = argparse.ArgumentParser()
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--policy', type=Path, required=True)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--rebuilt', type=Path, required=True)
    arguments = parser.parse_args()
    verify(json.loads(arguments.report.read_text()), json.loads(arguments.policy.read_text()),
           {'original': arguments.original, 'rebuilt': arguments.rebuilt})
    print('PASS copy SEH parent RET8 / local filter RET0, EAX1 / scope and qualified handler')
