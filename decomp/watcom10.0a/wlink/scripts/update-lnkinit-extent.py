#!/usr/bin/env python3
"""Refresh the reviewed, finite Watcom module-end witness after a build.

This recognizes exactly the two retained object records. Changed contribution
sizes/public offsets require a fresh review; it does not discover OMF functions.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WITNESS = ROOT / 'harness/lnkinit-extent.json'
POLICY = ROOT / 'policy-data.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    witness = json.loads(WITNESS.read_text())
    obj = ROOT / witness['object']['path']
    raw = obj.read_bytes()
    # Debug type metadata may move these unchanged records. Check physical
    # record boundaries, then match only the two already reviewed records.
    boundaries = set()
    cursor = 0
    while cursor < len(raw):
        boundaries.add(cursor)
        size = int.from_bytes(raw[cursor+1:cursor+3], 'little') + 3
        assert size >= 4 and cursor + size <= len(raw)
        assert sum(raw[cursor:cursor+size]) & 255 == 0
        cursor += size
    for key in ('SEGDEF', 'PUBDEF'):
        row = witness[key]
        expected = bytes.fromhex(row['record_hex'])
        at = raw.find(expected)
        assert at in boundaries and raw.find(expected, at+1) == -1, key + ' changed; review required'
        row['file_offset'] = at
        assert sum(expected) & 255 == 0
    image = ROOT / witness['rebuilt_PE']['path']
    map_path = ROOT / witness['rebuilt_MAP']['path']
    symbols = {}
    for line in map_path.read_text().splitlines():
        match = re.match(r'0001:([0-9a-fA-F]+)\s*[+*]?\s*(\S+)', line)
        if match:
            symbols[match[2]] = 0x410000 + int(match[1], 16)
    module = symbols['DoLinkFromCommandLine_']
    entry = symbols['LnkInit_']
    end = module + witness['SEGDEF']['length']
    assert entry - module == 398 and end - entry == 296
    # Adding a genuine CRT reference can change library extraction order.
    # Bind the actual next MAP public rather than assuming _setjmp is first.
    neighbor, neighbor_entry = min(
        ((name, address) for name, address in symbols.items() if address >= end),
        key=lambda item: (item[1], item[0]))
    gap = neighbor_entry - end
    assert gap in (0, 1), 'new alignment width requires review'
    data = image.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[pe:pe+4] == b'PE\0\0'
    optional = pe + 24
    assert struct.unpack_from('<I', data, optional+28)[0] == 0x400000
    sections = optional + struct.unpack_from('<H', data, pe+20)[0]
    count = struct.unpack_from('<H', data, pe+6)[0]
    matches = []
    for index in range(count):
        _, rva, size, offset = struct.unpack_from('<IIII', data, sections+index*40+8)
        if rva <= end-0x400000-1 and end-0x400000+1 <= rva+size:
            matches.append(offset + end-0x400000-rva)
    assert len(matches) == 1
    assert data[matches[0]-1] == 0xc3
    if gap:
        assert data[matches[0]] == 0
    padding = '00' if gap else ''
    for key, path in [('rebuilt_PE', image), ('rebuilt_MAP', map_path), ('object', obj)]:
        witness[key]['sha256'] = digest(path)
    witness['arithmetic'].update(module_start=module, LnkInit_entry=entry,
        contribution_end=end, last_RET=end-1, next_module_symbol=neighbor, next_module_entry=end+gap, padding_hex=padding)
    witness['scope'] = ('Exact original-compiler module contribution; ' +
        ('one-byte terminal linker alignment' if gap else 'no terminal linker alignment') +
        '. No text_data or additional body permission.')
    WITNESS.write_text(json.dumps(witness, indent=2) + '\n')
    policy = json.loads(POLICY.read_text())
    properties = policy.setdefault('function_properties', {}).setdefault('LnkInit', {})
    properties['rebuilt_extent'] = {
        'end': {'rebuilt_symbol': 'LnkInit_', 'offset': 296},
        'padding_hex': '00', 'image_sha256': digest(image),
        'map_sha256': digest(map_path), 'object': witness['object'],
        'evidence': {'path': 'harness/lnkinit-extent.json', 'sha256': digest(WITNESS)}
    }
    if not gap:
        del properties['rebuilt_extent']
        if not properties:
            del policy['function_properties']['LnkInit']
        if not policy['function_properties']:
            del policy['function_properties']
    POLICY.write_text(json.dumps(policy, indent=2) + '\n')
    print('LnkInit: refreshed exact object/MAP/image end; alignment bytes=' + str(gap))


if __name__ == '__main__':
    main()
