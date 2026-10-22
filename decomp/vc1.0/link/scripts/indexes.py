#!/usr/bin/env python3
"""Pure PE indexes and original-only masking for analysis; never execute a target."""
import hashlib
import json
import re
from pathlib import Path
from importlib.machinery import SourceFileLoader
from binary import PE, u32
wt = SourceFileLoader('wine_tool', str(Path(__file__).with_name('wine-tool.py'))).load_module()

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
    for line in Path('build/link.map').read_text().splitlines():
        m = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if m: result[m[1]] = int(m[2],16)
    for record in json.loads(Path("functions.json").read_text())["driver"]:
        symbol=record.get("symbol")
        if symbol in result:
            assert record["name"] not in result
            result[record["name"]]=result[symbol]
    return result

def indexes():
    orig = PE(wt.TC / 'bin/LINK.EXE')
    rec = PE('build/link.exe')
    cfg = json.loads(Path('functions.json').read_text())
    assert hashlib.sha256(orig.data).hexdigest() == cfg['target']['sha256'], 'target changed'
    original = json.loads(Path('runtime.json').read_text())
    original += imports(orig) + cfg['driver']
    reconstructed = json.loads(Path('build/runtime.json').read_text()) + imports(rec)
    names = publics()
    reconstructed = [r for r in reconstructed
                     if r['name'].startswith('import:') or names.get(r['name']) == r['start']]
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

