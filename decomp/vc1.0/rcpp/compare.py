#!/usr/bin/env python3
"""Replay the captured RCPP child with paired masked code and exact artifacts."""
import argparse
import concurrent.futures
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path
from importlib.machinery import SourceFileLoader
from binary import PE, u32

wt = SourceFileLoader('rcpp_wine', str(Path(__file__).with_name('wine-tool.py'))).load_module()
HERE = Path(__file__).resolve().parent
FIXTURE = HERE.parent / 'fixtures/resources-rich'
SCRATCH = wt.PREFIX.parent

def imports(pe):
    lookup = {}
    p = pe.offset(pe.base + u32(pe.data, pe.optional + 104))
    while u32(pe.data, p + 12):
        names, iat = u32(pe.data, p), u32(pe.data, p + 16)
        for n in range(10000):
            v = u32(pe.data, pe.offset(pe.base + names + n * 4))
            if not v:
                break
            name = 'ordinal_%d' % (v & 65535) if v & 0x80000000 else pe.string(pe.base + v + 2)
            lookup[pe.base + iat + n * 4] = name
        p += 20
    result = []
    for s in pe.sections:
        if not s['flags'] & 0x20:
            continue
        code = pe.data[s['off']:s['off'] + s['size']]
        for i in range(len(code) - 5):
            if code[i:i+2] == b'\xff\x25' and u32(code, i+2) in lookup:
                start = pe.base + s['rva'] + i
                result.append(dict(name='import:' + lookup[u32(code, i+2)], start=start, end=start+6))
    return result

def publics():
    found = {}
    for line in Path('build/rcpp.map').read_text().splitlines():
        m = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if m:
            found[m[1]] = int(m[2], 16)
    return found

def indexes():
    cfg = json.loads(Path('functions.json').read_text())
    original = PE(wt.ROOT / cfg['target']['path'])
    assert hashlib.sha256(original.data).hexdigest() == cfg['target']['sha256']
    rec = PE('build/rcpp.exe')
    names = publics()
    ends = sorted(set(names.values()) | {rec.base+s['rva']+s['virtual_size'] for s in rec.sections if s['flags'] & 0x20})
    assert len({f['name'] for f in cfg['functions']}) == len(cfg['functions']), 'duplicate logical function names'
    oi = imports(original) + cfg['functions']
    ordered = sorted(oi, key=lambda f:(f['start'],f['end']))
    for left, right in zip(ordered, ordered[1:]):
        assert left['end'] <= right['start'], 'overlapping original extents: '+left['name']+' / '+right['name']
    ri = imports(rec)
    for f in cfg['functions']:
        symbol = f.get('symbol', f['name'])
        assert symbol in names, symbol
        start = names[symbol]
        ri.append(dict(name=f['name'], start=start, end=next(a for a in ends if a > start)))
    return cfg, original, rec, oi, ri

def mask(pe, records, accepted, path):
    data = bytearray(pe.data)
    for s in pe.sections:
        if s['flags'] & 0x20:
            assert s['off']
            data[s['off']:s['off']+s['size']] = b'\xcc' * s['size']
    for f in records:
        if f['name'] in accepted:
            for a, b in [(f['start'], f['end'])] + f.get('shared', []):
                data[pe.offset(a):pe.offset(b-1)+1] = pe.read(a, b-a)
    Path(path).write_bytes(data)

def run(label, exe, args):
    cwd = SCRATCH / 'runs' / label
    if cwd.exists():
        shutil.rmtree(cwd)
    shutil.copytree(FIXTURE / 'source', cwd)
    shutil.copyfile(FIXTURE / 'rcpp/RC000360', cwd / 'RC000360')
    stem = SCRATCH / label
    command = ['wine', 'build/debug-run.exe', wt.windows(exe), args,
               wt.windows(str(stem)+'.stdout'), wt.windows(str(stem)+'.stderr'),
               wt.windows(str(stem)+'.events'), wt.windows(cwd)]
    subprocess.run(command, env=wt.environment(), check=True, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return Path(str(stem)+'.events').read_text()

def logical(events, pe, records):
    m = re.search(r'exception ([0-9a-f]+) rva ([0-9a-f]+)', events)
    if not m:
        assert re.fullmatch(r'exit \d+\s*', events), events
        return 'exit:' + events.split()[-1]
    address = pe.base + int(m[2], 16)
    matches = [f for f in records if f['start'] <= address < f['end']]
    if not matches:
        return 'unknown:%08x:%s' % (address, m[1])
    f = min(matches, key=lambda f: f['end']-f['start'])
    if m[1] != '80000003':
        return 'fault:' + f['name'] + ':' + m[1]
    return f['name'] + ('+%x' % (address-f['start']) if address != f['start'] else '')

def compare(count=None):
    cfg, original, rec, oi, ri = indexes()
    accepted = cfg['accepted'] if count is None else cfg['accepted'][:count]
    for name in accepted:
        assert any(f['name'] == name for f in oi), name
        assert any(f['name'] == name for f in ri), name
    mask(original, oi, accepted, 'build/original.exe')
    mask(rec, ri, accepted, 'build/reconstructed.exe')
    args = cfg['cases']['resources-rich']['arguments']
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        a = pool.submit(run, 'original', HERE/'build/original.exe', args)
        b = pool.submit(run, 'reconstructed', HERE/'build/reconstructed.exe', args)
        ae, be = a.result(), b.result()
    al, bl = logical(ae, original, oi), logical(be, rec, ri)
    print('original: %s; reconstruction: %s' % (al, bl), flush=True)
    assert al == bl, ae + be
    if al.startswith('exit:'):
        assert al == 'exit:0', al
        baseline = run('baseline', wt.ROOT / cfg['target']['path'], args)
        assert baseline == ae
        report = {}
        for label in ('original', 'reconstructed', 'baseline'):
            report[label] = {}
            for name in ('stdout', 'stderr'):
                data = Path(str(SCRATCH/label)+'.'+name).read_bytes()
                assert data == (FIXTURE/'rcpp'/ (name+'.bin')).read_bytes()
            data = (SCRATCH/'runs'/label/'RD000360').read_bytes()
            assert data == (FIXTURE/'rcpp/RD000360').read_bytes()
            report[label] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
        Path('validation.json').write_text(json.dumps(report, indent=2)+'\n')
        print('Exact streams, exit and 336686-byte preprocessing output match.')
    return al

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--replay', action='store_true')
    parser.add_argument('--count', type=int)
    opts = parser.parse_args()
    if opts.replay:
        cfg = json.loads(Path('functions.json').read_text())
        for n, expected in enumerate(cfg['accepted']):
            assert compare(n) == expected
        compare()
    else:
        compare(opts.count)
