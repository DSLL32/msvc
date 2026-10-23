"""Accept one explicitly reconstructed frontend function from fresh evidence.

Reconstruction/reference operation only: never imported or run by fixtures.
Requires a completed canonical/native build and a reviewed complete extent.
"""
import argparse
from virtual_table_owner import validate as validate_virtual_table_owner
from unused_conversion_text import validate as validate_unused_conversion_text
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
from case_paths import paths, CAPTURE_ROOT

BINARY=Path(__file__).resolve().parents[1]


def module(name, filename):
    spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/filename)
    result=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',default='cpp_basic')
    parser.add_argument('--candidate',required=True)
    parser.add_argument('--check-arguments-only',action='store_true',help='verify fresh current argument proof without capture or promotion')
    parser.add_argument('--original',type=Path)
    parser.add_argument('--native',type=Path)
    parser.add_argument('--build-timing',type=Path)
    parser.add_argument('--argument-proof',type=Path,required=True,
        help='explicit current-function actual ABI argument observations and logical bindings')
    args=parser.parse_args()
    started=time.perf_counter()
    config_path=BINARY/'functions.json'
    config=json.loads(config_path.read_text())
    case,fixture,reference,frontier_root=paths(config,args.case)
    assert args.candidate==case['terminal'] and args.candidate not in config['accepted']
    candidate=next(r for r in config['driver'] if r['name']==args.candidate)
    assert candidate['end']>candidate['start']+1,'complete original extent required'
    argument_proof=json.loads(args.argument_proof.read_text())
    assert argument_proof['status']=='match' and not argument_proof['unknown_inputs'], 'unobserved ABI inputs hold acceptance'
    assert argument_proof['function']==args.candidate and argument_proof['case']==args.case
    assert argument_proof['accepted_before']==len(config['accepted'])
    assert argument_proof['configuration_sha256']==sha(config_path), 'argument proof belongs to another configuration'
    assert len(argument_proof['formals'])==argument_proof['declared_formal_count']
    for name,digest in argument_proof['evidence_sha256'].items():
        assert sha(args.argument_proof.parent/name)==digest, 'argument evidence changed: '+name
    argument_directory=args.argument_proof.parent
    entry_original=json.loads((argument_directory/'original-receipt.json').read_text())
    entry_native=json.loads((argument_directory/'native-checkpoint.json').read_text())
    assert entry_original['terminal']==args.candidate and entry_original['candidate'] is None
    assert entry_original['accepted_count']==len(config['accepted'])
    assert entry_original['configuration_sha256']==sha(config_path)
    assert entry_native['function']==argument_proof['native_entry']
    assert argument_proof['native_executable_sha256']==sha(BINARY/'build/native/C1XX3232.EXE')
    assert argument_proof['abi_analysis']['original_extent']==[candidate['start'],candidate['end']]
    assert argument_proof['abi_analysis']['original_target_sha256']==entry_original['original_sha256']
    assert sorted(formal['name'] for formal in argument_proof['formals'])==entry_native['declared_arguments'], 'formal argument observation coverage differs'
    cleanup=re.search(r'@(\d+)$',args.candidate)
    if cleanup: assert argument_proof['abi_analysis']['argument_bytes']==int(cleanup.group(1)), 'argument bytes differ from function ABI'
    assert not argument_proof['register_inputs'] and not argument_proof['hidden_inputs'], 'add explicit reached register/hidden input observations before acceptance'
    if 'argument_observation' in argument_proof:
        from argument_words import observation as observe_argument_words
        observed = observe_argument_words((argument_directory/'original-events.txt').read_text(), argument_proof['argument_observation']['requested_words'])
        assert observed == argument_proof['argument_observation'] == entry_original['argument_observation']
        assert all(formal['stack_index'] < observed['actual_words'] for formal in argument_proof['formals'])
    else:
        assert all(formal['stack_index'] < 4 for formal in argument_proof['formals']), 'six-word ABI requires explicit count/read receipt'
    anchors = {item['name']: item for item in entry_original.get('diagnostic_observations', [])}
    original_states = [int(value, 16) for value in re.findall(r'^state \d+ ([0-9a-f]+)$', (argument_directory/'original-events.txt').read_text(), re.M)]
    diagnostics=entry_original.get('diagnostic_observations', [])
    assert len(original_states)==len(config['checkpoint_state'])+len(diagnostics), 'missing/unreadable exact-word observation'
    assert original_states[len(config['checkpoint_state']):]==[row['value'] for row in diagnostics], 'diagnostic word receipt differs from raw read'
    for index,field in enumerate(diagnostics):
        if field.get('bytes',4)==2:
            assert field['read_bytes']==2 and field['read_succeeded'], 'exact initialized WORD observation required'
            pattern=r'^observation %d address %08x requested 2 actual 2 success 1$'%(len(config['checkpoint_state'])+index,field['original'])
            assert re.search(pattern,(argument_directory/'original-events.txt').read_text(),re.M), 'missing/short/unreadable exact WORD read'
    for requirement in argument_proof.get('state_requirements', []):
        assert original_states[requirement['index']] == entry_native[requirement['native_field']] == requirement['equals']
    words=dict((int(index),int(value,16)) for index,value in re.findall(r'^argument (\d+) ([0-9a-f]+)$',(argument_directory/'original-events.txt').read_text(),re.M))
    for formal in argument_proof['formals']:
        assert formal['original_raw']==words[formal['stack_index']]
        assert formal['native_raw']==entry_native[formal['native_field']]
        binding=formal['binding']
        if binding['kind']=='object-offset':
            assert binding['native_object_address']==entry_native[binding['native_anchor_field']]
            assert formal['original_raw']-binding['original_object']==binding['offset'], 'original logical pointer offset differs'
            assert formal['native_raw']-binding['native_object_address']==binding['offset'], 'native logical pointer offset differs'
        elif binding['kind']=='unused-conversion-text':
            assert formal['result']==validate_unused_conversion_text(args.candidate, formal, anchors, entry_native, words)
        elif binding['kind']=='class-first-virtual-table':
            validate_virtual_table_owner(args.candidate, formal, anchors, entry_native, words)
        elif binding['kind']=='null-pointer':
            assert formal['original_raw']==formal['native_raw']==0, 'null pointer argument differs'
        elif binding['kind']=='scalar':
            mask=(1<<binding['bits'])-1
            assert formal['original_raw']&mask==formal['native_raw']&mask
        elif binding['kind']=='pointer-alias':
            anchor=anchors[binding['original_anchor_name']]
            assert anchor['original']==binding['original_anchor_address']
            assert binding['original_anchor_value']==anchor['value'], 'fresh global buffer owner differs'
            assert binding['native_anchor_value']==entry_native[binding['native_anchor_field']], 'actual native global owner differs'
            offset=binding.get('offset',0)
            if 'offset' in binding:
                assert anchor['bytes']==anchor['read_bytes']==4 and anchor['read_succeeded'], 'missing readable global buffer owner'
                assert binding['owner_bytes']==4096 and 0<=offset<binding['owner_bytes'], 'debug buffer offset outside owner span'
                assert 0<anchor['value']<=0xffffffff-binding['owner_bytes'], 'original buffer owner span wraps'
                assert 0<binding['native_anchor_value']<=0xffffffff-binding['owner_bytes'], 'native buffer owner span wraps'
            assert formal['original_raw']==anchor['value']+offset, 'actual original pointer differs from fresh global owner plus offset'
            assert formal['native_raw']==binding['native_anchor_value']+offset, 'actual native pointer differs from fresh global owner plus offset'
        elif binding['kind']=='caller-owned-pointer':
            registers=[row for row in entry_original['ownership_register_observations'] if row['register']==binding['original_register']]
            assert len(registers)==1 and registers[0]['read_succeeded'], 'missing actual owner register anchor'
            raw_anchors=re.findall(r'^ownership-anchor '+binding['original_register']+r' ([0-9a-f]{8})$',(argument_directory/'original-events.txt').read_text(),re.M)
            assert raw_anchors==['%08x'%registers[0]['value']], 'owner register evidence differs'
            assert re.findall(r'^caller ([0-9a-f]+)$',(argument_directory/'original-events.txt').read_text(),re.M)==['%08x'%binding['original_caller_rva']]
            projection=entry_original.get('ownership_entry_projection', entry_original.get('first_entry_projection'))
            assert binding['original_owner_function'] in projection['observed']
            assert binding['original_owner_function'] in config['accepted']
            assert projection['events_sha256']==sha(argument_directory/'original-events.txt'), 'owner entry events hash differs'
            if (argument_directory/'original-entries.plan').exists():
                assert projection['plan_sha256']==sha(argument_directory/'original-entries.plan'), 'owner entry plan hash differs'
            owner_record=next(row for row in config['driver'] if row['name']==binding['original_owner_function'])
            assert re.search(r'^entry '+re.escape(binding['original_owner_function'])+r' rva %08x byte [0-9a-f]{2}$'%(owner_record['start']-config['target']['base']),(argument_directory/'original-events.txt').read_text(),re.M), 'actual accepted owner entry/RVA missing'
            owner=entry_native['caller_locals'][binding['native_owner_field']]
            assert owner['function']==binding['native_owner_function'] and owner['local']==binding['native_owner_local'], 'actual native owner differs'
            assert owner.get('fields',[])==binding.get('native_owner_fields',[]), 'actual caller field differs'
            offset=binding.get('original_stack_offset',0)
            assert formal['original_raw']==binding['original_owner_value']==registers[0]['value']+offset, 'original caller owner plus offset differs'
            actual_native_owner=owner['address'] if binding.get('native_owner_address') else owner['value']
            assert formal['native_raw']==binding['native_owner_value']==actual_native_owner, 'native actual caller local owner differs'
            if 'original_stack_offset' in binding:
                assert offset in (34,36) and binding['original_register']=='esp' and binding['native_owner_address'], 'unsupported caller stack owner offset'
                if offset==34:
                    assert owner['function']=='emit_debug_field_list' and owner['local']=='conversion_text', 'actual output local owner differs'
                    assert owner['value'] is None and binding['output_only'], 'unwritten output local must be address-only'
                    assert 'original_prestate_word_anchor' not in binding, 'unwritten output prestate is not observed'
                else:
                    assert owner['function']=='emit_debug_field_list' and owner['local']=='member_index'
                    word=anchors[binding['original_prestate_word_anchor']]
                    assert word['bytes']==word['read_bytes']==4 and word['read_succeeded'], 'missing readable caller counter prestate'
                    assert word['original']==formal['original_raw'], 'caller counter observation address differs'
                    assert word['value']==owner['value']==entry_native[binding['native_pointee_field']]==0, 'defined caller counter prestate differs'
        elif binding['kind']=='pointee-word':
            anchor=anchors[binding['original_observation_name']]
            assert formal['original_raw']+binding.get('word_offset',0)==anchor['original']==binding['original_pointee_address'], 'actual argument no longer names observed word'
            assert binding['original_word']==anchor['value']==binding['native_word']==entry_native[binding['native_pointee_field']], 'observed actual input word differs'
            if binding.get('native_semantic_owner_field'):
                assert binding['native_word']==entry_native[binding['native_semantic_owner_field']], 'semantic input owner differs'
        elif binding['kind']=='debug-queue-word':
            queue=anchors[binding['original_queue_anchor']]
            word=anchors[binding['original_word_anchor']]
            assert word['bytes']==word['read_bytes']==2 and word['read_succeeded'], 'exact initialized WORD observation required'
            owner=entry_native['caller_locals'][binding['native_record_field']]
            assert owner['function']=='debug_tag_type' and owner['local']=='record' and owner['value']==binding['slot'], 'actual debug queue owner slot differs'
            offset=16*binding['slot']+12
            assert formal['original_raw']==queue['value']+offset==word['original'], 'actual original debug queue WORD owner differs'
            assert formal['native_raw']==entry_native[binding['native_queue_field']]+offset, 'actual native debug queue WORD owner differs'
            assert binding['original_word']==word['value']==binding['native_word']==entry_native[binding['native_word_field']]
        else:
            raise AssertionError('unbound actual argument holds acceptance: '+formal['name'])
    assert argument_proof.get('uncredited_unused_inputs', []) == [formal['name'] for formal in argument_proof['formals'] if formal['binding']['kind']=='unused-conversion-text'], 'uncredited unused input list differs'
    if args.check_arguments_only:
        print('fresh application arguments verified: '+args.candidate)
        return
    if args.original is None:
        captured=subprocess.check_output([sys.executable,str(BINARY/'scripts/capture-original.py'),'--candidate',args.candidate,'--case',args.case,'--entry-plan'],cwd=BINARY,text=True)
        args.original=Path(captured.strip().splitlines()[-1]).parent
    original=json.loads((args.original/'receipt.json').read_text())
    assert 'first_entry_projection' in original, 'fresh original application entry plan required'
    assert original['configuration_sha256']==sha(config_path)
    assert original.get('case','cpp_basic')==args.case
    assert original['candidate']==args.candidate and original['accepted_count']==len(config['accepted'])
    trace=module('c1xx_frontier_names','native-traces.py')
    if args.native is None:
        import tempfile
        args.native=Path(tempfile.mkdtemp(prefix='native-frontier-',dir=CAPTURE_ROOT))
        module('c1xx_native_capture','capture-native.py').capture(args.native,case_name=args.case)
    native=json.loads((args.native/'checkpoint.json').read_text())
    complete=original['terminal'].startswith('exit:')
    if complete:
        assert original['terminal']=='exit:0','nonzero original completion is not a successful case'
        assert native['signal']==0 and native['exit_code']==0 and native['function']=='exit:0',native
    else:
        assert native['signal']==5,'native did not stop on INT3'
        assert native['function']==trace.native_name(original['terminal']),(original['terminal'],native['function'])
    events=(args.original/'events.txt').read_text()
    buffers=[m[1] for m in re.findall(r'^buffer \d+ (\d+) ([0-9a-f]*)$',events,re.M)]
    states=[int(s,16) for s in re.findall(r'^state \d+ ([0-9a-f]+)$',events,re.M)]
    assert buffers==native['buffers'],'pending output buffers differ'
    assert states==native['states'],'checkpoint state differs'
    for stream in ('stdout.bin','stderr.bin'):
        assert (args.original/stream).read_bytes()==(args.native/stream).read_bytes(),stream+' differs'
    expected={p.name:p.read_bytes() for p in (args.original/'case').iterdir() if p.is_file()}
    actual={p.name:p.read_bytes() for p in (args.native/'case').iterdir() if p.is_file()}
    assert expected==actual,'visible input/output files differ'
    if complete:
        phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
        for output in phase['outputs']:
            assert expected[Path(output['path']).name]==(fixture/output['path']).read_bytes(), 'completed original phase artifact differs: '+output['path']
        for stream in ('stdout','stderr'):
            assert (args.original/(stream+'.bin')).read_bytes()==phase['expected_'+stream].encode('latin1'), 'completed original phase '+stream+' differs'
    audit=module('c1xx_candidate_graph','audit-graph.py')
    graph=audit.verify_candidate(args.candidate)
    left=set(graph['report']['functions'][args.candidate]['edges'])
    number=len(config['accepted'])
    name=re.sub(r'[^A-Za-z_0-9-]','-',args.candidate.lstrip('_@'))
    record=frontier_root/('%04d-'%number+name)
    record.mkdir(parents=True)
    shutil.copytree(args.argument_proof.parent,record/'arguments')
    for directory,label,names in [(args.original,'original',['receipt.json','events.txt','stdout.bin','stderr.bin','configuration.json']),
                                 (args.native,'native',['checkpoint.json','gdb.log','timing.json','stdout.bin','stderr.bin'])]:
        for filename in names:
            path=directory/filename
            if path.exists():shutil.copyfile(path,record/(label+'-'+filename))
    import gzip
    (record/'graph-audit.json.gz').write_bytes(gzip.compress(graph['raw'],mtime=0))
    (record/'graph-policy.json').write_text(json.dumps(dict(tool_sha256=graph['tool_sha256'],flags=graph['flags'],scope='Shared Rust CFG, ABI, references, callbacks, ownership and extent audit'),indent=2)+'\n')
    if args.build_timing:
        shutil.copyfile(args.build_timing,record/'build-timing.json')
    (record/'outputs').mkdir()
    for filename,data in expected.items():(record/'outputs'/filename).write_bytes(data)
    relative=lambda p:str(p.relative_to(BINARY))
    extension=dict(schema=1,accepted_before=number,accepted=args.candidate,terminal=original['terminal'],
        original_capture=relative(record/'original-receipt.json'),original_capture_sha256=sha(record/'original-receipt.json'),
        original_events=relative(record/'original-events.txt'),original_events_sha256=sha(record/'original-events.txt'),
        original_configuration=relative(record/'original-configuration.json'),
        scope='Fresh masked original and native checkpoint: exact buffers/state/files/streams; canonical call graph verified')
    extension_path=record/'frontier-extension.json'
    extension_path.write_text(json.dumps(extension,indent=2)+'\n')
    config['accepted'].append(args.candidate)
    case['terminal']=original['terminal']
    case['terminal_evidence']=relative(record/'original-events.txt')
    case.setdefault('frontier_extensions',[]).append(relative(extension_path))
    config_path.write_text(json.dumps(config,indent=2)+'\n')
    from original_entries import register
    register(args.case,args.original,record/'entry-reference')
    snapshot=reference/'canonical-map.txt'
    snapshot.write_text(''.join(line.rstrip()+'\n' for line in (BINARY/'build/c1xx.map').read_text().splitlines()))
    shutil.copyfile(snapshot,BINARY/'cases/canonical-map.txt')
    reference_path=reference/'manifest.json'
    metadata=json.loads(reference_path.read_text())
    metadata['terminal']=(dict(kind='exit',logical='exit:0',status=0) if complete else metadata['terminal'])
    metadata['terminal']['logical']=original['terminal']
    metadata['reference_receipt']=relative(record/'original-receipt.json')
    metadata['reference_receipt_sha256']=sha(record/'original-receipt.json')
    metadata['canonical_map_sha256']=sha(snapshot)
    metadata['capture_seconds']=original['capture_seconds']
    metadata['scope']='%d accepted frontiers; explicit original-backed %s frontend terminal'%(len(config['accepted']),'completed' if complete else 'current partial')
    for output in metadata['outputs']:
        path=reference/output['path']
        path.write_bytes(expected[output['path']])
        output.update(bytes=path.stat().st_size,sha256=sha(path))
    reference_path.write_text(json.dumps(metadata,indent=2)+'\n')
    subprocess.run([sys.executable,str(BINARY/'scripts/native-traces.py'),'--write'],cwd=BINARY,check=True)
    checked=time.perf_counter()
    fixture=subprocess.run([sys.executable,str(BINARY/'scripts/check-native.py'),'--trace'],cwd=BINARY,text=True,capture_output=True)
    (record/'native-fixture.log').write_text(fixture.stdout+fixture.stderr)
    assert fixture.returncode==0,fixture.stdout+fixture.stderr
    fixture_receipt=Path(re.search(r'receipt: (.+)',fixture.stdout)[1])
    shutil.copyfile(fixture_receipt,record/'native-fixture-receipt.json')
    report=dict(status='accepted',function=candidate,actual_argument_proof=relative(record/'arguments'/args.argument_proof.name),distinct_direct_callees=sorted(left),next_frontier=original['terminal'],
        pending_buffers=len(buffers),checkpoint_fields=len(states),fixture_seconds=time.perf_counter()-checked,
        total_operation_seconds=time.perf_counter()-started,original_capture_seconds=original['capture_seconds'])
    (record/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    cache=json.loads((BINARY/'cases/native-traces.json').read_text())
    application_count=len(cache['accepted_application'])
    current_path=BINARY/'validation.json'
    current=json.loads(current_path.read_text())
    current.update(active_case=args.case,status=args.case+(' reconstruction complete' if complete else ' reconstruction in progress'),complete_frontend_case=complete,accepted_frontiers=len(config['accepted']),driver_functions=application_count,
        reconstructed_source_functions=application_count+current['older_crt_functions'],
        current_frontier=original['terminal'],
        original_frontier_va=None if complete else hex(next(r['start'] for r in config['driver'] if r['name']==original['terminal'])),
        current_native_application_entries=len(cache['cases'][args.case]['expected']),
        latest_verification=relative(record/'validation.json'))
    current.setdefault('cases',{})[args.case]=dict(status='complete' if complete else 'partial',frontier=original['terminal'],latest_verification=relative(record/'validation.json'))
    current['graceful_pause'].update(paused=False, current_frontier=original['terminal'],
        original_va=current['original_frontier_va'])
    coverage=json.loads(subprocess.check_output([sys.executable,str(BINARY/'scripts/coverage.py')],cwd=BINARY,text=True))
    current['coverage']=coverage
    current['accepted_extent_coverage']=coverage
    current['next_operation']='Seed the next approved C++ queue case' if complete else 'Continue '+current['current_frontier']+' reconstruction'
    current_path.write_text(json.dumps(current,indent=2)+'\n')
    from status_documentation import refresh
    refresh()
    print(args.candidate+' -> '+original['terminal'])
    print(record/'validation.json')


if __name__=='__main__':main()
