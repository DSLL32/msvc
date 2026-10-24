#!/usr/bin/env python3
"""Compare bound fresh original/native frontier evidence; no acceptance mutation."""
import argparse
import json
import re
from pathlib import Path
from state import BINARY, TARGET, CANONICAL_CRT, load, receipt_bound, sha, masked, offset, trial_configuration, audit_input_hashes

def compare(original,native,validation=None):
    base,original_bytes,inventory=load()
    config,_=trial_configuration((BINARY/'functions.json').read_bytes())
    from case_inputs import manifest,normalized
    case_directory,case_value,target_environment=manifest(config)
    historical_base=None
    if validation:
        supplied=json.loads(Path(validation).read_text())
        if 'pending_metadata_correction' in supplied:
            from pending_metadata import certificate
            claim=supplied['pending_metadata_correction']
            correction_path=BINARY/claim['correction_path']
            assert (Path(validation).parent/'pending-correction.json').read_bytes()==correction_path.read_bytes(), 'static correction snapshot differs'
            assert claim==certificate(original,native,(BINARY/'functions.json').read_bytes(),correction_path,original_bytes,inventory), 'derived physical/static certificate differs'
            assert (Path(validation).parent/'configuration.json').read_bytes()==trial_configuration((BINARY/'functions.json').read_bytes())[1], 'derived audit configuration differs'
            historical_base=(original/'base_configuration.json').read_bytes()
    receipts=[]
    for path,is_native in ((original,False),(native,True)):
        receipt=json.loads((path/'receipt.json').read_text())
        assert (path/'base_configuration.json').read_bytes()==(historical_base if historical_base is not None else (BINARY/'functions.json').read_bytes()),'base configuration changed'
        receipt_bound(receipt,(path/'configuration.json').read_bytes(),is_native,historical_base)
        for stream in ('stdout','stderr'):
            data=(path/(stream+'.bin')).read_bytes()
            assert receipt[stream]=={'bytes':len(data),'sha256':sha(data)},'stream evidence changed'
        receipts.append(receipt)
    if config.get('data_model'):
        model_bytes=(BINARY/config['data_model']).read_bytes()
        assert all(r['data_model_sha256']==sha(model_bytes)for r in receipts)
        assert all((p/'data-model.json').read_bytes()==model_bytes for p in (original,native))
    if config.get('intrinsic_model'):
        model_bytes=(BINARY/config['intrinsic_model']).read_bytes();assert sha(model_bytes)==config['intrinsic_model_sha256']
        assert all((p/'intrinsic-model.json').read_bytes()==model_bytes for p in (original,native)),'intrinsic model snapshot changed'
    assert receipts[1]['resource_sha256']==sha((BINARY/'build/messages.rmr').read_bytes()),'prepared native resources changed'
    assert receipts[0]['candidates']==receipts[1]['candidates']==config['trial'],'different candidate sets'
    if validation is None:
        audit_file=original/'audit-receipt.json';report_file=original/'graph-audit.json'
        assert sha(audit_file.read_bytes())==receipts[0]['audit_receipt_sha256'],'initial audit receipt changed'
        audit=json.loads(audit_file.read_text())
    else:
        audit_file=Path(validation);report_file=audit_file.parent/'graph-audit.json'
        audit=json.loads(audit_file.read_text())
        assert audit['original_execution_receipt_sha256']==sha((original/'receipt.json').read_bytes()),'reaudit belongs to different execution'
    win_provenance=json.loads((report_file.parent/'win32-inputs.json').read_text())
    assert sha((report_file.parent/'win32-inputs.json').read_bytes())==audit['provenance_sha256'],'Win32 input provenance changed'
    assert win_provenance['binary_sha256']==audit['rebuilt_sha256']
    for name,digest in win_provenance['inputs'].items():
        assert sha((report_file.parent/'win32-inputs'/name).read_bytes())==digest,'immutable Win32 compiler input changed'
    graph=json.loads(report_file.read_text())
    from data_evidence import verify as verify_data
    data_file=report_file.parent/'data-evidence.json'
    assert sha(data_file.read_bytes())==audit['data_evidence_sha256']
    assert json.loads(data_file.read_text())==verify_data(config),'contextual static data proof differs'
    from import_evidence import verify
    residual_file=report_file.parent/'import-residuals.json'
    assert sha(residual_file.read_bytes())==audit['import_residuals_sha256']
    assert json.loads(residual_file.read_text())==verify(config,graph),'cached-import identity evidence differs'
    if '_initialize_pass_functions'in config['accepted']:
        from callback_evidence import verify as verify_callbacks
        callback_file=report_file.parent/'callback-evidence.json'
        assert sha(callback_file.read_bytes())==audit['callback_evidence_sha256']
        assert json.loads(callback_file.read_text())==verify_callbacks(config,graph)
    for support in config['support']:
        if support['kind']=='import'and support['name']in config['accepted']:
            identity=graph['import_catalogs']['original']['slots'][str(support['iat'])]
            assert identity=={'dll':support['dll'].lower(),'name':support['symbol']},'import-support slot identity differs'
    assert audit['configuration_sha256']==sha(trial_configuration((BINARY/'functions.json').read_bytes())[1]),'trial audit configuration differs'
    assert audit['rebuilt_sha256']==sha((BINARY/'build/c2.exe').read_bytes()),'current trial reconstruction differs'
    if config.get('retain_capture_binaries'):
        historical_audit=(original/'audit-receipt.json').read_bytes()
        assert sha(historical_audit)==receipts[0]['audit_receipt_sha256']
        historical_audit=json.loads(historical_audit)
        assert receipts[0]['rebuilt_snapshot_sha256']==sha((original/'rebuilt.exe').read_bytes())==receipts[0]['rebuilt_sha256']
        assert receipts[0]['rebuilt_snapshot_sha256']==historical_audit['rebuilt_sha256']
        assert receipts[0]['rebuilt_map_snapshot_sha256']==sha((original/'rebuilt.map').read_bytes())==receipts[0]['audit_inputs']['build/c2.map']
        assert receipts[0]['rebuilt_map_snapshot_sha256']==historical_audit['rebuilt_map_sha256']
        assert receipts[1]['binary_snapshot_sha256']==sha((native/'native.elf').read_bytes())==receipts[1]['native_sha256']
        for name,item in receipts[0].get('static_input_snapshots',{}).items():
            assert sha((original/name).read_bytes())==item['sha256']==receipts[0]['audit_inputs'][item['input']]
    assert audit['rebuilt_map_sha256']==audit_input_hashes()['build/c2.map'],'audit MAP differs'
    assert audit['runtime_sha256']==audit_input_hashes()['runtime.json'],'audit original runtime index differs'
    assert audit['rebuilt_runtime_sha256']==audit_input_hashes()['build/runtime.json'],'audit rebuilt runtime index differs'
    assert audit['metadata_sha256']==audit_input_hashes()['build/graph-metadata.json'],'audit roots/chunks metadata differs'
    assert audit['report_sha256']==sha(report_file.read_bytes()),'graph report changed'
    assert audit['tool_sha256']==audit_input_hashes()['graph-audit'],'audit tool changed: explicit reaudit required, not original replay'
    assert not receipts[0]['unmasked'],'frontier requires masked original'
    assert sha((original/'events.txt').read_bytes())==receipts[0]['events_sha256'],'original events changed'
    assert sha((native/'state.json').read_bytes())==receipts[1]['state_sha256'],'native state changed'
    assert sha((original/'loaded-msvcrt.bin').read_bytes())==CANONICAL_CRT,'provider file changed'
    if 'helper_inputs_sha256'in receipts[0]:
        assert sha((original/'helper-inputs.json').read_bytes())==receipts[0]['helper_inputs_sha256']
        helper=json.loads((original/'helper-inputs.json').read_text())
        assert helper['binary_sha256']==receipts[0]['debugger_sha256']==sha((original/'capture-helper.exe').read_bytes()),'historical capture helper differs'
        for name,digest in helper['inputs'].items():
            assert sha((original/'helper-inputs'/name).read_bytes())==digest,'immutable controller compile input differs'

    expected=bytearray(masked(config,original_bytes,inventory,receipts[0]['candidates']))
    for record in config['driver']+config['support']:
        if record['name']in config['accepted']+receipts[0]['candidates']:
            expected[offset(inventory,record['start'])]=0xcc
    assert (original/'C2.EXE').read_bytes()==bytes(expected),'masked image differs from configuration'
    assert sha(bytes(expected))==receipts[0]['executable_sha256']
    for name,digest in receipts[0]['execution_inputs'].items():
        assert sha((original/name).read_bytes())==digest,'raw execution input changed'
    child_env=b'\0'.join(v.encode()for v in config['case']['child_environment_input'])+b'\0\0'
    assert (original/'child-environment-input.bin').read_bytes()==child_env,'original actual child environment changed'
    flags=[v[14:].decode()for v in child_env.split(b'\0')if v.startswith(b'MSC_CMD_FLAGS=')]
    assert (flags[0]if flags else None)==config['case']['environment']['MSC_CMD_FLAGS']
    command='"'+case_value['canonical_argv'][0]+'"'+(' '+' '.join(config['case']['arguments'])if config['case']['arguments']else'')
    assert (original/'child-command-input.bin').read_bytes()==command.encode()+b'\0','original actual command changed'
    assert all(r['helper_status']==0 for r in receipts),'capture helper failed'
    text=(original/'events.txt').read_text()
    assert re.findall(r'^image-base ([0-9a-f]+)$',text,re.M)==['00400000'],'unexpected original image base'
    assert re.findall(r'^module ([0-9a-f]+) [0-9a-f]+ MSVCRT.dll$',text,re.M)==['78000000'],'unexpected canonical CRT load base'
    assert 'timeout'not in text,'original capture timed out'
    for group in ('gdb_inputs','gdb_outputs'):
        for name,digest in receipts[1][group].items():
            assert sha((native/name).read_bytes())==digest,'GDB input/output provenance changed'
    provenance=json.loads((native/'compiled-inputs.json').read_text())
    assert sha((native/'compiled-inputs.json').read_bytes())==receipts[1]['provenance_sha256']
    assert provenance['binary_sha256']==receipts[1]['native_sha256']
    source_prefix=str((BINARY/'src').relative_to(BINARY.parents[2]))+'/'
    assert {n:v for n,v in provenance['inputs'].items()if n.startswith(source_prefix)}=={n:v for n,v in win_provenance['inputs'].items()if n.startswith(source_prefix)},'Win32/native reconstructed source snapshots differ'
    for name,digest in provenance['inputs'].items():
        assert sha((native/'compiled-inputs'/name).read_bytes())==digest,'immutable compiled input changed'
    state=json.loads((native/'state.json').read_text())
    import importlib.util
    module_spec=importlib.util.spec_from_file_location('c2_native_elf_proof',BINARY/'scripts/native-proof.py')
    proof=importlib.util.module_from_spec(module_spec);module_spec.loader.exec_module(proof)
    assert state['arguments']==config['case']['arguments']and state['MSC_CMD_FLAGS']==config['case']['environment']['MSC_CMD_FLAGS'],'native actual execution input differs'
    if config['case']['id']=='numeric-control-flow':
        assert all(r.get('case_id')==config['case']['id']and 'case_inputs'in r for r in receipts),'new compilation proof lacks case/input binding'
        assert 'target_environment_sha256'in receipts[1],'new compilation proof lacks full bound environment'
    if 'target_environment_sha256'in receipts[1]:
        assert receipts[1]['target_environment_sha256']==sha((native/'target-environment.bin').read_bytes())==sha(target_environment),'bound target environment differs'
    for path,receipt in ((original,receipts[0]),(native,receipts[1])):
        if 'case_inputs'in receipt:
            expected_inputs={item['working_name']:item['sha256']for item in case_value.get('inputs',[])}
            if 'source'in case_value:expected_inputs[Path(case_value['source']['path']).name]=case_value['source']['sha256']
            assert receipt['case_inputs']==expected_inputs,'case intermediate/source input differs'
    traps=re.findall(r'^trap ([0-9a-f]+) ([0-9a-f]+) ',text,re.M)
    if not traps:
        assert re.findall(r'^exit ([0-9a-f]+)$',text,re.M)==['%08x'%config['case']['status']],'original has no real expected process exit'
        assert 'trap 'not in text and 'exception 'not in text,'exception cannot be completion'
        native_proof=proof.verify_completion(BINARY/'build/native/c2',(native/'native-maps.txt').read_text(),(native/'gdb.stdout').read_text(),state,config,config['case']['status'])
        names={r['start']:r['name']for r in config['driver']if r['name']in config['accepted']}
        trace=[names[int(n,16)]for n in re.findall(r'^entry ([0-9a-f]+)$',text,re.M)if int(n,16)in names]
        assert trace and trace==state['trace'],'terminal source-entry progression differs'
        assert set(config['trial'])<=set(trace),'terminal candidate was not entered'
        for stream in ('stdout','stderr'):
            master=(case_directory/('reference/'+stream+'.bin'if config['case']['id']!='unsupported-help-diagnostic'else stream+'.bin')).read_bytes()
            assert (original/(stream+'.bin')).read_bytes()==(native/(stream+'.bin')).read_bytes()==master,'terminal output differs'
        assert receipts[0]['stdout']['bytes']==case_value['expected']['stdout']['bytes']and receipts[0]['stderr']['bytes']==case_value['expected']['stderr']['bytes']
        for index,item in enumerate(case_value['expected'].get('artifacts',[])):
            master=normalized((case_directory/item['path']).read_bytes(),item)
            for path,receipt in ((original,receipts[0]),(native,receipts[1])):
                actual=receipt['artifacts'][index];assert actual['output_name']==item['output_name']and actual['present']
                data=(path/actual['path']).read_bytes();assert sha(data)==actual['sha256']and len(data)==actual['bytes']
                assert normalized(data,item)==master,'terminal object differs beyond identified clock field'
        return {'validation':str(audit_file),'graph_report':str(report_file),'matched':'completion','original_exit_status':config['case']['status'],'native_exit_status':config['case']['status'],'source_trace':trace,'native_elf_proof':native_proof,'candidates':config['trial'],'acceptance':False}

    assert len(traps)==1 and int(traps[0][0],16)==0x80000003,'original did not stop at one INT3 frontier'
    address=int(traps[0][1],16)
    matching=[r for r in config['stubs']if r['start']==address]
    assert len(matching)==1,'unknown original frontier'
    expected=matching[0]
    if expected['name']!='_c2_crt_startup':
        assert expected.get('state_policy')in ('pre-main-no-arguments','main-arguments-and-initial-state','exit-callback','command-line-state','environment-flags','match-option','wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','message-load','message-unescape','diagnostic-emit','cached-message','format-prefix','format-prefix-args','format-integer','exit-cleanup','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete')and expected.get('state'),'new frontier needs explicit state decoder'
        if expected['state_policy']=='pre-main-no-arguments':
            assert expected['abi']=='cdecl, no parameters','parameter ABI has no decoder'
    native_proof=proof.verify(BINARY/'build/native/c2',(native/'native-maps.txt').read_text(),(native/'gdb.stdout').read_text(),state,config)
    assert state['arguments']==config['case']['arguments'],'native actual arguments changed'
    assert state['MSC_CMD_FLAGS']==config['case']['environment']['MSC_CMD_FLAGS'],'native target environment differs'
    assert state['function']==expected['native'],'logical frontier mismatch'
    assert 'SIGTRAP' in state['signal'],'native not stopped at INT3'
    declared=[r['name']for r in config['driver']+config['support']if r['name']in config['accepted']and r.get('kind')!='import']
    entry_names={r['start']:r['name']for r in config['driver']+config['support']if r['name']in declared}
    original_trace=[entry_names[int(a,16)]for a in re.findall(r'^entry ([0-9a-f]+)$',text,re.M)if int(a,16)in entry_names]
    assert original_trace==state['trace'],'accepted source-entry order differs'
    if expected.get('state'):
        section=text.rsplit('observe %08x '%address,1)
        assert len(section)==2,'frontier state is missing'
        raw={int(a,16):int(v,16)for a,v in re.findall(r'^global ([0-9a-f]+) ([0-9a-f]+)$',section[1],re.M)}
        if expected.get('state_policy')in ('symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
            # The already-frozen complete runtime.data observation contains
            # these newly encountered scalars. Extract only these typed DWORDs;
            # the remaining bytes still receive no data/payload/extent credit.
            import struct
            image=(original/'unmodeled-original-data.bin').read_bytes()
            preserved=receipts[0]['unmodeled_original_data']
            assert len(image)==preserved['bytes']==67328 and sha(image)==preserved['sha256']
            for pointer in (0x49739c,0x4973b0,0x4973a0,0x497390,0x497380,0x497c6c,0x4973c0,0x495f1c,0x490b78,0x4973f8,0x490b70):
                observed=struct.unpack_from('<I',image,pointer-0x48e000)[0]
                if pointer in raw:assert raw[pointer]==observed,'scalar and preserved data observations differ'
                raw[pointer]=observed
        required={view['original']:raw[int(view['original'],16)]for view in expected['state']}
        assert state['globals']==required,'frontier globals/runtime state differs'
    if expected.get('state_policy')in ('command-line-state','environment-flags','match-option','wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
        segment=text.rsplit('observe %08x '%address,1)[1]
        values=re.findall(r'^command-state argc=(\d+) argv=([0-9a-f]+) input=([0-9a-f]+)$',segment,re.M)
        assert len(values)==1
        expected_argv=b'\0'.join(value.encode()for value in case_value['canonical_argv'])+b'\0\0'
        consumed=expected.get('argv_consumed',0)
        strings=([v.encode()for v in expected['expected_argv']]if 'expected_argv'in expected else expected_argv[:-2].split(b'\0')[consumed:])
        assert (original/'command-argv.bin').read_bytes()==b'\0'.join(strings)+b'\0\0'
        assert int(values[0][0])==expected.get('expected_argc',len(strings))
        assert state['command_state']=={'argc':expected.get('expected_argc',len(strings)),'argv':[v.decode()for v in strings]}
        if expected['state_policy']=='environment-flags':
            assert int(values[0][2],16)==0 and state['flags']is None,'absent MSC_CMD_FLAGS input is not NULL'
    if expected.get('state_policy')in ('message-load','message-unescape'):
        import struct
        loading=expected['state_policy']=='message-load'
        if loading:
            raw=re.findall(r'^message-load id=(\d+) buffer=([0-9a-f]+) capacity=(\d+) allow=(\d+) initialized=(\d+)$',text,re.M)
            assert raw==[('301','0049e4d0','384','1','1')],'first recursive message load ABI differs'
        else:
            assert re.findall(r'^message-unescape buffer=([0-9a-f]+) initialized=(\d+)$',text,re.M)==[('0049e4d0','1')]
        ids=[301,302,303,310,311,312,0]
        contents=bytes(384)if loading else b'warning'+bytes(377)
        assert (original/'message-cache.bin').read_bytes()==struct.pack('<I',1)+b''.join(struct.pack('<II',n,0)for n in ids)+contents
        pointer=proof.symbols(BINARY/'build/native/c2',data=True)['message_buffer'][0]+int(native_proof['load_bias'],16)
        assert state['message_load']=={'message':301 if loading else 0,'buffer':pointer,'capacity':384,'allow_missing':1,'initialized':1,'cache':[[n,0]for n in ids],'bytes':contents.hex()},'message cache/loader state differs'
    if expected.get('state_policy')in ('diagnostic-emit','cached-message'):
        import struct
        raw=re.findall(r'^diagnostic-emit category=(\d+) message=(\d+) format=([0-9a-f]+) arguments=([0-9a-f]+) argument=([0-9a-f]+) unexpected=([0-9a-f]+)$',text,re.M)
        emitting=expected['state_policy']=='diagnostic-emit'
        if emitting:
            assert len(raw)==1 and raw[0][:3]==('1000','7','0049e4d0')and raw[0][4]==raw[0][5],'diagnostic call ABI/aliases differ'
        else:
            assert re.findall(r'^cached-message id=(\d+)$',text.rsplit('observe %08x '%address,1)[1],re.M)==['303']and state['cached_message_id']==303
        fmt=b"unrecognized flag '%s' in '%s'"
        if emitting:
            assert (original/'diagnostic-format.bin').read_bytes()==fmt+b'\0'
            assert (original/'fatal-argument.bin').read_bytes()==b'/?\0'and(original/'fatal-phase.bin').read_bytes()==b'p2\0'
        expected_cache=[[301,'warning'],[302,'error'],[303,'fatal error'],[310,'compiler generated'],[311,'compiler intermediate'],[312,'source'],[0,None]]
        blob=(original/'message-cache.bin').read_bytes();assert len(blob)==444 and struct.unpack_from('<I',blob)[0]==1
        pointers=[]
        for i,(id,string)in enumerate(expected_cache):
            actual_id,pointer=struct.unpack_from('<II',blob,4+8*i);assert actual_id==id
            if string is None:assert pointer==0
            else:
                assert pointer and pointer!=0x49e4d0 and pointer not in pointers;pointers.append(pointer)
                assert (original/('cache-message-%d.bin'%id)).read_bytes()==string.encode()+b'\0'
        contents=fmt+bytes(384-len(fmt));assert blob[60:]==contents
        assert state['message_cache']=={'initialized':1,'cache':expected_cache,'bytes':contents.hex()},'message cache state differs'
        if emitting:
            assert state['diagnostic']=={'category':1000,'message':7,'format':fmt.decode(),'format_alias':1,'argument':'/?','phase':'p2','argument_alias':1},'diagnostic call state differs'
    if expected.get('state_policy')in ('format-prefix','format-prefix-args'):
        raw=re.findall(r'^format-prefix buffer=([0-9a-f]+) offset=(\d+) format=([0-9a-f]+) character=(\d+) number=(\d+)$',text.rsplit('observe %08x '%address,1)[1],re.M)
        assert len(raw)==1 and raw[0][1]==('67'if expected['state_policy']=='format-prefix-args'else'51')and raw[0][3:]==('67','1007'),'format prefix original stack ABI differs'
        assert (original/'format-pattern.bin').read_bytes()==b' %c%d: \0'and(original/'format-prefix.bin').read_bytes()==b'fatal error\0'
        assert state['format_prefix']=={'format':' %c%d: ','character':67,'number':1007,'output_offset':11,'prefix':b'fatal error\0'.hex()},'native prefix formatter ABI/output identity differs'
    if expected.get('state_policy')=='format-integer':
        raw=re.findall(r'^format-integer value=(\d+) buffer=([0-9a-f]+) base=(\d+)$',text.rsplit('observe %08x '%address,1)[1],re.M)
        assert len(raw)==1 and raw[0][0]=='1007'and raw[0][2]=='10','integer formatter original ABI differs'
        assert (original/'integer-prefix.bin').read_bytes()==b'fatal error C'
        assert state['integer_format']=={'value':1007,'base':10,'output_offset':2,'prefix':b'fatal error C'.hex()},'integer formatter native ABI/output differs'
    if expected.get('state_policy')=='environment-copy':
        initial=re.findall(r'^flags-environment entry=([0-9a-f]+) returned=([0-9a-f]+) alias=(\d+)$',text,re.M)
        copied=re.findall(r'^environment-copy flags=([0-9a-f]+)$',text.rsplit('observe %08x '%address,1)[1],re.M)
        assert len(initial)==1 and initial[0][2]=='1'and int(initial[0][1],16)==int(initial[0][0],16)+14
        assert copied==[initial[0][1]],'copy caller flags lost canonical environment alias'
        flags=config['case']['environment']['MSC_CMD_FLAGS']
        assert (original/'copy-flags.bin').read_bytes()==flags.encode()+b'\0'
        assert state['environment_copy']=={'flags':flags,'environment_alias':True},'native flags must borrow actual bound target environment'
    original_overrides=native_overrides=None
    if expected.get('state_policy')in ('alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
        initial=re.findall(r'^flags-environment entry=([0-9a-f]+) returned=([0-9a-f]+) alias=(\d+)$',text,re.M)
        assert len(initial)==1 and initial[0][2]=='1';origin=int(initial[0][1],16)
        raw=re.findall(r'^alignment-option input=([0-9a-f]+) current=([0-9a-f]+) operand=([0-9a-f]+) argv=([0-9a-f]+)$',text.rsplit('observe %08x '%address,1)[1],re.M)
        assert raw==[('00000000','%08x'%(origin+109),'%08x'%(origin+42),'00498030')],'completed option parser pointer identities differ'
        assert (original/'current-option.bin').read_bytes()==b'-G5\0'and(original/'option-operand.bin').read_bytes()==b'1\0'
        assert state['alignment_option']=={'alignment':None,'current':'-G5','current_offset':109,'operand':'1','operand_offset':42,'argv_offset':56,'argv_value_alias':True},'alignment input/argument-vector state differs'
        scalar={'c2_runtime_ml':1,'c2_internal_error_active':1,'c2_dos_messages':1,'c2_omit_frame_pointer':1,'c2_warning_level':1,'c2_stack_probe_limit':4096,'c2_cpu_generation':5}
        strings={'c2_intermediate_base':(4,'.\\a01068'),'c2_source_name':(16,'numeric-control-flow.c'),'c2_output_name':(52,'numeric-control-flow.obj'),'c2_pdb_name':(84,'vc50.idb')}
        original_overrides=dict(scalar,**{name:origin+offset for name,(offset,value)in strings.items()})
        native_overrides=dict(scalar,**{name:value for name,(offset,value)in strings.items()})
    if expected.get('state_policy')in ('header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        rows=re.findall(r'^header-streams input=([0-9a-f]+) globals=([0-9a-f]+) iob=([0-9a-f]+)$',segment,re.M)
        assert len(rows)==1;first,second,iob=[int(v,16)for v in rows[0]]
        assert first==iob+3*32 and second==iob+4*32,'original FILE pointers are not distinct canonical iob slots'
        first_data=struct.pack('<8I',0,0,0,1,3,0,0,0);second_data=struct.pack('<8I',0,0,0,1,4,0,0,0)
        if expected['state_policy']in ('il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            consumed={'il-word-read':1,'il-long-read':3,'symbol-reset':12,'symbol-table-empty':12,'allocate-node':12,'node-size':12,'pool-allocate':12,'pool-result':12,'real-from-int':12,'extended-store':12,'extended-results':12,'intrinsics-initialized':12,'machine-initialized':12,'section-constructor':12,'section-symbol':12,'section-linked':12,'section-defaults':12,'sections-initialized':12,'global-symbols':12,'symbol-record':12,'symbol-serial-read':13,'symbol-name-read':15,'symbol-name-intern':38,'symbol-name-copy':38,'filename-record':38,'symbol-type-input':55,'type-code-input':55,'compact-value-input':57,'compact-short-input':67,'symbol-storage-input':71,'section-lookup-input':71,'section-serial-input':71,'symbol-put-input':71}[expected['state_policy']]
            original_stream=(original/'header-streams.bin').read_bytes();assert len(original_stream)==64 and original_stream[:32]==first_data
            words=struct.unpack('<8I',original_stream[32:])
            assert words[2]and words[0]==words[2]+consumed and words[1]==271-consumed and words[3:]==(9,4,0,4096,0),'canonical post-byte FILE/buffer layout differs'
            native_words=struct.unpack('<8I',bytes.fromhex(state['header_streams']['globals']))
            assert native_words[2]and native_words[0]==native_words[2]+consumed and native_words[1]==271-consumed and native_words[3:]==(9,4,0,4096,0),'native post-byte FILE/buffer layout differs'
            master=(case_directory/next(v['path']for v in case_value['inputs']if v['working_name']=='a01068gl')).read_bytes()
            assert len(master)==271 and(original/'il-buffer.bin').read_bytes()==master
            assert re.findall(r'^il-buffer valid=(\d+) pointer-offset=(\d+)$',segment,re.M)==[('271',str(consumed))]
            second_data=bytes.fromhex(state['header_streams']['globals'])
        else:
            assert (original/'header-streams.bin').read_bytes()==first_data+second_data,'new canonical CRT streams are not unread read-mode files'
        control=int(re.findall(r'^x87-control ([0-9a-f]+)$',segment,re.M)[0],16)&0xffff
        expected_streams={'input_index':3,'globals_index':4,'input':first_data.hex(),'globals':second_data.hex(),'distinct':True,'x87_control':control}
        if expected['state_policy']in ('il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            assert re.findall(r'^active-il-stream ([0-9a-f]+)$',segment,re.M)==['%08x'%second],'header byte reader does not use globals IL stream'
            expected_streams['active_index']=4
        if expected['state_policy']in ('il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):expected_streams['valid_buffer']=master.hex()
        assert state['header_streams']==expected_streams,'native FILE identity/layout/state differs'
        if expected['state_policy']in ('symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            header={'0x490b7c':0,'0x4972f8':1,'0x497df8':146,'0x491050':146,'0x497f6c':0,'0x497de8':0}
            if expected['state_policy']=='symbol-put-input':header['0x491050']=147
            assert {key:state['globals'][key]for key in header}==header,'decoded header fields differ from frozen original format19960416/count146'
        if expected['state_policy']=='il-long-read':
            assert re.findall(r'^il-word-result ([0-9a-f]+)$',segment,re.M)==['0386']and state['il_word_result']==0x386,'16-bit machine result/caller local differs'
    if expected.get('state_policy')in ('symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        hash_bytes=bytearray((original/'symbol-hash.bin').read_bytes());assert len(hash_bytes)==4096
        assert proof.symbols(BINARY/'build/native/c2',data=True)['global_scope'][1]==4100
        if expected['state_policy']in ('section-linked','section-defaults'):
            import struct
            node=struct.unpack_from('<I',(original/'pool-descriptors.bin').read_bytes(),0)[0]+72
            assert struct.unpack_from('<I',hash_bytes,20)[0]==node,'original bucket5 lost section identity'
            hash_bytes[20:24]=bytes(4);assert hash_bytes==bytes(4096),'original other1023 hash buckets changed'
            assert state['symbol_hash']=={'slots':1024,'section_slot':5,'section_alias':True,'normalized_sha256':sha(bytes(4096))},'native symbol bucket identity or untouched extent differs'
        elif expected['state_policy']in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            import struct
            head=struct.unpack_from('<I',(original/'pool-descriptors.bin').read_bytes(),0)[0]
            buckets={5:72,2:160,3:248,4:336,9:424}
            if expected['state_policy']=='symbol-put-input':buckets[146]=676
            expected_hash=bytearray(4096)
            for bucket,displacement in buckets.items():struct.pack_into('<I',expected_hash,bucket*4,head+displacement)
            assert hash_bytes==expected_hash,'completed original section hash pointers or untouched slots differ'
            assert state['symbol_hash']=={'slots':1024,'section_offsets':{str(k):v for k,v in buckets.items()}},'native final hash aliases differ'
        else:
            assert hash_bytes==bytes(4096),'original hash reset did not clear all1024 slots'
            assert state['symbol_hash']=={'slots':1024,'sha256':sha(bytes(4096))},'native symbol hash extent/contents differ'
    if expected.get('state_policy')=='allocate-node':
        segment=text.rsplit('observe %08x '%address,1)[1]
        assert re.findall(r'^allocate-node kind=(-?\d+) pool=(-?\d+)$',segment,re.M)==[('1','0')]
        assert state['allocate_node']=={'kind':1,'pool':0},'fastcall node kind/pool ABI differs'
    if expected.get('state_policy')=='node-size':
        segment=text.rsplit('observe %08x '%address,1)[1]
        assert re.findall(r'^node-size kind=(-?\d+) caller-kind=(-?\d+) pool=(-?\d+)$',segment,re.M)==[('1','1','0')]
        assert state['node_size']=={'kind':1,'caller_kind':1,'pool':0},'node size/caller saved fastcall state differs'
        assert all(state['globals'][hex(0x491040+4*i)]==0 for i in range(4)),'allocation counters changed before sizing'
    if expected.get('state_policy')=='pool-allocate':
        segment=text.rsplit('observe %08x '%address,1)[1]
        assert re.findall(r'^pool-allocate pool=(-?\d+) bytes=(\d+) caller-kind=(-?\d+) caller-pool=(-?\d+)$',segment,re.M)==[('0','64','1','0')]
        assert state['pool_allocate']=={'pool':0,'bytes':64,'caller_kind':1,'caller_pool':0},'pool allocator fastcall bytes/pool or saved node state differs'
        assert [state['globals'][hex(0x491040+4*i)]for i in range(4)]==[64,0,0,0],'node allocation accounting differs'
    if expected.get('state_policy')in ('machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        assert [state['globals'][hex(a)]for a in (0x491074,0x495f34,0x49745c)]==[0x1d0,0x0e,16],'machine save/clobber masks or stack alignment differ'
    if expected.get('state_policy')in ('symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1];head=struct.unpack_from('<I',(original/'pool-descriptors.bin').read_bytes(),0)[0]
        assert re.findall(r'^type-input symbol=([0-9a-f]+) decoded-kind=(\d+) retention=(\d+) filenames=([0-9a-f]+)$',segment,re.M)==[('%08x'%(head+548),('4'if expected['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input')else'14'),'0','%08x'%(head+536))],'type reader symbol/caller/list ABI differs'
        symbol=bytearray(110);symbol[4]=4;struct.pack_into('<I',symbol,40,85)
        if expected['state_policy']in ('compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):struct.pack_into('<I',symbol,54,0x52)
        if expected['state_policy']in ('compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            struct.pack_into('<I',symbol,36,5);struct.pack_into('<I',symbol,54,0x4052)
            struct.pack_into('<i',symbol,58,4);struct.pack_into('<i',symbol,86,1132)
        if expected['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            assert re.findall(r'^storage-input symbol=([0-9a-f]+) caller-symbol=([0-9a-f]+) table=([0-9a-f]+)$',segment,re.M)==[('%08x'%(head+548),'%08x'%(head+548),'0048fb68')],'storage updater loses actual caller symbol or global table identity'
            struct.pack_into('<I',symbol,20,0x20);struct.pack_into('<I',symbol,24,head+660)
            if expected['state_policy']=='symbol-put-input':struct.pack_into('<I',symbol,36,146)
            struct.pack_into('<H',symbol,98,3);struct.pack_into('<H',symbol,100,54);struct.pack_into('<I',symbol,106,40)
        assert (original/'function-symbol.bin').read_bytes()==symbol,'current function record fields or untouched110-byte payload differ'
        assert (original/'filename-node.bin').read_bytes()==struct.pack('<III',86,head+512,0),'typed filename-list node differs'
        template=bytearray(64);template[4]=18;struct.pack_into('<I',template,40,86)
        raw=bytearray((original/'symbol-template.bin').read_bytes());assert len(raw)==64 and struct.unpack_from('<I',raw,24)[0]==head+512;raw[24:28]=bytes(4);assert raw==template
        prior=b'numeric-control-flow.c\0';scratch=bytearray(prior+bytes(2058-len(prior)));scratch[:13]=b'mix_unsigned\0'
        decorated=b'_'+prior+bytes(2059-len(prior)-1)
        assert (original/'symbol-name.bin').read_bytes()==scratch and (expected['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input')or (original/'decorated-name.bin').read_bytes()==decorated),'next input name residual bytes or decoration scratch differ'
        if expected['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            decorated=bytearray(decorated);decorated[:14]=b'_mix_unsigned\0';decorated=bytes(decorated)
            assert (original/'decorated-name.bin').read_bytes()==decorated,'completed decoration scratch differs'
            symbol[24:28]=bytes(4)
        observed=re.findall(r'^C2-SYMBOL-TYPE-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(observed)==1 and json.loads(observed[0])==state['symbol_type_input'],'type reader lacks actual raw GDB observation'
        expected_type_input={'formals':([{'name':'symbol','type':'C2Symbol *'}]if expected['state_policy']in ('symbol-type-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input')else[]),
            'symbol_offset':548,'caller_symbol_alias':True,'caller_kind':(4 if expected['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input')else 14),'retain_locals':0,
            'symbol_bytes':symbol.hex(),'filename_offset':536,'filename_serial':86,'filename_name_offset':512,
            'filename_next_null':True,'filename_name_alias':True,'filename_size':12,'template_bytes':template.hex(),
            'input_bytes':scratch.hex(),'decorated_bytes':decorated.hex()}
        if expected['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):expected_type_input.update(caller_table_global=True,symbol_name_offset=660,symbol_name='_mix_unsigned')
        if expected['state_policy']=='section-lookup-input':
            expected_type_input.update(formals=[{'name':'serial','type':'unsigned int'}],lookup_serial=5)
            assert re.findall(r'^lookup-input serial=(\d+) symbol=([0-9a-f]+)$',segment,re.M)==[('5','%08x'%(head+548))],'section lookup actualfastcall serial or caller symbol differs'
        if expected['state_policy']=='section-serial-input':
            expected_type_input.update(formals=[],selected_section_code_alias=True)
            assert re.findall(r'^next-serial-input selected-section=([0-9a-f]+) identifier=(\d+)$',segment,re.M)==[('%08x'%(head+72),'5')],'lookup returned wrong section pointer or caller serial'
        if expected['state_policy']=='symbol-put-input':
            expected_type_input.update(formals=[{'name':'scope','type':'C2ScopeHash *'},{'name':'symbol','type':'C2Symbol *'}],
                scope_global_alias=True,scope_caller_alias=True,pending_function_alias=True,pending_insertion_head_alias=True,
                section_offset=676,section_owner_alias=True,section_parent_code_alias=True,section_name_alias=True,
                group_offset=764,group_bytes=bytes(12).hex(),group_size=12)
            assert re.findall(r'^put-input scope=([0-9a-f]+) symbol=([0-9a-f]+) caller-symbol=([0-9a-f]+) table=([0-9a-f]+) retention=(\d+) pending=([0-9a-f]+) insertion=([0-9a-f]+)$',segment,re.M)==[('0048fb68','%08x'%(head+548),'%08x'%(head+548),'0048fb68','0','%08x'%(head+548),'004973d4')],'put_symbol actual two-fastcall arguments or pending queue aliases differ'
            assert state['globals']['0x491050']==147,'serial allocator did not increment shared symbol count exactly once'
        assert state['symbol_type_input']==expected_type_input,'type reader actual formal/current record/filename list state differs'
    if expected.get('state_policy')=='globals-complete':
        import struct
        from globals_state import ROOTS,complete_globals
        model=(BINARY/expected['state_model']).read_bytes();assert sha(model)==expected['state_model_sha256'],'complete data model changed'
        image=(original/'unmodeled-original-data.bin').read_bytes();block=(original/'pool-block.bin').read_bytes()
        preserved=receipts[0]['unmodeled_original_data'];assert len(image)==preserved['bytes']==67328 and sha(image)==preserved['sha256']
        snapshots=receipts[0]['globals_complete_hashes']
        assert set(snapshots)==({'unmodeled-original-data.bin','pool-block.bin','header-streams.bin'} | ({'writer-pool.bin','output-file.bin'}if expected.get('frontier_contract')in ('object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input')else set()) | ({'directive-pool.bin'}if expected.get('frontier_contract')in ('output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input')else set()) | ({'initializer-buffer.bin'} if expected.get('frontier_contract')in ('initializers-complete','backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input')else set()) | ({'backend-streams.bin'}if expected.get('frontier_contract')in ('backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input')else set()) | ({'output-directive-value.bin'}if expected.get('frontier_contract')=='output-directive-input'else set()))
        for name,digest in snapshots.items():assert sha((original/name).read_bytes())==digest,'complete raw snapshot changed'
        head=struct.unpack_from('<I',image,0x48e020-0x48e000)[0]
        writer=expected.get('frontier_contract')in ('object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input')
        directives=expected.get('frontier_contract')in ('output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input')
        directive_head=struct.unpack_from('<I',image,0x48e02c-0x48e000)[0] if directives else 0
        directive_block=(original/'directive-pool.bin').read_bytes()if directives else b''
        if directives:assert len(directive_block)==4096
        if directives:
            from data_evidence import reader
            assert struct.unpack_from('<I',directive_block,16)[0]==0x49a658
            assert struct.unpack_from('<I',directive_block,28)[0]==0x497fdc
            assert reader(TARGET)(0x4980c0,4)==bytes(4)
            import re as _re
            found=_re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+_output_directives\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),_re.M)
            assert len(found)==1 and reader(BINARY/'build/c2.exe')(int(found[0],16),4)==bytes(4)
            assert proof.symbols(BINARY/'build/native/c2',data=True)['output_directives'][1]==4

        writer_head=struct.unpack_from('<I',image,0x48e074-0x48e000)[0]if writer else 0
        writer_block=(original/'writer-pool.bin').read_bytes()if writer else b''
        if writer:assert len(writer_block)==4096
        def read(address,size):
            if writer_head and writer_head<=address and address+size<=writer_head+4096:return writer_block[address-writer_head:address-writer_head+size]
            if head<=address and address+size<=head+4096:return block[address-head:address-head+size]
            if directive_head<=address and address+size<=directive_head+4096:return directive_block[address-directive_head:address-directive_head+size]
            assert 0x48e000<=address and address+size<=0x49e700,'unmodeled external data access'
            return image[address-0x48e000:address-0x48e000+size]
        wanted=json.loads(json.dumps(complete_globals(read,{name:item[0]for name,item in ROOTS.items()},directives=directives,writer=writer,coff=expected.get('frontier_contract')in ('coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input'),initialized_output=expected.get('frontier_contract')in ('coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input'),list_pending=expected.get('frontier_contract')=='coff-chunk-input',list_constructed=expected.get('frontier_contract')=='coff-source-input')))
        native_complete=dict(state['globals_complete']);streams=native_complete.pop('streams');wanted['formals']=[];wanted['opaque_context_anchor_alias']=True
        if expected.get('frontier_contract')=='initializer-input':
            wanted['formals']=[{'name':'input','type':'ShimFILE *'}];wanted['input_file']={'index':3,'main_alias':True}
        assert struct.unpack('<I',read(0x497f58,4))[0]==0x49e300,'original opaque context identity changed'
        if writer:wanted['formals']=[{'name':'writer','type':'C2ObjectOutput *'}]if expected.get('frontier_contract')=='object-writer-input'else([{'name':'capacity','type':'unsigned short'}]if expected.get('frontier_contract')=='coff-table-input'else[{'name':'bytes','type':'unsigned int'}])
        if expected.get('frontier_contract')in ('coff-lists-input','coff-chunk-input'):wanted['formals']=[]
        if expected.get('frontier_contract')=='coff-list-input':wanted['formals']=[{'name':'kind','type':'int'},{'name':'context','type':'void *'}]
        if expected.get('frontier_contract')=='coff-source-input':wanted['formals']=[{'name':'name','type':'char *'}]
        if expected.get('frontier_contract')=='output-directive-input':
            wanted['formals']=[{'name':'kind','type':'int'},{'name':'value','type':'char *'}]
        assert native_complete==wanted,'completed global symbols/sections/queue/arena or actualformals differ'
        segment=text.rsplit('observe %08x '%address,1)[1]
        if expected.get('frontier_contract')=='initializer-input':
            actual=re.findall(r'^globals-complete initializer-input file=([0-9a-f]+) iob=([0-9a-f]+)$',segment,re.M);assert len(actual)==1
            file,iob=[int(v,16)for v in actual[0]]
            assert file==iob+96==struct.unpack('<I',read(0x499698,4))[0],'initializer reader actualfastcall FILE does not alias main stream slot3'
        elif expected.get('frontier_contract')=='output-directive-input':assert re.findall(r'^globals-complete output-directive-input$',segment,re.M)==['globals-complete output-directive-input']
        else:assert re.findall(r'^globals-complete no-formals$',segment,re.M)==['globals-complete no-formals']
        actual=re.findall(r'^C2-GLOBALS-COMPLETE (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(actual)==1 and json.loads(actual[0])==state['globals_complete'],'missing actual complete-data GDB observation'
        assert state['globals']['0x491050']==151 and state['globals']['0x491040']==1759,'completed symbol counter/accounting differs'
        source_streams=(original/'header-streams.bin').read_bytes();assert len(source_streams)==64
        input_words=struct.unpack('<8I',source_streams[:32]);closed_words=struct.unpack('<8I',source_streams[32:])
        native_input=bytes.fromhex(streams['input_bytes']);native_closed=struct.unpack('<8I',bytes.fromhex(streams['closed_globals_bytes']))
        if expected.get('frontier_contract')not in ('backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input'):
            assert closed_words==(0,0,0,0,4,0,4096,0)and native_closed==closed_words,'canonical closed global FILE layout differs'
        assert streams['input_index']==3 and streams['globals_index']==4
        active=struct.unpack('<I',read(0x490b80,4))[0]
        if expected.get('frontier_contract')in ('backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input'):
            opened=(original/'backend-streams.bin').read_bytes();assert len(opened)==64
            actual=re.findall(r'^backend-inputs-open expression=([0-9a-f]+) symbols=([0-9a-f]+) iob=([0-9a-f]+)$',segment,re.M);assert len(actual)==1
            expression,symbols,iob=[int(value,16)for value in actual[0]]
            assert expression==iob+4*32==struct.unpack('<I',read(0x4973dc,4))[0]
            assert symbols==iob+5*32==struct.unpack('<I',read(0x4973b4,4))[0]
            assert expression==struct.unpack('<I',read(0x497fd4,4))[0],'reopened expression file must reuse globals slot4'
            assert streams['backend_inputs']==[{'index':4,'bytes':opened[:32].hex()},{'index':5,'bytes':opened[32:].hex()}],'opened FILE bytes/identities differ'
            assert source_streams[32:]==opened[:32]and native_closed==struct.unpack('<8I',opened[:32])
            assert struct.unpack('<8I',opened[:32])==(0,0,0,1,4,0,4096,0)
            assert struct.unpack('<8I',opened[32:])==(0,0,0,1,5,0,0,0)
        if expected.get('frontier_contract')in ('initializers-complete','backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input'):
            native_words=struct.unpack('<8I',native_input)
            for words in (input_words,native_words):
                assert words[2] and words[0]==words[2] and words[1]==0 and words[3:]==(25,3,0,4096,0),'postEOF canonical FILE layout differs'
            valid=(case_directory/'inputs/a01068in').read_bytes();assert len(valid)==15
            assert (original/'initializer-buffer.bin').read_bytes()==bytes.fromhex(streams['valid_input_bytes'])==valid,'defined initializer buffer bytes differ'
            assert streams['active_globals_alias'] is False and streams['active_input_alias'] is True
            assert active==struct.unpack('<I',read(0x499698,4))[0],'active initializer stream lost mainFILE alias'
            assert re.findall(r'^initializer-input-complete pointer=([0-9a-f]+) base=([0-9a-f]+) count=(-?\d+) flag=([0-9a-f]+) valid=(\d+)$',segment,re.M)==[('%08x'%input_words[0],'%08x'%input_words[2],'0','00000019','15')]
        else:
            assert input_words==(0,0,0,1,3,0,0,0)and native_input==source_streams[:32],'main IL stream was unexpectedly consumed'
            assert streams['active_globals_alias'] is True
            assert active==struct.unpack('<I',read(0x497fd4,4))[0],'target active stream no longer aliases closed globals FILE'
    if expected.get('frontier_contract')in ('output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input'):
        from callback_evidence import model as callback_model
        import struct
        boundary=callback_model(config)
        assert all(sha((p/'output-callback-boundary.json').read_bytes())==config['output_callback_boundary_sha256']for p in (original,native))
        image=(original/'unmodeled-original-data.bin').read_bytes()
        read_word=lambda address:struct.unpack_from('<I',image,address-0x48e000)[0]
        names={r['original']:r['native']for r in boundary['targets']}
        symbols=proof.symbols(BINARY/'build/native/c2',data=True)
        bias=int(native_proof['load_bias'],16)
        observed=state['output_setup'];wanted=[]
        assert [read_word(row['original'])for row in boundary['slots']]==boundary['numeric_case_expected_targets'],'selected callback alternatives differ from actual numeric option state'
        for row in boundary['slots']:
            actual=read_word(row['original']);assert actual in row['alternatives']
            symbol,size=symbols[row['native']];assert size==4
            target,target_size=symbols[names[actual]];assert target_size>0
            wanted.append({'address':symbol+bias,'target':target+bias})
        assert observed=={'group_name':'FLAT','enabled':1,'slots':wanted},'actual callback slot/target identities or group differ'
        group=read_word(0x49973c);assert image[group-0x48e000:group-0x48e000+5]==b'FLAT\0'
        assert read_word(0x497c74)==1
        raw=re.findall(r'^C2-OUTPUT-SETUP (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(raw)==1 and json.loads(raw[0])==observed,'missing actual callback GDB observation'
    if expected.get('frontier_contract')in ('object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input'):
        from writer_evidence import verify as verify_writer
        value=verify_writer(config,graph)
        writer_evidence=report_file.parent/'writer-evidence.json'
        assert sha(writer_evidence.read_bytes())==audit['writer_evidence_sha256']
        assert json.loads(writer_evidence.read_text())==value
        assert all(sha((p/'writer-callback-boundary.json').read_bytes())==config['writer_callback_boundary_sha256']for p in (original,native))
        actual=re.findall(r'^object-writer-input writer=([0-9a-f]+) caller=([0-9a-f]+) file=([0-9a-f]+) iob=([0-9a-f]+)$',segment,re.M)
        assert len(actual)==1
        argument,caller,file,iob=[int(v,16)for v in actual[0]]
        assert (argument==caller==writer_head+8 if expected.get('frontier_contract')in ('object-writer-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input')else argument==(300 if expected.get('frontier_contract')=='coff-table-input'else 32)and caller==writer_head+8) and file==iob+6*32
        assert file==struct.unpack_from('<I',writer_block,20)[0]
        expected_file=struct.pack('<8I',0,0,0,2,6,0,0,0)
        assert (original/'output-file.bin').read_bytes()==expected_file
        wanted={'formals':[{'name':'writer','type':'C2ObjectOutput *'}],
            'pool_offset':8,'caller_writer_alias':True,'file_index':6,'file_bytes':expected_file.hex()}
        if expected.get('frontier_contract')=='coff-table-input':wanted.update(formals=[{'name':'capacity','type':'unsigned short'}],capacity=300)
        if expected.get('frontier_contract')=='coff-allocate-input':
            wanted.update(formals=[{'name':'bytes','type':'unsigned int'}],capacity=300,allocation_bytes=32,caller_function='new_output_table',caller_formals=[{'name':'capacity','type':'unsigned short'}])
            assert re.findall(r'^observe-stack ([^\n]+)$',segment,re.M)[0].split()[0]=='00453d8b','allocator did not come from the actual first constructor callsite'
            assert re.findall(r'^coff-allocate-input bytes=(\d+) capacity=(\d+)$',segment,re.M)==[('32','300')]
        if expected.get('frontier_contract')=='coff-lists-input':
            wanted['formals']=[]
            original_buffer=struct.unpack('<I',read(0x497d48,4))[0]
            assert re.findall(r'^coff-initialized malloc-result=([0-9a-f]+)$',segment,re.M)==[('%08x'%original_buffer)]
        if expected.get('frontier_contract')=='coff-list-input':
            wanted.update(formals=[{'name':'kind','type':'int'},{'name':'context','type':'void *'}],kind=1,context_null=True)
            assert re.findall(r'^coff-list-input kind=(-?\d+) context=([0-9a-f]+)$',segment,re.M)==[('1','00000000')]
        if expected.get('frontier_contract')=='coff-chunk-input':
            wanted.update(formals=[],buffered=1,count_only=0,list_offset=1320,caller_formals=[{'name':'buffered','type':'int'},{'name':'count_only','type':'int'}])
            assert re.findall(r'^coff-chunk-input buffered=(-?\d+) count-only=(-?\d+) list=([0-9a-f]+)$',segment,re.M)==[('1','0','%08x'%(writer_head+1320))]
        if expected.get('frontier_contract')=='coff-source-input':
            wanted.update(formals=[{'name':'name','type':'char *'}],name='numeric-control-flow.c',name_global_alias=True,name_pool_offset=40)
            assert re.findall(r'^coff-source-input name=([0-9a-f]+)$',segment,re.M)==[('%08x'%(writer_head+40))]
        assert state['object_writer']==wanted
        raw=re.findall(r'^C2-OBJECT-WRITER (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(raw)==1 and json.loads(raw[0])==wanted
        for directory,receipt in ((original,receipts[0]),(native,receipts[1])):
            artifact=receipt['artifacts'][0]
            assert artifact['present']and artifact['bytes']==0 and artifact['output_name']=='numeric-control-flow.obj'
            assert (directory/artifact['path']).read_bytes()==b'' and artifact['sha256']==sha(b'')
    if expected.get('frontier_contract')=='output-directive-input':
        assert (original/'output-directive-value.bin').read_bytes()==b'LIBC\0'
        actual=re.findall(r'^output-directive-input kind=(\d+) value=([0-9a-f]+) result=([0-9a-f]+)$',segment,re.M)
        assert actual==[('4','0049a658','0049a658')],'actual library pointer/EAX-result/ECX-kind transfer differs'
        wanted={'kind':4,'value':'LIBC','caller_value_alias':True,'formals':[{'name':'kind','type':'int'},{'name':'value','type':'char *'}]}
        assert state['output_directive']==wanted,'directive actual fastcall formals/value/caller-result alias differ'
        actual=re.findall(r'^C2-OUTPUT-DIRECTIVE (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(actual)==1 and json.loads(actual[0])==wanted,'actual native directive observation differs'
    if expected.get('state_policy')=='filename-record':
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        head=struct.unpack_from('<I',(original/'pool-descriptors.bin').read_bytes(),0)[0]
        assert re.findall(r'^filename-input symbol=([0-9a-f]+) name=([0-9a-f]+) serial=(\d+)$',segment,re.M)==[('00497308','%08x'%(head+512),'86')],'filename symbol/returned string alias differs'
        template=bytearray(64);template[4]=18;struct.pack_into('<I',template,40,86)
        raw=bytearray((original/'symbol-template.bin').read_bytes());assert len(raw)==64 and struct.unpack_from('<I',raw,24)[0]==head+512;raw[24:28]=bytes(4)
        assert raw==template,'arena copier changed unrelated record storage'
        name=b'numeric-control-flow.c\0';scratch=name+bytes(2058-len(name));decorated=b'_'+name+bytes(2059-len(name)-1)
        assert (original/'symbol-name.bin').read_bytes()==scratch and (original/'decorated-name.bin').read_bytes()==decorated,'arena copier changed either source name buffer'
        assert (original/'source-name-pool.bin').read_bytes()==bytes(12),'source pool changed before filename registration'
        observed=re.findall(r'^C2-FILENAME-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(observed)==1 and json.loads(observed[0])==state['filename_input'],'filename registry lacks actual raw GDB observation'
        assert state['filename_input']=={'formals':[{'name':'symbol','type':'C2Symbol *'}],
            'symbol_alias':True,'name_pool_offset':512,'name':'numeric-control-flow.c','template_bytes':template.hex(),
            'input_bytes':scratch.hex(),'decorated_bytes':decorated.hex(),'fifth_pool':bytes(12).hex()},'filename registry actual formal/record/returned string state differs'
    if expected.get('state_policy')=='symbol-name-copy':
        import struct
        assert proof.symbols(BINARY/'build/native/c2',data=True)['decorated_name'][1]==2059,'native decoration workspace ELF extent differs'
        segment=text.rsplit('observe %08x '%address,1)[1]
        assert re.findall(r'^symbol-copy-input name=([0-9a-f]+) pool=(\d+) caller-symbol=([0-9a-f]+)$',segment,re.M)==[('00497461','0','00497308')],'name copier fastcall arguments/caller symbol differ'
        template=bytearray(64);template[4]=18;struct.pack_into('<I',template,40,86)
        name=b'numeric-control-flow.c\0';scratch=name+bytes(2058-len(name));decorated=b'_'+name+bytes(2059-len(name)-1)
        assert (original/'symbol-template.bin').read_bytes()==template,'registrar changed unrelated transient record fields'
        assert (original/'symbol-name.bin').read_bytes()==scratch,'registrar changed input name storage'
        assert (original/'decorated-name.bin').read_bytes()==decorated,'decoration character/name/NUL/untouched workspace differs'
        assert (original/'source-name-pool.bin').read_bytes()==bytes(12),'source pool changed before copier'
        observed=re.findall(r'^C2-SYMBOL-COPY-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(observed)==1 and json.loads(observed[0])==state['symbol_copy_input'],'name copier lacks actual raw GDB observation'
        assert state['symbol_copy_input']=={'formals':[{'name':'name','type':'char *'},{'name':'pool','type':'int'}],
            'name_alias':True,'pool':0,'caller_pool':0,'caller_symbol_alias':True,
            'template_bytes':template.hex(),'input_bytes':scratch.hex(),'decorated_bytes':decorated.hex(),
            'workspace_size':2059,'fifth_pool':bytes(12).hex()},'name copier actual formals/aliases or complete workspace differs'
    if expected.get('state_policy')=='symbol-name-intern':
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        assert re.findall(r'^symbol-intern-input symbol=([0-9a-f]+) name=([0-9a-f]+) pool=(\d+) caller-kind=(-?\d+) retention=(\d+)$',segment,re.M)==[('00497308','00496a00','0','18','0')],'symbol-name registration actual fastcall/stack arguments differ'
        template=bytearray(64);template[4]=18;struct.pack_into('<I',template,40,86)
        name=b'numeric-control-flow.c\0';scratch=name+bytes(2058-len(name))
        assert (original/'symbol-template.bin').read_bytes()==template,'string input changed unrelated transient record fields'
        assert (original/'symbol-name.bin').read_bytes()==scratch,'string input output/NUL/untouched scratch tail differs'
        assert (original/'source-name-pool.bin').read_bytes()==bytes(12),'source pool changed before name registration'
        observed=re.findall(r'^C2-SYMBOL-INTERN-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(observed)==1 and json.loads(observed[0])==state['symbol_intern_input'],'name registration lacks actual raw GDB observation'
        assert state['symbol_intern_input']=={'formals':[{'name':'symbol','type':'C2Symbol *'},{'name':'name','type':'char *'},{'name':'pool','type':'int'}],
            'symbol_alias':True,'name_alias':True,'pool':0,'kind':18,'retain_locals':0,
            'template_bytes':template.hex(),'name_bytes':scratch.hex(),'fifth_pool':bytes(12).hex()},'name registration actual formals/ABI/input storage differs'
    if expected.get('state_policy')=='symbol-name-read':
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        assert re.findall(r'^symbol-name-input buffer=([0-9a-f]+) capacity=(\d+) caller-kind=(-?\d+) retention=(\d+)$',segment,re.M)==[('00496a00','2058','18','0')],'string reader fastcall arguments differ'
        template=bytearray(64);template[4]=18;struct.pack_into('<I',template,40,86)
        assert (original/'symbol-template.bin').read_bytes()==template,'unsigned input result/untouched transient template differs'
        assert (original/'symbol-name.bin').read_bytes()==bytes(2058),'name scratch changed before string reader'
        assert (original/'source-name-pool.bin').read_bytes()==bytes(12),'source-name pool changed before string reader'
        observed=re.findall(r'^C2-SYMBOL-NAME-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(observed)==1 and json.loads(observed[0])==state['symbol_name_input'],'name reader state lacks actual raw GDB observation'
        assert state['symbol_name_input']=={'formals':[{'name':'buffer','type':'char *'},{'name':'capacity','type':'unsigned int'}],
            'buffer_alias':True,'capacity':2058,'kind':18,'retain_locals':0,'template_bytes':template.hex(),
            'name_bytes':bytes(2058).hex(),'fifth_pool':bytes(12).hex()},'name reader actual formals/ABI/template state differs'
    if expected.get('state_policy')=='symbol-serial-read':
        segment=text.rsplit('observe %08x '%address,1)[1]
        template=bytearray(64);template[4]=18
        observed=re.findall(r'^C2-SYMBOL-SERIAL-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(observed)==1 and json.loads(observed[0])==state['symbol_serial_input'],'unsigned reader state lacks its actual raw GDB observation'
        assert re.findall(r'^symbol-serial-input kind=(-?\d+) retention=(\d+) formals=0 template=00497308 bytes=64 name-bytes=2058$',segment,re.M)==[('18','0')]
        assert (original/'symbol-template.bin').read_bytes()==template,'original transient filename template differs'
        assert (original/'symbol-name.bin').read_bytes()==bytes(2058),'original input scratch unexpectedly changed'
        assert (original/'source-name-pool.bin').read_bytes()==bytes(12),'original source-name pool unexpectedly changed'
        assert state['symbol_serial_input']=={'formals':[],'kind':18,'retain_locals':0,
            'template_bytes':template.hex(),'name_bytes':bytes(2058).hex(),
            'source_template_size':64,'fifth_pool':bytes(12).hex()},'unsigned input reader/caller/template/scratch/pool state differs'
        globals_expected={'0x49739c':0,'0x4973b0':0,'0x4973a0':0,'0x497390':0,'0x497380':0,'0x497c6c':0,'0x4973c0':1,'0x495f1c':0}
        assert {key:state['globals'][key]for key in globals_expected}==globals_expected,'decoder support globals initial state differs'
    if expected.get('state_policy')=='symbol-record':
        segment=text.rsplit('observe %08x '%address,1)[1]
        assert re.findall(r'^symbol-record-input retention=(\d+) caller-scope=([0-9a-f]+) table=([0-9a-f]+) caller-retention=(\d+) bytes=4100 buckets=4$',segment,re.M)==[('0','00000000','0048fb68','0')]
        assert state['symbol_record_input']=={'retain_locals':0,'caller_null_scope':True,'caller_retention':0,'global_scope_alias':True,'scope_bytes':4100,'bucket_offset':4},'record-reader fastcall/caller scope layout or aliases differ'
        scope=(original/'symbol-scope.bin').read_bytes();assert scope==bytes(4)+(original/'symbol-hash.bin').read_bytes(),'global scope header/1024-bucket contiguous layout differs'
        expected_globals={'0x4973e8':0,'0x490b84':0,'0x4973d4':0,'0x4973f0':0,'0x4972fc':0,'0x49d634':0xffffffff}
        assert {key:state['globals'][key]for key in expected_globals}==expected_globals,'initial record queues/scopes/source-file sentinel differ'
    if expected.get('state_policy')=='global-symbols':
        segment=text.rsplit('observe %08x '%address,1)[1]
        values=re.findall(r'^global-symbols-input scope=([0-9a-f]+) active=([0-9a-f]+) globals=([0-9a-f]+)$',segment,re.M)
        assert len(values)==1 and values[0][0]=='00000000'and values[0][1]==values[0][2]and int(values[0][1],16)
        assert state['global_symbols_input']=={'null_scope':True,'active_globals_alias':True},'global symbols scope/active stream identity differs'
    if expected.get('state_policy')in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        head=struct.unpack_from('<I',(original/'pool-descriptors.bin').read_bytes(),0)[0]
        count=6 if expected['state_policy']=='symbol-put-input'else 5
        addresses=[head+72+88*i for i in range(5)]+([head+676]if count==6 else[])
        assert (original/'section-addresses.bin').read_bytes()==struct.pack('<%dI'%count,*addresses)
        assert (original/'section-list.bin').read_bytes()==struct.pack('<2I',addresses[0],addresses[-1])
        assert (original/'section-globals.bin').read_bytes()==struct.pack('<7I',*addresses[:4],0,0,addresses[4])
        assert re.findall(r'^sections-initialized records=%d head=([0-9a-f]+) tail=([0-9a-f]+) UNMODELED-context=0049e300$'%count,segment,re.M)==[('%08x'%addresses[0],'%08x'%addresses[-1])]
        raw=(original/'sections.bin').read_bytes();assert len(raw)==count*86
        specifications=[('_TEXT','CODE',5,16,1,0,1),('_DATA','DATA',2,4,0,1,2),('CONST','CONST',3,4,0,4,3),('_BSS','BSS',4,4,0,3,4),('_TLS','TLS',9,4,1,11,7)]
        if count==6:specifications.append(('_mix_unsigned','CODE',146,16,1,0,0))
        native_rows=state['sections']['rows'];assert len(native_rows)==count
        original_text_ids=[];native_text_ids=[]
        for i,(name,class_name,serial,alignment,attribute,kind,order)in enumerate(specifications):
            record=bytearray(raw[i*86:(i+1)*86]);nameptr,context,serial_value=struct.unpack_from('<III',record,24)[0],struct.unpack_from('<I',record,36)[0],struct.unpack_from('<I',record,40)[0]
            nextptr=struct.unpack_from('<I',record,50)[0];classptr=struct.unpack_from('<I',record,54)[0]
            assert context==(head+764 if i==5 else 0x49e300)and serial_value==serial and nextptr==(addresses[i+1]if i<count-1 else 0)
            if i==5:
                assert nameptr==head+660 and struct.unpack_from('<II',record,74)==(head+72,head+548),'COMDAT name/parent/owner pointer identities differ'
                record[74:78]=bytes(4);record[78:82]=bytes(4)
            assert (original/('section-name-%d.bin'%i)).read_bytes()==name.encode()+b'\0'
            assert (original/('section-class-%d.bin'%i)).read_bytes()==class_name.encode()+b'\0'
            original_text_ids.extend([nameptr,classptr]);row=dict(native_rows[i]);native_text_ids.extend([row.pop('name_pointer'),row.pop('class_pointer')])
            for field in (24,36,50,54):record[field:field+4]=bytes(4)
            wanted=bytearray(86);wanted[4]=9;struct.pack_into('<I',wanted,40,serial);struct.pack_into('<I',wanted,62,alignment);wanted[66:69]=bytes([attribute,7 if i==5 else 2,kind]);struct.pack_into('<I',wanted,78,order)
            if i==5:wanted[82]=2
            assert record==wanted,'original section scalar fields or untouched bytes differ'
            assert row=={'bytes':wanted.hex(),'name':name,'class_name':class_name,'context_alias':i!=5,'pool_offset':676 if i==5 else 72+88*i},'native complete section fields/aliases differ'
        assert all((a==b)==(native_text_ids[i]==native_text_ids[j])for i,a in enumerate(original_text_ids)for j,b in enumerate(original_text_ids)),'section name/class sharing or distinct-address identity differs'
        assert original_text_ids[4]==original_text_ids[5],'CONST default name/class must share address'
        summary=dict(state['sections']);summary.pop('rows')
        assert summary=={'tail_alias':True,'outputs':[0,1,2,3,None,None,4],'output_nulls':[False,False,False,False,True,True,False],'context_identity_alias':True,'default_alignment':4}
        unmodeled=(original/'unmodeled-original-data.bin').read_bytes();assert len(unmodeled)==67328
        assert receipts[0]['unmodeled_original_data']=={'classification':'UNMODELED; preservation only, no payload/extent/data credit','bytes':len(unmodeled),'sha256':sha(unmodeled)}
        assert struct.unpack_from('<I',unmodeled,0x497f58-0x48e000)[0]==0x49e300
    if expected.get('state_policy')=='section-symbol':
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        rows=re.findall(r'^section-symbol node=([0-9a-f]+) local=(\d+) name=([0-9a-f]+) class=([0-9a-f]+) name-alias=(\d+)$',segment,re.M)
        assert len(rows)==1 and rows[0][1]=='0'and rows[0][3:] == ('004973cc','1')
        node,_,name,class_name,_=(int(value,16)for value in rows[0])
        raw=bytearray((original/'section-node.bin').read_bytes());assert len(raw)==86
        assert struct.unpack_from('<I',raw,24)[0]==name and struct.unpack_from('<I',raw,54)[0]==class_name
        raw[24:28]=bytes(4);raw[54:58]=bytes(4)
        expected_node=bytearray(86);expected_node[4]=9;struct.pack_into('<I',expected_node,40,5)
        assert raw==expected_node,'original section fields or untouched bytes differ'
        assert (original/'section-name.bin').read_bytes()==b'_TEXT\0'and(original/'section-class.bin').read_bytes()==b'CODE\0'
        assert (original/'section-globals.bin').read_bytes()==bytes(28)
        assert state['section_symbol']=={'local':0,'bytes':expected_node.hex(),'name':'_TEXT','class_name':'CODE','name_alias':True,'pool_offset':72,'initial_sections':[0]*7},'section symbol ABI/content/aliases differ'
    if expected.get('state_policy')=='section-linked':
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        rows=re.findall(r'^section-linked node=([0-9a-f]+) name=([0-9a-f]+) class=([0-9a-f]+) name-alias=(\d+)$',segment,re.M)
        assert len(rows)==1 and rows[0][2:] == ('004973cc','1')
        node,name,class_name,_=(int(value,16)for value in rows[0])
        raw=bytearray((original/'section-node.bin').read_bytes());assert len(raw)==86
        assert struct.unpack_from('<I',raw,24)[0]==name and struct.unpack_from('<I',raw,54)[0]==class_name
        raw[24:28]=bytes(4);raw[54:58]=bytes(4)
        expected_node=bytearray(86);expected_node[4]=9;struct.pack_into('<I',expected_node,40,5)
        assert raw==expected_node,'original section fields or untouched bytes differ'
        assert (original/'section-name.bin').read_bytes()==b'_TEXT\0'and(original/'section-class.bin').read_bytes()==b'CODE\0'
        assert (original/'section-globals.bin').read_bytes()==bytes(28)
        assert state['section_linked']=={'bytes':expected_node.hex(),'name':'_TEXT','class_name':'CODE','name_alias':True,'pool_offset':72,'initial_sections':[0]*7},'section symbol ABI/content/aliases differ'
    if expected.get('state_policy')=='section-defaults':
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        rows=re.findall(r'^section-defaults node=([0-9a-f]+) head=([0-9a-f]+) tail=([0-9a-f]+)$',segment,re.M)
        assert len(rows)==1 and len(set(rows[0]))==1,'original section list head/tail do not alias argument'
        node=int(rows[0][0],16);name=0x49d4cc;class_name=0x4973cc
        raw=bytearray((original/'section-node.bin').read_bytes());assert len(raw)==86
        assert struct.unpack_from('<I',raw,24)[0]==name and struct.unpack_from('<I',raw,54)[0]==class_name
        raw[24:28]=bytes(4);raw[54:58]=bytes(4)
        expected_node=bytearray(86);expected_node[4]=9;struct.pack_into('<I',expected_node,40,5)
        assert raw==expected_node,'original section fields or untouched bytes differ'
        assert (original/'section-name.bin').read_bytes()==b'_TEXT\0'and(original/'section-class.bin').read_bytes()==b'CODE\0'
        assert (original/'section-globals.bin').read_bytes()==struct.pack('<7I',node,0,0,0,0,0,0)
        assert (original/'section-list.bin').read_bytes()==struct.pack('<2I',node,node)
        assert re.findall(r'^section-default-input context=([0-9a-f]+) alignment=(\d+)$',segment,re.M)==[('0049e300','4')]
        unmodeled=(original/'unmodeled-original-data.bin').read_bytes();assert len(unmodeled)==67328
        assert receipts[0]['unmodeled_original_data']=={'classification':'UNMODELED; preservation only, no payload/extent/data credit','bytes':len(unmodeled),'sha256':sha(unmodeled)}
        assert struct.unpack_from('<I',unmodeled,0x497f58-0x48e000)[0]==0x49e300

        assert state['section_defaults']=={'bytes':expected_node.hex(),'name':'_TEXT','class_name':'CODE','name_alias':True,'pool_offset':72,'code_section_alias':True,'other_sections':[0]*6,'head_alias':True,'tail_alias':True,'context_identity_alias':True},'section symbol ABI/content/aliases differ'
    if expected.get('state_policy')=='section-constructor':
        segment=text.rsplit('observe %08x '%address,1)[1]
        rows=re.findall(r'^section-constructor name=([0-9a-f]+) class=([0-9a-f]+) serial=(\d+) name-alias=(\d+)$',segment,re.M)
        assert len(rows)==1 and rows[0][1:] == ('004973cc','5','1')
        assert (original/'section-name.bin').read_bytes()==b'_TEXT\0'and(original/'section-class.bin').read_bytes()==b'CODE\0'
        assert (original/'section-globals.bin').read_bytes()==bytes(28)
        assert state['section_constructor']=={'name':'_TEXT','class_name':'CODE','serial':5,'name_alias':True,'initial_sections':[0]*7},'section constructor inputs/alias/initial globals differ'
    if expected.get('state_policy')in ('intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
        import struct
        from intrinsic_evidence import model as intrinsic_model,verify_original_state as verify_original_intrinsics,verify_native_state as verify_native_intrinsics
        value=intrinsic_model(config);assert value is not None
        verify_original_intrinsics(value,(original/'intrinsic-table.bin').read_bytes())
        assert proof.symbols(BINARY/'build/native/c2',data=True)['intrinsics'][1]==344*16
        verify_native_intrinsics(value,state['intrinsic_table'])
        segment=text.rsplit('observe %08x '%address,1)[1]
        assert re.findall(r'^intrinsics-initialized records=(\d+) bytes=(\d+)$',segment,re.M)==[('344','5504')]
        records=['000000000000000000000400','0000000000000080ff3f0400','000000000000008000400400']
        assert (original/'real-records.bin').read_bytes()==bytes.fromhex(''.join(records))
        fpu=(original/'x87-final.bin').read_bytes();assert len(fpu)==112
        assert tuple(n&0xffff for n in struct.unpack('<3I',fpu[:12]))==(0x27f,0,0xffff)
        assert state['real_results']=={'control':0x27f,'status':0,'tag':0xffff,'records':records},'intrinsic initialization changed real constants/x87 state'
    if expected.get('state_policy')=='extended-results':
        import struct,subprocess
        zero=bytes(10)+b'\x04\x00';one=bytes.fromhex('0000000000000080ff3f0400');two=bytes.fromhex('000000000000008000400400')
        values=[zero,one,two]
        assert (original/'real-records.bin').read_bytes()==b''.join(values),'original full12-byte real results differ'
        assert (original/'extended-destination.bin').read_bytes()==zero
        assert (original/'extended-after.bin').read_bytes()==zero,'FSTP changed the caller kind bytes'
        before=(original/'x87-state.bin').read_bytes();after=(original/'x87-after.bin').read_bytes();final=(original/'x87-final.bin').read_bytes()
        assert len(before)==len(after)==len(final)==112
        assert tuple(n&0xffff for n in struct.unpack('<3I',before[:12]))==(0x27f,0x3800,0x7fff)
        assert tuple(n&0xffff for n in struct.unpack('<3I',after[:12]))==(0x27f,0,0xffff),'original FSTP did not pop the x87 stack'
        assert struct.unpack_from('<I',after,12)[0]==0x46575e and struct.unpack_from('<I',after,20)[0]==0x48f880
        assert tuple(n&0xffff for n in struct.unpack('<3I',final[:12]))==(0x27f,0,0xffff)
        assert struct.unpack_from('<I',final,12)[0]==0x46575e and struct.unpack_from('<I',final,20)[0]==0x491058
        assert re.findall(r'^extended-after destination=([0-9a-f]+) control=([0-9a-f]+) status=([0-9a-f]+) tag=([0-9a-f]+) instruction=([0-9a-f]+)$',text,re.M)==[('0048f880','027f','0000','ffff','0046575e')]
        assert re.findall(r'^extended-results control=([0-9a-f]+) status=([0-9a-f]+) tag=([0-9a-f]+)$',text,re.M)==[('027f','0000','ffff')]
        current=state['extended_results'];assert (current['control'],current['status'],current['tag'])==(0x27f,0,0xffff)
        assert current['records']==[v.hex()for v in values]
        assert len(current['entries'])==3 and len(current['steps'])==6,'missing actual store observations'
        observed=(native/'gdb.stdout').read_text()
        assert [json.loads(row)for row in re.findall(r'^C2-STORE-ABI-ENTRY (.+)$',observed,re.M)]==current['entries']
        assert [json.loads(row)for row in re.findall(r'^C2-STORE-STEP (.+)$',observed,re.M)]==current['steps']
        assert [json.loads(row)for row in re.findall(r'^C2-EXTENDED-RESULTS (.+)$',observed,re.M)]==[current]
        assert receipts[1]['command'][receipts[1]['command'].index('set displaced-stepping off')-1]=='-ex'
        decoded=subprocess.check_output(['objdump','-d','-Mintel','--disassemble=store_extended',str(BINARY/'build/native/c2')],text=True)
        stores=re.findall(r'^\s*([0-9a-f]+):\s+db 3e\s+fstp\s+TBYTE PTR \[esi\]$',decoded,re.M)
        assert len(stores)==1
        at=int(stores[0],16)+int(native_proof['load_bias'],16)
        symbols=proof.symbols(BINARY/'build/native/c2',data=True)
        for index,(entry,value,name)in enumerate(zip(current['entries'],values,('c2_real_zero','c2_real_one','c2_real_two'))):
            first,second=current['steps'][index*2:index*2+2]
            pointer=symbols[name][0]+int(native_proof['load_bias'],16)
            assert entry['destination']==first['pointer']==second['pointer']==pointer
            assert (first['phase'],second['phase'],first['pc'],second['pc'])==('before','after',at,at+2)
            assert first['payload']==zero.hex()and second['payload']==value.hex(),'native ten-byte store/full caller record differs'
            assert first['payload'][20:]==second['payload'][20:]=='0400','store overwrote caller-owned kind'
            assert (first['control'],first['status'],first['tag'])==(0x27f,0x3800,0x7fff if index==0 else 0x3fff)
            assert first['registers'][0]==value[:10].hex()
            assert all(entry[key]==first[key]for key in ('control','status','tag','instruction','data_pointer','registers')),'naked tracing changed incoming ESI/x87 state'
            assert entry['saved_flags']==first['flags']==second['flags']and not entry['saved_flags']&0x100,'debugger TF or tracing changed saved flags'
            assert (second['control'],second['status'],second['tag'])==(0x27f,0,0xffff),'native FSTP did not pop its input'
            assert second['instruction']==at and second['data_pointer']==pointer,'native FSTP used a debugger scratch instruction or wrong destination'
        from import_evidence import pe_read
        rebuilt_store=re.findall(r'^\s+\S+\s+_store_extended\s+([0-9a-f]+)\b',(BINARY/'build/c2.map').read_text(),re.M)
        assert len(rebuilt_store)==1
        assert original_bytes[offset(inventory,0x46575e,4):offset(inventory,0x46575e,4)+4]==pe_read(BINARY/'build/c2.exe')(int(rebuilt_store[0],16),4)==bytes.fromhex('db3e9bc3'),'private store must be exact FSTP80/FWAIT/RET'
    if expected.get('state_policy')=='extended-store':
        import struct,subprocess
        segment=text.rsplit('observe %08x '%address,1)[1]
        raw=(original/'x87-state.bin').read_bytes();assert len(raw)==112
        control,status,tag,instruction,selector,data_pointer,data_selector=struct.unpack('<7I',raw[:28])
        assert (control&0xffff,status&0xffff,tag&0xffff,instruction)==(0x27f,0x3800,0x7fff,0x453090)
        stack=re.match(r'[^\n]* esp=([0-9a-f]+)\n',segment)
        assert stack and data_pointer==int(stack[1],16)+32,'original FILD input lost caller-local identity'
        assert re.findall(r'^extended-store destination=([0-9a-f]+) control=([0-9a-f]+) status=([0-9a-f]+) tag=([0-9a-f]+) instruction=([0-9a-f]+)$',segment,re.M)==[('0048f880','027f','3800','7fff','00453090')]
        destination=bytes(10)+b'\x04\x00'
        assert (original/'extended-destination.bin').read_bytes()==destination
        current=state['extended_store'];assert current['destination_is_zero']and current['source_alias']and current['destination']==destination.hex()
        assert current['pointer']==proof.symbols(BINARY/'build/native/c2',data=True)['c2_real_zero'][0]+int(native_proof['load_bias'],16),'ESI does not identify the real zero record'
        assert (current['control'],current['status'],current['tag'])==(control&0xffff,status&0xffff,tag&0xffff)
        assert current['registers']==[raw[28+10*i:38+10*i].hex()for i in range(8)]
        entry=dict(current['entry']);assert entry.pop('destination')==current['pointer']
        if 'saved_flags'in entry:assert not entry.pop('saved_flags')&0x100,'debugger TF leaked into the saved target flags'
        assert entry=={key:current[key]for key in ('control','status','tag','instruction','data_pointer','registers')},'naked-entry instrumentation changed the incoming x87/ESI ABI'
        observed=(native/'gdb.stdout').read_text()
        assert [json.loads(row)for row in re.findall(r'^C2-STORE-ABI-ENTRY (.+)$',observed,re.M)]==[current['entry']]
        assert [json.loads(row)for row in re.findall(r'^C2-STORE-ABI-STOP (.+)$',observed,re.M)]==[current]
        decoded=subprocess.check_output(['objdump','-d','-Mintel','--disassemble=c2_real_from_int',str(BINARY/'build/native/c2')],text=True)
        loads=re.findall(r'^\s*([0-9a-f]+):\s+db 07\s+fild\s+DWORD PTR \[edi\]$',decoded,re.M)
        assert len(loads)==1 and current['instruction']-int(native_proof['load_bias'],16)==int(loads[0],16),'native x87 instruction pointer is not the compiled caller FILD'
    if expected.get('state_policy')=='real-from-int':
        segment=text.rsplit('observe %08x '%address,1)[1]
        rows=re.findall(r'^real-from-int input=([0-9a-f]+) output=([0-9a-f]+) value=(-?\d+)$',segment,re.M)
        assert len(rows)==1 and int(rows[0][1],16)==0x48f880 and rows[0][2]=='0'
        stack=re.match(r'[^\n]* esp=([0-9a-f]+)\n',segment)
        assert stack and int(rows[0][0],16)==int(stack[1],16)+12,'original constant input must alias the caller stack local'
        assert (original/'real-records.bin').read_bytes()==bytes(36)
        assert state['real_from_int']=={'value':0,'output_is_zero':True,'caller_local_alias':True,'records':['00'*12]*3},'constant initializer input/records/aliases differ'
    if expected.get('state_policy')in ('pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        rows=re.findall(r'^pool-result free=([0-9a-f]+) types=([0-9a-f]+) head=([0-9a-f]+) cursor=([0-9a-f]+) remaining=(\d+)$',segment,re.M)
        assert len(rows)==1
        free,types,head,cursor=(int(value,16)for value in rows[0][:4]);remaining=int(rows[0][4])
        descriptors=(original/'pool-descriptors.bin').read_bytes();block=(original/'pool-block.bin').read_bytes()
        assert len(descriptors)==48 and len(block)==4096 and head
        assert struct.unpack('<12I',descriptors)==(head,cursor,remaining)+(0,)*9
        payload=bytearray(4088);payload[4]=1
        expected_cursor,expected_remaining,node_bytes=72,4024,64
        if expected['state_policy']in ('section-symbol','section-linked','section-defaults'):
            expected_cursor,expected_remaining,node_bytes=160,3936,150
            node=(original/'section-node.bin').read_bytes();payload[64:150]=node
        if expected['state_policy']in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            expected_cursor,expected_remaining,node_bytes=512,3584,494
            if expected['state_policy']in ('symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):node_bytes=516
            if expected['state_policy']=='filename-record':
                expected_cursor,expected_remaining=536,3560
                payload[504:527]=b'numeric-control-flow.c\0'
            if expected['state_policy']in ('symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
                expected_cursor,expected_remaining,node_bytes=660,3436,626
                payload[504:527]=b'numeric-control-flow.c\0'
                payload[528:540]=(original/'filename-node.bin').read_bytes()
                payload[540:650]=(original/'function-symbol.bin').read_bytes()
                if expected['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
                    expected_cursor,expected_remaining,node_bytes=676,3420,639
                    payload[652:666]=b'_mix_unsigned\0'
                    if expected['state_policy']=='symbol-put-input':
                        expected_cursor,expected_remaining,node_bytes=776,3320,725
                        payload[668:754]=(original/'sections.bin').read_bytes()[430:516]
            section_bytes=(original/'sections.bin').read_bytes()
            for i in range(5):payload[64+88*i:150+88*i]=section_bytes[86*i:86*(i+1)]
        assert block==struct.pack('<II',0,4088)+payload,'original pool block header/zeroed payload/node kind differs'
        if expected['state_policy']in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            for i in range(5):
                for field in (24,36,50,54):payload[64+88*i+field:68+88*i+field]=bytes(4)
            if expected['state_policy']in ('symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):payload[532:536]=bytes(4)

        if expected['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):payload[564:568]=bytes(4)
        if expected['state_policy']=='symbol-put-input':
            for at in (692,704,718,722,742,746):payload[at:at+4]=bytes(4)
        if expected['state_policy']in ('section-symbol','section-linked','section-defaults'):
            payload[88:92]=bytes(4);payload[118:122]=bytes(4)
            observed_node=re.findall(r'^section-(?:symbol|linked|defaults) node=([0-9a-f]+) ',segment,re.M)
            assert observed_node==['%08x'%(head+72)],'symbol argument lost allocation alias'
        assert (free,types-head,cursor-head,remaining)==(0,8,expected_cursor,expected_remaining)
        assert state['pool_result']=={'free':0,'types_offset':8,'cursor_offset':expected_cursor,'remaining':expected_remaining,
            'next':0,'capacity':4088,'payload':payload.hex(),'other_pools':['00'*12]*3},'native pool allocation contents/aliases/accounting differ'
        assert [state['globals'][hex(0x491040+4*i)]for i in range(4)]==[node_bytes,0,0,0]
    if expected.get('state_policy')=='intermediate-open':
        raw=re.findall(r'^intermediate-open base=([0-9a-f]+) suffix=([0-9a-f]+) mode=([0-9a-f]+) description=([0-9a-f]+)$',text.rsplit('observe %08x '%address,1)[1],re.M)
        assert len(raw)==1 and int(raw[0][0],16)==origin+4
        for name,value in [('base',b'.\\a01068'),('suffix',b'in'),('mode',b'r'),('description',b'init il')]:
            assert (original/('intermediate-'+name+'.bin')).read_bytes()==value+b'\0'
        control=int(re.findall(r'^x87-control ([0-9a-f]+)$',text.rsplit('observe %08x '%address,1)[1],re.M)[0],16)&0xffff
        assert state['intermediate_open']=={'base':'.\\a01068','suffix':'in','mode':'r','description':'init il','base_alias':True,'x87_control':control},'intermediate opener typed ABI/input/x87 state differs'
    if expected.get('state_policy')=='integer-option':
        initial=re.findall(r'^flags-environment entry=([0-9a-f]+) returned=([0-9a-f]+) alias=(\d+)$',text,re.M)
        assert len(initial)==1 and initial[0][2]=='1'
        origin=int(initial[0][1],16)
        raw=re.findall(r'^integer-option input=([0-9a-f]+) current=([0-9a-f]+) operand=([0-9a-f]+) argv=([0-9a-f]+)$',text.rsplit('observe %08x '%address,1)[1],re.M)
        assert raw==[('%08x'%(origin+42),'%08x'%(origin+39),'%08x'%(origin+42),'00498010')],'parsed option input/storage aliases differ'
        assert re.findall(r'^integer-ctype mb-address=([0-9a-f]+) mb=(\d+) table-address=([0-9a-f]+) table=([0-9a-f]+) character=(\d+) flags=([0-9a-f]+)$',text.rsplit('observe %08x '%address,1)[1],re.M)==[('7803fba0','1','7803fb9c','7803fbaa','49','0084')]
        assert (original/'integer-input.bin').read_bytes()==b'1\0'and(original/'current-option.bin').read_bytes()==b'-W\0'
        assert state['integer_option']=={'text':'1','input_offset':42,'current':'-W','current_offset':39,'operand_alias':True,'argv_offset':24,'argv_value_alias':True,'mb_cur_max':1,'character_flags':0x84},'native parsed environment argument aliases differ'
        original_overrides={'c2_intermediate_base':origin+4,'c2_source_name':origin+16}
        native_overrides={'c2_intermediate_base':'.\\a01068','c2_source_name':'numeric-control-flow.c'}
    if expected.get('state_policy')=='message-initialize':
        raw=re.findall(r'^message-initialize current=([0-9a-f]+) unexpected=([0-9a-f]+)$',text,re.M)
        assert len(raw)==1 and raw[0][0]==raw[0][1],'message initialization option aliases differ'
        assert (original/'current-option.bin').read_bytes()==b'/?\0'
        pointer=state['message_initialization']['current_pointer'];assert pointer>0
        assert state['message_initialization']=={'current':'/?','current_alias':1,'current_pointer':pointer}
        original_overrides={'c2_unexpected_argument':int(raw[0][0],16)}
        native_overrides={'c2_unexpected_argument':'/?'}
    if expected.get('state_policy')=='message-lookup':
        raw=re.findall(r'^message-lookup id=(\d+) allow=(\d+) current=([0-9a-f]+) unexpected=([0-9a-f]+)$',text,re.M)
        assert len(raw)==1 and int(raw[0][0])==1007 and int(raw[0][1])==0 and raw[0][2]==raw[0][3],'message lookup inputs/aliases differ'
        assert (original/'current-option.bin').read_bytes()==b'/?\0'
        pointer=state['lookup']['current_pointer'];assert pointer>0
        assert state['lookup']=={'message':1007,'allow_missing':0,'current':'/?','current_alias':1,'current_pointer':pointer},'message lookup ABI differs'
        original_overrides={'c2_unexpected_argument':int(raw[0][2],16)}
        native_overrides={'c2_unexpected_argument':'/?'}
    if expected.get('state_policy')=='fatal-option':
        raw=re.findall(r'^fatal-option message=(\d+) argument=([0-9a-f]+) phase=([0-9a-f]+) current=([0-9a-f]+) unexpected=([0-9a-f]+)$',text,re.M)
        assert len(raw)==1 and int(raw[0][0])==7 and raw[0][1]==raw[0][3]==raw[0][4],'fatal message/argument aliases differ'
        assert (original/'fatal-argument.bin').read_bytes()==b'/?\0'and(original/'fatal-phase.bin').read_bytes()==b'p2\0'
        pointer=state['fatal']['argument_pointer']
        assert state['fatal']=={'message':7,'argument':'/?','phase':'p2','current_alias':1,'unexpected_alias':1,'argument_pointer':pointer},'fatal varargs differ'
        assert pointer>0
        original_overrides={'c2_unexpected_argument':int(raw[0][1],16)}
        native_overrides={'c2_unexpected_argument':'/?'}
    if expected.get('state_policy')=='apply-option':
        raw=re.findall(r'^apply-state option=([0-9a-f]+) value=([0-9a-f]+) mode=(\d+) current=([0-9a-f]+)$',text,re.M)
        assert len(raw)==1 and int(raw[0][0],16)==0x4999d0 and raw[0][1]==raw[0][3]and int(raw[0][2])==0
        assert (original/'option-value.bin').read_bytes()==b'/?\0'
        table=proof.symbols(BINARY/'build/native/c2',data=True)['c2_options'][0]+int(native_proof['load_bias'],16)
        assert state['apply']=={'option':table+52*12,'value':'/?','mode':0,'current_alias':1},'option application ABI differs'
    if expected.get('state_policy')in ('wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
        from data_evidence import model,plan,verify_original_state,verify_native_state
        data_model=model(config)
        assert (original/'data-plan.txt').read_text()==''.join('%08x %d\n'%(address,size)for name,address,size in plan(data_model))
        assert sha((original/'data-state.bin').read_bytes())==receipts[0]['data_state_sha256']
        verify_original_state(data_model,(original/'data-state.bin').read_bytes(),original_overrides)
        verify_native_state(data_model,state['data_state'],int(native_proof['load_bias'],16),proof.symbols(BINARY/'build/native/c2',data=True),native_overrides)
    if expected.get('state_policy')=='wildcard-match':
        raw=re.findall(r'^wildcard-state pattern=([0-9a-f]+) input=([0-9a-f]+) kind=(\d+) current=([0-9a-f]+)$',text,re.M)
        assert len(raw)==1 and raw[0][1]==raw[0][3]and int(raw[0][2])==34
        assert (original/'wildcard-pattern.bin').read_bytes()==b'*\0' and(original/'wildcard-input.bin').read_bytes()==b'/?\0'
        assert state['wildcard']=={'pattern':'*','input':'/?','kind':34,'current_alias':1},'wildcard ABI inputs differ'
    if expected.get('state_policy')=='match-option':
        raw=re.findall(r'^matcher-state table=([0-9a-f]+) input=([0-9a-f]+) callback=([0-9a-f]+) mode=(\d+)$',text,re.M)
        assert len(raw)==1
        pending=next(r for r in config['pending_data']if r['name']=='c2_options')
        assert int(raw[0][0],16)==pending['original'] and int(raw[0][2],16)==0x452bb7 and int(raw[0][3])==0
        assert (original/'matcher-input.bin').read_bytes()==b'/?\0'
        table,size=proof.symbols(BINARY/'build/native/c2',data=True)['c2_options']
        bias=int(native_proof['load_bias'],16)
        callback=proof.symbols(BINARY/'build/native/c2')['c2_next_argument'][0]+bias
        assert size==pending['storage_records']*12
        assert state['matcher']=={'table':table+bias,'input':'/?','next':callback,'mode':0,
                                  'record_size':12,'storage_records':pending['storage_records']},'matcher ABI/logical table identity differs'
        line='C2-MATCHER table=%s next=%s mode=0'%(hex(table+bias),hex(callback))
        assert re.findall(r'^C2-MATCHER .*$',(native/'gdb.stdout').read_text(),re.M)==[line]
    if expected.get('state_policy')=='exit-callback':
        callback=next(r for r in config['stubs']if r['name']=='_c2_cleanup')
        assert re.findall(r'^exit-callback ([0-9a-f]+)$',text.rsplit('observe %08x '%address,1)[1],re.M)==['%08x'%callback['start']]
        callback_native=proof.symbols(BINARY/'build/native/c2')[callback['native']][0]+int(native_proof['load_bias'],16)
        assert state['callback']==callback_native,'exit callback address is not the named native function'
        assert re.findall(r'^C2-CALLBACK (0x[0-9a-f]+)$',(native/'gdb.stdout').read_text(),re.M)==[hex(callback_native)]
    if expected.get('state_policy')=='main-arguments-and-initial-state':
        raw_main=re.findall(r'^main-input argc=(\d+) argv=([0-9a-f]+) environment=([0-9a-f]+)$',text,re.M)
        assert len(raw_main)==1 and int(raw_main[0][0])==2
        def vector_bytes(name):
            data=(original/name).read_bytes();assert data.endswith(b'\0\0')
            return [v.decode()for v in data[:-2].split(b'\0')]
        expected_main={'argc':2,'argv':vector_bytes('main-argv.bin'),'environment':vector_bytes('main-environment.bin'),
                       'initenv_alias':1,'x87_control':int(re.findall(r'^x87-control ([0-9a-f]+)$',text,re.M)[-1],16)&0xffff}
        assert re.findall(r'^main-initenv-alias (\d+)$',text,re.M)==['1']
        assert (original/'main-argv.bin').read_bytes()==(BINARY/'cases/unsupported-help-diagnostic/argv.bin').read_bytes()
        assert (original/'main-environment.bin').read_bytes()==(BINARY/'cases/unsupported-help-diagnostic/environment.bin').read_bytes()
        assert state['main']==expected_main,'main argument vectors/alias/x87 differ'
    assert all((original/(s+'.bin')).read_bytes()==(native/(s+'.bin')).read_bytes()for s in ('stdout','stderr')),'partial stream mismatch'
    return {'validation':str(audit_file),'graph_report':str(report_file),'matched':expected['name'],'original':hex(address),'native':state['function'],'argument_state':expected.get('state_policy','no arguments at initial entry'),'native_elf_proof':native_proof,'source_trace':state['trace'],'candidates':config['trial'],'acceptance':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--validation',type=Path)
    a=p.parse_args();print(json.dumps(compare(a.original,a.native,a.validation),indent=2))
