#!/usr/bin/env python3
"""Refresh the reviewed, finite Watcom module-end witness after a build.

This recognizes the finite reviewed object records and public boundaries.
Changed LnkInit offsets require review; it does not discover OMF functions.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
POLICY = ROOT / 'policy-data.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    # These are the two already reviewed, checksummed records of the actual
    # original-compiler contribution. A changed module requires review.
    witness = {
        'object': {'path': 'build/obj/src/wlink.c.obj'},
        'SEGDEF': {'record_hex': '99090029b602000007020173', 'length': 694},
        'PUBDEF': {'record_hex': '912d00020116446f4c696e6b46726f6d436f6d6d616e644c696e655f0000000000084c6e6b496e69745f8e01000000ff'},
        'rebuilt_PE': {'path': 'build/WLINK.EXE'},
        'rebuilt_MAP': {'path': 'build/wlink.map'},
    }
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
    # The authentic DoDefaultSystem body now follows LnkInit in this source
    # module. Its genuine PUBLIC starts exactly at the former contribution
    # end, so LnkInit needs no linker-padding descriptor in this variant.
    next_public = bytes.fromhex(
        '914300020116446f4c696e6b46726f6d436f6d6d616e644c696e655f0000000000'
        '084c6e6b496e69745f8e0100000010446f44656661756c7453797374656d5fb602000000c5')
    if raw.find(next_public) in boundaries:
        witness['SEGDEF'] = {
            'record_hex': '9909002964030000070201c4', 'length': 868}
        witness['PUBDEF']['record_hex'] = next_public.hex()
        witness['next_public'] = 'DoDefaultSystem_'
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
    if 'next_public' in witness:
        end = symbols[witness['next_public']]
        assert end - module == 694
    else:
        end = module + witness['SEGDEF']['length']
    assert entry - module == 398 and end - entry == 296
    # Adding a genuine CRT reference can change library extraction order.
    # Bind the actual next MAP public rather than assuming _setjmp is first.
    neighbor, neighbor_entry = min(
        ((name, address) for name, address in symbols.items() if address >= end),
        key=lambda item: (item[1], item[0]))
    gap = neighbor_entry - end
    # The reviewed set386 contribution requests paragraph alignment. Its
    # PUBLIC memset_ starts at offset0, so the exact next boundary is known.
    if gap not in (0, 1):
        assert neighbor == 'memset_' and 1 < gap <= 15
        assert neighbor_entry == ((end + 15) & ~15)
        library = ROOT.parents[2] / 'toolchains/watcom10.0a/lib386/NT/CLIB3R.LIB'
        assert digest(library) == 'efb920061a686cb8b105134c6c3948621b668d30aa749b2e4abc069def9eb86d'
        library_bytes = library.read_bytes()
        records = [(133949, '9909006918000000030201d7'),
                   (133993, '900e000001076d656d7365745f00000070')]
        for at, expected_hex in records:
            expected = bytes.fromhex(expected_hex)
            assert library_bytes[at:at+len(expected)] == expected
            assert sum(expected) & 255 == 0

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
        if rva <= end-0x400000-1 and end-0x400000+max(gap, 1) <= rva+size:
            matches.append(offset + end-0x400000-rva)
    assert len(matches) == 1
    assert data[matches[0]-1] == 0xc3
    if gap:
        assert data[matches[0]:matches[0]+gap] == bytes(gap)
    padding = '00' * gap
    policy = json.loads(POLICY.read_text())
    properties = policy.setdefault('function_properties', {}).setdefault('LnkInit', {})
    properties['rebuilt_extent'] = {
        'end': {'rebuilt_symbol': 'LnkInit_', 'offset': 296},
        'padding_hex': padding, 'image_sha256': digest(image),
        'map_sha256': digest(map_path),
        'object': {'path': witness['object']['path'], 'sha256': digest(obj)}
    }
    if not gap:
        del properties['rebuilt_extent']
        if not properties:
            del policy['function_properties']['LnkInit']
        if not policy['function_properties']:
            del policy['function_properties']
    # One already identified shipped CRT contribution, not an OMF matcher.
    # Its public longjmp_ starts63 bytes into the187-byte stjmp386 segment.
    # This runtime-only record lets strict tail traversal inspect the real RET;
    # nonlocal-jump behavior remains a separate public-ABI qualification.
    library = ROOT.parents[2] / 'toolchains/watcom10.0a/lib386/NT/CLIB3R.LIB'
    assert digest(library) == 'efb920061a686cb8b105134c6c3948621b668d30aa749b2e4abc069def9eb86d'
    library_bytes = library.read_bytes()
    at = 94631
    size = int.from_bytes(library_bytes[at+1:at+3], 'little') + 3
    record = library_bytes[at:at+size]
    assert record[0] == 0xa0 and sum(record) & 255 == 0
    assert record[3:6] == bytes.fromhex('010000') and len(record[6:-1]) == 187
    expected = record[6:-1][63:]
    start = symbols['longjmp_']
    # Actual library extraction order can place time_ before the exit member.
    # Keep the exact shipped187B contribution/body and next genuine public.
    next_body = min(address for address in symbols.values() if address > start)
    assert next_body == start + len(expected)

    def image_offset(address, size):
        found = []
        for index in range(count):
            _, rva, raw_size, offset = struct.unpack_from('<IIII', data, sections+index*40+8)
            if rva <= address-0x400000 and address-0x400000+size <= rva+raw_size:
                found.append(offset + address-0x400000-rva)
        assert len(found) == 1
        return found[0]

    body_offset = image_offset(start, len(expected))
    body = data[body_offset:body_offset+len(expected)]
    fixups = {18, 19, 20, 21, 26, 27, 28, 29, 42, 43, 44, 45}
    assert all(a == b for i, (a, b) in enumerate(zip(body, expected)) if i not in fixups)
    assert struct.unpack_from('<I', body, 18)[0] == start + 30
    unwind = start + 30 + struct.unpack_from('<i', body, 26)[0]
    assert data[image_offset(unwind, 6):image_offset(unwind, 6)+2] == bytes.fromhex('ff25')
    handler_slot = struct.unpack_from('<I', body, 42)[0]
    handler = struct.unpack_from('<I', data, image_offset(handler_slot, 4))[0]
    assert data[image_offset(handler, 1)] == 0xc3
    handler_public = re.search(r'0002:([0-9a-fA-F]+)\s+___longjmp_handler', map_path.read_text())
    # CODE may grow beyond one64KiB section as reconstruction advances.
    # Resolve the actual DGROUP section instead of assuming its old420000 VA.
    assert re.search(r'^DGROUP\s+0002:00000000\s+', map_path.read_text(), re.M)
    dgroup = []
    for index in range(count):
        header = sections + index * 40
        if data[header:header+8].rstrip(b'\0') == b'DGROUP':
            flags = struct.unpack_from('<I', data, header+36)[0]
            assert flags & 0x40 and not flags & 0x20
            dgroup.append(0x400000 + struct.unpack_from('<I', data, header+12)[0])
    assert len(dgroup) == 1
    assert handler_public and handler_slot == dgroup[0] + int(handler_public[1], 16)
    runtime = [{
        'name': 'longjmp', 'start': start, 'end': start + len(expected),
        'shared': [], 'code_chunks': [],
        'evidence': {'image_sha256': digest(image), 'map_sha256': digest(map_path),
                     'library_sha256': digest(library), 'member': 'stjmp386',
                     'member_public_offset': 63, 'member_segment_length': 187,
                     'body_sha256': hashlib.sha256(body).hexdigest()}
    }]
    # Shipped nexpand splits wrapper0..58, helper58..457, epilogue457..472.
    # Original old CRT combines these roles; keep the actual rebuilt ranges.
    if '_expand_' in symbols:
        expand = symbols['_expand_']
        assert symbols['_nexpand_'] == expand
        assert symbols['__HeapManager_expand_'] == expand + 58
        expected_expand = bytearray(472)
        for at, expected_offset in ((150426, 0), (150654, 175), (150865, 348)):
            length = int.from_bytes(library_bytes[at+1:at+3], 'little') + 3
            record = library_bytes[at:at+length]
            assert record[0] == 0xa1 and sum(record) & 255 == 0
            assert record[3] == 1
            offset = int.from_bytes(record[4:8], 'little')
            assert offset == expected_offset
            expected_expand[offset:offset+len(record)-9] = record[8:-1]
        offset = image_offset(expand, 472)
        actual_expand = data[offset:offset+472]
        relocations = (12, 17, 33, 47, 167, 179, 268, 333, 385, 407, 442, 459)
        relocation_bytes = {i for at in relocations for i in range(at, at+4)}
        assert all(a == b for i, (a, b) in enumerate(zip(actual_expand, expected_expand))
                   if i not in relocation_bytes)
        def map_data_symbol(name):
            found = re.search(r'0002:([0-9a-fA-F]+)\s*[+*]?\s*' + re.escape(name) + r'\s*$', map_path.read_text(), re.M)
            assert found, name
            return dgroup[0] + int(found[1], 16)
        for at, name in ((12, '__AccessNHeap'), (47, '__ReleaseNHeap'),
                         (459, '__ReleaseNHeap'), (17, '___nheapbeg'),
                         (167, '___nheapbeg'), (179, '___nheapbeg'),
                         (385, '___nheapbeg'), (407, '___nheapbeg'),
                         (268, '___fheap_clean'), (333, '___fheap_clean')):
            assert struct.unpack_from('<I', actual_expand, at)[0] == map_data_symbol(name)
        assert expand + 37 + struct.unpack_from('<i', actual_expand, 33)[0] == expand + 58
        assert expand + 446 + struct.unpack_from('<i', actual_expand, 442)[0] == symbols['_nfree_']
        assert min(address for address in symbols.values() if address > expand + 58) >= expand + 472
        runtime.append({
            'name': '_nexpand', 'start': expand, 'end': expand + 58,
            'shared': [], 'code_chunks': [[expand + 457, expand + 472]],
            'evidence': {'image_sha256': digest(image), 'map_sha256': digest(map_path),
                         'library_sha256': digest(library), 'member': 'nexpand',
                         'member_segment_length': 472, 'wrapper_offsets': [[0,58],[457,472]],
                         'contribution_sha256': hashlib.sha256(actual_expand).hexdigest()}
        })
    if 'realloc_' in symbols:
        start = symbols['realloc_']
        assert symbols['_nrealloc_'] == start
        at = 155681
        length = int.from_bytes(library_bytes[at+1:at+3], 'little') + 3
        record = library_bytes[at:at+length]
        assert record[0] == 0xa1 and sum(record) & 255 == 0
        assert record[3:8] == bytes.fromhex('0100000000')
        expected_realloc = record[8:-1]
        assert len(expected_realloc) == 131
        body = data[image_offset(start, 131):image_offset(start, 131)+131]
        calls = ((19, '_nmalloc_'), (30, '_nfree_'), (39, '_nmsize_'),
                 (48, '_nexpand_'), (62, '_nmalloc_'), (104, '_nfree_'),
                 (115, '_nexpand_'))
        fields = {i for at, _ in calls for i in range(at, at+4)}
        assert all(a == b for i,(a,b) in enumerate(zip(body, expected_realloc)) if i not in fields)
        for at, name in calls:
            assert start + at + 4 + struct.unpack_from('<i', body, at)[0] == symbols[name]
        assert body[-1] == 0xc3
        assert min(address for address in symbols.values() if address > start) >= start + 131
        runtime.append({
            'name': 'realloc', 'start': start, 'end': start + 131,
            'shared': [], 'code_chunks': [],
            'evidence': {'image_sha256': digest(image), 'map_sha256': digest(map_path),
                         'library_sha256': digest(library), 'member': 'nrealloc',
                         'member_segment_length': 131,
                         'body_sha256': hashlib.sha256(body).hexdigest()}
        })
    runtime_path = ROOT / '.build/runtime-rebuilt.json'
    runtime_path.parent.mkdir(exist_ok=True)
    runtime_path.write_text(json.dumps(runtime, indent=2) + '\n')
    POLICY.write_text(json.dumps(policy, indent=2) + '\n')
    print('LnkInit: refreshed exact object/MAP/image end; alignment bytes=' + str(gap))


if __name__ == '__main__':
    main()
