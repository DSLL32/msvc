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
import re
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
    parser.add_argument('--stop-function',help='Explicit first-entry trap for one accepted application ABI observation')
    parser.add_argument('--direct-function',help='Explicit debugger-only support callback after real marker; no public-reach claim')
    parser.add_argument('--direct-arguments',nargs='*',type=lambda s:int(s,0))
    parser.add_argument('--expression-unit',type=int,choices=range(6),help='External direct expression unit; owned test input, not target-global seeding')
    parser.add_argument('--direct-body',action='store_true',help='Restore exactly selected candidate body/shared text for explicit direct verification')
    parser.add_argument('--restore-function',action='append',default=[],help='Explicit candidate body/shared-text restoration, without accepting it')
    parser.add_argument('--pdb-cleanup-probe',action='store_true',help='Observe real PDB list owner and free sequence at cleanup')
    parser.add_argument('--trace-enum-callbacks', action='store_true',help='Explicit debugger-only all-call enum comparator proof')
    parser.add_argument('--trace-flags',help='Logical function whose full scanner/parser words are observed at entry/return')
    parser.add_argument('--trace-aborted-function',action='store_true',help='Explicit original-only aborted definition globals/word masks/return observation')
    parser.add_argument('--trace-browser-limit',action='store_true',help='Explicit original-only browser-limit input and post-state observation')
    parser.add_argument('--trace-declaration-scope',action='store_true',help='Explicit original-only owned scope input and return observation')
    parser.add_argument('--trace-complete-type',action='store_true',help='Explicit original tag flag byte and stdcall return observation')
    parser.add_argument('--trace-strdup',action='store_true',help='Explicit original-only strdup input/returned copied text')
    parser.add_argument('--trace-asm-segment',action='store_true',help='Explicit original-only segment operand entry/return ABI/state')
    parser.add_argument('--trace-return8',action='store_true',help='Explicit original-only recovery-value register/stack observation')
    parser.add_argument('--trace-timestamps',action='store_true',help='Observe source descriptor metadata adapter return and output timestamp')
    parser.add_argument('--trace-function-finishes',action='store_true',help='Observe current function symbol at finish entry and caller return')
    parser.add_argument('--entry-from', type=int)
    parser.add_argument('--entry-to', type=int)
    parser.add_argument('--enum-callback-function', choices=['legacy','pdb'], default='legacy')
    options = parser.parse_args()
    enum_callback = '_compare_pdb_enumerators' if options.enum_callback_function == 'pdb' else '_compare_legacy_enumerators'
    assert options.enum_callback_function == 'legacy' or options.trace_enum_callbacks
    if options.trace_function_finishes:
        assert options.trace_flags is None
        options.trace_flags = '_finish_function_definition'
    if options.trace_timestamps:
        assert options.trace_flags is None and not options.trace_function_finishes
        options.trace_flags = '_get_file_timestamp@8'
    if options.trace_return8:
        assert options.unmasked and options.trace_flags is None
        options.trace_flags = '_recovery_token_value@4'
    if options.trace_aborted_function:
        assert options.unmasked and options.trace_flags is None
        options.trace_flags='_abort_function_definition'
    if options.trace_browser_limit:
        assert options.unmasked and options.trace_flags is None
        options.trace_flags='_browser_index_overflow'
    if options.trace_declaration_scope:
        assert options.unmasked and options.trace_flags is None
        options.trace_flags='_enclosing_declaration_scope'
    if options.trace_complete_type:
        assert options.unmasked and options.trace_flags is None
        options.trace_flags='_require_complete_type@4'
    if options.trace_strdup:
        assert options.unmasked and options.trace_flags is None
        options.trace_flags='__strdup'
    if options.trace_asm_segment:
        assert options.unmasked and options.trace_flags is None
        options.trace_flags = '_make_asm_segment_value@4'
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
        for logical in options.restore_function:
            record=next(r for r in drivers if r['name']==logical)
            for left,right in [[record['start'],record['end']]]+record.get('shared',[]):
                data[original.offset(left):original.offset(right-1)+1]=original.read(left,right-left)
        if options.restore_function:target.write_bytes(data)
    direct = None
    if options.direct_function:
        assert options.stop_function and not options.unmasked and not entry_mode
        direct = next(r for r in config['driver'] if r['name']==options.direct_function)
        assert options.direct_arguments is not None
        if options.expression_unit is None:
            assert 1 <= len(options.direct_arguments) <= 2
        else:
            assert options.stop_function=='_finish_declarations'
            assert options.direct_function==('_copy_expression_tree@8'if options.expression_unit else'_make_reusable_zero')
            assert options.direct_arguments==([0,2 if options.expression_unit>=4 else 6]if options.expression_unit else[])
        assert all(0 <= v <= 0xffffffff for v in options.direct_arguments)
        if options.direct_body:
            for left,right in [[direct['start'],direct['end']]]+direct.get('shared',[]):
                assert right-left > 1
                a,b=original.offset(left),original.offset(right-1)+1
                data[a:b]=original.data[a:b]
            target.write_bytes(data)
    else:
        assert not options.direct_body and options.direct_arguments is None
    if options.stop_function:
        assert not options.unmasked and not entry_mode
        probe = next(r for r in config['driver'] if r['name']==options.stop_function)
        assert probe['name'] in config['accepted']
        data[original.offset(probe['start'])] = 0xcc
        target.write_bytes(data)
    flags = phase['environment']['MSC_CMD_FLAGS'].format(
        bin=wt.windows(wt.TC/'bin'), include=wt.windows(wt.TC/'include'))
    buffers = ','.join('%x'%(r['original']-original.base) for r in config['buffered_streams'])
    command = ['wine', wt.windows(options.debugger.resolve()), wt.windows(target), '',
               wt.windows(directory/'stdout'), wt.windows(directory/'stderr'),
               wt.windows(directory/'events'), canonical or wt.windows(work), 'MSC_CMD_FLAGS='+flags, buffers]
    if phase.get('stdio', {}).get('stderr_to_stdout'):
        command.append('stderr-to-stdout')
    elif entry_mode:
        command.append('separate-stderr')
    if entry_mode:
        command.append(wt.windows(plan))
    host_environment = wt.environment()
    host_environment.pop('C1_ENUM_CALLBACK_RVA',None)
    host_environment.pop('C1_FLAG_PROBE_RVAS',None)
    host_environment.pop('C1_FUNCTION_SYMBOL_RVA',None)
    host_environment.pop('C1_TIMESTAMP_PROBE',None)
    host_environment.pop('C1_RETURN8_PROBE',None)
    host_environment.pop('C1_ASM_SEGMENT_PROBE',None)
    host_environment.pop('C1_DIRECT_CALL',None)
    host_environment.pop('C1_TEMPLATE_UNIT',None)
    host_environment.pop('C1_PDB_CLEANUP',None)
    if options.pdb_cleanup_probe:host_environment['C1_PDB_CLEANUP']='1'
    if options.expression_unit is not None:
        assert direct is not None
        host_environment['C1_TEMPLATE_UNIT']=str(options.expression_unit)
    host_environment.pop('C1_STRDUP_PROBE',None)
    host_environment.pop('C1_ABORT_PROBE',None)
    if options.trace_aborted_function:host_environment['C1_ABORT_PROBE']='1'
    host_environment.pop('C1_BROWSER_LIMIT_PROBE',None)
    if options.trace_browser_limit:host_environment['C1_BROWSER_LIMIT_PROBE']='1'
    if options.trace_declaration_scope:host_environment['C1_SCOPE_PROBE']='1'
    host_environment.pop('C1_COMPLETE_TYPE_RETURN',None)
    if options.trace_complete_type:host_environment['C1_COMPLETE_TYPE_RETURN']='1'
    if options.trace_strdup:host_environment['C1_STRDUP_PROBE']='1'
    if direct:
        args = options.direct_arguments + [0]*(2-len(options.direct_arguments))
        host_environment['C1_DIRECT_CALL'] = '%x,%x,%d,%x,%x'%(probe['start']-original.base,direct['start']-original.base,len(options.direct_arguments),*args)
        host_environment['C1_ASM_OPERAND_PROBE']='1'

    if options.trace_asm_segment:
        host_environment['C1_ASM_SEGMENT_PROBE']='1'
        host_environment['C1_ASM_OPERAND_PROBE']='1'
    if options.trace_return8:host_environment['C1_RETURN8_PROBE']='1'
    abi_strings = {item['original']:item for record in config['driver'] for item in record.get('abi_strings',[])}
    if abi_strings:
        host_environment['C1_READ_STRINGS'] = ','.join('%x:%x'%(address-original.base,item['maximum'])for address,item in sorted(abi_strings.items()))
    abi_globals = {item['original']:item for record in config['driver'] for item in record.get('abi_globals',[])}
    if abi_globals:
        extra = ','.join('%x:%x'%(address-original.base,item['bytes'])for address,item in sorted(abi_globals.items()))
        prior = host_environment.get('C1_READ_MEMORY')
        host_environment['C1_READ_MEMORY'] = prior+','+extra if prior else extra
    if options.trace_timestamps: host_environment['C1_TIMESTAMP_PROBE']='1'
    if options.trace_function_finishes:
        assert original.read(0x2b148,10)==bytes.fromhex('c70540b1050000000000')
        host_environment['C1_FUNCTION_SYMBOL_RVA'] = '%x' % (0x5b140-original.base)
    if options.trace_flags:
        assert not entry_mode and not options.trace_enum_callbacks
        probe = next(r for r in config['driver']+json.loads((BINARY/'runtime.json').read_text()) if r['name']==options.trace_flags)
        assert probe['name'] in config['accepted'] or probe['name'] in options.restore_function or ((options.trace_return8 or options.trace_asm_segment or options.trace_strdup or options.trace_declaration_scope or options.trace_browser_limit or options.trace_aborted_function) and options.unmasked)
        # Identity derived and checked against raw operand bytes by audit-flags.py.
        host_environment['C1_FLAG_PROBE_RVAS'] = '%x,%x,%x' % (probe['start']-original.base,0x6bb44-original.base,0x6bb48-original.base)
    if options.trace_enum_callbacks:
        assert not entry_mode
        callback = next(r for r in config['driver'] if r['name']==enum_callback)
        assert callback['name'] in config['accepted']
        host_environment['C1_ENUM_CALLBACK_RVA'] = '%x' % (callback['start']-original.base)
    preparation = time.perf_counter()-start
    run_start = time.perf_counter()
    with (directory/'host-stdout').open('wb') as host_out, (directory/'host-stderr').open('wb') as host_err:
        result = subprocess.run(command, env=host_environment, stdout=host_out,
                                stderr=host_err, stdin=subprocess.DEVNULL, timeout=60)
    (directory/'capture-status.json').write_text(json.dumps(dict(
        host_exit=result.returncode,preparation_seconds=preparation,
        execution_seconds=time.perf_counter()-run_start,total_seconds=time.perf_counter()-start,
        wine_prefix=str(wt.PREFIX),target_sha256=sha(target)),indent=2)+'\n')
    assert result.returncode == 0, (directory/'host-stderr').read_text(encoding='latin1')
    events = (directory/'events').read_text()
    terminal = re.search(r'^exit ([0-9]+)$',events,re.M)
    terminal_exit = int(terminal[1]) if terminal else None
    if options.unmasked or entry_mode:
        assert terminal_exit == phase['expected_exit'], ('original terminal status differs',terminal_exit,phase['expected_exit'])
    if entry_mode:
        events = (directory/'events').read_text()
        entries = [line.split()[1] for line in events.splitlines() if line.startswith('entry ')]
        assert entries == expected_entries, ('original first-entry sequence differs',entries,expected_entries)
        assert 'entry-divergence' not in events
    if canonical:
        shutil.copytree(work,directory/'work',copy_function=shutil.copy2)
    receipt = dict(restored_functions=options.restore_function,pdb_cleanup_probe=options.pdb_cleanup_probe,expression_unit=options.expression_unit,aborted_function_probe=options.trace_aborted_function,browser_limit_probe=options.trace_browser_limit,declaration_scope_probe=options.trace_declaration_scope,complete_type_probe=options.trace_complete_type,strdup_probe=options.trace_strdup,direct_call=dict(function=direct['name'],arguments=options.direct_arguments,body_restored=options.direct_body,public_reachability=False)if direct else None,asm_segment_probe=options.trace_asm_segment,return8_probe=options.trace_return8,stop_function=options.stop_function,terminal_exit=terminal_exit,expected_exit=phase['expected_exit'],case=str(case), phase=options.phase, unmasked=options.unmasked,
                   enum_callback_trace=options.trace_enum_callbacks,enum_callback_function=enum_callback if options.trace_enum_callbacks else None,flag_word_probe=options.trace_flags,function_finish_probe=options.trace_function_finishes,timestamp_probe=options.trace_timestamps,
                   accepted_count=len(config['accepted']), accepted_sha256=hashlib.sha256(
                       json.dumps(config['accepted']).encode()).hexdigest(),
                   executable_sha256=sha(target), original_sha256=phase['sha256'],
                   debugger_sha256=sha(options.debugger), debugger_memory_request=host_environment.get("C1_READ_MEMORY"),debugger_string_request=host_environment.get("C1_READ_STRINGS"), child_arguments=[],
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
