#!/usr/bin/env python3
"""Mask both .text sections, run concurrently, and assert matching trap sites."""
import concurrent.futures
import hashlib
import json
import os
import re
import subprocess
import shutil
from pathlib import Path
from importlib.machinery import SourceFileLoader
from binary import PE, u32

wt = SourceFileLoader('wine_tool', str(Path(__file__).resolve().with_name('wine-tool.py'))).load_module()
os.chdir(wt.PROJECT)

COMPILE_CASES = {
    'test.c': ('compile', 'test'),
    '/O2 /G5 hello.c': ('hello', 'hello'),
}

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
    for line in Path('build/cl.map').read_text().splitlines():
        m = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if m: result[m[1]] = int(m[2],16)
    return result

def indexes():
    orig = PE(wt.TC / 'bin/CL.EXE')
    rec = PE('build/cl.exe')
    cfg = json.loads(Path('functions.json').read_text())
    assert hashlib.sha256(orig.data).hexdigest() == cfg['target']['sha256'], 'target changed'
    original = json.loads(Path('runtime.json').read_text())
    original += imports(orig) + cfg['driver']
    reconstructed = json.loads(Path('build/runtime.json').read_text()) + imports(rec)
    names = publics()
    reconstructed = [r for r in reconstructed
                     if r['name'].startswith('import:') or names.get(r['name'],r['start']) == r['start']]
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

def run(label, exe, case='/?'):
    stem = Path('build') / label
    args = ['wine', 'build/debug-run.exe', wt.windows(exe), case,
            wt.windows(str(stem)+'.stdout'), wt.windows(str(stem)+'.stderr'),
            wt.windows(str(stem)+'.events')]
    if case in COMPILE_CASES:
        prefix, source = COMPILE_CASES[case]
        directory = Path('build') / (prefix + '-' + label)
        if directory.exists():
            shutil.rmtree(directory)
        directory.mkdir()
        shutil.copyfile('cases/'+source+'.c', directory / (source+'.c'))
        args.append(wt.windows(directory))
    subprocess.run(args, env=wt.environment(), check=True, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return Path(str(stem)+'.events').read_text()

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

def prepare(accepted_count=None):
    orig, rec, cfg, oi, ri = indexes()
    accepted = cfg['accepted'] if accepted_count is None else cfg['accepted'][:accepted_count]
    for name in accepted:
        assert any(r['name'] == name for r in oi), 'missing original '+name
        assert any(r['name'] == name for r in ri), 'missing reconstruction '+name
    mask(orig, oi, accepted, 'build/original.exe')
    mask(rec, ri, accepted, 'build/reconstructed.exe')
    return orig, rec, cfg, oi, ri

def artifact_bytes(path):
    """Ignore only the COFF timestamp, which depends on the run's wall clock."""
    data = bytearray(path.read_bytes())
    if path.suffix == '.obj':
        assert data[:2] == b'\x4c\x01', 'expected i386 COFF object'
        offset = 4
    else:
        assert data[:2] == b'MZ', 'expected PE executable'
        header = u32(data, 0x3c)
        assert data[header:header+4] == b'PE\0\0', 'expected PE signature'
        offset = header + 8
    data[offset:offset+4] = b'\0'*4
    return bytes(data), offset

def check_artifacts(baseline, case):
    prefix, source = COMPILE_CASES[case]
    labels = ['original', 'reconstructed'] + (['baseline'] if baseline else [])
    report = {}
    for label in labels:
        directory = Path('build') / (prefix + '-' + label)
        assert {p.name for p in directory.iterdir()} == {source+'.c', source+'.obj', source+'.exe'}, 'unexpected compile products: '+label
    for name in (source+'.obj', source+'.exe'):
        report[name] = {}
        reference = None
        for label in labels:
            path = Path('build') / (prefix + '-' + label) / name
            normalized, offset = artifact_bytes(path)
            if reference is None:
                reference = normalized
            assert normalized == reference, name+' differs beyond COFF timestamp: '+label
            raw = path.read_bytes()
            report[name][label] = dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(),
                                      normalized_sha256=hashlib.sha256(normalized).hexdigest(),
                                      timestamp_offset=offset)
    Path('build/'+prefix+'-validation.json').write_text(json.dumps(report, indent=2)+'\n')
    if prefix == 'hello':
        check_gui(Path('build/hello-reconstructed/hello.exe'))
        print('Object and executable match (excluding COFF timestamps); GUI paint and close checks pass.')
        return
    # The generated program is part of the case, not just a file comparison.
    with concurrent.futures.ThreadPoolExecutor(len(labels)) as pool:
        runs = [pool.submit(run, 'program-'+label,
                            Path('build') / ('compile-'+label) / 'test.exe', '')
                for label in labels]
        for label, result in zip(labels, runs):
            assert result.result().strip() == 'exit 0', 'generated executable failed: '+label
            for stream in ('stdout', 'stderr'):
                assert not Path('build/program-'+label+'.'+stream).read_bytes(), 'generated executable produced output'
    print('Object and executable match (excluding COFF timestamps); generated programs exit 0.')

def check_gui(executable):
    bitmap = Path('build/hello-window.bmp')
    report = Path('build/hello-window.json')
    report.unlink(missing_ok=True)
    subprocess.run(['xvfb-run', '-a', 'wine', 'build/gui-check.exe', wt.windows(executable),
                    wt.windows(bitmap), wt.windows(report)], env=wt.environment(), check=True,
                   timeout=45, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)
    result = json.loads(report.read_text())
    assert result['exit_status'] == 0 and result['dark_pixels'] >= 20, result
    print('Hello window painted; screenshot: '+str(bitmap))

def compare(accepted_count=None, case='/?', baseline=True):
    orig, rec, cfg, oi, ri = prepare(accepted_count)
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        a = pool.submit(run, 'original', 'build/original.exe', case)
        b = pool.submit(run, 'reconstructed', 'build/reconstructed.exe', case)
        ae, be = a.result(), b.result()
    al, bl = logical(ae,orig,oi), logical(be,rec,ri)
    print('original: %s; reconstruction: %s' % (al,bl), flush=True)
    if al != bl: raise RuntimeError('logical trap mismatch\n'+ae+be)
    if al.startswith('exit:'):
        for stream in ('stdout','stderr'):
            assert Path('build/original.'+stream).read_bytes() == Path('build/reconstructed.'+stream).read_bytes(), stream+' differs'
        print('Output streams and exit status match.')
        if baseline:
            events = run('baseline', wt.TC / 'bin/CL.EXE', case)
            assert events == ae, 'unmodified original exit status differs'
            for stream in ('stdout', 'stderr'):
                assert Path('build/baseline.'+stream).read_bytes() == Path('build/original.'+stream).read_bytes(), 'unmodified original '+stream+' differs'
            print('Unmodified original output streams and exit status also match.')
        if case in COMPILE_CASES:
            assert al == 'exit:0', 'compile case failed'
            check_artifacts(baseline, case)
    return al

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', action='store_true', help='advance exact library matches one trap at a time')
    parser.add_argument('--mask-only', action='store_true')
    parser.add_argument('--replay', action='store_true', help='verify every recorded trap frontier again')
    parser.add_argument('--case', default='/?', help='Windows command-line arguments for the case')
    args = parser.parse_args()
    if args.mask_only:
        prepare()
        raise SystemExit(0)
    if args.replay:
        cfg = json.loads(Path('functions.json').read_text())
        case = next(c for c in cfg['cases'].values() if c['arguments'] == args.case)
        first, last = case.get('accepted_from', 0), case.get('accepted_count', len(cfg['accepted']))
        for count in range(first, last):
            expected = cfg['accepted'][count]
            print('frontier %d/%d: %s' % (count, last, expected), flush=True)
            actual = compare(count, args.case, baseline=False)
            assert actual == expected, 'recorded frontier changed: '+actual+' != '+expected
        compare(case=args.case)
        raise SystemExit(0)
    while True:
        site = compare(case=args.case)
        if not args.runtime or site.startswith('exit:'): break
        orig, rec, cfg, oi, ri = indexes()
        record = next((r for r in oi if r['name'] == site and 'library' in r),None)
        if record is None: break
        cfg['accepted'].append(site)
        Path('functions.json').write_text(json.dumps(cfg,indent=2)+'\n')
        with Path('build/progress.log').open('a') as f:
            f.write('%08x %s %s\n' % (record['start'],site,record['library']))
