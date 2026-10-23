#!/usr/bin/env python3
"""Verify fresh candidate-in-prefix evidence before promoting one function."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import time
from mask import BINARY, ROOT, construct


def sha(data): return hashlib.sha256(data).hexdigest()


def trial_for(current, candidate):
    assert candidate not in current['accepted'], 'candidate already accepted'
    records = current['driver'] + json.loads((BINARY / 'runtime.json').read_text())
    matches = [r for r in records if r['name'] == candidate]
    assert len(matches) == 1 and not matches[0].get('pending_extent'), 'unknown/pending candidate'
    assert matches[0].get('status') == 'implemented', 'candidate remains a stub'
    result = copy.deepcopy(current)
    result['accepted'].append(candidate)
    return result


def bind_capture(directory, expected, kind):
    receipt = json.loads((directory / 'receipt.json').read_text())
    config = (directory / 'functions.json').read_bytes()
    captured=json.loads(config)
    if kind=='original':
        assert captured['target']==expected['target'] and captured['accepted']==expected['accepted'], 'original capture is not the current trial prefix'
        assert captured['cases']['help']['arguments']==expected['cases']['help']['arguments']==['/?']
    else:
        assert captured == expected, 'captured configuration is not current trial'
        assert config == (json.dumps(expected, indent=2) + '\n').encode(), 'captured trial configuration bytes differ'
    assert sha(config) == receipt['configuration_sha256'], 'captured configuration hash differs'
    assert (directory / 'runtime.json').read_bytes() == (BINARY / 'runtime.json').read_bytes(), 'stale runtime index'
    for name, record in receipt['files'].items():
        data = (directory / name).read_bytes()
        assert len(data) == record['bytes'] and sha(data) == record['sha256'], 'capture file changed: ' + name
    if kind!='original':
        assert receipt['native_sha256'] == sha((BINARY / 'build/native/LINK.EXE').read_bytes()), 'stale native executable'
        assert receipt['rebuilt_sha256'] == sha((BINARY / 'build/link.exe').read_bytes()), 'stale rebuilt executable'
        sources = {str(p.relative_to(BINARY)):sha(p.read_bytes()) for p in sorted(p for p in (BINARY/'src').iterdir() if p.suffix in ('.c','.h','.cpp'))}
        assert receipt['source_files'] == sources, 'compiled sources changed after capture'
    for name,value in receipt.get('source_content',{}).items():
        assert sha((directory/'source'/name).read_bytes())==value, 'frozen source content changed'
    if kind == 'original':
        assert receipt['mode'] == 'masked' and receipt['helper_status'] == 0
        assert receipt['actual_loaded_crt_verified'] and receipt['actual_loaded_target_hfile_verified']
        assert receipt['original_sha256'] == expected['target']['sha256']
        assert sha((ROOT / expected['target']['path']).read_bytes()) == expected['target']['sha256']
        assert sha((directory / 'module-000.bin').read_bytes()) == expected['target']['sha256'], 'actual target hFile changed'
        # A validator/debugger build update does not change this captured target
        # execution. Its original helper identity remains in the frozen receipt.
        crt_hash = 'e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
        assert receipt['actual_loaded_crt_hfile_copies'], 'no actual CRT module hFile'
        for name in receipt['actual_loaded_crt_hfile_copies']:
            data = (directory / name).read_bytes()
            assert name.startswith('module-') and len(data) == 277776 and sha(data) == crt_hash, 'actual CRT module differs'
        mask, _ = construct(expected)
        mutable = bytearray(mask)
        mask_rva = int.from_bytes(mask[:4], 'little')
        records = expected['driver'] + json.loads((BINARY / 'runtime.json').read_text())
        expected_entries = []
        for record in records:
            if record['name'] in expected['accepted']:
                rva = record['start'] - 0x400000
                position = 8 + rva - mask_rva
                expected_entries.append('%x %x 0 %s\n' % (rva, mutable[position], record['name']))
                mutable[position] = 0xcc
        from observers import plan
        for rva,name in plan(expected['accepted']):
            position=8+rva-mask_rva
            expected_entries.append('%x %x 0 %s\n'%(rva,mutable[position],name));mutable[position]=0xcc
        mask = bytes(mutable)
        assert (directory / 'entries.txt').read_text() == ''.join(expected_entries), 'entry trace plan changed'
        assert (directory / 'mask.bin').read_bytes() == mask and receipt['mask_sha256'] == sha(mask)
    else:
        assert receipt['kind'] == 'native-i386-gdb-frontier' and receipt['status'] == 0
    return receipt


def verify_frontier(original, native, expected_name, expected):
    assert expected_name, 'nonempty expected frontier is mandatory'
    events = [json.loads(line) for line in (original / 'events.jsonl').read_text().splitlines()]
    assert events[-1] == dict(event='summary', failed=0, status=999), 'original did not stop cleanly'
    traps = [event for event in events if event['event'] == 'trap']
    assert len(traps) == 1, 'original must have exactly one terminal trap'
    records = expected['driver'] + json.loads((BINARY / 'runtime.json').read_text())
    if expected_name == '_mainCRTStartup' and not expected['accepted']:
        record = dict(start=0x465280)
    else:
        record, = [record for record in records if record['name'] == expected_name]
    assert traps[0]['address'] == record['start'], 'wrong original frontier'
    state = json.loads((native / 'state.json').read_text())
    native_name = record.get('native_symbol',{'_mainCRTStartup':'target_startup', '_main':'link_main'}.get(expected_name, expected_name.lstrip('_')))
    assert state.get('signal_number') == 5 and state.get('previous_byte') == 204 and state['function'] == native_name, 'wrong native frontier'
    for name in ['stdout.bin', 'stderr.bin']:
        assert (original / name).read_bytes() == (native / name).read_bytes(), 'partial stream differs: ' + name
    # State comparison is frontier-specific and reads raw debugger dumps. Until
    # that contract exists, a matching address/stream alone cannot accept code.
    from boundary_state import compare
    compare(expected_name, original, native, traps[0], state)
    if state.get('system_calls'):
        binding=json.loads((native/'environment/binding.json').read_text())
        assert binding['reference_sha256']==sha((original/'receipt.json').read_bytes()), 'native environment belongs to a different original sample'
        assert (native/'environment/system-after.bin').read_bytes()==(original/'system-after.bin').read_bytes()
        assert (native/'environment/memory-after.bin').read_bytes()==(original/'memory-after.bin').read_bytes()


def audit_trial(path):
    tool = ROOT / 'tooling/graph-audit/target/release/graph-audit'
    command = [str(tool), '--profile', 'link', '--discovery', 'cfg', '--repo', str(ROOT), '--project', str(BINARY),
               '--original', str(ROOT / 'toolchains/5.10/bin/LINK.EXE'), '--rebuilt', str(BINARY / 'build/link.exe'),
               '--map', str(BINARY / 'build/link.map'), '--config', str(path),
               '--runtime', str(BINARY / 'runtime.json'), '--rebuilt-runtime', str(BINARY / 'build/runtime.json'),
               '--metadata', str(BINARY / 'build/graph.json'), '--check-ret', '--check-references',
               '--check-ownership', '--check-extents', '--check-callbacks']
    trial = json.loads(path.read_text())
    runtime = json.loads((BINARY/'runtime.json').read_text())
    import_leaves = {record['name'] for record in runtime if record.get('classification')=='import'}
    strict = copy.deepcopy(trial)
    strict['accepted'] = [name for name in strict['accepted'] if name not in import_leaves]
    strict_path = path.parent/'strict-ret-config.json'
    strict_path.write_text(json.dumps(strict,indent=2)+'\n')
    inputs = [tool, ROOT/'toolchains/5.10/bin/LINK.EXE', BINARY/'build/link.exe',
              BINARY/'build/link.map',path,BINARY/'runtime.json',BINARY/'build/runtime.json',BINARY/'graph.json',BINARY/'build/graph.json']
    inputs.append(strict_path)
    cached_bodies={'_main','_initialize_debug@8','_initialize_repro','_parse_command_line@16'} & set(trial['accepted'])
    imported=copy.deepcopy(trial)
    imported['accepted']=[name for name in imported['accepted'] if name not in cached_bodies]
    import_path=path.parent/'direct-import-config.json'
    import_path.write_text(json.dumps(imported,indent=2)+'\n')
    inputs.append(import_path)
    if cached_bodies:inputs += [BINARY/'scripts/import-residual.py',BINARY/'build/driver.obj',BINARY/'build/driver-stubs.obj',BINARY/'build/reproduction.obj',BINARY/'build/arguments.obj']
    before = {str(p):sha(p.read_bytes()) for p in inputs}
    full_command = [arg for arg in command if arg!='--check-ret'] + ['--include-evidenced-runtime']
    started=time.perf_counter()
    full = subprocess.run(full_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
    strict_command = command.copy()
    strict_command[strict_command.index('--config')+1] = str(strict_path)
    strict_command.append('--include-evidenced-runtime')

    result = subprocess.run(strict_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    import_command=full_command.copy()
    import_command[import_command.index('--config')+1]=str(import_path)
    import_command.append('--check-imports')
    checked_imports=subprocess.run(import_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
    residual=None
    if cached_bodies:
        import importlib
        from binary import PE
        module=importlib.import_module('import-residual')
        residual={}
        original_pe=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');rebuilt_pe=PE(BINARY/'build/link.exe')
        map_text=(BINARY/'build/link.map').read_text()
        if '_main' in cached_bodies:residual['_main']=module.check(original_pe,rebuilt_pe,(BINARY/'build/driver.obj').read_bytes(),map_text)
        if '_initialize_debug@8' in cached_bodies:residual['_initialize_debug@8']=module.check_debug(original_pe,rebuilt_pe,(BINARY/'build/driver-stubs.obj').read_bytes(),map_text)
    if '_initialize_repro' in cached_bodies:residual['_initialize_repro']=module.check_repro(original_pe,rebuilt_pe,(BINARY/'build/reproduction.obj').read_bytes(),map_text)
    if '_parse_command_line@16' in cached_bodies:residual['_parse_command_line@16']=module.check_commandline(original_pe,rebuilt_pe,(BINARY/'build/arguments.obj').read_bytes(),map_text)
    elapsed=time.perf_counter()-started
    after = {str(p):sha(p.read_bytes()) for p in inputs}
    assert before == after, 'audit inputs changed during discovery'
    report = json.loads(result.stdout)
    full_report = json.loads(full.stdout)
    import_report=json.loads(checked_imports.stdout)
    assert set(import_report['functions'])==set(imported['accepted'])
    assert report['discovery'] == 'cfg'
    assert full_report['discovery'] == 'cfg'
    assert set(report['functions']) == set(strict['accepted']), 'strict body subset differs'
    assert set(full_report['functions']) == set(trial['accepted']), 'full trial graph subset differs'
    return dict(elapsed_seconds=elapsed,full_command=full_command,strict_command=strict_command,import_command=import_command,import_report=import_report,cached_import_residual=residual,
                strict_projection_excludes_only_verified_import_thunks=sorted(import_leaves & set(trial['accepted'])),
                input_sha256_before=before, input_sha256_after=after,
                full_report=full_report,strict_report=report,
                qualification='Exact FF25 import leaves have no local RET; typed external ABI remains at imported-library boundary. Every actual application/CRT body retains strict RET comparison.',
                stderr=full.stderr.decode()+result.stderr.decode())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--write-trial', type=Path)
    parser.add_argument('--original', type=Path)
    parser.add_argument('--native', type=Path)
    parser.add_argument('--expected-next')
    parser.add_argument('--promote', action='store_true')
    args = parser.parse_args()
    current = json.loads((BINARY / 'functions.json').read_text())
    trial = trial_for(current, args.candidate)
    if args.write_trial:
        args.write_trial.write_text(json.dumps(trial, indent=2) + '\n')
        return
    assert current.get('review_gate') == 'approved', 'root scaffold review has not approved acceptance'
    subprocess.run(['make','-q','native','original'], cwd=BINARY, check=True)
    assert args.original and args.native and args.expected_next, 'fresh captures and nonempty frontier required'
    bind_capture(args.original, trial, 'original')
    bind_capture(args.native, trial, 'native')
    verify_frontier(args.original, args.native, args.expected_next, trial)
    original_events = [json.loads(line) for line in (args.original / 'events.jsonl').read_text().splitlines()]
    assert args.candidate in [event['name'] for event in original_events if event['event']=='entry'], 'candidate not reached in original trial'
    native_state = json.loads((args.native / 'state.json').read_text())
    assert args.candidate in native_state.get('entries', []), 'candidate not reached in native trial'
    audit = audit_trial(args.original / 'functions.json')
    validation = dict(candidate=args.candidate, expected_next=args.expected_next,
                      trial_sha256=sha((args.original / 'functions.json').read_bytes()),
                      original=str(args.original), native=str(args.native), audit=audit)
    (args.native / 'acceptance-validation.json').write_text(json.dumps(validation, indent=2) + '\n')
    if args.promote:
        (BINARY / 'functions.json').write_text(json.dumps(trial, indent=2) + '\n')
    print(json.dumps(validation, indent=2))


if __name__ == '__main__': main()
