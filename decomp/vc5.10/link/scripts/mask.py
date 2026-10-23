"""Explicit ownership-based target masking, not graph discovery."""
import hashlib
import json
from pathlib import Path
import struct
from binary import PE, u32

ROOT = Path(__file__).resolve().parents[4]
BINARY = Path(__file__).resolve().parents[1]


def loader_ranges(pe):
    ranges = []
    def add(start, size, reason):
        ranges.append(dict(start=start, end=start + size, reason=reason))
    def string(address, prefix=0):
        value = pe.string(address + prefix)
        add(address, prefix + len(value.encode('latin1')) + 1, 'import string')
    directory = pe.base + u32(pe.data, pe.optional + 104)
    descriptor = directory
    while pe.read(descriptor, 20) != b'\0' * 20:
        lookup, _, _, name, iat = struct.unpack('<IIIII', pe.read(descriptor, 20))
        if not lookup:
            raise ValueError('original import names missing')
        string(pe.base + name)
        count = 0
        while True:
            value = u32(pe.read(pe.base + lookup + count * 4, 4), 0)
            if not value: break
            if not value & 0x80000000: string(pe.base + value, 2)
            count += 1
        add(pe.base + lookup, (count + 1) * 4, 'import lookup table including terminator')
        add(pe.base + iat, (count + 1) * 4, 'loader-written import address table including terminator')
        descriptor += 20
    add(directory, descriptor + 20 - directory, 'import descriptors including terminator')
    return ranges


def construct(config):
    pe = PE(ROOT / config['target']['path'])
    assert hashlib.sha256(pe.data).hexdigest() == config['target']['sha256'], 'target changed'
    section, = [s for s in pe.sections if s['flags'] & 0x20]
    begin = pe.base + section['rva']
    end = begin + section['virtual_size']
    data = bytearray(b'\xcc' * section['virtual_size'])
    ownership = loader_ranges(pe)
    records = config['driver'] + json.loads((BINARY / 'runtime.json').read_text())
    names = [item['name'] for item in records]
    assert len(names) == len(set(names)), 'duplicate function name'
    assert len(config['accepted']) == len(set(config['accepted'])), 'duplicate acceptance'
    assert set(config['accepted']) <= set(names), 'unknown accepted function'
    ordered = sorted(records, key=lambda item:item['start'])
    for record in ordered:
        assert begin <= record['start'] < record['end'] <= end, 'non-code/unbacked function'
        for item in ownership:
            assert record['end'] <= item['start'] or item['end'] <= record['start'], 'function overlaps loader data'
    for left, right in zip(ordered, ordered[1:]):
        assert left['end'] <= right['start'], 'overlapping function ownership'
    for item in config.get('text_data', []):
        if item.get('accepted'):
            assert item.get('evidence') and item.get('sha256') == hashlib.sha256(pe.read(item['start'],item['end']-item['start'])).hexdigest(), 'unevidenced text data'
            for record in records:
                assert item['end'] <= record['start'] or record['end'] <= item['start'], 'text data overlaps function ownership'
            ownership.append(dict(start=item['start'], end=item['end'], reason=item['reason']))
    accepted = set(config['accepted'])
    for item in records:
        if item['name'] in accepted:
            assert not item.get('pending_extent'), 'cannot unmask pending extent'
            assert item.get('status') == 'implemented', 'cannot unmask stub'
            ownership.append(dict(start=item['start'], end=item['end'], reason='accepted ' + item['name']))
    for item in ownership:
        start, stop = item['start'], item['end']
        if begin <= start < stop <= end:
            data[start - begin:stop - begin] = pe.read(start, stop - start)
        elif stop <= begin or start >= end:
            continue
        else:
            raise ValueError('ownership crosses code section')
    for record in records:
        if record['name'] not in accepted:
            assert data[record['start'] - begin] == 0xcc, 'unaccepted function entry unmasked'
    return struct.pack('<II', section['rva'], len(data)) + data, ownership
