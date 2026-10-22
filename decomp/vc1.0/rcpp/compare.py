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
    aliases = sorted(ri,key=lambda f:(f['start'],f['end']))
    for left,right in zip(aliases,aliases[1:]):
        assert left['end'] <= right['start'], 'overlapping reconstruction aliases: '+left['name']+' / '+right['name']
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

def case_spec(name):
    cfg = json.loads(Path('functions.json').read_text())
    settings = cfg['cases'][name]
    fixture = (HERE / settings['fixture']).resolve()
    if name == 'resources-rich':
        return dict(name=name, fixture=fixture, arguments=settings['arguments'],
                    expected_exit=0, stdout=fixture/'rcpp/stdout.bin',
                    stderr=fixture/'rcpp/stderr.bin',
                    outputs={'RD000360':fixture/'rcpp/RD000360'}, extra_input='RC000360')
    manifest = json.loads((fixture/'manifest.json').read_text())
    assert manifest['target_sha256'] == cfg['target']['sha256']
    for item in manifest['source_inputs']:
        data = (fixture/item['path']).read_bytes()
        assert len(data) == item['bytes'] and hashlib.sha256(data).hexdigest() == item['sha256']
    for item in manifest['toolchain_dependencies']:
        assert hashlib.sha256((wt.ROOT/item['path']).read_bytes()).hexdigest() == item['sha256']
    case = next(c for c in manifest['cases'] if c['id'] == name)
    arguments = [arg.replace('{bin}', r'T:\bin').replace('{include}',r'T:\include')
                 for arg in case['arguments']]
    for item in [case['stdout'],case['stderr']] + case['outputs']:
        data = (fixture/item['path']).read_bytes()
        assert len(data) == item['bytes'] and hashlib.sha256(data).hexdigest() == item['sha256']
    return dict(name=name, fixture=fixture, arguments=subprocess.list2cmdline(arguments),
                expected_exit=case['expected_exit'], stdout=fixture/case['stdout']['path'],
                stderr=fixture/case['stderr']['path'],
                outputs={x['output_name']:fixture/x['path'] for x in case['outputs']},
                extra_input=None, requested_output=arguments[arguments.index('-g')+1])

def location(label, spec):
    stem = SCRATCH / 'cases' / spec['name'] / label
    return SCRATCH/'runs'/spec['name']/label, stem

def run(label, exe, args, spec=None):
    spec = case_spec('resources-rich') if spec is None else spec
    cwd, stem = location(label, spec)
    if cwd.exists(): shutil.rmtree(cwd)
    shutil.copytree(spec['fixture']/'source', cwd)
    if spec['extra_input']:
        shutil.copyfile(spec['fixture']/'rcpp'/spec['extra_input'],cwd/spec['extra_input'])
    original_inputs = {p.relative_to(cwd):p.read_bytes() for p in cwd.rglob('*') if p.is_file()}
    stem.parent.mkdir(parents=True,exist_ok=True)
    command = ['wine', 'build/debug-run.exe', wt.windows(exe), args,
               wt.windows(str(stem)+'.stdout'), wt.windows(str(stem)+'.stderr'),
               wt.windows(str(stem)+'.events'), wt.windows(cwd)]
    subprocess.run(command, env=wt.environment(), check=True, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for relative, expected in original_inputs.items():
        assert (cwd/relative).read_bytes() == expected, 'target modified input: '+str(relative)
    return Path(str(stem)+'.events').read_text()

def validate(label, spec, events):
    assert events.strip() == 'exit %d'%spec['expected_exit'], events
    cwd, stem = location(label, spec)
    for stream in ('stdout','stderr'):
        actual = Path(str(stem)+'.'+stream).read_bytes()
        expected = spec[stream].read_bytes()
        assert actual == expected, (spec['name'],label,stream,actual,expected)
    report = {}
    for filename, expected_path in spec['outputs'].items():
        actual = (cwd/filename).read_bytes()
        expected = expected_path.read_bytes()
        assert actual == expected, (spec['name'],label,filename,len(actual),len(expected))
        report[filename] = dict(bytes=len(actual),sha256=hashlib.sha256(actual).hexdigest())
    requested = spec.get('requested_output')
    if requested and requested not in spec['outputs']:
        assert not (cwd/requested).exists(), (spec['name'], 'unexpected output',requested)
    return report

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

def compare(count=None, case_name="resources-rich"):
    cfg, original, rec, oi, ri = indexes()
    accepted = cfg['accepted'] if count is None else cfg['accepted'][:count]
    for name in accepted:
        assert any(f['name'] == name for f in oi), name
        assert any(f['name'] == name for f in ri), name
    mask(original, oi, accepted, 'build/original.exe')
    mask(rec, ri, accepted, 'build/reconstructed.exe')
    spec = case_spec(case_name)
    args = spec['arguments']
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        a = pool.submit(run, 'original', HERE/'build/original.exe', args, spec)
        b = pool.submit(run, 'reconstructed', HERE/'build/reconstructed.exe', args, spec)
        ae, be = a.result(), b.result()
    al, bl = logical(ae, original, oi), logical(be, rec, ri)
    print('%s: original: %s; reconstruction: %s' % (case_name, al, bl), flush=True)
    assert al == bl, ae + be
    if al.startswith('exit:'):
        assert al == 'exit:%d'%spec['expected_exit'], al
        baseline = run('baseline',wt.ROOT/cfg['target']['path'],args,spec)
        assert baseline == ae
        report = {label:validate(label,spec,events) for label,events in
                  [('original',ae),('reconstructed',be),('baseline',baseline)]}
        if case_name == 'resources-rich':
            Path('validation.json').write_text(json.dumps(
                {label:outputs['RD000360'] for label,outputs in report.items()},indent=2)+'\n')
        else:
            path = Path('followup-validation.json')
            completed = json.loads(path.read_text()) if path.exists() else {}
            completed[case_name] = dict(exit=spec['expected_exit'],runs=report,exact=True,
                streams={name:dict(bytes=len(spec[name].read_bytes()),sha256=hashlib.sha256(spec[name].read_bytes()).hexdigest()) for name in ('stdout','stderr')})
            path.write_text(json.dumps(completed,indent=2)+'\n')
        print('Exact streams, status and all output bytes match for '+case_name+'.',flush=True)
    return al

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--replay',action='store_true')
    parser.add_argument('--count',type=int)
    parser.add_argument('--case',default='resources-rich')
    parser.add_argument('--original-only',action='store_true')
    opts = parser.parse_args()
    cfg = json.loads(Path('functions.json').read_text())
    selected = list(cfg['cases']) if opts.case == 'all' else [opts.case]
    if opts.original_only:
        report = {}
        for name in selected:
            spec = case_spec(name)
            events = run('baseline',wt.ROOT/cfg['target']['path'],spec['arguments'],spec)
            report[name] = dict(exit=spec['expected_exit'],outputs=validate('baseline',spec,events),exact=True,
                streams={stream:dict(bytes=len(spec[stream].read_bytes()),sha256=hashlib.sha256(spec[stream].read_bytes()).hexdigest()) for stream in ('stdout','stderr')})
            print('Original oracle verified: '+name,flush=True)
        Path('original-followup-replay.json').write_text(json.dumps(report,indent=2)+'\n')
    elif opts.replay:
        for n,expected in enumerate(cfg['accepted']):
            name = cfg.get('acceptance_cases',{}).get(expected,'resources-rich')
            assert compare(n,name) == expected
        for name in selected: compare(case_name=name)
    else:
        for name in selected: compare(opts.count,name)
