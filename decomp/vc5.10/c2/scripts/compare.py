#!/usr/bin/env python3
"""Compare bound fresh original/native frontier evidence; no acceptance mutation."""
import argparse
import json
import re
from pathlib import Path
from state import BINARY, CANONICAL_CRT, load, receipt_bound, sha, masked, offset, trial_configuration, audit_input_hashes

def compare(original,native,validation=None):
    base,original_bytes,inventory=load()
    config,_=trial_configuration((BINARY/'functions.json').read_bytes())
    from case_inputs import manifest,normalized
    case_directory,case_value,target_environment=manifest(config)
    receipts=[]
    for path,is_native in ((original,False),(native,True)):
        receipt=json.loads((path/'receipt.json').read_text())
        assert (path/'base_configuration.json').read_bytes()==(BINARY/'functions.json').read_bytes(),'base configuration changed'
        receipt_bound(receipt,(path/'configuration.json').read_bytes(),is_native)
        for stream in ('stdout','stderr'):
            data=(path/(stream+'.bin')).read_bytes()
            assert receipt[stream]=={'bytes':len(data),'sha256':sha(data)},'stream evidence changed'
        receipts.append(receipt)
    if config.get('data_model'):
        model_bytes=(BINARY/config['data_model']).read_bytes()
        assert all(r['data_model_sha256']==sha(model_bytes)for r in receipts)
        assert all((p/'data-model.json').read_bytes()==model_bytes for p in (original,native))
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
    for support in config['support']:
        if support['kind']=='import'and support['name']in config['accepted']:
            identity=graph['import_catalogs']['original']['slots'][str(support['iat'])]
            assert identity=={'dll':support['dll'].lower(),'name':support['symbol']},'import-support slot identity differs'
    assert audit['configuration_sha256']==receipts[0]['configuration_sha256'],'trial audit configuration differs'
    assert audit['rebuilt_sha256']==sha((BINARY/'build/c2.exe').read_bytes()),'current trial reconstruction differs'
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
        assert expected.get('state_policy')in ('pre-main-no-arguments','main-arguments-and-initial-state','exit-callback','command-line-state','environment-flags','match-option','wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','message-load','message-unescape','diagnostic-emit','cached-message','format-prefix','format-prefix-args','format-integer','exit-cleanup','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read')and expected.get('state'),'new frontier needs explicit state decoder'
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
        required={view['original']:raw[int(view['original'],16)]for view in expected['state']}
        assert state['globals']==required,'frontier globals/runtime state differs'
    if expected.get('state_policy')in ('command-line-state','environment-flags','match-option','wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read'):
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
    if expected.get('state_policy')in ('alignment-option','intermediate-open','header-read','il-byte-read','il-word-read'):
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
    if expected.get('state_policy')in ('header-read','il-byte-read','il-word-read'):
        import struct
        segment=text.rsplit('observe %08x '%address,1)[1]
        rows=re.findall(r'^header-streams input=([0-9a-f]+) globals=([0-9a-f]+) iob=([0-9a-f]+)$',segment,re.M)
        assert len(rows)==1;first,second,iob=[int(v,16)for v in rows[0]]
        assert first==iob+3*32 and second==iob+4*32,'original FILE pointers are not distinct canonical iob slots'
        first_data=struct.pack('<8I',0,0,0,1,3,0,0,0);second_data=struct.pack('<8I',0,0,0,1,4,0,0,0)
        if expected['state_policy']=='il-word-read':
            original_stream=(original/'header-streams.bin').read_bytes();assert len(original_stream)==64 and original_stream[:32]==first_data
            words=struct.unpack('<8I',original_stream[32:])
            assert words[2]and words[0]==words[2]+1 and words[1]==270 and words[3:]==(9,4,0,4096,0),'canonical post-byte FILE/buffer layout differs'
            native_words=struct.unpack('<8I',bytes.fromhex(state['header_streams']['globals']))
            assert native_words[2]and native_words[0]==native_words[2]+1 and native_words[1]==270 and native_words[3:]==(9,4,0,4096,0),'native post-byte FILE/buffer layout differs'
            master=(case_directory/next(v['path']for v in case_value['inputs']if v['working_name']=='a01068gl')).read_bytes()
            assert len(master)==271 and(original/'il-buffer.bin').read_bytes()==master
            assert re.findall(r'^il-buffer valid=(\d+) pointer-offset=(\d+)$',segment,re.M)==[('271','1')]
            second_data=bytes.fromhex(state['header_streams']['globals'])
        else:
            assert (original/'header-streams.bin').read_bytes()==first_data+second_data,'new canonical CRT streams are not unread read-mode files'
        control=int(re.findall(r'^x87-control ([0-9a-f]+)$',segment,re.M)[0],16)&0xffff
        expected_streams={'input_index':3,'globals_index':4,'input':first_data.hex(),'globals':second_data.hex(),'distinct':True,'x87_control':control}
        if expected['state_policy']in ('il-byte-read','il-word-read'):
            assert re.findall(r'^active-il-stream ([0-9a-f]+)$',segment,re.M)==['%08x'%second],'header byte reader does not use globals IL stream'
            expected_streams['active_index']=4
        if expected['state_policy']=='il-word-read':expected_streams['valid_buffer']=master.hex()
        assert state['header_streams']==expected_streams,'native FILE identity/layout/state differs'
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
    if expected.get('state_policy')in ('wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read'):
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
