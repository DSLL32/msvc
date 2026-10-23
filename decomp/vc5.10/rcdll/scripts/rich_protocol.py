"""Typed resource-case checkpoints, built on the common immutable raw gate."""
import json
import hashlib
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


def prepare_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_prepare_input'and len(config['accepted'])==12
    a,b,proof=checkpoint(original,native,'rc_write_bytes',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c286d6 and a['words'][3]==46
    text=(original/'events.txt').read_text();arguments=b['arguments']
    expected='#line 1"resources.rc"\r\n'.encode('utf-16le')
    assert arguments['bytes']==len(expected)==46
    assert raw_bytes(text,'write-payload')==bytes.fromhex(arguments['payload'])==expected
    assert raw_bytes(text,'write-filename')==bytes.fromhex(arguments['filename'])=='resources.rc'.encode('utf-16le')+b'\0'*(514-24)
    raw=(original/'globals.bin').read_bytes()
    assert struct.unpack_from('<I',raw,0x149c4-0x14000)[0]==0x71c37f08
    assert raw[0x17f08-0x14000:0x17f08-0x14000+514]==raw_bytes(text,'write-filename')
    old=struct.unpack('<8I',raw_bytes(text,'write-file-state'));new=struct.unpack('<8I',bytes.fromhex(arguments['file_state']))
    assert old[:4]+old[5:]==new[:4]+new[5:]==(0,0,0,128,0,0,0)and old[4]>=3 and new[4]>=3
    receipt=json.loads((native/'receipt.json').read_text())
    temporary=raw_bytes(text,'input-temporary')[:-1].decode('ascii')
    assert temporary.startswith(receipt['canonical_cwd']+'\\')
    temporary_name=temporary[len(receipt['canonical_cwd'])+1:];assert '/'not in temporary_name and '\\'not in temporary_name
    assert arguments['file_host_path']==receipt['case_cwd']+'/'+temporary_name
    assert re.findall(r'^pos:\s*(\d+)$',arguments['file_fdinfo'],re.M)==['0']
    assert receipt['case_files'][temporary_name]==dict(bytes=0,sha256=hashlib.sha256(b'').hexdigest())
    assert all(not info['bytes']for name,info in receipt['case_files'].items()if name not in json.loads((HERE/'cases/resources-rich/reference/manifest.json').read_text())['source_inputs'])
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3
    assert identity['options']==1 and identity['initial']==identity['maximum']==0
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    assert proof['application_trace'][-1]=='rc_prepare_input'
    gdb=(native/'gdb.stdout').read_text()
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_prepare_input\s*\(',gdb,re.M)
    frame=arguments['parent_frame'];assert frame['caller']=='rc_prepare_input'
    records=re.findall(r'^RCDLL_PARENT_FRAME=([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',gdb,re.M)
    assert records==[tuple('%08x'%frame[key]for key in ['pc','esp','ebp'])]
    assert frame['esp']==b['registers']['esp']and 0<frame['ebp']-frame['esp']<65536
    objects=frame['objects'];assert [(row['name'],row['bytes'])for row in objects]==[('escaped',520),('directive',8196),('line',8354)]
    assert re.findall(r'^RCDLL_FRAME_OBJECT (\w+)=([0-9a-f]+) (\d+)$',gdb,re.M)==[(row['name'],'%08x'%row['address'],str(row['bytes']))for row in objects]
    assert all(frame['esp']<=row['address']<row['address']+row['bytes']<=frame['ebp']for row in objects)
    ordered=sorted(objects,key=lambda row:row['address'])
    assert all(x['address']+x['bytes']<=y['address']for x,y in zip(ordered,ordered[1:]))
    assert arguments['data']==next(row['address']for row in objects if row['name']=='directive')
    low,high=[int(value,16)for value in frame['stack_mapping'].split()[0].split('-')]
    assert frame['stack_mapping'].endswith('[stack]')and low<=frame['esp']<frame['ebp']<high
    entry=re.findall(r'^compiler-probe-entry eax=([0-9a-f]+) esp=([0-9a-f]+) return=([0-9a-f]+) ecx=([0-9a-f]+) ebx=([0-9a-f]+) esi=([0-9a-f]+) edi=([0-9a-f]+) ebp=([0-9a-f]+)$',text,re.M)
    leave=re.findall(r'^compiler-probe-return eax=([0-9a-f]+) esp=([0-9a-f]+) ecx=([0-9a-f]+) ebx=([0-9a-f]+) esi=([0-9a-f]+) edi=([0-9a-f]+) ebp=([0-9a-f]+)$',text,re.M)
    assert len(entry)==len(leave)==1
    before=[int(value,16)for value in entry[0]];after=[int(value,16)for value in leave[0]]
    assert before[0]==0x42c4 and before[2]==0x71c28576
    assert after==[before[2],before[1]+4-before[0]]+before[3:]
    assert len(raw_bytes(text,'compiler-probe-lowest-stack'))==4
    import importlib.util
    spec=importlib.util.spec_from_file_location('compiler_support',HERE/'scripts/check-compiler-support.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    provider=module.verify()
    support=config['support_compiler'];assert len(support)==1
    assert support[0]['name']=='__chkstk'and support[0]['trial']and not support[0]['accepted']and support[0]['entry_eax']=='0x42c4'
    proof.update(trial='rc_prepare_input',write=dict(bytes=46,payload=expected.hex(),filename=arguments['filename']),compiler_support=dict(provider=provider,original_entry=before,original_return=after,native_frame=frame,application_source_credit=0),file=dict(position=0,temporary=temporary))
    return proof


def write_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_write_bytes'and len(config['accepted'])==13
    a,b,proof=checkpoint(original,native,'rc_detect_encoding',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c286df and a['registers']['eax']==46
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_prepare_input\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();arguments=b['arguments']
    expected='#line 1"resources.rc"\r\n'.encode('utf-16le')
    assert raw_bytes(text,'detect-output-buffer')==bytes.fromhex(arguments['output_buffer'])==expected
    assert re.findall(r'^write-result (\d+)$',text,re.M)==['46']
    assert arguments['write_returns']==json.loads((native/'writes.json').read_text())==[46]
    old_input=struct.unpack('<8I',raw_bytes(text,'detect-input-state'));new_input=struct.unpack('<8I',bytes.fromhex(arguments['input_state']))
    assert old_input[:4]+old_input[5:]==new_input[:4]+new_input[5:]==(0,0,0,1,0,0,0)
    assert old_input[4]>=3 and new_input[4]>=3
    old_output=struct.unpack('<8I',raw_bytes(text,'detect-output-state'));new_output=struct.unpack('<8I',bytes.fromhex(arguments['output_state']))
    for state in [old_output,new_output]:
        assert state[0]-state[2]==46 and state[0]>state[2]>0
        assert (state[1],state[3],state[5],state[6],state[7])==(4050,0x8a,0,4096,0)and state[4]>=3
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3
    assert identity['options']==1 and identity['initial']==identity['maximum']==0
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    assert proof['application_trace'][-1]=='rc_write_bytes'
    proof.update(trial='rc_write_bytes',write=dict(bytes=46,result=46,payload=expected.hex(),buffered=True,buffer_bytes=4096,remaining=4050),files=dict(input_flag=1,output_flag=0x8a,descriptors='semantic live handles, not raw descriptor identity'))
    return proof


def detect_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_detect_encoding'and len(config['accepted'])==14
    a,b,proof=checkpoint(original,native,'rc_is_text_unicode',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c308e7 and a['words'][2]==250
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_detect_encoding\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();arguments=b['arguments'];source=(original/'source/resources.rc').read_bytes()
    assert arguments['bytes']==250 and arguments['flags']==0xffff and arguments['position']==0
    assert raw_bytes(text,'text-flags')==b'\xff\xff\0\0'
    assert raw_bytes(text,'text-sample')==bytes.fromhex(arguments['sample'])==source[:250]+b'\0\0'
    assert re.findall(r'^text-original-position (\d+)$',text,re.M)==['0']
    assert raw_bytes(text,'text-file-buffer')==bytes.fromhex(arguments['file_buffer'])==source
    old=struct.unpack('<8I',raw_bytes(text,'text-file-state'));new=struct.unpack('<8I',bytes.fromhex(arguments['file_state']))
    for state in [old,new]:
        assert state[0]-state[2]==250 and state[0]>state[2]>0
        assert (state[1],state[3],state[5],state[6],state[7])==(len(source)-250,9,0,4096,0)and state[4]>=3
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    assert proof['application_trace'][-1]=='rc_detect_encoding'
    proof.update(trial='rc_detect_encoding',sample=dict(bytes=250,terminator_bytes=2,flags=0xffff,original_position=0,contents=arguments['sample']),file=dict(buffered_bytes=len(source),consumed=250,remaining=len(source)-250,buffer_bytes=4096))
    return proof


def unicode_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_is_text_unicode'and len(config['accepted'])==15
    a,b,proof=checkpoint(original,native,'rc_read_wide_line',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c28702 and a['words'][2:4]==[0x1051,0]
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_prepare_input\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();arguments=b['arguments']
    assert arguments['capacity']==0x1051 and arguments['wide']==0
    assert re.findall(r'^line-input-args capacity=(\d+) wide=(\d+)$',text,re.M)==[('4177','0')]
    assert re.findall(r'^text-test-result (\d+) flags=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert arguments['unicode_results']==json.loads((native/'unicode-results.json').read_text())==[dict(result=0,flags=0)]
    assert re.findall(r'^cpinfo-call page=(\d+) last-error=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert re.findall(r'^cpinfo-result (\d+) last-error=([0-9a-f]+)$',text,re.M)==[('1','00000000')]
    before=raw_bytes(text,'cpinfo-before');after=raw_bytes(text,'cpinfo-after');assert len(before)==len(after)==20
    expected=b'\x01\0\0\0?\0'+b'\0'*12
    assert after[:18]==expected and after[18:]==before[18:]
    assert arguments['cpinfo']==json.loads((native/'cpinfo.json').read_text())and len(arguments['cpinfo'])==1
    call=arguments['cpinfo'][0]
    assert call['page']==0 and call['result']==1 and call['last_error_before']==call['last_error_after']==0
    new_before=bytes.fromhex(call['before']);new_after=bytes.fromhex(call['after']);assert len(new_before)==len(new_after)==20
    assert new_after[:18]==expected and new_after[18:]==new_before[18:]
    old=struct.unpack('<8I',raw_bytes(text,'line-input-state'));new=struct.unpack('<8I',bytes.fromhex(arguments['file_state']))
    for state in [old,new]:
        assert state[0]==state[2]>0
        assert (state[1],state[3],state[5],state[6],state[7])==(0,9,0,512,0)and state[4]>=3
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    assert proof['application_trace'][-1]=='rc_is_text_unicode'
    proof.update(trial='rc_is_text_unicode',classification=dict(result=0,flags=0,encoding=1,next_wide=0),codepage=dict(page_argument=0,max_char_size=1,defined_fields=expected.hex(),padding='each caller retains its own2bytes',errors='preserved'),input_file=dict(remaining=0,buffer_bytes=512,profile='MSVCRT510 fseek resets buffer capacity'))
    return proof


def line_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_read_wide_line'and len(config['accepted'])==16
    a,b,proof=checkpoint(original,native,'rc_prefix_equal',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c28765 and a['words'][1]==0x71c2156c and a['words'][2]==a['registers']['ebp']
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_prepare_input\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();arguments=b['arguments'];source=(original/'source/resources.rc').read_bytes()
    first_line=source.split(b'\n',1)[0];assert b'\r'not in first_line and b'\t'not in first_line
    expected=(first_line.decode('ascii')+'\0').encode('utf-16le')
    assert raw_bytes(text,'line-prefix')==bytes.fromhex(arguments['prefix'])=='rcinclude\0'.encode('utf-16le')
    assert raw_bytes(text,'line-text')==bytes.fromhex(arguments['text'])==expected
    assert arguments['current_line']==0 and re.findall(r'^line-current-number (\d+)$',text,re.M)==['0']
    assert (original/'globals.bin').read_bytes()[0x16ce8-0x14000:0x16ce8-0x14000+4]==b'\0'*4
    assert re.findall(r'^line-local-allocate (\d+) (\d+)$',text,re.M)==[('64','4177')]
    assert raw_bytes(text,'line-local-initial')==b'\0'*4177
    final=first_line+b'\0'*(4177-len(first_line))
    assert raw_bytes(text,'line-local-final')==final
    pointers=re.findall(r'^line-local-pointer ([0-9a-f]+)$',text,re.M);assert len(pointers)==1 and int(pointers[0],16)>0
    assert re.findall(r'^line-local-free-pointer ([0-9a-f]+)$',text,re.M)==pointers
    assert re.findall(r'^line-local-free-result (\d+)$',text,re.M)==['0']
    assert arguments['locals']==json.loads((native/'locals.json').read_text())and len(arguments['locals'])==2
    allocated,freed=arguments['locals']
    assert allocated['operation']=='allocate'and allocated['flags']==64 and allocated['bytes']==4177 and allocated['result']>0
    assert bytes.fromhex(allocated['contents'])==b'\0'*4177
    assert allocated['live']==[dict(pointer=allocated['result'],bytes=4177,flags=64)]
    assert freed['operation']=='free'and freed['pointer']==allocated['result']and freed['result']==0 and freed['live']==[]
    assert bytes.fromhex(freed['contents'])==final
    assert re.findall(r'^line-ctype (\d+) (\d+)$',text,re.M)==[('35','8')]
    assert re.findall(r'^line-ctype-result (\d+)$',text,re.M)==['0']
    assert arguments['ctype']==json.loads((native/'ctype.json').read_text())==[dict(character=35,mask=8,result=0)]
    old=struct.unpack('<8I',raw_bytes(text,'line-read-file-state'));new=struct.unpack('<8I',bytes.fromhex(arguments['file_state']))
    consumed=len(first_line)+1
    for state in [old,new]:
        assert state[0]-state[2]==consumed and state[0]>state[2]>0
        assert (state[1],state[3],state[5],state[6],state[7])==(512-consumed,9,0,4096,0)and state[4]>=3
    assert raw_bytes(text,'line-read-file-buffer')==bytes.fromhex(arguments['file_buffer'])==source[:512]
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    assert proof['application_trace'][-1]=='rc_read_wide_line'
    proof.update(trial='rc_read_wide_line',first_line=dict(utf16=expected.hex(),input_bytes=consumed,line_number=0),local_allocation=dict(flags=64,bytes=4177,original_pointer=int(pointers[0],16),native_pointer=allocated['result'],freed=True),file=dict(remaining=512-consumed,meaningful_buffer_bytes=512,next_buffer_capacity=4096),ctype=dict(character=35,mask=8,result=0))
    return proof


def prefix_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_prefix_equal'and len(config['accepted'])==17
    a,b,proof=checkpoint(original,native,'rc_notify_progress',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c28724 and a['words'][1:3]==[1,64]
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_prepare_input\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();arguments=b['arguments'];source=(original/'source/resources.rc').read_bytes()
    assert arguments['kind']==1 and arguments['value']==64 and arguments['current_line']==63
    assert re.findall(r'^progress-current-line (\d+)$',text,re.M)==['63']
    assert (original/'globals.bin').read_bytes()[0x16ce8-0x14000:0x16ce8-0x14000+4]==struct.pack('<I',63)
    expected_line=(source.splitlines()[63].decode('ascii')+'\0').encode('utf-16le')
    assert raw_bytes(text,'progress-line')==bytes.fromhex(arguments['line'])==expected_line
    assert re.findall(r'^prefix-first-result (-?\d+)$',text,re.M)==['0']
    assert arguments['prefix_results']==json.loads((native/'prefix-results.json').read_text())==[0]
    calls=re.findall(r'^prefix-towupper (\d+) raw=([0-9a-f]+)$',text,re.M)
    returns=re.findall(r'^prefix-towupper-result (\d+) raw=([0-9a-f]+)$',text,re.M)
    assert len(calls)==len(returns)==2
    assert [(int(value),int(result))for(value,raw),(result,register)in zip(calls,returns)]==[(114,82),(35,35)]
    assert all(int(value)==(int(raw,16)&65535)for value,raw in calls+returns)
    assert arguments['towupper']==json.loads((native/'towupper.json').read_text())==[dict(character=114,result=82),dict(character=35,result=35)]
    template='#line %d\r\n\0'.encode('utf-16le');formatted='#line 1\r\n\0'.encode('utf-16le')
    initial='#line 1"resources.rc"\r\n\0'.encode('utf-16le')
    assert raw_bytes(text,'line-format-template')==template
    assert re.findall(r'^line-format-number (-?\d+)$',text,re.M)==['1']and re.findall(r'^line-format-result (-?\d+)$',text,re.M)==['9']
    before=raw_bytes(text,'line-format-before');after=raw_bytes(text,'line-format-after')
    assert len(before)==len(after)==8196 and before.startswith(initial)and after==formatted+before[len(formatted):]
    formats=arguments['line_format'];assert formats==json.loads((native/'line-format.json').read_text())and len(formats)==1
    call=formats[0];assert call['number']==1 and call['result']==9 and bytes.fromhex(call['format'])==template
    new_before=bytes.fromhex(call['before']);new_after=bytes.fromhex(call['after'])
    assert len(new_before)==len(new_after)==8196 and new_before.startswith(initial)and new_after==formatted+new_before[len(formatted):]
    for label,reading in [('input',True),('output',False)]:
        old=struct.unpack('<8I',raw_bytes(text,'progress-'+label+'-state'));new=struct.unpack('<8I',bytes.fromhex(arguments[label+'_state']))
        original_buffer=raw_bytes(text,'progress-'+label+'-buffer');native_buffer=bytes.fromhex(arguments[label+'_buffer']);assert original_buffer==native_buffer
        if reading:
            assert original_buffer==source[512:]and len(original_buffer)==2928
            consumed=sum(len(line)for line in source.splitlines(keepends=True)[:64])-512
            for state in [old,new]:assert state[0]-state[2]==consumed and (state[1],state[3],state[5],state[6],state[7])==(len(original_buffer)-consumed,9,0,4096,0)
        else:
            assert len(original_buffer)==3776
            for state in [old,new]:assert state[0]-state[2]==len(original_buffer)and (state[1],state[3],state[5],state[6],state[7])==(4096-len(original_buffer),0x8a,0,4096,0)
        assert old[4]>=3 and new[4]>=3
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    assert proof['application_trace'][-1]=='rc_prefix_equal'
    proof.update(trial='rc_prefix_equal',comparison=dict(result=0,towupper=[(114,82),(35,35)],abi='16-bit CRT argument/result; raw upper EAX bits not value'),line_format=dict(number=1,result=9,bytes=formatted.hex(),tail='each caller retains its own unwritten stack bytes'),progress=dict(kind=1,value=64,current_line=63,output_buffer_bytes=3776,input_buffer_bytes=2928))
    return proof



def prepared_case_state(original,native,b,argv,extra_blocks=0,filename=None):
    text=(original/'events.txt').read_text();arguments=b['arguments'];source=(original/'source/resources.rc').read_bytes()
    assert arguments['count']==20 and arguments['argv']==argv
    assert arguments['prepared_lines']==len(source.splitlines())==126 and arguments['current_line']==0
    raw=(original/'globals.bin').read_bytes()
    assert struct.unpack_from('<I',raw,0x29458-0x14000)[0]==126 and struct.unpack_from('<I',raw,0x16ce8-0x14000)[0]==0
    strict_files=filename is not None
    filename=filename or 'resources.rc';filename_bytes=filename.encode('utf-16le');assert len(filename_bytes)<256
    assert raw_bytes(text,'prepared-filename')==bytes.fromhex(arguments['filename'])==filename_bytes+b'\0'*(514-len(filename_bytes))
    old=struct.unpack('<8I',raw_bytes(text,'prepared-closed-input'));new=struct.unpack('<8I',bytes.fromhex(arguments['closed_input']))
    assert old[:4]+old[5:]==new[:4]+new[5:]==(0,0,0,0,0,4096,0)and old[4]>=3 and new[4]>=3
    if strict_files:assert old==new and old[4]==4,'closed input descriptor differs after host-only I/O separation'
    assert b['globals']['rc_window']=='00000000','this case cannot invoke an unproved window callback'
    input_temporary=raw_bytes(text,'input-temporary')[:-1].decode('ascii');output_temporary=raw_bytes(text,'output-temporary')[:-1].decode('ascii')
    assert argv[:4]==['RCPP','-CP','1252','-f']and argv[4]==input_temporary and argv[5:7]==['-g',output_temporary]
    receipt=json.loads((original/'receipt.json').read_text());temporary=input_temporary.rsplit('\\',1)[-1]
    artifact=receipt['case_files'][temporary]
    assert artifact==dict(bytes=6762,sha256='3e1e06b1f0652c2ad397213fcd2a2d91f940a865d1dda5633330b2c096e6f8ed')
    assert (original/'source'/temporary).read_bytes().startswith('#line 1"resources.rc"\r\n'.encode('utf-16le'))
    assert output_temporary.rsplit('\\',1)[-1]not in receipt['case_files']
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3+extra_blocks
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    return dict(lines=126,current_line=0,artifact=artifact,closed_input=True)


def notify_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_notify_progress'and len(config['accepted'])==18
    a,b,proof=checkpoint(original,native,'rc_preprocess',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c28359 and a['words'][1:4]==[20,0x71c45590,0]
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rcdll_command_main\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();assert b['arguments']['unused']==0
    assert re.findall(r'^preprocessor-args (\d+) ([0-9a-f]+) (\d+)$',text,re.M)==[('20','71c45590','0')]
    assert re.findall(r'^preprocessor-count (\d+)$',text,re.M)==['20']
    assert re.findall(r'^prepared-lines (\d+) current=(\d+)$',text,re.M)==[('126','0')]
    argv=[raw_bytes(text,'preprocessor-argv-%d'%i)[:-1].decode('ascii')for i in range(20)]
    prepared=prepared_case_state(original,native,b,argv)
    assert proof['application_trace'][-1]=='rc_notify_progress'
    proof.update(trial='rc_notify_progress',notification=dict(window=None,interval=64,no_callback=True),prepared=prepared,preprocessor=dict(count=20,argv=argv,unused=0))
    return proof


def preprocess_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_preprocess'and len(config['accepted'])==19
    a,b,proof=checkpoint(original,native,'rc_widen_arguments',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][:7]==[0x71c28b7f,20,0x71c45590,0x71c28359,20,0x71c45590,0]
    gdb=(native/'gdb.stdout').read_text()
    frames=[]
    for number,name in [(0,'rc_widen_arguments'),(1,'rc_preprocess')]:
        matches=re.findall(r'^#'+str(number)+r'\s+0x[0-9a-f]+ in '+name+r' \(count=20, arguments=(0x[0-9a-f]+)(?: <[^>]+>)?'+(', unused=0'if number==1 else'')+r'\)',gdb,re.M)
        assert len(matches)==1;frames.append(matches[0])
    assert frames[0]==frames[1],'argument-vector pointer was not forwarded'
    text=(original/'events.txt').read_text()
    assert re.findall(r'^widen-args (\d+) ([0-9a-f]+)$',text,re.M)==[('20','71c45590')]
    argv=[raw_bytes(text,'widen-argv-%d'%i)[:-1].decode('ascii')for i in range(20)]
    prepared=prepared_case_state(original,native,b,argv)
    assert proof['application_trace'][-1]=='rc_preprocess'
    proof.update(trial='rc_preprocess',prepared=prepared,forwarded=dict(count=20,argv=argv,original_pointer=0x71c45590,native_pointer=frames[0],unused_outer_slot=0,callee='rc_widen_arguments'))
    return proof


def wide_argument_state(original,native,a,b,filename=None):
    text=(original/'events.txt').read_text();arguments=b['arguments']
    # The previously accepted forwarding boundary records this exact case's
    # ANSI vector. Only its two actual temporary paths vary between captures.
    expected=json.loads((HERE/'cases/resources-rich/acceptance-0020.json').read_text())['proof']['forwarded']['argv'][:]
    expected[4]=raw_bytes(text,'input-temporary')[:-1].decode('ascii');expected[6]=raw_bytes(text,'output-temporary')[:-1].decode('ascii')
    assert len(expected)==20 and all(value.isascii()for value in expected)
    prepared=prepared_case_state(original,native,b,expected,extra_blocks=1,filename=filename)
    original_pointer=int(re.findall(r'^wide-args 20 ([0-9a-f]+)$',text,re.M)[0],16);native_pointer=arguments['vector_pointer'];assert native_pointer>0
    vector=arguments['wide_vector'];assert len(vector)==20 and arguments['wide_terminator']==0
    assert re.findall(r'^wide-terminator ([0-9a-f]+)$',text,re.M)==['00000000']
    old_pointers=[int(value,16)for value in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)];assert len(old_pointers)==20
    wanted=[(value+'\0').encode('utf-16le')for value in expected]
    size=8+4*21+sum(len(value)for value in wanted)
    original_block=raw_bytes(text,'wide-argument-block');assert len(original_block)==size and original_block[:8]==b'\0'*8
    identity=b['heap_identity'];block=next(row for row in identity['blocks']if row['payload']+8==native_pointer)
    assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
    native_block=bytes.fromhex(block['contents']);assert len(native_block)==size and native_block[:8]==b'\0'*8
    cursor=4*21
    for index,value in enumerate(wanted):
        assert old_pointers[index]==original_pointer+cursor
        assert vector[index]['pointer']==native_pointer+cursor
        assert struct.unpack_from('<I',original_block,8+4*index)[0]==old_pointers[index]
        assert struct.unpack_from('<I',native_block,8+4*index)[0]==vector[index]['pointer']
        assert raw_bytes(text,'wide-argv-%d'%index)==bytes.fromhex(vector[index]['bytes'])==value
        assert original_block[8+cursor:8+cursor+len(value)]==native_block[8+cursor:8+cursor+len(value)]==value
        cursor+=len(value)
    assert 8+cursor==size
    assert original_block[8+4*20:8+4*21]==native_block[8+4*20:8+4*21]==b'\0'*4
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(requests)==len(results)==len(payloads)==5
    assert int(requests[-1][2])==size and requests[-1][1]=='9'and requests[-1][3]=='00000000'
    assert int(results[-1][0],16)+8==original_pointer and results[-1][1]=='00000000'
    assert int(payloads[-1][0])==size and bytes.fromhex(payloads[-1][1])==b'\0'*size
    raw=(original/'globals.bin').read_bytes();assert int(requests[-1][0],16)==struct.unpack_from('<I',raw,0x25f54-0x14000)[0]
    allocations=[row for row in b['heap']if row['operation']=='allocate'];assert len(allocations)==5
    last=allocations[-1];assert last['heap']==identity['handle']and last['flags']==9 and last['bytes']==size and last['result']+8==native_pointer
    assert bytes.fromhex(last['payload'])==b'\0'*size and last['last_error_before']==last['last_error_after']==0
    return dict(prepared=prepared,widened=dict(count=20,argv=expected,allocation_bytes=size,header_bytes=8,pointer_array_bytes=84,original_base=original_pointer,native_base=native_pointer,terminator=0,normalization='only pointers into the one owned packed allocation; UTF16 bytes exact'),ansi_reference=dict(path='cases/resources-rich/acceptance-0020.json',sha256=sha(HERE/'cases/resources-rich/acceptance-0020.json'),transport_slots=[4,6]))


def widen_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_widen_arguments'and len(config['accepted'])==20
    a,b,proof=checkpoint(original,native,'rc_preprocessor_main',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c28b89 and a['words'][1]==20 and a['words'][2]!=0
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_preprocess\s*\(',(native/'gdb.stdout').read_text(),re.M)
    proof.update(wide_argument_state(original,native,a,b))
    assert proof['application_trace'][-1]=='rc_widen_arguments'
    proof['trial']='rc_widen_arguments'
    return proof


def preprocessor_state(original,native,b,state,remaining,advance):
    specification=json.loads((native/'preprocess-data.json').read_text())
    assert sha(native/'preprocess-data.json')==sha(HERE/'preprocess-data.json')
    observed=b['arguments']['preprocessor']
    assert observed['schema_sha256']==sha(native/'preprocess-data.json')
    raw=(original/'globals.bin').read_bytes();text=(original/'relocated-text.bin').read_bytes()
    def read(rva,size):
        data=raw if rva>=0x14000 else text
        offset=rva-(0x14000 if rva>=0x14000 else 0x1000)
        assert 0<=offset<=len(data)-size
        return data[offset:offset+size]
    def packed(pointer):
        offset=pointer-state['widened']['original_base']
        assert 0<=offset<state['widened']['allocation_bytes']-8
        return state['widened']['native_base']+offset
    def wide(pointer):
        value=bytearray()
        block=raw_bytes((original/'events.txt').read_text(),'wide-argument-block')
        for index in range(4177):
            at=pointer+index*2
            offset=at-state['widened']['original_base']
            word=block[8+offset:10+offset]if 0<=offset<len(block)-9 else read(at-0x71c20000,2)
            assert len(word)==2;value.extend(word)
            if word==b'\0\0':return bytes(value)
        raise AssertionError('unterminated descriptor string')
    addresses={0:0}
    for row in specification['globals']:
        rva=int(row['original_rva'],16);actual=observed['globals'][row['name']]
        addresses[0x71c20000+rva]=actual['address']
        old=read(rva,row['bytes']);new=bytes.fromhex(actual['bytes'])
        assert len(new)==row['bytes']
        if row['kind']=='wide-vector':
            assert int.from_bytes(old,'little')==state['widened']['original_base']+4*advance
            assert int.from_bytes(new,'little')==state['widened']['native_base']+4*advance
        elif row['kind']=='option-list':
            assert old[:4]==new[:4]
            for offset in range(4,row['bytes'],4):
                left=int.from_bytes(old[offset:offset+4],'little');right=int.from_bytes(new[offset:offset+4],'little')
                assert right==(packed(left)if left else 0)
        elif row['kind']=='packed-argument':
            left=int.from_bytes(old,'little');right=int.from_bytes(new,'little')
            assert bool(left)==bool(right)
            if left:
                offset=left-state['widened']['original_base']
                assert 0<=offset<state['widened']['allocation_bytes']-8
                assert right==state['widened']['native_base']+offset
        elif row['kind']=='wide-string':
            assert bytes.fromhex(actual['text'])==wide(int.from_bytes(old,'little'))
        else:assert old==new,'preprocessor global differs: '+row['name']
    assert observed['globals']['rc_cpp_remaining']['bytes']==remaining.to_bytes(4,'little').hex()
    for key in ['options','letters']:
        table=specification[key];rva=int(table['original_rva'],16)
        addresses[0x71c20000+rva]=observed['tables'][key]['address']
    for key in ['options','letters']:
        table=specification[key];rows=observed['tables'][key]['rows'];assert len(rows)==table['rows']
        for index,value in enumerate(rows):
            data=read(int(table['original_rva'],16)+index*12,12)
            if key=='options':
                pointer,destination,keep,action,reserved=struct.unpack('<IIbBH',data)
                assert value['pattern']==(wide(pointer).hex()if pointer else None)
                assert (value['keep'],value['action'],value['reserved'])==(keep,action,reserved)
            else:
                letter,action,destination=struct.unpack('<iiI',data)
                assert (value['letter'],value['action'])==(letter,action)
            assert destination in addresses and value['destination']==addresses[destination]
    return dict(remaining=remaining,advanced_slots=advance,vector='same owned packed wide argument vector',globals=len(specification['globals']),option_rows=31,letter_rows=9,record_bytes=12,source_schema_sha256=sha(HERE/'preprocess-data.json'))

def preprocessor_main_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_preprocessor_main'and len(config['accepted'])==21
    a,b,proof=checkpoint(original,native,'rc_next_argument',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c2bce0
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_preprocessor_main\s*\(',(native/'gdb.stdout').read_text(),re.M)
    state=wide_argument_state(original,native,a,b);proof.update(state)
    proof.update(trial='rc_preprocessor_main',preprocessor=preprocessor_state(original,native,b,state,20,0))
    assert proof['application_trace'][-1]=='rc_preprocessor_main'
    return proof


def next_argument_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_next_argument'and len(config['accepted'])==22
    a,b,proof=checkpoint(original,native,'rc_scan_options',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c2bcec
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_preprocessor_main\s*\(',(native/'gdb.stdout').read_text(),re.M)
    state=wide_argument_state(original,native,a,b);proof.update(state)
    parser=preprocessor_state(original,native,b,state,19,1)
    scan=b['arguments']['scan'];original_text=(original/'events.txt').read_text()
    wanted=('-CP'+'\0').encode('utf-16le')
    assert raw_bytes(original_text,'option-argument')==bytes.fromhex(scan['argument_bytes'])==wanted
    assert a['words'][1]==0x71c34e90 and scan['options']==b['arguments']['preprocessor']['tables']['options']['address']
    original_pointer=int(re.findall(r'^wide-pointer 1 ([0-9a-f]+)$',original_text,re.M)[0],16)
    assert a['words'][2]==original_pointer and scan['argument']==b['arguments']['wide_vector'][1]['pointer']
    assert a['words'][3]==0x71c2bc61 and scan['next']==scan['getter']>0
    assert a['words'][4]==scan['duplicate']==0
    assert proof['application_trace'][-1]=='rc_next_argument'
    proof.update(trial='rc_next_argument',preprocessor=parser,scanner=dict(argument='-CP',duplicate=0,getter='rc_next_argument',getter_abi='cdecl0 returning WCHAR*',table='rc_cpp_options'))
    return proof


def scan_options_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_scan_options'and len(config['accepted'])==23
    a,b,proof=checkpoint(original,native,'rc_apply_option',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c229e0
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_scan_options\s*\(',(native/'gdb.stdout').read_text(),re.M)
    state=wide_argument_state(original,native,a,b);proof.update(state)
    parser=preprocessor_state(original,native,b,state,18,2)
    apply=b['arguments']['apply'];text=(original/'events.txt').read_text()
    wide_vector=b['arguments']['wide_vector'];wanted=('1252'+'\0').encode('utf-16le')
    assert raw_bytes(text,'option-operand')==bytes.fromhex(apply['operand_bytes'])==wanted
    table=b['arguments']['preprocessor']['tables']['options']
    assert a['words'][1]==0x71c34e90+3*12 and apply['option']==table['address']+3*12
    assert a['words'][2]==int(re.findall(r'^wide-pointer 2 ([0-9a-f]+)$',text,re.M)[0],16)
    assert apply['operand']==wide_vector[2]['pointer']
    assert a['words'][3]==apply['duplicate']==0
    row=table['rows'][3];assert (row['action'],row['keep'],row['pattern'])==(0x24,1,('-CP#'+'\0').encode('utf-16le').hex())
    assert raw_bytes(text,'option-descriptor')==bytes.fromhex((original/'globals.bin').read_bytes()[0x14e90-0x14000+36:0x14e90-0x14000+48].hex())
    pointer=b['arguments']['preprocessor']['globals']['rc_cpp_option_argument']['bytes']
    assert int.from_bytes(bytes.fromhex(pointer),'little')==wide_vector[1]['pointer']
    assert proof['application_trace'][-1]=='rc_scan_options'
    proof.update(trial='rc_scan_options',preprocessor=parser,option=dict(index=3,pattern='-CP#',action=0x24,keep_scanning=1,operand='1252',duplicate=0,callback_consumed_slots=1))
    return proof


def apply_option_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_apply_option'and len(config['accepted'])==24
    a,b,proof=checkpoint(original,native,'rc_option_number',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c22aca
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_apply_option\s*\(',(native/'gdb.stdout').read_text(),re.M)
    state=wide_argument_state(original,native,a,b);proof.update(state)
    parser=preprocessor_state(original,native,b,state,18,2)
    number=b['arguments']['number'];text=(original/'events.txt').read_text();parent=number['parent']
    old_pointer=int(re.findall(r'^wide-pointer 2 ([0-9a-f]+)$',text,re.M)[0],16)
    wanted=('1252'+'\0').encode('utf-16le')
    assert a['words'][1]==old_pointer and number['operand']==parent['operand']==b['arguments']['wide_vector'][2]['pointer']
    assert raw_bytes(text,'number-operand')==bytes.fromhex(number['operand_bytes'])==wanted
    assert re.findall(r'^number-parent ([0-9a-f]+) ([0-9a-f]+) (\d+)$',text,re.M)==[('71c34eb4','%08x'%old_pointer,'0')]
    assert parent['option']==b['arguments']['preprocessor']['tables']['options']['address']+3*12 and parent['duplicate']==0
    assert proof['application_trace'][-1]=='rc_apply_option'
    proof.update(trial='rc_apply_option',preprocessor=parser,numeric=dict(argument='1252',abi='stdcall1/ret4',descriptor_index=3,action=0x24,duplicate=0,pointer_alias='packed argv[2]'))
    return proof




def entry_file_state(original,native):
    text=(original/'events.txt').read_text();history=json.loads((native/'errno-history.json').read_text());result={}
    for phase,label in [('dll-entry','_DllMainCRTStartup'),('RC-entry','RC')]:
        old_errno=re.findall(r'^errno-boundary '+re.escape(phase)+r' (-?\d+)$',text,re.M)
        records=[row for row in history if row['phase']==label];assert len(records)==1
        new=records[0];assert old_errno==['0']and new['value']==0
        old_lowio=[dict(slot=int(slot),flags=int(flags,16))for slot,flags,handle in re.findall(r'^lowio-open '+re.escape(phase)+r' (\d+) flags=([0-9a-f]+) handle=([0-9a-f]+)$',text,re.M)]
        assert old_lowio==new['lowio']==[dict(slot=0,flags=0xc1),dict(slot=1,flags=0x81),dict(slot=2,flags=0x81)]
        data=raw_bytes(text,'iob-'+phase);assert len(data)==640
        old_files=[]
        for index in range(20):
            fields=struct.unpack_from('<8I',data,index*32)
            if fields[3]:old_files.append(dict(slot=index,flags=fields[3],descriptor=fields[4]))
        assert old_files==new['iob']==[dict(slot=0,flags=0x101,descriptor=0),dict(slot=1,flags=2,descriptor=1),dict(slot=2,flags=2,descriptor=2)]
        capacities=re.findall(r'^lowio-capacity '+re.escape(phase)+r' (\d+)$',text,re.M);assert len(capacities)==1
        result[phase]=dict(errno=0,lowio=old_lowio,files=old_files,original_capacity=int(capacities[0]),native_capacity=new['lowio_capacity'],capacity_scope='unused allocation capacity is recorded separately; only live descriptor/FILE inventory equivalence is claimed')
    return dict(boundaries=result,original_events_sha256=sha(original/'events.txt'),native_history_sha256=sha(native/'errno-history.json'))

def parsed_option_state(original,native,b,original_arguments):
    text=(original/'events.txt').read_text();input_name=raw_bytes(text,'input-temporary')[:-1].decode('ascii')
    state=wide_argument_state(original,native,None,b,filename=input_name)
    parser=preprocessor_state(original,native,b,state,0,19)
    opened=b['arguments']['open'];globals=b['arguments']['preprocessor']['globals'];wide=b['arguments']['wide_vector']
    old_pointers=[int(value,16)for value in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)]
    assert original_arguments==[old_pointers[4],old_pointers[6],0x71c344f8,0x71c34690]
    assert opened==dict(input=wide[4]['pointer'],output=wide[6]['pointer'],defines=globals['rc_cpp_defines']['address'],undefines=globals['rc_cpp_undefines']['address'])
    for field,expected in [('rc_cpp_input',wide[4]['pointer']),('rc_cpp_output',wide[6]['pointer']),('rc_cpp_path_characters',wide[9]['pointer']+6),('rc_cpp_option_argument',wide[18]['pointer'])]:
        assert int.from_bytes(bytes.fromhex(globals[field]['bytes']),'little')==expected
    assert globals['rc_cpp_active']['bytes']==globals['rc_cpp_E']['bytes']=='01000000'and globals['rc_cpp_P']['bytes']=='00000000'
    for field,slots in [('rc_cpp_defines',[(99,7,4),(98,8,4),(97,19,0)]),('rc_cpp_includes',[(99,11,4),(98,13,0),(97,15,0),(96,17,0)]),('rc_cpp_undefines',[])]:
        raw=bytes.fromhex(globals[field]['bytes']);count=struct.unpack_from('<I',raw)[0];assert count==100-len(slots)
        wanted=[0]*100
        for slot,index,offset in slots:wanted[slot]=wide[index]['pointer']+offset
        assert list(struct.unpack('<100I',raw[4:]))==wanted
    return state,parser,text,input_name


def option_number_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_option_number'and len(config['accepted'])==25
    a,b,proof=checkpoint(original,native,'rc_open_preprocessor',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c2bdf5
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_preprocessor_main\s*\(',(native/'gdb.stdout').read_text(),re.M)
    state,parser,text,input_name=parsed_option_state(original,native,b,a['words'][1:5]);proof.update(state)
    wide=b['arguments']['wide_vector']
    assert re.findall(r'^number-conversion end=([0-9a-f]+) base=(\d+) last-error=([0-9a-f]+) errno=(-?\d+)$',text,re.M)==[('00000000','10','00000000','0')]
    assert raw_bytes(text,'number-conversion-input')==('1252'+'\0').encode('utf-16le')
    assert re.findall(r'^number-conversion-result (-?\d+) last-error=([0-9a-f]+) errno=(-?\d+)$',text,re.M)==[('1252','00000000','0')]
    calls=b['arguments']['conversion'];assert calls==json.loads((native/'numeric-conversion.json').read_text())and len(calls)==1
    call=calls[0];assert call['caller']=='rc_option_number'and call['text']==wide[2]['pointer']and call['end']==0 and call['base']==10
    assert call['result']==1252 and call['errno_before']==call['errno_after']==call['last_error_before']==call['last_error_after']==0
    assert call['before']==call['after']==('1252'+'\0').encode('utf-16le').hex()
    assert proof['application_trace'][-1]=='rc_option_number'
    proof.update(trial='rc_option_number',entry_files=entry_file_state(original,native),preprocessor=parser,numeric=dict(text='1252',result=1252,errno=[0,0],last_error=[0,0],end=None,base=10),parsed_options=dict(remaining=0,defines=3,undefines=0,includes=4,input=input_name,output=raw_bytes(text,'output-temporary')[:-1].decode('ascii'),active=1,E=1,P=0))
    return proof


def open_preprocessor_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_open_preprocessor'and len(config['accepted'])==26
    a,b,proof=checkpoint(original,native,'rc_set_character_token',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c2450b and a['words'][1:3]==[1,0x24]
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_open_preprocessor\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();forwarded=list(struct.unpack('<4I',raw_bytes(text,'setup-forwarded')))
    state,parser,text,input_name=parsed_option_state(original,native,b,forwarded);proof.update(state)
    assert b['arguments']['character']==dict(character=1,token=0x24,formal_bytes=dict(character=2,token=2))
    assert re.findall(r'^frontier-errors errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert b['crt_errors']==dict(errno=0,last_error=0)
    assert proof['application_trace'][-1]=='rc_open_preprocessor'
    proof.update(trial='rc_open_preprocessor',preprocessor=parser,character_assignment=dict(character=1,token=0x24,argument_bytes=2,callee_cleanup_bytes=8),numeric_prefix_reference=dict(path='cases/resources-rich/acceptance-0026.json',sha256=sha(HERE/'cases/resources-rich/acceptance-0026.json'),scope='previous typed numeric/import and host-entry proofs; current parsed state,full argument allocation,trace and frontier errors are freshly checked'))
    return proof


def character_token_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_set_character_token'and len(config['accepted'])==27
    a,b,proof=checkpoint(original,native,'rc_set_character_class',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c2454a and a['words'][1:3]==[36,16]
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_open_preprocessor\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();forwarded=list(struct.unpack('<4I',raw_bytes(text,'setup-forwarded')))
    state,parser,text,input_name=parsed_option_state(original,native,b,forwarded);proof.update(state)
    globals=b['arguments']['preprocessor']['globals']
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])==struct.pack('<5H',3,36,37,38,39)
    assert globals['rc_character_tokens[36]']['bytes']=='2800'
    assert b['arguments']['character']==dict(character=36,flags=16,formal_bytes=dict(character=2,flags=2))
    assert re.findall(r'^frontier-errors errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert b['crt_errors']==dict(errno=0,last_error=0)
    assert proof['application_trace'][-1]=='rc_set_character_token'
    proof.update(trial='rc_set_character_token',preprocessor=parser,character_tokens=dict(entries=[3,36,37,38,39],dollar=40,scope='encountered indices0..4 and36 only; other lexical entries unclaimed'),character_class_assignment=dict(character=36,flags=16,formal_bytes=2,callee_cleanup_bytes=8),setup_reference=dict(path='cases/resources-rich/acceptance-0027.json',sha256=sha(HERE/'cases/resources-rich/acceptance-0027.json')))
    return proof
