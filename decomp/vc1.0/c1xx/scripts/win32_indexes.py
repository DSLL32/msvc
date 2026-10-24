"""Read-only PE/function indexes for canonical compiler and ABI audits.

This module never starts a process or imports a Wine execution helper.
"""
import hashlib
import json
import re
from pathlib import Path
from binary import PE, u32

TC = Path(__file__).resolve().parents[4] / 'toolchains/1.0-nt'

def imports(pe):
    result = []
    p = pe.offset(pe.base + u32(pe.data, pe.optional + 104))
    lookup = {}
    while u32(pe.data, p+12):
        names, iat = u32(pe.data, p), u32(pe.data, p+16)
        for n in range(10000):
            v = u32(pe.data, pe.offset(pe.base + names + 4*n))
            if not v: break
            if v & 0x80000000: name = 'ordinal_%d' % (v & 65535)
            else: name = pe.string(pe.base + v + 2)
            lookup[pe.base + iat + n*4] = name
        p += 20
    for s in pe.sections:
        if not s['flags'] & 0x20: continue
        code = pe.data[s['off']:s['off']+s['virtual_size']]
        for i in range(len(code)-5):
            if code[i:i+2] == b'\xff\x25' and u32(code,i+2) in lookup:
                start = pe.base + s['rva'] + i
                result.append(dict(name='import:'+lookup[u32(code,i+2)], start=start,
                                   end=start+6, library='KERNEL32.LIB'))
    return result

def publics():
    result = {}
    for line in Path('build/c1xx.map').read_text().splitlines():
        m = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if m: result[m[1]] = int(m[2],16)
    return result

def indexes():
    orig = PE(TC / 'bin/C1XX3232.EXE')
    rec = PE('build/c1xx.exe')
    cfg = json.loads(Path('.build/functions.json').read_text())
    assert hashlib.sha256(orig.data).hexdigest() == cfg['target']['sha256'], 'target changed'
    for function in cfg['driver']:
        assert function['start'] < function['end'], 'invalid original extent: ' + function['name']
        assert any(s['flags'] & 0x20 and
                   orig.base + s['rva'] <= function['start'] < function['end'] <=
                   orig.base + s['rva'] + s['virtual_size'] for s in orig.sections), \
            'original extent outside code: ' + function['name']
    driver_addresses = {r['start'] for r in cfg['driver']}
    original = [r for r in json.loads(Path('runtime.json').read_text())
                if r['start'] not in driver_addresses]
    compatible=[r for r in original if r['name']=='___strgtold12']
    if compatible:
        from decimal_variant import default_record, verify
        assert compatible==[default_record()], 'unexpected compatible CRT index row'
        verify(compatible[0],original=orig)
    original += imports(orig) + cfg['driver']
    reconstructed = json.loads(Path('build/runtime.json').read_text()) + imports(rec)
    names = publics()
    for value in cfg.get('runtime_data', []):
        expected = value['value'].to_bytes(value['bytes'], 'little')
        assert orig.read(value['original'], value['bytes']) == expected, 'original CRT global changed'
        assert rec.read(names[value['name']], value['bytes']) == expected, 'reconstructed CRT global differs: '+value['name']
    driver_publics = {names[r['name']] for r in cfg['driver'] if r['name'] in names}
    reconstructed = [r for r in reconstructed
                     if r['start'] not in driver_publics and
                     (r['name'].startswith('import:') or names.get(r['name'],r['start']) == r['start'])]
    # Driver extents come from the compiler's next public function (or code end).
    code_end = max(rec.base+s['rva']+s['virtual_size'] for s in rec.sections if s['flags']&0x20)
    addresses = sorted(set(names.values()) | {code_end})
    for r in cfg['driver']:
        if r['name'] in names:
            start = names[r['name']]
            end = next(a for a in addresses if a > start)
            reconstructed.append(dict(name=r['name'], start=start, end=end))
    return orig, rec, cfg, original, reconstructed

def mask(pe, records, accepted, output):
    data = bytearray(pe.data)
    for s in pe.sections:
        if s['flags'] & 0x20:
            data[s['off']:s['off']+s['size']] = b'\xcc'*s['size']
    for r in records:
        if r['name'] in accepted:
            for start, end in [[r['start'], r['end']]] + r.get('shared', []):
                a, b = pe.offset(start), pe.offset(end-1)+1
                data[a:b] = pe.data[a:b]
    Path(output).write_bytes(data)


def logical(events, pe, records):
    m = re.search(r'exception ([0-9a-f]+) rva ([0-9a-f]+)', events)
    if not m: return 'exit:'+events.strip().split()[-1]
    addr = pe.base + int(m[2],16)
    candidates = [r for r in records if r['start'] <= addr < r['end']]
    # Public aliases with zero size aren't executed as separate functions.
    if not candidates: return 'unknown:%08x:%s' % (addr,m[1])
    r = min(candidates, key=lambda r:r['end']-r['start'])
    if m[1] != '80000003': return 'fault:%s:%s' % (m[1],r['name'])
    return r['name'] + ('+%x' % (addr-r['start']) if addr != r['start'] else '')
