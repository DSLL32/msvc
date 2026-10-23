"""Typed resource-case checkpoints, built on the common immutable raw gate."""
import json
import re
import struct
from pathlib import Path
from protocol import HERE,ROOT,sha,read_configuration,original_raw,native_raw,help_state,callbacks,logical_trace

def raw_bytes(text,label):
    records=re.findall(r'^'+re.escape(label)+r' (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(records)==1,'missing/nonunique bytes '+label
    size,raw=records[0];value=bytes.fromhex(raw)
    assert len(value)==int(size),'wrong byte length '+label
    return value

def resource_inputs(original,native,b):
    r=json.loads((original/'receipt.json').read_text())
    assert r['case']=='resources-rich' and r['args']==['/r','/dCASE_FROM_COMMAND=7','/iinclude','/foresources.res','resources.rc']
    assert sha(original/'loaded-driver.bin')==sha(ROOT/'toolchains/5.10/bin/RC.EXE')
    text=(original/'events.txt').read_text()
    assert re.findall(r'^export-caller ([0-9a-f]+)$',text,re.M)==['010011fd']
    slots=re.findall(r'^export-args ((?:[0-9a-f]+ ){4})(\d+) ([0-9a-f]+)$',text,re.M)
    assert len(slots)==1 and slots[0][0].split()==['00000000','00000000','01001100','00000000']and int(slots[0][1])==6
    before=[]
    for i in range(6):
        value=raw_bytes(text,'export-before-argv-%d'%i)
        assert value.endswith(b'\0')and b'\0'not in value[:-1]
        before.append(value)
    assert [v[:-1].decode('ascii')for v in before]==[r['argv0']]+r['args']
    old_pointers=[int(v,16)for v in re.findall(r'^export-before-pointer \d+ ([0-9a-f]+)$',text,re.M)]
    current=[int(v,16)for v in re.findall(r'^export-frontier-pointer \d+ ([0-9a-f]+)$',text,re.M)]
    assert len(old_pointers)==len(current)==len(b['export_vector'])==6
    for i,value in enumerate(b['export_vector']):
        assert bytes.fromhex(value['bytes'])==raw_bytes(text,'export-frontier-storage-%d'%i)
        assert value['text'].encode()+b'\0'==raw_bytes(text,'export-frontier-argv-%d'%i)
        original_offsets=[(j,current[i]-pointer)for j,pointer in enumerate(old_pointers)if pointer<=current[i]<pointer+len(before[j])]
        native_offsets=[(j,value['pointer']-other['storage'])for j,other in enumerate(b['export_vector'])if other['storage']<=value['pointer']<other['storage']+len(bytes.fromhex(other['bytes']))]
        assert len(original_offsets)==1 and original_offsets==native_offsets,'argument storage/alias differs'
    assert re.findall(r'^export-frontier-terminator ([0-9a-f]+)$',text,re.M)==['00000000']
    native_receipt=json.loads((native/'receipt.json').read_text())
    assert native_receipt['canonical_cwd']==r['child_cwd']
    cwd=native/'drive-s'/r['child_cwd'][3:].replace('\\','/')
    runs=[value for value in native_receipt['command']if value.startswith('run ')]
    assert len(runs)==1
    recorded_root=Path(runs[0].split()[1]).parent
    expected_cwd=recorded_root/'drive-s'/r['child_cwd'][3:].replace('\\','/')
    assert str(expected_cwd)==native_receipt['case_cwd']==b['case_inputs']['host_cwd']
    assert r['case_files']==native_receipt['case_files'],'partial file writes/artifacts differ'
    for name,info in r['case_files'].items():
        assert sha(original/'source'/name)==sha(cwd/name)==info['sha256'],name
    identity=json.loads((native/'case-reference.json').read_text())
    path=HERE/'cases/resources-rich/reference/manifest.json'
    assert identity==dict(manifest=str(path),sha256=sha(path),profile_sha256=sha(HERE/'cases/resources-rich/api-profile.json'),original_inputs=str(Path(r['command'][1]).parent),original_receipt_sha256=sha(original/'receipt.json'))
    manifest=json.loads(path.read_text())
    for name,digest in manifest['files'].items():assert sha(path.parent/name)==digest
    for name,info in manifest['source_inputs'].items():assert r['case_files'][name]==info
    profile=json.loads((HERE/'cases/resources-rich/api-profile.json').read_text())
    assert sha(HERE/'cases/resources-rich'/profile['source'])==profile['source_sha256']and profile['ansi_codepage']==1252
    headers=json.loads((path.parent/'include-tree.json').read_text())
    for name,info in headers['files'].items():assert sha(ROOT/headers['root']/name)==info['sha256']
    # Raw Windows environment is retained as input. Only observed getenv keys
    # are transported into the native test environment, whose controls are host-only.
    environment=(original/'environment-utf16.bin').read_bytes()
    assert environment.endswith(b'\0\0\0\0')and not len(environment)%2
    environment_strings=environment.decode('utf-16le').rstrip('\0').split('\0')
    assert b['case_inputs']['environment']==environment_strings,'target-visible environment differs'
    assert (native/'case-environment-utf16.bin').read_bytes()==environment
    assert all(not value.startswith('RECON_')for value in environment_strings),'host profile controls leaked into target environment'
    values=dict(value.split('=',1)for value in environment_strings if value and not value.startswith('='))
    assert b['case_inputs']['include']==values.get('INCLUDE')and b['case_inputs']['codepage']==values.get('RCCODEPAGE')
    assert raw_bytes(text,'process-cwd-utf16').decode('utf-16le').rstrip('\\')==r['child_cwd']
    return dict(count=6,before=[value.hex()for value in before],environment_sha256=sha(original/'environment-utf16.bin'),cwd=r['child_cwd'])

def checkpoint(original,native,function,extra_special=()):
    a=original_raw(original);b=native_raw(native)
    config=read_configuration(original/'functions.json')
    assert a['function']==b['function']==function
    row=next(f for f in config['functions']if f['name']==function)
    assert a['rva']==int(b['rva'],16)==int(row['start'],16)
    state=help_state(original,native,a,b,extra_special)
    inputs=resource_inputs(original,native,b)
    assert b['jump']and b['jump']['opaque_setjmp3']==1
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    assert callbacks(original)==callbacks(native)==[]
    text=(original/'events.txt').read_text()
    table={int(f['start'],16):f for f in config['functions']}
    old_entries=[int(v,16)for v in re.findall(r'^entry ([0-9a-f]+)$',text,re.M)]
    original_trace=[table[rva]['name']for rva in old_entries if rva in table and table[rva]['kind']=='application']
    assert len(original_trace)==len(set(original_trace))
    assert original_trace==logical_trace((native/'trace.txt').read_text())
    return a,b,dict(matched_frontier=function,globals=state,inputs=inputs,application_trace=original_trace)

def release_entry(original,native):
    a,b,proof=checkpoint(original,native,'rc_release')
    assert a['words'][0]==0x71c27dc6
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rcdll_command_main\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text()
    assert re.findall(r'^getacp-result (\d+) last-error=([0-9a-f]+)$',text,re.M)==[('1252','00000000')]
    assert bytes.fromhex(b['arguments']['payload'])==raw_bytes(text,'release-block')
    assert b['arguments']['include'].encode()+b'\0'==raw_bytes(text,'release-include')==b'include\0'
    identity=b['heap_identity'];assert identity['options']==1 and identity['initial']==identity['maximum']==0
    blocks=identity['blocks'];assert len(blocks)==2
    pointer=b['arguments']['block'];first=next(block for block in blocks if block['payload']+8==pointer)
    assert first['owner']==identity['handle']and first['flags']==9 and first['bytes']==528
    assert first['contents']==b['arguments']['payload']
    assert bytes.fromhex(b['arguments']['new_payload'])==raw_bytes(text,'release-new-block')
    second=next(block for block in blocks if block['payload']+8==b['arguments']['new_paths'])
    assert second['owner']==identity['handle']and second['flags']==9 and second['bytes']==528
    assert second['contents']==b['arguments']['new_payload']
    assert int.from_bytes(bytes.fromhex(b['globals']['rc_include_paths']),'little')==pointer
    old=(original/'globals.bin').read_bytes()
    assert struct.unpack_from('<I',old,0x11588)[0]==a['words'][1]!=0
    calls=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    pointers=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    assert len(calls)==len(pointers)==2
    assert all(flags=='9'and size=='528'and error=='00000000'for heap,flags,size,error in calls)
    assert int(pointers[0][0],16)+8==a['words'][1]
    assert all(error=='00000000'for pointer,error in pointers)
    proof['release']=dict(bytes=528,header_bytes=8,original_argument=a['words'][1],native_argument=pointer,contents=first['contents'])
    return proof


def release_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_release' and len(config['accepted'])==11
    a,b,proof=checkpoint(original,native,'rc_prepare_input',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c28340 and a['words'][1]==0x71c45c38
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rcdll_command_main\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();raw=(original/'globals.bin').read_bytes();rich=b['rich']
    assert raw_bytes(text,'prepare-path')==b['arguments']['path'].encode()+b'\0'==b'resources.rc\0'
    for name,rva,size in [('rc_output_path',0x24f50,260),('rc_input_path',0x25c38,260),('rc_input_name',0x25b30,260),('rc_preprocessed_path',0x22e40,260),('rc_notice_text',0x22c38,520)]:
        assert raw[rva-0x14000:rva-0x14000+size].hex()==rich['buffers'][name],name
    count=re.findall(r'^preprocessor-count (\d+)$',text,re.M);assert count==['20']
    arguments=[raw_bytes(text,'preprocessor-argv-%d'%i)[:-1].decode('ascii')for i in range(20)]
    assert arguments==rich['preprocessor_argv']
    for field,label in [('rc_input_temporary','input-temporary'),('rc_output_temporary','output-temporary')]:
        assert raw_bytes(text,label)==rich['temporaries'][field].encode()+b'\0'
    old_files=struct.unpack('<8I',raw_bytes(text,'output-file-state'));new_files=struct.unpack('<8I',bytes.fromhex(rich['output_file']))
    assert old_files[:4]+old_files[5:]==new_files[:4]+new_files[5:]==(0,0,0,128,0,0,0)
    assert old_files[4]>=3 and new_files[4]>=3
    rn=json.loads((native/'receipt.json').read_text())
    assert rich['output_file_host_path']==rn['case_cwd']+'/resources.res'
    assert re.findall(r'^pos:\s*(\d+)$',rich['output_file_fdinfo'],re.M)==['0']
    assert rn['case_files']['resources.res']==dict(bytes=0,sha256='e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855')
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    allocations=[record for record in b['heap']if record['operation']=='allocate']
    assert [int(record[2])for record in requests]==[528,528,268,268]==[record['bytes']for record in allocations]
    assert all(record['flags']==9 and record['payload']=='00'*record['bytes']for record in allocations)
    old_frees=re.findall(r'^heap-free ([0-9a-f]+) (\d+) ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    assert len(old_frees)==1 and old_frees[0][1]=='1'and old_frees[0][3]=='00000000'
    results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    assert old_frees[0][2]==results[0][0]
    assert re.findall(r'^heap-free-result (\d+) last-error=([0-9a-f]+)$',text,re.M)==[('1','00000000')]
    frees=[record for record in b['heap']if record['operation']=='free'];assert len(frees)==1
    free=frees[0];assert free['flags']==1 and free['result']==1 and free['last_error_before']==free['last_error_after']==0
    assert free['heap']==identity['handle']and free['payload']==allocations[0]['result']
    assert free['live_after']==[dict(payload=allocations[1]['result'],bytes=528,flags=9)]
    assert free['live_after'][0]['payload']!=free['payload']
    # Later allocations may legitimately reuse the freed address. Match the
    # allocation generation and size, rather than asserting permanent absence.
    assert all(block['bytes']!=528 or block['payload']!=free['payload']for block in blocks)
    assert proof['application_trace'][-1]=='rc_release'
    proof.update(trial='rc_release',private_free=dict(flags=1,result=1,original_base=int(old_frees[0][2],16),native_base=free['payload'],freed_generation_absent=True,base_reused=any(block['payload']==free['payload']for block in blocks)),preprocessor_arguments=arguments,next_input_file=b['arguments']['path'],artifact=dict(name='resources.res',bytes=0),registered_module=b['module'])
    return proof
