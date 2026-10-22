#!/usr/bin/env python3
"""Explicit original-only capture for reconstruction; never a fixture dependency."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import time
from binary import PE
from images import imports

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', required=True, type=Path)
    parser.add_argument('--phase', default='c1')
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--debugger', required=True, type=Path,
                        help='Explicit original-only Win32 debugger executable')
    parser.add_argument('--unmasked', action='store_true')
    options = parser.parse_args()
    start = time.perf_counter()
    spec = importlib.util.spec_from_file_location('c1_original_tool',Path(__file__).with_name('wine-tool.py'))
    wt = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(wt)
    case = options.case.resolve()
    phase = json.loads((case/'manifest.json').read_text())['phases'][options.phase]
    config = json.loads((BINARY/'functions.json').read_text())
    original = PE(ROOT/phase['target'])
    assert hashlib.sha256(original.data).hexdigest() == phase['sha256']
    directory = options.directory.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    work = directory/'work'
    work.mkdir()
    for item in phase['inputs']:
        source = case/item['path']
        assert sha(source) == item['sha256']
        (work/source.name).write_bytes(source.read_bytes())
    for item in phase.get('toolchain_dependencies', []):
        assert sha(ROOT/item['path']) == item['sha256']
    target = ROOT/phase['target']
    if not options.unmasked:
        drivers = config['driver']
        addresses = {r['start'] for r in drivers}
        records = [r for r in json.loads((BINARY/'runtime.json').read_text()) if r['start'] not in addresses]
        records += imports(original) + drivers
        data = bytearray(original.data)
        for section in original.sections:
            if section['flags'] & 0x20:
                data[section['off']:section['off']+section['size']] = b'\xcc'*section['size']
        accepted = set(config['accepted'])
        for record in records:
            if record['name'] not in accepted:
                continue
            for left,right in [[record['start'],record['end']]]+record.get('shared',[]):
                a,b = original.offset(left),original.offset(right-1)+1
                data[a:b] = original.data[a:b]
        target = directory/'original-masked.exe'
        target.write_bytes(data)
    flags = phase['environment']['MSC_CMD_FLAGS'].format(
        bin=wt.windows(wt.TC/'bin'), include=wt.windows(wt.TC/'include'))
    buffers = ','.join('%x'%(r['original']-original.base) for r in config['buffered_streams'])
    command = ['wine', str(options.debugger.resolve()), wt.windows(target), '',
               wt.windows(directory/'stdout'), wt.windows(directory/'stderr'),
               wt.windows(directory/'events'), wt.windows(work), 'MSC_CMD_FLAGS='+flags, buffers]
    preparation = time.perf_counter()-start
    run_start = time.perf_counter()
    result = subprocess.run(command, env=wt.environment(), stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, stdin=subprocess.DEVNULL, timeout=60)
    (directory/'host-stdout').write_bytes(result.stdout)
    (directory/'host-stderr').write_bytes(result.stderr)
    assert result.returncode == 0, result.stderr.decode('latin1')
    receipt = dict(case=str(case), phase=options.phase, unmasked=options.unmasked,
                   accepted_count=len(config['accepted']), accepted_sha256=hashlib.sha256(
                       json.dumps(config['accepted']).encode()).hexdigest(),
                   executable_sha256=sha(target), original_sha256=phase['sha256'],
                   debugger_sha256=sha(options.debugger), child_arguments=[],
                   child_environment={'MSC_CMD_FLAGS':flags},
                   preparation_seconds=preparation, execution_seconds=time.perf_counter()-run_start,
                   total_seconds=time.perf_counter()-start,
                   files=[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(work.iterdir())])
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print((directory/'events').read_text().splitlines()[0])
    print(directory/'receipt.json')


if __name__ == '__main__':
    main()
