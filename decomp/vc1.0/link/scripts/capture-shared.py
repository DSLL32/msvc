#!/usr/bin/env python3
"""Explicit original-only migration capture. Never imported by a fixture."""
import argparse, copy, fcntl, hashlib, importlib.util, json, os
from pathlib import Path
import shutil, struct, subprocess, sys, time, tempfile
from case_paths import ROOT, SCRATCH, PREFIX, PROGRAM, LIBRARY, windows
from binary import PE
BINARY = Path(__file__).resolve().parents[1]
OUTPUT_ROOT = BINARY / 'cases/shared-prefix'
TRACE_CACHE = BINARY / 'cases/native-traces.json'

def module(name, path):
    s = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(s)
    s.loader.exec_module(m)
    return m

def digest(data):
    return hashlib.sha256(data).hexdigest()

def record(path):
    data = path.read_bytes()
    return dict(bytes=len(data), sha256=digest(data))

def capture(name, registry, repeat=False):
    start = time.perf_counter()
    checker = module('captured_reference_parser', BINARY / 'scripts/check-native.py')
    traces = module('native_traces', BINARY / 'scripts/native-traces.py')
    cache = json.loads(TRACE_CACHE.read_text())
    case_alias = traces.ALIASES.get(name, name)
    segment = cache['cases'][case_alias]
    old = registry[name]
    olddir = Path(old['directory'])
    oldmeta = json.loads((olddir / 'manifest.json').read_text())
    phase = copy.deepcopy(oldmeta['phases'][old['phase']])
    canonical = 'S:\\vc1.0\\link\\' + name
    work = SCRATCH / 'vc1.0/link' / name
    out = OUTPUT_ROOT / name
    if not repeat:
        assert not out.exists(), 'Refuse to overwrite captured references: ' + str(out)
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    capture_root = SCRATCH / 'vc1.0/link/captures' / name
    capture_root.mkdir(parents=True, exist_ok=True)
    capture_dir = Path(tempfile.mkdtemp(prefix='repeat-' if repeat else 'capture-', dir=capture_root))
    for item in phase['inputs']:
        source = (olddir / item['path']).resolve()
        checker.captured(source, item)
        shutil.copyfile(source, work / source.name)
    for dependency in oldmeta.get('toolchain_dependencies', []) + phase.get('toolchain_dependencies', []):
        checker.captured(ROOT / dependency['path'], dependency)
    cfg = json.loads((BINARY / 'functions.json').read_text())
    pe = PE(ROOT / 'toolchains/1.0-nt/bin/LINK.EXE')
    assert digest(pe.data) == cfg['target']['sha256'], 'Original LINK changed'
    sec = next((s for s in pe.sections if s['flags'] & 32))
    code = bytearray(b'\xcc' * sec['size'])
    accepted = set(cfg['accepted'])
    records = json.loads((BINARY / 'runtime.json').read_text()) + cfg['driver']
    idx = module('capture_indexes', BINARY / 'scripts/indexes.py')
    records += idx.imports(pe)
    for r in records:
        if r['name'] not in accepted:
            continue
        for a, b in [(r['start'], r['end'])] + [tuple(v) for v in r.get('shared', [])]:
            offset = a - pe.base - sec['rva']
            assert offset >= 0 and offset + b - a <= len(code), (r['name'], a, b)
            code[offset:offset + b - a] = pe.read(a, b - a)
    seen = set(cache['accepted_application'][:segment['seen_count']])
    lines = []
    for r in cfg['driver']:
        if r['name'] not in accepted or r.get('source') in (None, 'src/runtime.c'):
            continue
        n = traces.native_name(r['name'])
        offset = r['start'] - pe.base - sec['rva']
        byte = pe.read(r['start'], 1)[0]
        lines.append('%x %x %d %s\n' % (r['start'] - pe.base, byte, n in seen, n))
        if n not in seen:
            code[offset] = 204
    (capture_dir / 'text.bin').write_bytes(struct.pack('<II', sec['rva'], len(code)) + code)
    (capture_dir / 'entries.txt').write_text(''.join(lines))
    env = dict(os.environ, WINEPREFIX=str(PREFIX), WINEDEBUG='-all', LIB=LIBRARY, PATH=os.environ['PATH'], TZ='PST8PDT')
    for key in ['CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS', 'LINK32_DB'] + phase.get('unset_environment', []):
        env.pop(key, None)
    env.update({k: v.replace('{lib}', LIBRARY) for k, v in phase.get('environment', {}).items()})
    env['WINEPATH'] = 'R:\\toolchains\\1.0-nt\\bin'
    env['TMP'] = env['TEMP'] = windows(work)
    helper = SCRATCH / 'vc1.0/link/tooling/capture-original.exe'
    args = [windows(helper), PROGRAM, subprocess.list2cmdline(phase['arguments']), windows(capture_dir / 'stdout.bin'), windows(capture_dir / 'stderr.bin'), windows(capture_dir / 'events.jsonl'), canonical, windows(capture_dir / 'text.bin'), windows(capture_dir / 'entries.txt'), windows(capture_dir / 'before.dll'), windows(capture_dir / 'after.dll'), 'R:\\toolchains\\1.0-nt\\bin\\CVPACK.EXE' if name == 'hello-debug-dll' else '-']
    launch = time.perf_counter()
    result = subprocess.run(['wine', *args], env=env, cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
    elapsed = time.perf_counter() - launch
    (capture_dir / 'host.stdout').write_bytes(result.stdout)
    (capture_dir / 'host.stderr').write_bytes(result.stderr)
    events = [json.loads(s) for s in (capture_dir / 'events.jsonl').read_text().splitlines()]
    summary = events[-1]
    assert result.returncode == 0 and summary['failed'] == 0, (name, result.returncode, summary)
    assert summary['root_exit'] == phase['expected_exit'], summary
    observed = [e['name'] for e in events if e['event'] == 'entry']
    assert observed == segment['expected'], (name, observed, segment['expected'])
    streams = {k: (capture_dir / (k + '.bin')).read_bytes() for k in ['stdout', 'stderr']}
    parser = checker.parser_module(olddir, oldmeta)
    outputs = []
    for item in phase.get('outputs', []):
        actual = work / item['output_name']
        data = actual.read_bytes()
        kind = item.get('artifact_kind', 'plain-pe')
        fields = parser.timestamp_fields(data, kind)
        normalized = parser.normalize_artifact(data, kind) if hasattr(parser, 'normalize_artifact') else parser.normalized(data, fields)
        rec = dict(item, path='expected/' + item['output_name'], **record(actual), ignored_fields=fields, normalized_sha256=digest(normalized))
        outputs.append(rec)
    if repeat:
        frozen = json.loads((out / 'manifest.json').read_text())['phases']['reference']
        assert streams['stdout'].decode('latin1') == frozen['expected_stdout'] and streams['stderr'].decode('latin1') == frozen['expected_stderr']
        for rec in outputs:
            assert rec['normalized_sha256'] == next((x['normalized_sha256'] for x in frozen['outputs'] if x['output_name'] == rec['output_name'])), rec
    else:
        out.mkdir(parents=True, exist_ok=False)
        (out / 'expected').mkdir()
        (out / 'capture').mkdir()
        for rec in outputs:
            shutil.copyfile(work / rec['output_name'], out / rec['path'])
        for path in ['events.jsonl', 'stdout.bin', 'stderr.bin', 'entries.txt']:
            shutil.copyfile(capture_dir / path, out / 'capture' / path)
        for item in phase['inputs']:
            item['path'] = os.path.relpath((olddir / item['path']).resolve(), out)
        phase.update(canonical_working_directory=canonical, outputs=outputs, expected_stdout=streams['stdout'].decode('latin1'), expected_stderr=streams['stderr'].decode('latin1'))
        metadata = {k: copy.deepcopy(v) for k, v in oldmeta.items() if k in ['toolchain', 'artifact_parser', 'reference_artifact_parser', 'toolchain_dependencies']}
        metadata.update(case=name, phases={'reference': phase}, source_reference=str(olddir.relative_to(ROOT)), migration='Fresh original execution at fixed R:/S: paths; old frozen shared references unchanged')
        (out / 'manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')
    receipt = dict(case=name, status='pass', prefix=str(PREFIX), canonical_working_directory=canonical, original_sha256=digest(pe.data), helper_sha256=digest(helper.read_bytes()), accepted_count=len(cfg['accepted']), accepted_sha256=digest(json.dumps(cfg['accepted']).encode()), prefix_application=cache['accepted_application'][:segment['seen_count']], expected=segment['expected'], observed=observed, terminal=summary, events=record(capture_dir / 'events.jsonl'), execution_seconds=elapsed, total_seconds=time.perf_counter() - start, command=['wine', *args], capture_directory=str(capture_dir))
    (capture_dir / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    if not repeat:
        shutil.copyfile(capture_dir / 'receipt.json', out / 'capture/receipt.json')
    print(name, 'PASS', len(observed), 'entries', round(elapsed, 3), 's', flush=True)
    return receipt

def main():
    global OUTPUT_ROOT, TRACE_CACHE
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output-root', type=Path, default=OUTPUT_ROOT)
    ap.add_argument('--trace-cache', type=Path, default=TRACE_CACHE)
    ap.add_argument('--case', action='append')
    ap.add_argument('--repeat', action='store_true')
    args = ap.parse_args()
    assert (PREFIX / 'system.reg').exists(), 'Shared prefix missing'
    assert (PREFIX / 'dosdevices/r:').resolve() == ROOT
    assert (PREFIX / 'dosdevices/s:').resolve() == SCRATCH
    TRACE_CACHE = args.trace_cache.resolve()
    OUTPUT_ROOT = args.output_root.resolve()
    windows(OUTPUT_ROOT)
    registry = {n: dict(directory=str(BINARY / r['directory']), phase='reference') for n, r in json.loads((BINARY / 'cases/reference-cases.json').read_text()).items()}
    lock = (SCRATCH / 'vc1.0/link/capture.lock').open('a')
    with lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        for name in args.case or registry:
            capture(name, registry, args.repeat)
if __name__ == '__main__':
    main()
