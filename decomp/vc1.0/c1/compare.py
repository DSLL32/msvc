#!/usr/bin/env python3
"""Mask both .text sections, run concurrently, and assert matching trap sites."""
import concurrent.futures
import hashlib
import json
import re
import subprocess
import shutil
from pathlib import Path
from importlib.machinery import SourceFileLoader
from binary import PE, u32

wt = SourceFileLoader('wine_tool', 'wine-tool.py').load_module()

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
    orig = PE(wt.TC / 'bin/C13232.EXE')
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

FIXTURES = {'minimal': 'minimal-c', 'hello-win32': 'hello-win32', 'hello-dll': 'hello-dll', 'numeric-varargs': 'numeric-varargs', 'pch-create': 'pch', 'pch-use': 'pch'}
CASE_NAME = 'minimal'
FIXTURE = Path('../fixtures') / FIXTURES[CASE_NAME]
CASE = json.loads((FIXTURE / 'manifest.json').read_text())['phases']['c1']

def select_case(name):
    global CASE_NAME, FIXTURE, CASE
    CASE_NAME = name
    FIXTURE = Path('../fixtures') / FIXTURES[name]
    CASE = json.loads((FIXTURE / 'manifest.json').read_text())['phases'][{'pch-create':'c1-create', 'pch-use':'c1-use'}.get(name,'c1')]

def target_namespace(label, directory):
    """Give paired children the same DOS cwd in independent private prefixes."""
    canonical = CASE.get('canonical_working_directory')
    env = wt.environment()
    if not canonical:
        return env, wt.windows(directory)
    assert canonical.startswith('N:\\') and len(canonical.split('\\')) == 2, 'unhandled canonical namespace'
    scratch = wt.PREFIX.parent / 'paired-prefixes' / label
    prefix, root = scratch / 'wine', scratch / 'root'
    if not (prefix / 'system.reg').exists():
        scratch.mkdir(parents=True, exist_ok=True)
        shutil.copytree(wt.PREFIX, prefix, symlinks=True,
                        ignore=shutil.ignore_patterns('dosdevices'))
        (prefix / 'dosdevices').mkdir()
        (prefix / 'dosdevices/c:').symlink_to('../drive_c')
        (prefix / 'dosdevices/z:').symlink_to('/')
        (prefix / 'dosdevices/n:').symlink_to(root)
    root.mkdir(parents=True, exist_ok=True)
    mapped = root / canonical.split('\\')[1]
    if mapped.is_symlink():
        mapped.unlink()
    assert not mapped.exists()
    mapped.symlink_to(directory.resolve())
    env['WINEPREFIX'] = str(prefix)
    return env, canonical

def artifact_bytes(name, data):
    """Only the explicitly captured timestamp fields vary between runs."""
    spec = next((r for r in CASE['outputs'] if r['output_name'] == name), {})
    data = bytearray(data)
    for field in spec.get('ignored_fields', []):
        a, b = field['offset'], field['offset'] + field['bytes']
        if len(data) >= b:
            data[a:b] = bytes(b-a)
    return bytes(data)

def check_cross_outputs(directory, complete=False, pending=None):
    for check in CASE.get('cross_output_checks', []):
        assert check['kind'] == 'equal-u32'
        fields = []
        for side in ('left', 'right'):
            spec = check[side]
            path = directory / spec['file']
            data = path.read_bytes() if path.exists() else b''
            data += (pending or {}).get(spec['file'], b'')
            start = spec['offset']
            if len(data) < start + 4:
                assert not complete, 'missing cross-output field: ' + str(path)
                break
            fields.append(u32(data, start))
        if len(fields) == 2:
            assert fields[0] == fields[1], 'database/reference timestamps disagree: ' + str(directory)

def run(label, exe, case='', dependencies=None):
    stem = Path('build') / label
    directory = Path('build') / (CASE_NAME + '-' + label)
    if directory.exists():
        shutil.rmtree(directory)
    directory.mkdir()
    for source in CASE['inputs']:
        data = (FIXTURE / source['path']).read_bytes()
        assert hashlib.sha256(data).hexdigest() == source['sha256'], 'fixture input changed'
        name = Path(source['path']).name
        override = (dependencies or {}).get(label, {}).get(name)
        if override is not None:
            produced = Path(override).read_bytes()
            assert produced == data, 'generated phase dependency differs: ' + name
            data = produced
        (directory / name).write_bytes(data)
    env, cwd = target_namespace(label, directory)
    flags = CASE['environment']['MSC_CMD_FLAGS'].format(
        bin=wt.windows(wt.TC / 'bin'), include=wt.windows(wt.TC / 'include'))
    args = ['wine', 'build/debug-run.exe', wt.windows(exe), case,
            wt.windows(str(stem)+'.stdout'), wt.windows(str(stem)+'.stderr'),
            wt.windows(str(stem)+'.events'), cwd,
            'MSC_CMD_FLAGS=' + flags]
    cfg = json.loads(Path('functions.json').read_text())
    if label == 'reconstructed':
        addresses = publics()
        image = PE('build/c1.exe')
        rvas = [addresses[stream['name']] - image.base for stream in cfg['buffered_streams']]
    else:
        image = PE(wt.TC/'bin/C13232.EXE')
        rvas = [stream['original'] - image.base for stream in cfg['buffered_streams']]
    args.append(','.join('%x' % rva for rva in rvas))
    subprocess.run(args, env=env, check=True, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
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

def check_artifacts(labels):
    expected = {Path(source['path']).name for source in CASE['inputs']} | {r['output_name'] for r in CASE['outputs']}
    for label in labels:
        directory = Path('build') / (CASE_NAME + '-' + label)
        assert {p.name for p in directory.iterdir()} == expected, 'unexpected files: ' + label
        check_cross_outputs(directory, complete=True)
        for output in CASE['outputs']:
            data = (directory / output['output_name']).read_bytes()
            assert len(data) == output['bytes'], output['output_name'] + ' size differs: ' + label
            data = artifact_bytes(output['output_name'], data)
            digest = output.get('normalized_sha256', output['sha256'])
            assert hashlib.sha256(data).hexdigest() == digest, output['output_name'] + ' differs: ' + label
    if any(output.get('ignored_fields') for output in CASE['outputs']):
        print('All captured frontend outputs match; only declared database timestamp fields are normalized.')
    else:
        print('All captured frontend outputs match raw bytes.')

def compare(accepted_count=None, case='', baseline=True, dependencies=None):
    orig, rec, cfg, oi, ri = prepare(accepted_count)
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        a = pool.submit(run, 'original', 'build/original.exe', case, dependencies)
        b = pool.submit(run, 'reconstructed', 'build/reconstructed.exe', case, dependencies)
        ae, be = a.result(), b.result()
    al, bl = logical(ae,orig,oi), logical(be,rec,ri)
    print('original: %s; reconstruction: %s' % (al,bl), flush=True)
    if al != bl: raise RuntimeError('logical trap mismatch\n'+ae+be)
    for stream in ('stdout', 'stderr'):
        assert Path('build/original.'+stream).read_bytes() == Path('build/reconstructed.'+stream).read_bytes(), 'frontier '+stream+' differs'
    def pending_rows(events, label):
        rows, pending = [], {}
        directory = Path('build') / (CASE_NAME + '-' + label)
        for row in re.findall(r'^buffer .*$', events, re.M):
            match = re.fullmatch(r'buffer (\d+) (\d+) ([0-9a-f]*)', row)
            if match:
                stream = cfg['buffered_streams'][int(match[1])]
                name = stream.get('output_name')
                if name:
                    data = bytes.fromhex(match[3])
                    pending[name] = data
                    # The canonical auxiliary stream fits in its first buffer.
                    if not (directory / name).stat().st_size:
                        row = 'buffer %s %s %s' % (match[1], match[2], artifact_bytes(name, data).hex())
            rows.append(row)
        check_cross_outputs(directory, pending=pending)
        return rows
    pending_a = pending_rows(ae, 'original')
    pending_b = pending_rows(be, 'reconstructed')
    assert pending_a == pending_b, 'pending intermediate bytes differ\n'+ae+be
    assert not any(re.search(r'unreadable|invalid', row) for row in pending_a), \
        'could not inspect pending intermediate bytes\n'+ae
    # At each matched frontier, compare the visible file writes made so far.
    left = Path('build') / (CASE_NAME + '-original')
    right = Path('build') / (CASE_NAME + '-reconstructed')
    assert {p.name for p in left.iterdir()} == {p.name for p in right.iterdir()}, 'frontier file sets differ'
    check_cross_outputs(left)
    check_cross_outputs(right)
    for path in left.iterdir():
        assert artifact_bytes(path.name, path.read_bytes()) == artifact_bytes(path.name, (right / path.name).read_bytes()), 'frontier file differs: ' + path.name
    if al.startswith('exit:'):
        for stream in ('stdout','stderr'):
            assert Path('build/original.'+stream).read_bytes() == Path('build/reconstructed.'+stream).read_bytes(), stream+' differs'
        for stream in ('stdout', 'stderr'):
            assert Path('build/original.'+stream).read_bytes() == CASE.get('expected_'+stream, '').encode('utf-8'), 'captured '+stream+' differs'
        print('Output streams and exit status match the capture.')
        if baseline:
            events = run('baseline', wt.TC / 'bin/C13232.EXE', case, dependencies)
            assert events == ae, 'unmodified original exit status differs'
            for stream in ('stdout', 'stderr'):
                assert Path('build/baseline.'+stream).read_bytes() == Path('build/original.'+stream).read_bytes(), 'unmodified original '+stream+' differs'
            print('Unmodified original output streams and exit status also match.')
        assert al == 'exit:0', 'frontend failed'
        check_artifacts(['original', 'reconstructed'] + (['baseline'] if baseline else []))
    return al

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--paired-pch', action='store_true', help='verify generated producer PCH inputs in the dependent consumer')
    parser.add_argument('--runtime', action='store_true', help='advance exact library matches one trap at a time')
    parser.add_argument('--mask-only', action='store_true')
    parser.add_argument('--replay', action='store_true', help='verify every recorded trap frontier again')
    parser.add_argument('--fixture', choices=sorted(FIXTURES), default='minimal')
    parser.add_argument('--case', default='', help='Windows command-line arguments for the case')
    args = parser.parse_args()
    select_case(args.fixture)
    if args.paired_pch:
        assert not (args.mask_only or args.replay or args.runtime or args.case)
        select_case('pch-create')
        assert compare() == 'exit:0'
        dependencies = {label: {'state.pch': Path('build') / ('pch-create-' + label) / 'state.pch'}
                        for label in ('original', 'reconstructed', 'baseline')}
        select_case('pch-use')
        assert compare(dependencies=dependencies) == 'exit:0'
        print('Producer-to-consumer dependencies match raw bytes in all three private namespaces.')
        raise SystemExit(0)
    if args.mask_only:
        prepare()
        raise SystemExit(0)
    if args.replay:
        cfg = json.loads(Path('functions.json').read_text())
        case = cfg['cases'][args.fixture]
        assert case['arguments'] == args.case
        first, last = case.get('accepted_from', 0), case.get('accepted_count', len(cfg['accepted']))
        for count in range(first, last):
            expected = cfg['accepted'][count]
            print('frontier %d/%d: %s' % (count, last, expected), flush=True)
            actual = compare(count, args.case, baseline=False)
            assert actual == expected, 'recorded frontier changed: '+actual+' != '+expected
        compare(accepted_count=last, case=args.case)
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
