#!/usr/bin/env python3
"""Explicit original-only capture for reconstruction; never a fixture dependency."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import time
import shutil
import fcntl
from case_paths import SCRATCH, canonical_parts
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
    parser.add_argument('--entry-from', type=int)
    parser.add_argument('--entry-to', type=int)
    options = parser.parse_args()
    start = time.perf_counter()
    spec = importlib.util.spec_from_file_location('c1_original_tool',Path(__file__).with_name('wine-tool.py'))
    wt = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(wt)
    case = options.case.resolve()
    phase = json.loads((case/'manifest.json').read_text())['phases'][options.phase]
    config = json.loads((BINARY/'functions.json').read_text())
    entry_mode = options.entry_from is not None
    expected_entries = []
    if entry_mode:
        assert not options.unmasked and options.entry_to is not None
        assert 0 <= options.entry_from <= options.entry_to <= len(config['accepted'])
        projection = json.loads((BINARY/'cases/native-traces.json').read_text())
        application = {r['logical'] for r in projection['application_definitions'].values()}
        expected_entries = [n for n in config['accepted'][options.entry_from:options.entry_to] if n in application]
    original = PE(ROOT/phase['target'])
    assert hashlib.sha256(original.data).hexdigest() == phase['sha256']
    directory = options.directory.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    canonical = phase.get('canonical_working_directory')
    lease_path = SCRATCH/'vc1.0/c1/capture.lock'
    lease_path.parent.mkdir(parents=True,exist_ok=True)
    capture_lease = lease_path.open('a')
    fcntl.flock(capture_lease,fcntl.LOCK_EX)
    if canonical:
        work = SCRATCH.joinpath(*canonical_parts(canonical))
        work.mkdir(parents=True,exist_ok=True)
        for previous in work.iterdir():
            assert previous.is_file() and not previous.is_symlink(), 'unexpected capture cwd entry'
            previous.unlink()
    else:
        work = directory/'work'
        work.mkdir()
    for item in phase['inputs']:
        source = case/item['path']
        assert sha(source) == item['sha256']
        target_input = work/source.name
        target_input.write_bytes(source.read_bytes())
        if 'mtime_ns' in item:
            import os
            os.utime(target_input,ns=(item['mtime_ns'],item['mtime_ns']))
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
        if entry_mode:
            prefix = set(config['accepted'][:options.entry_from])
            for record in drivers:
                if record['name'] in accepted and record['name'] in application and record['name'] not in prefix:
                    data[original.offset(record['start'])] = 0xcc
            plan = directory/'entry-plan.txt'
            by_name = {r['name']:r for r in drivers}
            plan.write_text(''.join('%x %02x %s\n' % (by_name[n]['start']-original.base,
                original.read(by_name[n]['start'],1)[0],n) for n in expected_entries))
        target = directory/'original-masked.exe'
        target.write_bytes(data)
    flags = phase['environment']['MSC_CMD_FLAGS'].format(
        bin=wt.windows(wt.TC/'bin'), include=wt.windows(wt.TC/'include'))
    buffers = ','.join('%x'%(r['original']-original.base) for r in config['buffered_streams'])
    command = ['wine', str(options.debugger.resolve()), wt.windows(target), '',
               wt.windows(directory/'stdout'), wt.windows(directory/'stderr'),
               wt.windows(directory/'events'), canonical or wt.windows(work), 'MSC_CMD_FLAGS='+flags, buffers]
    if phase.get('stdio', {}).get('stderr_to_stdout'):
        command.append('stderr-to-stdout')
    elif entry_mode:
        command.append('separate-stderr')
    if entry_mode:
        command.append(wt.windows(plan))
    preparation = time.perf_counter()-start
    run_start = time.perf_counter()
    result = subprocess.run(command, env=wt.environment(), stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, stdin=subprocess.DEVNULL, timeout=60)
    (directory/'host-stdout').write_bytes(result.stdout)
    (directory/'host-stderr').write_bytes(result.stderr)
    (directory/'capture-status.json').write_text(json.dumps(dict(
        host_exit=result.returncode,preparation_seconds=preparation,
        execution_seconds=time.perf_counter()-run_start,total_seconds=time.perf_counter()-start,
        wine_prefix=str(wt.PREFIX),target_sha256=sha(target)),indent=2)+'\n')
    assert result.returncode == 0, result.stderr.decode('latin1')
    if entry_mode:
        events = (directory/'events').read_text()
        entries = [line.split()[1] for line in events.splitlines() if line.startswith('entry ')]
        assert entries == expected_entries, ('original first-entry sequence differs',entries,expected_entries)
        assert 'entry-divergence' not in events
    if canonical:
        shutil.copytree(work,directory/'work',copy_function=shutil.copy2)
    receipt = dict(case=str(case), phase=options.phase, unmasked=options.unmasked,
                   accepted_count=len(config['accepted']), accepted_sha256=hashlib.sha256(
                       json.dumps(config['accepted']).encode()).hexdigest(),
                   executable_sha256=sha(target), original_sha256=phase['sha256'],
                   debugger_sha256=sha(options.debugger), child_arguments=[],
                   child_environment={'MSC_CMD_FLAGS':flags},
                   child_working_directory=canonical or wt.windows(work),
                   child_stdio=phase.get('stdio', {'stderr_to_stdout':False}),
                   host_working_directory=str(work),working_directory_snapshot='work',
                   wine_prefix=str(wt.PREFIX),fixed_drives={'R:':str(ROOT),'S:':str(SCRATCH)},
                   preparation_seconds=preparation, execution_seconds=time.perf_counter()-run_start,
                   total_seconds=time.perf_counter()-start,
                   files=[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(work.iterdir())])
    if entry_mode:
        receipt['first_entry_projection'] = dict(accepted_from=options.entry_from,
            accepted_count=options.entry_to,expected=expected_entries,observed=entries,
            seeded_prefix_sha256=hashlib.sha256(json.dumps(config['accepted'][:options.entry_from]).encode()).hexdigest(),
            plan_sha256=sha(plan),source='Explicit fresh shared-prefix original first-entry traps; one-byte restore/EIP rewind, complete accepted function bodies/shared text.')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print((directory/'events').read_text().splitlines()[0])
    print(directory/'receipt.json')


if __name__ == '__main__':
    main()
