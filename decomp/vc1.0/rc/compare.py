#!/usr/bin/env python3
"""Replay exact paired INT3 frontiers for the resource-rich RC case."""
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

wt = importlib.machinery.SourceFileLoader('wine_tool', 'wine-tool.py').load_module()
FIXTURE = Path('../fixtures/resources-rich').resolve()
SCRATCH = Path.home() / '.ai-tmp/vc1.0-rc'
ARGUMENTS = '/dCASE_FROM_COMMAND=7 /Iinclude /foresources.res resources.rc'


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
            result[match[1]] = int(match[2], 16)
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
    ri = [r for r in ri if r['name'].startswith('import:') or names.get(r['name'], r['start']) == r['start']]
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
    work = SCRATCH / ('case-' + label)
    if work.exists():
        shutil.rmtree(work)
    shutil.copytree(FIXTURE / 'source', work)
    stem = Path('build') / label
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
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        left = pool.submit(run, 'original', 'build/original.exe')
        right = pool.submit(run, 'reconstructed', 'build/reconstructed.exe')
        oe, revents = left.result(), right.result()
    ol, rl = logical(oe, original, oi), logical(revents, reconstruction, ri)
    print('original: %s; reconstruction: %s' % (ol, rl), flush=True)
    assert ol == rl, 'logical trap mismatch\n' + oe + revents
    if ol in ('_write_resource@8', '_copy_resource_data@16'):
        menu = captured_resource(0)
        for label in ('original', 'reconstructed'):
            assert Path('build/' + label + '.payload').read_bytes() == menu, (label, 'menu payload differs')
        print('Both first menu payloads match the captured %d bytes exactly.' % len(menu))
    if ol == '_resource_free@4':
        dialog = captured_resource(1)
        for label in ('original', 'reconstructed'):
            assert Path('build/' + label + '.payload').read_bytes() == dialog, (label, 'dialog payload differs')
        print('Both first dialog payloads match the captured %d bytes exactly.' % len(dialog))
    if ol in ('_parse_dialog@4', '_parse_string_table@4'):
        expected = (FIXTURE / 'rc/resources.res').read_bytes()
        prefixes = []
        for label in ('original', 'reconstructed'):
            written = (SCRATCH / ('case-' + label) / 'resources.res').read_bytes()
            assert len(written) > 32 and expected.startswith(written), (label, 'first resource header differs')
            prefixes.append(written)
        assert prefixes[0] == prefixes[1], 'flushed output prefixes differ'
        print('Both flushed output prefixes match the captured %d bytes exactly.' % len(prefixes[0]))
    if ol == '__freebuf':
        expected = (FIXTURE / 'rcpp/RC000360').read_bytes()
        for label in ('original', 'reconstructed'):
            temporary = list((SCRATCH / ('case-' + label)).glob('RC[0-9]*'))
            assert len(temporary) == 1, (label, 'missing prepared RCPP input')
            assert temporary[0].read_bytes() == expected, (label, 'prepared RCPP input differs')
        print('Both prepared RCPP inputs match the captured 3,381 bytes exactly.')
    if ol.startswith('exit:'):
        assert ol == 'exit:0'
        for stream in ('stdout', 'stderr'):
            expected = (FIXTURE / 'rc' / (stream + '.bin')).read_bytes()
            assert Path('build/original.' + stream).read_bytes() == expected
            assert Path('build/reconstructed.' + stream).read_bytes() == expected
        expected = (FIXTURE / 'rc/resources.res').read_bytes()
        for label in ('original', 'reconstructed'):
            assert (SCRATCH / ('case-' + label) / 'resources.res').read_bytes() == expected
        print('Both raw streams, exit status and 3,684-byte resource product match the oracle exactly.')
    return ol


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mask-only', action='store_true')
    parser.add_argument('--replay', action='store_true')
    parser.add_argument('--accept', help='Accept only a matching current function frontier')
    parser.add_argument('--advance-imports', action='store_true')
    parser.add_argument('--advance-runtime', action='store_true')
    args = parser.parse_args()
    if args.mask_only:
        prepare()
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
            Path('functions.json').write_text(json.dumps(cfg, indent=2) + '\n')
    elif args.replay:
        cfg = json.loads(Path('functions.json').read_text())
        for number, name in enumerate(cfg['accepted']):
            assert compare(number) == name
        compare()
    elif args.accept:
        assert compare() == args.accept
        cfg = json.loads(Path('functions.json').read_text())
        assert args.accept not in cfg['accepted']
        cfg['accepted'].append(args.accept)
        Path('functions.json').write_text(json.dumps(cfg, indent=2) + '\n')
        compare()
    else:
        compare()
