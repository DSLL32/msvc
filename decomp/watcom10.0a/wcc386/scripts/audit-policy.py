#!/usr/bin/env python3
"""Materialize the finite object-proven final-contribution alignment contracts.

This reads finite OMF contribution metadata, never discovers a CFG or identifies
CRT functions. Shared registry-export and graph-audit validate the descriptor.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct


def index(data, at):
    value = data[at]
    at += 1
    if value & 128:
        value = ((value & 127) << 8) | data[at]
        at += 1
    return value, at


def code_contribution(raw, owner):
    names, segments, publics, initialized = [''], [], [], []
    at = 0
    while at < len(raw):
        kind, size = raw[at], struct.unpack_from('<H', raw, at + 1)[0]
        end = at + 3 + size
        assert size > 0 and end <= len(raw)
        record = raw[at:end]
        assert sum(record) & 255 == 0
        data, at = record[3:-1], end
        if kind == 0x96:
            p = 0
            while p < len(data):
                n = data[p]
                p += 1
                names.append(data[p:p + n].decode('ascii'))
                p += n
        elif kind in (0x98, 0x99):
            attr, p = data[0], 1
            if attr >> 5 == 0:
                p += 3
            width = 4 if kind & 1 else 2
            length = int.from_bytes(data[p:p + width], 'little')
            p += width
            name, p = index(data, p)
            cls, p = index(data, p)
            segments.append((names[name], names[cls], length))
        elif kind in (0x90, 0x91):
            group, p = index(data, 0)
            segment, p = index(data, p)
            if segment == 0:
                p += 2
            while p < len(data):
                n = data[p]
                p += 1
                name = data[p:p + n].decode('ascii')
                p += n
                width = 4 if kind & 1 else 2
                offset = int.from_bytes(data[p:p + width], 'little')
                p += width
                _, p = index(data, p)
                publics.append((name, segment, offset))
        elif kind in (0xa0, 0xa1):
            segment, p = index(data, 0)
            width = 4 if kind & 1 else 2
            offset = int.from_bytes(data[p:p + width], 'little')
            initialized.append((segment, offset, data[p + width:]))
        elif kind in (0x8a, 0x8b):
            assert at == len(raw)
            break
    code = [(i + 1, size) for i, (name, cls, size) in enumerate(segments)
            if name == '_TEXT' and cls == 'CODE']
    if owner == 'Wait_for_return':
        total, start = 173, 113
        expected = [('CCusage_', 0), ('Wait_for_return_', 113)]
    elif owner == 'TransferMemoryFlags':
        # The final genuine varusage public ends its CODE contribution. Earlier
        # bodies grow as this case progresses; no instruction graph is inferred.
        assert len(code) == 1 and code[0][0] == 1
        total = code[0][1]
        code_publics = [(name, off) for name, seg, off in publics if seg == 1]
        assert [name for name, off in code_publics] == [
            'FindReferences_', 'SearchDefUse_', 'UseDefGlobals_', 'Use_',
            'Define_', 'CoveringDefinitions_', 'TransferAllMemBlockUsage_',
            'TransferMemBlockUsage_', 'TransferOneMemBlockUsage_',
            'TransferBlockUsage_', 'TransferTempBlockUsage_',
            'TransferOneTempBlockUsage_', 'TransferTempFlags_',
            'TransferOneTempFlag_', 'TransferMemoryFlags_']
        start = code_publics[-1][1]
        assert total - start == 157
        expected = code_publics
    else:
        assert owner == 'Length' 
        # Equal is a genuine newly encountered utils.c public, before CopyStr
        # and the still-final 43-byte Length contribution. Bind the actual OMF
        # public order and contribution end rather than stale earlier offsets.
        assert len(code) == 1 and code[0][0] == 1
        total = code[0][1]
        code_publics = [(name, off) for name, seg, off in publics if seg == 1]
        assert [name for name, off in code_publics] == [
            'Copy_', 'Equal_', 'CopyStr_', 'Length_']
        start = code_publics[-1][1]
        assert total - start == 43
        expected = code_publics
    assert code == [(1, total)]
    assert [(name, off) for name, seg, off in publics if seg == 1] == expected
    image, covered = bytearray(total), bytearray(total)
    for segment, offset, data in initialized:
        if segment != 1:
            continue
        assert 0 <= offset <= offset + len(data) <= total
        assert not any(covered[offset:offset + len(data)])
        image[offset:offset + len(data)] = data
        covered[offset:offset + len(data)] = b'\1' * len(data)
    assert all(covered)
    if owner == 'Wait_for_return':
        assert image[start:start + 14] == bytes.fromhex('53515256575589e581ec00000000')
        assert image[-7:] == bytes.fromhex('5d5f5e5a595bc3')
    else:
        assert image[-1] == 0xc3
    return total - start


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    image_path, map_path = project / 'build/WCC386.EXE', project / 'build/wcc386.map'
    publics = [(int(a, 16), name) for a, name in re.findall(
        r'^0001:([0-9a-fA-F]{8})[ *+]\s+(\S+)', map_path.read_text(), re.M)]
    policy = json.loads((project / 'harness/policy.json').read_text())
    image = image_path.read_bytes()
    pe = struct.unpack_from('<I', image, 0x3c)[0]
    assert image[:2] == b'MZ' and image[pe:pe + 4] == b'PE\0\0'
    count, optional = struct.unpack_from('<H', image, pe + 6)[0], struct.unpack_from('<H', image, pe + 20)[0]
    sections = pe + 24 + optional
    code = []
    for k in range(count):
        at = sections + 40 * k
        if struct.unpack_from('<I', image, at + 36)[0] & 0x20:
            code.append(struct.unpack_from('<III', image, at + 12))
    rva, size, offset = code[0]
    assert rva == 0x10000
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    for owner, source, expected_length in [('Wait_for_return', 'usage', 60), ('Length', 'utils', 43), ('TransferMemoryFlags', 'varusage', 157)]:
        relative = 'build/obj/src/' + source + '.c.obj'
        obj = project / relative
        length = code_contribution(obj.read_bytes(), owner)
        assert length == expected_length
        starts = [a for a, name in publics if name == owner + '_']
        assert len(starts) == 1
        end = starts[0] + length
        envelope = min(a for a, name in publics if a > starts[0])
        gap = envelope - end
        assert 0 <= gap <= 15 and envelope <= size
        assert image[offset + end:offset + envelope] == bytes(gap)
        if gap:
            policy.setdefault('function_properties', {})[owner] = {
                'rebuilt_extent': {
                    'end': {'rebuilt_symbol': owner + '_', 'offset': length},
                    'padding_hex': '00' * gap,
                    'image_sha256': digest(image_path), 'map_sha256': digest(map_path),
                    'object': {'path': relative, 'sha256': digest(obj)},
                }}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(policy, indent=2) + '\n')


if __name__ == '__main__':
    main()
