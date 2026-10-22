"""Pure PE indexes for explicit original-toolchain ABI and graph audits.

No process launch, masking, paired runner, Wine fallback or reference capture.
"""
import hashlib
import json
from pathlib import Path
import re
from binary import PE, u32
ROOT = Path(__file__).resolve().parents[4]
TC = ROOT / 'toolchains/1.0-nt'

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
    for line in Path('build/c1.map').read_text().splitlines():
        m = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if m: result[m[1]] = int(m[2],16)
    return result

def indexes():
    orig = PE(TC / 'bin/C13232.EXE')
    rec = PE('build/c1.exe')
    cfg = json.loads(Path('functions.json').read_text())
    assert hashlib.sha256(orig.data).hexdigest() == cfg['target']['sha256'], 'target changed'
    driver_addresses = {r['start'] for r in cfg['driver']}
    assert len(driver_addresses) == len(cfg['driver']), 'duplicate source-function entry address'
    assert len({r['name'] for r in cfg['driver']}) == len(cfg['driver']), 'duplicate logical source-function name'
    original = [r for r in json.loads(Path('runtime.json').read_text())
                if r['start'] not in driver_addresses]
    original += imports(orig) + cfg['driver']
    reconstructed = json.loads(Path('build/runtime.json').read_text()) + imports(rec)
    names = publics()
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
