"""Explicit masked-original capture; never called by native fixtures.

Run from the binary root after a canonical build. Saves terminal evidence and,
for a successful complete case, clock-normalized output verified against its
independently captured fixture. Wine and the shared-prefix capture lock are confined here.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from case_paths import C2_SCRATCH, capture_lock, canonical_parts, windows
import pe_indexes as index
from artifacts import compare


def sha(data):
    return hashlib.sha256(data).hexdigest()


def capture_locked(profile, receipt_path, entries=False, reject_plan=False, reject_restore=False):
    started = time.perf_counter()
    original, rebuilt, config, records, unused = index.indexes()
    case = config['cases'][profile]
    manifest = Path(case['fixture_manifest'])
    raw = manifest.read_bytes()
    assert sha(raw) == case['fixture_manifest_sha256']
    phase = json.loads(raw)['phases']['c2']
    assert phase['arguments'] == [] and set(phase['environment']) == {'MSC_CMD_FLAGS'}
    directory = Path(tempfile.mkdtemp(prefix='original-'+profile+'-',
                                    dir=C2_SCRATCH))
    canonical = phase['canonical_working_directory']
    work = C2_SCRATCH.parent.parent.joinpath(*canonical_parts(canonical))
    work.mkdir(parents=True,exist_ok=True)
    for previous in work.iterdir():
        assert previous.is_file() and not previous.is_symlink(), 'unexpected capture cwd entry'
        previous.unlink()
    for source in phase['inputs']:
        data = (manifest.parent/source['path']).read_bytes()
        assert sha(data) == source['sha256']
        (work/Path(source['path']).name).write_bytes(data)
    image = directory/'C23232.EXE'
    index.mask(original, records, config['accepted'], image)
    expected_entries = []
    if entries:
        cache = json.loads(Path('cases/native-traces.json').read_text())
        definitions = cache['application_definitions']
        case_cache = cache['cases'][profile]
        seeded = set(case_cache['seen'])
        accepted = set(config['accepted'])
        application = {row['logical'] for row in definitions.values()}
        drivers = {row['name']:row for row in config['driver']}
        expected_entries = [definitions[name]['logical'] for name in case_cache['expected']
                            if definitions[name]['logical'] in accepted]
        data = bytearray(image.read_bytes())
        for name, row in definitions.items():
            if row['logical'] in accepted and name not in seeded:
                data[original.offset(drivers[row['logical']]['start'])] = 0xcc
        image.write_bytes(data)
        planned = list(expected_entries)
        if reject_plan:
            assert len(planned) >= 2
            planned[0], planned[1] = planned[1], planned[0]
        plan = directory/'entry-plan.txt'
        plan.write_text(''.join('%x %02x %s\n' % (drivers[name]['start']-original.base,
            0xcc if reject_restore and position == 0 else original.read(drivers[name]['start'],1)[0],name)
            for position,name in enumerate(planned)))
    environment = index.wt.environment()
    environment['MSC_CMD_FLAGS'] = phase['environment']['MSC_CMD_FLAGS'].format(
        bin=index.wt.windows(index.wt.TC/'bin'))
    command = ['wine', windows(Path('build/debug-run.exe')), index.wt.windows(image), '',
               *[index.wt.windows(directory/name) for name in ('stdout','stderr','events')],
               index.wt.windows(work)]
    if entries:
        command.append(windows(plan))
    prepared = time.perf_counter()
    result = subprocess.run(command, env=environment, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (directory/'host-stdout').write_bytes(result.stdout)
    (directory/'host-stderr').write_bytes(result.stderr)
    executed = time.perf_counter()
    all_events = (directory/'events').read_text()
    if reject_plan or reject_restore:
        assert result.returncode == 5 and 'entry-divergence' in all_events
        receipt = dict(rejected='wrong restoration byte' if reject_restore else 'wrong first-entry plan',returncode=result.returncode,
            events=all_events,wine_prefix=str(index.wt.PREFIX),
            canonical_working_directory=canonical,total_seconds=time.perf_counter()-started)
        receipt_path.write_text(json.dumps(receipt,indent=2)+'\n')
        return receipt
    assert result.returncode == 0, (result.returncode,all_events,result.stderr)
    observed = [line.split()[1] for line in all_events.splitlines() if line.startswith('entry ')]
    assert observed == expected_entries
    events = '\n'.join(line for line in all_events.splitlines() if not line.startswith('entry '))+'\n'
    receipt_path.with_suffix('.events').write_text(all_events)
    if entries:
        shutil.copyfile(plan,receipt_path.with_suffix('.entry-plan.txt'))
    assert not (directory/'stdout').read_bytes() and not (directory/'stderr').read_bytes()
    for stream in ['stdout','stderr']:
        shutil.copyfile(directory/stream,receipt_path.with_suffix('.'+stream))
    receipt = dict(label=receipt_path.stem, profile=profile, accepted=len(config['accepted']),
                   events=events, target_sha256=sha(original.data), masked_sha256=sha(image.read_bytes()),
                   config_sha256=sha(Path('functions.json').read_bytes()), manifest_sha256=sha(raw),
                   debugger_sha256=sha(Path('build/debug-run.exe').read_bytes()),
                   debugger_source_sha256=sha(Path('src/debug-run.c').read_bytes()),
                   inputs={s['path']:s['sha256'] for s in phase['inputs']},
                   environment=phase['environment'], directory=windows(directory),
                   wine_prefix=str(index.wt.PREFIX),canonical_working_directory=canonical,
                   fixed_drives={'R:':str(index.wt.ROOT),'S:':str(C2_SCRATCH.parents[1])},
                   first_entry_projection=dict(expected=expected_entries,observed=observed,
                       seeded=case_cache['seen'] if entries else [],
                       entry_plan_sha256=sha(plan.read_bytes()) if entries else None))
    if events == 'exit 0\n':
        receipt.update(logical='exit:0', original_va=None)
        artifacts = []
        artifact_directory = receipt_path.with_suffix('')
        artifact_directory.mkdir(parents=True, exist_ok=False)
        for output in phase['outputs']:
            expected = (manifest.parent/output['path']).read_bytes()
            name = output.get('output_name', Path(output['path']).name)
            data = (work/name).read_bytes()
            comparison = compare(data, expected, output)
            target = artifact_directory/name
            shutil.copyfile(work/name, target)
            artifacts.append(dict(path=str(target), sha256=sha(data), **comparison))
        receipt['outputs'] = artifacts
    else:
        match = re.fullmatch(r'exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+) first 1\nexit 99\n',events)
        assert match, events
        address = int(match[1],16)+int(match[2],16)
        found = [r for r in records if r['start'] == address]
        if len(found)>1:
            # Only these independently identified LIBC aliases are understood.
            # The encountered __shift archive relocation names _memcpy; the
            # numeric support evidence retains the exact linked body identity.
            aliases={frozenset(('_printf','_wprintf')):'_printf',
                     frozenset(('_memcpy','_memmove')):'_memcpy'}
            names=frozenset(r['name'] for r in found)
            assert names in aliases, 'Unresolved original library aliases: '+repr(names)
            assert len({(r['start'],r['end'],r['library']) for r in found}) == 1
            receipt['indexed_aliases'] = found
            selected=aliases[names]
            receipt['alias_choice'] = ('narrow printf call' if selected=='_printf' else
                '__shift archive call relocation names _memcpy')+'; exact archive identities and actual arguments/results require separate runtime evidence'
            found = [r for r in found if r['name']==selected]
        assert len(found) == 1 and found[0]['name'] not in config['accepted']
        receipt.update(logical=found[0]['name'], original_va=hex(address))
    receipt.update(setup_seconds=prepared-started, execution_seconds=executed-prepared,
                   check_seconds=time.perf_counter()-executed, total_seconds=time.perf_counter()-started)
    receipt_path.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))
    return receipt


def capture(profile, receipt_path, entries=False, reject_plan=False, reject_restore=False):
    with capture_lock():
        return capture_locked(profile,receipt_path,entries,reject_plan,reject_restore)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', required=True)
    parser.add_argument('--receipt', required=True, type=Path)
    parser.add_argument('--entries',action='store_true',help='Explicit fresh current accepted-prefix first-entry capture')
    parser.add_argument('--reject-plan',action='store_true',help='Explicit negative reference-plan check')
    parser.add_argument('--reject-restore',action='store_true',help='Explicit wrong restoration-byte rejection check')
    args = parser.parse_args()
    assert not args.receipt.exists(), 'refusing to replace reference evidence'
    assert not (args.reject_plan and args.reject_restore)
    assert not (args.reject_plan or args.reject_restore) or args.entries
    capture(args.profile, args.receipt,args.entries,args.reject_plan,args.reject_restore)
