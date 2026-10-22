#!/usr/bin/env python3
"""Replay exact paired INT3 frontiers and complete captured RC cases."""
import argparse
import concurrent.futures
import hashlib
import importlib.machinery
import json
from pathlib import Path
import re
import shutil
import subprocess
from binary import PE, imports, u32

wt = importlib.machinery.SourceFileLoader('wine_tool', str(Path(__file__).with_name('wine-tool.py'))).load_module()
direct = importlib.machinery.SourceFileLoader('direct_case', str(Path(__file__).with_name('direct-case.py'))).load_module()
interrupt = importlib.machinery.SourceFileLoader('interrupt_case', str(Path(__file__).with_name('interrupt-case.py'))).load_module()
remaining = importlib.machinery.SourceFileLoader('remaining_case', str(Path(__file__).with_name('remaining-case.py'))).load_module()
FIXTURE = Path('../fixtures/resources-rich').resolve()
SCRATCH = Path.home() / '.ai-tmp/vc1.0-rc'
ARGUMENTS = '/dCASE_FROM_COMMAND=7 /Iinclude /foresources.res resources.rc'
CASE = 'resources-rich'
ORACLE = FIXTURE / 'rc'
EXPECTED_EXIT = 0
EXECUTION = None


def select_case(name):
    global CASE, FIXTURE, ORACLE, ARGUMENTS, EXPECTED_EXIT, EXECUTION
    cfg = json.loads(Path('functions.json').read_text())
    settings = cfg['cases'][name]
    CASE, ARGUMENTS = name, settings['arguments']
    EXECUTION = settings.get('execution')
    if EXECUTION in ('verbose-native-pid','missing-sibling-PATH'):
        remaining.prepare()
    EXPECTED_EXIT = settings.get('exit', 0)
    if settings.get('fixture'):
        FIXTURE = Path(settings['fixture']).resolve()
        ORACLE = FIXTURE / settings['oracle']
        verify_suite(settings)
    elif name == 'resources-rich':
        FIXTURE = Path('../fixtures/resources-rich').resolve()
        ORACLE = FIXTURE / 'rc'
    else:
        FIXTURE = (Path('cases') / name).resolve()
        ORACLE = FIXTURE / 'oracle'


def verify_suite(settings):
    """Pin shared fixture bytes, including the original child binary identity."""
    raw = (FIXTURE / 'manifest.json').read_bytes()
    assert hashlib.sha256(raw).hexdigest() == settings['suite_manifest_sha256']
    manifest = json.loads(raw)
    cfg = json.loads(Path('functions.json').read_text())
    assert manifest['target_sha256'] == cfg['target']['sha256']
    case = next(c for c in manifest['cases'] if c['id'] == CASE)
    assert ARGUMENTS == ' '.join(case['arguments']) and EXPECTED_EXIT == case['expected_exit']
    for record in (manifest['source_inputs'] + [case['stdout'], case['stderr']] + case['outputs'] +
                   manifest.get('analysis_helper', []) + manifest.get('evidence', []) + manifest.get('limitations', [])):
        data = (FIXTURE / record['path']).read_bytes()
        assert len(data) == record['bytes'] and hashlib.sha256(data).hexdigest() == record['sha256']
    for record in manifest['toolchain_dependencies']:
        assert hashlib.sha256((wt.ROOT / record['path']).read_bytes()).hexdigest() == record['sha256']


def work_directory(label):
    suffix = '' if CASE == 'resources-rich' else CASE + '-'
    return SCRATCH / ('case-' + suffix + label)


def captured_resource(number):
    product = (FIXTURE / 'rc/resources.res').read_bytes()
    offset = 0
    while offset < len(product):
        size, header = u32(product, offset), u32(product, offset + 4)
        assert header >= 8 and offset + header + size <= len(product)
        if size:
            if number == 0:
                return product[offset + header:offset + header + size]
            number -= 1
        offset = (offset + header + size + 3) & ~3
    raise AssertionError('missing captured resource')


def publics():
    result = {}
    for line in Path('build/rc.map').read_text().splitlines():
        match = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if match:
            # Publics precede static symbols in this linker's map. Preserve a
            # real public definition if a library has a same-named private copy.
            result.setdefault(match[1], int(match[2], 16))
    return result


def indexes():
    original = PE(wt.TC / 'bin/RC.EXE')
    reconstruction = PE('build/rc.exe')
    cfg = json.loads(Path('functions.json').read_text())
    assert hashlib.sha256(original.data).hexdigest() == cfg['target']['sha256']
    source_names = {r['name'] for r in cfg['driver']}
    oi = [r for r in json.loads(Path('runtime.json').read_text())
          if r['name'] not in source_names] + imports(original) + cfg['driver']
    names = publics()
    ri = [r for r in json.loads(Path('build/runtime.json').read_text())
          if r['name'] not in source_names] + imports(reconstruction)
    # A byte-identical RET/padding fragment can masquerade as an unrelated
    # CRT symbol. The reconstruction's map must prove both name and address.
    ri = [r for r in ri if r['name'].startswith('import:') or
          (r['name'] in names and names[r['name']] == r['start'])]
    code_end = max(reconstruction.base + s['rva'] + s['virtual_size']
                   for s in reconstruction.sections if s['flags'] & 0x20)
    boundaries = sorted(set(names.values()) | {code_end})
    for record in cfg['driver']:
        if record['name'] in names:
            start = names[record['name']]
            ri.append(dict(name=record['name'], start=start,
                           end=next(a for a in boundaries if a > start)))
    return original, reconstruction, cfg, oi, ri


def mask(pe, index, accepted, output):
    data = bytearray(pe.data)
    for section in pe.sections:
        if section['flags'] & 0x20:
            begin = section['off']
            data[begin:begin + section['size']] = b'\xcc' * section['size']
    for name in accepted:
        records = [r for r in index if r['name'] == name]
        assert records, 'missing accepted function: ' + name
        for record in records:
            for start, end in [(record['start'], record['end'])] + record.get('shared', []):
                if start != end:
                    begin, finish = pe.offset(start), pe.offset(end - 1) + 1
                    data[begin:finish] = pe.data[begin:finish]
    Path(output).write_bytes(data)


def prepare(count=None):
    original, reconstruction, cfg, oi, ri = indexes()
    accepted = cfg['accepted'] if count is None else cfg['accepted'][:count]
    mask(original, oi, accepted, 'build/original.exe')
    mask(reconstruction, ri, accepted, 'build/reconstructed.exe')
    return original, reconstruction, cfg, oi, ri


def run(label, executable):
    child = Path(executable).resolve().parent / 'rcpp.exe'
    if EXECUTION != 'direct-helper':
        assert child.is_symlink() and child.resolve() == (wt.TC / 'bin/RCPP.EXE').resolve(), 'RC must execute original RCPP'
    work = work_directory(label)
    if work.exists():
        shutil.rmtree(work)
    if (FIXTURE / 'source').exists():
        shutil.copytree(FIXTURE / 'source', work)
    else:
        work.mkdir(parents=True)
    stem = Path('build') / label
    if EXECUTION == 'direct-helper':
        return direct.run(label,executable,ARGUMENTS,work,stem,publics())
    if EXECUTION == 'real-console-ctrl-c':
        return interrupt.run(executable, ARGUMENTS, work, stem, original=label == 'original')
    if EXECUTION in ('verbose-native-pid','missing-sibling-PATH'):
        return remaining.run(label,executable,ARGUMENTS,work,stem,EXECUTION=='missing-sibling-PATH',publics())
    payload = Path(str(stem) + '.payload')
    payload.unlink(missing_ok=True)
    if label == 'original':
        pointer_rva, size_rva = 0x15030, 0x15034
    else:
        base = PE(executable).base
        names = publics()
        pointer_rva = names['_resource_data'] - base
        size_rva = names['_resource_data_size'] - base
    env = wt.environment()
    env.update(TEMP=wt.windows(work), TMP=wt.windows(work))
    subprocess.run(['wine', 'build/debug-run.exe', wt.windows(executable), ARGUMENTS,
                    wt.windows(str(stem) + '.stdout'), wt.windows(str(stem) + '.stderr'),
                    wt.windows(str(stem) + '.events'), wt.windows(work),
                    wt.windows(payload), '%x' % pointer_rva, '%x' % size_rva], env=env,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL, check=True, timeout=90)
    return Path(str(stem) + '.events').read_text()


def logical(events, pe, index):
    match = re.search(r'exception ([0-9a-f]+) rva ([0-9a-f]+)', events)
    if not match:
        assert events.startswith('exit '), events
        return 'exit:' + events.split()[1]
    address = pe.base + int(match[2], 16)
    candidates = [r for r in index if r['start'] <= address < r['end']]
    if not candidates:
        return 'unknown:%08x:%s' % (address, match[1])
    # Source overrides deliberately replace older CRT/library candidates.
    names = {r['name'] for r in json.loads(Path('functions.json').read_text())['driver']}
    record = min(candidates, key=lambda r: (r['name'] not in names, r['end'] - r['start']))
    if match[1] != '80000003':
        return 'fault:%s:%s' % (match[1], record['name'])
    return record['name'] + ('+%x' % (address - record['start']) if address != record['start'] else '')


def compare(count=None):
    original, reconstruction, cfg, oi, ri = prepare(count)
    print('case: %s; accepted: %d' % (CASE, len(cfg['accepted']) if count is None else count), flush=True)
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        left = pool.submit(run, 'original', 'build/original.exe')
        right = pool.submit(run, 'reconstructed', 'build/reconstructed.exe')
        oe, revents = left.result(), right.result()
    ol, rl = logical(oe, original, oi), logical(revents, reconstruction, ri)
    print('original: %s; reconstruction: %s' % (ol, rl), flush=True)
    assert ol == rl, 'logical trap mismatch\n' + oe + revents
    if CASE == 'resources-rich' and ol in ('_write_resource@8', '_copy_resource_data@16'):
        menu = captured_resource(0)
        for label in ('original', 'reconstructed'):
            assert Path('build/' + label + '.payload').read_bytes() == menu, (label, 'menu payload differs')
        print('Both first menu payloads match the captured %d bytes exactly.' % len(menu))
    if CASE == 'resources-rich' and ol == '_resource_free@4':
        dialog = captured_resource(1)
        for label in ('original', 'reconstructed'):
            assert Path('build/' + label + '.payload').read_bytes() == dialog, (label, 'dialog payload differs')
        print('Both first dialog payloads match the captured %d bytes exactly.' % len(dialog))
    if CASE == 'resources-rich' and ol in ('_parse_dialog@4', '_parse_string_table@4'):
        expected = (FIXTURE / 'rc/resources.res').read_bytes()
        prefixes = []
        for label in ('original', 'reconstructed'):
            written = (work_directory(label) / 'resources.res').read_bytes()
            assert len(written) > 32 and expected.startswith(written), (label, 'first resource header differs')
            prefixes.append(written)
        assert prefixes[0] == prefixes[1], 'flushed output prefixes differ'
        print('Both flushed output prefixes match the captured %d bytes exactly.' % len(prefixes[0]))
    if CASE == 'resources-rich' and ol == '__freebuf':
        expected = (FIXTURE / 'rcpp/RC000360').read_bytes()
        for label in ('original', 'reconstructed'):
            temporary = list(work_directory(label).glob('RC[0-9]*'))
            assert len(temporary) == 1, (label, 'missing prepared RCPP input')
            assert temporary[0].read_bytes() == expected, (label, 'prepared RCPP input differs')
        print('Both prepared RCPP inputs match the captured 3,381 bytes exactly.')
    if ol.startswith('exit:'):
        assert ol == 'exit:%d' % EXPECTED_EXIT
        for stream in ('stdout', 'stderr'):
            expected = (ORACLE / (stream + '.bin')).read_bytes()
            for label in ('original', 'reconstructed'):
                if EXECUTION == 'verbose-native-pid' and stream == 'stdout':
                    remaining.verify_verbose(label)
                else:
                    assert Path('build/' + label + '.' + stream).read_bytes() == expected, (label, stream, 'raw stream differs')
        products = {str(p.relative_to(FIXTURE / 'source')):p.read_bytes()
                    for p in (FIXTURE / 'source').rglob('*') if p.is_file()}
        settings = cfg['cases'][CASE]
        if settings.get('fixture'):
            suite = json.loads((FIXTURE/'manifest.json').read_text())
            case = next(c for c in suite['cases'] if c['id']==CASE)
            for record in case['outputs']:
                products[record.get('output_name',Path(record['path']).name)] = (FIXTURE/record['path']).read_bytes()
        else:
            for product in ORACLE.iterdir():
                if product.name not in ('stdout.bin', 'stderr.bin') and product.is_file():
                    products[product.name] = product.read_bytes()
        for label in ('original', 'reconstructed'):
            work = work_directory(label)
            observed = {str(p.relative_to(work)):p.read_bytes() for p in work.rglob('*') if p.is_file()}
            assert observed == products, (label, 'case files or product set differ')
        if CASE == 'resources-rich':
            print('Both raw streams, exit status and 3,684-byte resource product match the oracle exactly.')
        elif EXECUTION == 'direct-helper':
            for label in ('original','reconstructed'):
                direct.retain(label,ARGUMENTS,work_directory(label),Path('build')/label)
            print('Direct helper %s: exact returned data, stdcall cleanup and nonvolatile ABI match; no CLI reachability claimed.' % ARGUMENTS)
        elif EXECUTION == 'verbose-native-pid':
            for label in ('original','reconstructed'):
                remaining.retain_verbose(label)
            print('Both verbose stdout streams match all 349 bytes with only the two exact six-digit native PID fields compared symbolically; stderr, exit status and complete case file set match the verbose-names oracle exactly.')
        else:
            print('Both raw streams, exit status and complete case file set match the %s oracle exactly.' % CASE)

    return ol


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', default='resources-rich')
    parser.add_argument('--mask-only', action='store_true')
    parser.add_argument('--all-cases', action='store_true')
    parser.add_argument('--replay', action='store_true')
    parser.add_argument('--accept', help='Accept only a matching current function frontier')
    parser.add_argument('--advance-imports', action='store_true')
    parser.add_argument('--advance-runtime', action='store_true')
    args = parser.parse_args()
    select_case(args.case)
    if args.mask_only:
        prepare()
    elif args.all_cases:
        cfg = json.loads(Path('functions.json').read_text())
        for name, settings in cfg['cases'].items():
            if settings.get('status') == 'complete':
                select_case(name)
                assert compare() == 'exit:%d' % settings.get('exit', 0)
    elif args.advance_imports or args.advance_runtime:
        while True:
            frontier = compare()
            cfg = json.loads(Path('functions.json').read_text())
            runtime = {r['name'] for r in json.loads(Path('runtime.json').read_text())}
            runtime &= {r['name'] for r in json.loads(Path('build/runtime.json').read_text())}
            source = {r['name'] for r in cfg['driver']}
            if not (frontier.startswith('import:') or
                    (args.advance_runtime and frontier in runtime - source)):
                break
            assert frontier not in cfg['accepted']
            cfg['accepted'].append(frontier)
            cfg.setdefault('frontier_cases', {})[frontier] = CASE
            Path('functions.json').write_text(json.dumps(cfg, indent=2) + '\n')
    elif args.replay:
        cfg = json.loads(Path('functions.json').read_text())
        for number, name in enumerate(cfg['accepted']):
            select_case(cfg.get('frontier_cases', {}).get(name, 'resources-rich'))
            assert compare(number) == name
        for name, settings in cfg['cases'].items():
            if settings.get('status') == 'complete' or name == args.case:
                select_case(name)
                assert compare() == 'exit:%d' % settings.get('exit', 0)
    elif args.accept:
        assert compare() == args.accept
        cfg = json.loads(Path('functions.json').read_text())
        assert args.accept not in cfg['accepted']
        cfg['accepted'].append(args.accept)
        cfg.setdefault('frontier_cases', {})[args.accept] = CASE
        Path('functions.json').write_text(json.dumps(cfg, indent=2) + '\n')
        compare()
    else:
        compare()
