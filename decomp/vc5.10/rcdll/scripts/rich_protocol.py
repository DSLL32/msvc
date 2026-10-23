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
    assert r['case']in ['resources-rich','resources-rich-trailing-space']
    assert r['args']==['/r','/dCASE_FROM_COMMAND=7'+(' 'if r['case']=='resources-rich-trailing-space'else''),'/iinclude','/foresources.res','resources.rc']
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
    path=HERE/('cases/'+r['case']+'/reference/manifest.json')
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



def prepared_case_state(original,native,b,argv,extra_blocks=0,filename=None,output_open=False):
    text=(original/'events.txt').read_text();arguments=b['arguments'];source=(original/'source/resources.rc').read_bytes()
    assert arguments['count']==20 and arguments['argv']==argv
    assert arguments['prepared_lines']==len(source.splitlines())==126 and arguments['current_line']==0
    raw=(original/'globals.bin').read_bytes()
    assert struct.unpack_from('<I',raw,0x29458-0x14000)[0]==126 and struct.unpack_from('<I',raw,0x16ce8-0x14000)[0]==0
    strict_files=filename is not None
    filename=filename or 'resources.rc';filename_bytes=filename.encode('utf-16le');assert len(filename_bytes)<256
    assert raw_bytes(text,'prepared-filename')==bytes.fromhex(arguments['filename'])==filename_bytes+b'\0'*(514-len(filename_bytes))
    old=struct.unpack('<8I',raw_bytes(text,'prepared-closed-input'));new=struct.unpack('<8I',bytes.fromhex(arguments['closed_input']))
    buffered_output=b['function']=='rc_quote_source_filename'
    if buffered_output:
        assert old[0]-old[2]==new[0]-new[2]==16 and old[1:2]+old[3:]==new[1:2]+new[3:]==(4080,138,4,0,4096,0)
    else:assert old[:4]+old[5:]==new[:4]+new[5:]==(0,0,0,128 if output_open else 0,0,4096,0)and old[4]>=3 and new[4]>=3
    if output_open:
        assert raw_bytes(text,'prepared-closed-input')==raw_bytes(text,'cpp-output-file')
        assert bytes.fromhex(arguments['closed_input'])==bytes.fromhex(arguments['preprocessor']['globals']['rc_cpp_output_file']['file_state'])
    if strict_files:
        assert old[4]==new[4]==4,'closed input descriptor differs after host-only I/O separation'
        if not buffered_output:assert old==new
    assert b['globals']['rc_window']=='00000000','this case cannot invoke an unproved window callback'
    input_temporary=raw_bytes(text,'input-temporary')[:-1].decode('ascii');output_temporary=raw_bytes(text,'output-temporary')[:-1].decode('ascii')
    assert argv[:4]==['RCPP','-CP','1252','-f']and argv[4]==input_temporary and argv[5:7]==['-g',output_temporary]
    receipt=json.loads((original/'receipt.json').read_text());temporary=input_temporary.rsplit('\\',1)[-1]
    artifact=receipt['case_files'][temporary]
    assert artifact==dict(bytes=6762,sha256='3e1e06b1f0652c2ad397213fcd2a2d91f940a865d1dda5633330b2c096e6f8ed')
    assert (original/'source'/temporary).read_bytes().startswith('#line 1"resources.rc"\r\n'.encode('utf-16le'))
    if output_open:assert receipt['case_files'][output_temporary.rsplit('\\',1)[-1]]==dict(bytes=0,sha256=hashlib.sha256(b'').hexdigest())
    else:assert output_temporary.rsplit('\\',1)[-1]not in receipt['case_files']
    identity=b['heap_identity'];blocks=identity['blocks'];assert len(blocks)==3+extra_blocks
    for field,label,size in [('rc_include_paths','include-current-block',528),('rc_input_temporary','input-temporary-block',268),('rc_output_temporary','output-temporary-block',268)]:
        pointer=int.from_bytes(bytes.fromhex(b['globals'][field]),'little')
        block=next(block for block in blocks if block['payload']+8==pointer)
        assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])==raw_bytes(text,label)
    return dict(lines=126,current_line=0,artifact=artifact,input_was_closed=True,current_FILE_state="reused for preprocessor output"if output_open else"closed")


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


def wide_argument_state(original,native,a,b,filename=None,extra_blocks=0,output_open=False):
    text=(original/'events.txt').read_text();arguments=b['arguments']
    # The previously accepted forwarding boundary records this exact case's
    # ANSI vector. Only its two actual temporary paths vary between captures.
    expected=json.loads((HERE/'cases/resources-rich/acceptance-0020.json').read_text())['proof']['forwarded']['argv'][:]
    expected[4]=raw_bytes(text,'input-temporary')[:-1].decode('ascii');expected[6]=raw_bytes(text,'output-temporary')[:-1].decode('ascii')
    assert len(expected)==20 and all(value.isascii()for value in expected)
    prepared=prepared_case_state(original,native,b,expected,extra_blocks=1+extra_blocks,filename=filename,output_open=output_open)
    original_pointer=int(re.findall(r'^wide-args 20 ([0-9a-f]+)$',text,re.M)[0],16);native_pointer=arguments['vector_pointer'];assert native_pointer>0
    vector=arguments['wide_vector'];assert len(vector)==20 and arguments['wide_terminator']==0
    assert re.findall(r'^wide-terminator ([0-9a-f]+)$',text,re.M)==['00000000']
    old_pointers=[int(value,16)for value in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)];assert len(old_pointers)==20
    wanted=[(value+'\0').encode('utf-16le')for value in expected]
    variant=json.loads((original/'receipt.json').read_text())['case']=='resources-rich-trailing-space'
    size=8+4*21+sum(len(value)for value in wanted)+(2 if variant else 0)
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
    assert 8+cursor+(2 if variant else 0)==size
    if variant:assert original_block[-2:]==native_block[-2:]==b'\0'*2
    assert original_block[8+4*20:8+4*21]==native_block[8+4*20:8+4*21]==b'\0'*4
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(requests)==len(results)==len(payloads)==5+extra_blocks
    assert int(requests[4][2])==size and requests[4][1]=='9'and requests[4][3]=='00000000'
    assert int(results[4][0],16)+8==original_pointer and results[4][1]=='00000000'
    assert int(payloads[4][0])==size and bytes.fromhex(payloads[4][1])==b'\0'*size
    raw=(original/'globals.bin').read_bytes();assert int(requests[4][0],16)==struct.unpack_from('<I',raw,0x25f54-0x14000)[0]
    allocations=[row for row in b['heap']if row['operation']=='allocate'];assert len(allocations)==5+extra_blocks
    last=allocations[4];assert last['heap']==identity['handle']and last['flags']==9 and last['bytes']==size and last['result']+8==native_pointer
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



def macro_table_state(original,b,old_heads,new_heads):
    text=(original/'events.txt').read_text();nodes=b['arguments']['preprocessor']['macros']
    rows=re.findall(r'^macro-node (\d+) bucket=(\d+) pointer=([0-9a-f]+) next=([0-9a-f]+) name=([0-9a-f]+) text=([0-9a-f]+) kind=([0-9a-f]+)$',text,re.M)
    assert re.findall(r'^macro-nodes (\d+)$',text,re.M)==['3']and len(rows)==len(nodes)==3
    expected={'CASE_FROM_COMMAND':'7','_WIN32':'1','RC_INVOKED':'1'};seen=set();old_wanted=[0]*256;new_wanted=[0]*256;proof=[]
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    allocations=[row for row in b['heap']if row['operation']=='allocate'];assert len(requests)==len(results)==len(allocations)>=14
    for index,(row,node)in enumerate(zip(rows,nodes)):
        number,bucket,pointer,next_pointer,name_pointer,text_pointer,kind=[int(x,10 if i<2 else 16)for i,x in enumerate(row)]
        assert number==index and bucket==node['bucket']and next_pointer==node['next']==kind==node['parameters']==node['flags']==node['reserved']==0
        old_name=raw_bytes(text,'macro-node-name-%d'%index);new_name=bytes.fromhex(node['name_block']);assert old_name==new_name and old_name[:8]==b'\0'*8 and old_name[-2:]==b'\0'*2
        name=old_name[8:-2].decode('utf-16le')[:-1];assert name in expected and name not in seen;seen.add(name)
        assert bucket==sum(ord(c)&0x5f for c in name)&255
        old_wanted[bucket]=pointer;new_wanted[bucket]=node['pointer']
        old_record=raw_bytes(text,'macro-node-record-%d'%index);new_record=bytes.fromhex(node['record'])
        assert old_record==b'\0'*8+struct.pack('<4I',0,name_pointer,text_pointer,0)
        assert new_record==b'\0'*8+struct.pack('<4I',0,node['name'],node['text'],0)
        wanted_text=b'\0'*8+(expected[name]+'\0D\0').encode('utf-16le')
        assert raw_bytes(text,'macro-node-text-%d'%index)==bytes.fromhex(node['text_block'])==wanted_text
        for old_pointer,new_pointer,contents in [(pointer,node['pointer'],new_record),(name_pointer,node['name'],new_name),(text_pointer,node['text'],wanted_text)]:
            owned=[item for item in b['heap_identity']['blocks']if item['payload']+8==new_pointer];assert len(owned)==1;block=owned[0]
            assert block['owner']==b['heap_identity']['handle']and block['flags']==9 and block['bytes']==len(contents)and bytes.fromhex(block['contents'])==contents
            found=[i for i,result in enumerate(results)if int(result[0],16)+8==old_pointer];assert len(found)==1;allocation_index=found[0];assert 5<=allocation_index<14
            assert int(requests[allocation_index][2])==len(contents)and requests[allocation_index][1:] == ('9',str(len(contents)),'00000000')and results[allocation_index][1]=='00000000'
            native_allocation=allocations[allocation_index]
            assert native_allocation['result']+8==new_pointer and native_allocation['bytes']==len(contents)and native_allocation['flags']==9 and native_allocation['payload']=='00'*len(contents)
        proof.append(dict(bucket=bucket,name=name,replacement=expected[name],parameters=0,flags=0))
    assert seen==set(expected)and list(struct.unpack('<256I',old_heads))==old_wanted and list(struct.unpack('<256I',new_heads))==new_wanted
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
        elif row['kind']=='macro-buckets':
            if any(old):state['macro_records']=macro_table_state(original,b,old,new)
            else:assert new==old and not observed['macros']
        elif row['kind']=='crt-file':
            assert bool(int.from_bytes(old,'little'))==bool(int.from_bytes(new,'little'))
            if any(old):
                events=(original/'events.txt').read_text()
                assert re.findall(r'^cpp-output-pointer ([0-9a-f]+)$',events,re.M)==['%08x'%int.from_bytes(old,'little')]
                left=struct.unpack('<8I',raw_bytes(events,'cpp-output-file'));right=struct.unpack('<8I',bytes.fromhex(actual['file_state']))
                if b['function']=='rc_quote_source_filename':
                    assert left[2]and right[2]and left[0]-left[2]==right[0]-right[2]==16
                    assert left[1:2]+left[3:]==right[1:2]+right[3:]==(4080,138,4,0,4096,0)
                    assert raw_bytes(events,'cpp-output-buffer')==bytes.fromhex(actual['file_buffer'])[:16]and len(raw_bytes(events,'cpp-output-buffer'))==16
                    assert re.findall(r'^cpp-output-position (\d+)$',events,re.M)==re.findall(r'^pos:\s*(\d+)$',b['arguments']['output_fdinfo'],re.M)==['0']
                else:assert left==right
            else:assert old==new
        elif row['kind']=='source-buffer':
            left=int.from_bytes(old,'little');right=int.from_bytes(new,'little')
            if left:
                original_frame=struct.unpack('<9I',read(0x18e80,36));native_frame=struct.unpack('<9I',bytes.fromhex(observed['globals']['rc_source_frames']['bytes'])[:36])
                assert left==original_frame[5]and right==native_frame[5]and right>0
            else:assert old==new
        elif row['kind']=='source-stack':
            if any(old):
                left=struct.unpack('<9I',old[:36]);right=struct.unpack('<9I',new[:36])
                assert old[36:]==new[36:]==b'\0'*(36*1023)
                old_file=int.from_bytes(read(0x21e88,4),'little');new_file=int.from_bytes(bytes.fromhex(observed['globals']['rc_source_file']['bytes']),'little')
                old_buffer=int.from_bytes(read(0x18120,4),'little');new_buffer=int.from_bytes(bytes.fromhex(observed['globals']['rc_source_cursor']['bytes']),'little')
                old_name=new_name=0
                if b['function']in ['rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename']:
                    old_name=int(re.findall(r'^source-name-result ([0-9a-f]+)$',(original/'events.txt').read_text(),re.M)[0],16)
                    new_name=b['arguments']['source_name_copies'][0]['result']
                assert left==(4098,old_file,0,old_name,0,old_buffer,3381 if b['function']in ['rc_duplicate_source_name','rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename']else 0,2,0)
                assert right==(4098,new_file,0,new_name,0,new_buffer,3381 if b['function']in ['rc_duplicate_source_name','rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename']else 0,2,0)
            else:assert old==new
        elif row['kind']=='crt-buffered-file':
            if any(old):
                events=(original/'events.txt').read_text();left=struct.unpack('<8I',raw_bytes(events,'cpp-source-file'));right=struct.unpack('<8I',bytes.fromhex(actual['file_state']))
                assert re.findall(r'^cpp-source-pointer ([0-9a-f]+)$',events,re.M)==['%08x'%int.from_bytes(old,'little')]
                assert int.from_bytes(new,'little')>0 and left[2]and right[2]
                assert left[0]-left[2]==right[0]-right[2]and left[1:2]+left[3:]==right[1:2]+right[3:]
                assert left[4]==5 and left[6]==512 and left[7]==0
                assert raw_bytes(events,'cpp-source-buffer')==bytes.fromhex(actual['file_buffer'])and len(bytes.fromhex(actual['file_buffer']))==left[6]
                position=re.findall(r'^cpp-source-position (\d+)$',events,re.M);assert len(position)==1
                assert re.findall(r'^pos:\s*(\d+)$',b['arguments']['source_fdinfo'],re.M)==position
            else:assert old==new
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

def parsed_option_state(original,native,b,original_arguments,extra_blocks=0,output_open=False):
    text=(original/'events.txt').read_text();input_name=raw_bytes(text,'input-temporary')[:-1].decode('ascii')
    state=wide_argument_state(original,native,None,b,filename=input_name,extra_blocks=extra_blocks,output_open=output_open)
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


def character_class_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_set_character_class'and len(config['accepted'])==28
    a,b,proof=checkpoint(original,native,'rc_hash_macro_name',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c245e0 and a['words'][1]==0x71c38128
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_open_preprocessor\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();forwarded=list(struct.unpack('<4I',raw_bytes(text,'setup-forwarded')))
    assert forwarded[2]==0x71c344f8+4+97*4 and b['arguments']['setup_iteration']==97
    forwarded[2]=0x71c344f8 # The original reuses its parameter slot for the live list cursor.
    state,parser,text,input_name=parsed_option_state(original,native,b,forwarded);proof.update(state)
    globals=b['arguments']['preprocessor']['globals']
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])==struct.pack('<5H',3,36,37,38,39)
    assert globals['rc_character_tokens[36]']['bytes']=='2800'and globals['rc_character_classes[36]']['bytes']=='1000'
    name=b['arguments']['macro_name'];expected=('CASE_FROM_COMMAND'+'\0').encode('utf-16le')
    assert raw_bytes(text,'macro-hash-name')==bytes.fromhex(name['bytes'])==expected
    assert name['pointer']==globals['rc_macro_name']['address']
    assert bytes.fromhex(globals['rc_macro_name']['bytes'])==expected+b'\0'*(1024-len(expected))
    assert re.findall(r'^frontier-errors errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert b['crt_errors']==dict(errno=0,last_error=0)
    assert proof['application_trace'][-1]=='rc_set_character_class'
    proof.update(trial='rc_set_character_class',preprocessor=parser,character_classes=dict(dollar=16,scope='encountered dollar entry only; other lexical entries unclaimed'),macro_name='CASE_FROM_COMMAND',definition_cursor=dict(index=97,original_parameter_reused=True,native_local_index=97),token_reference=dict(path='cases/resources-rich/acceptance-0028.json',sha256=sha(HERE/'cases/resources-rich/acceptance-0028.json')))
    return proof


def macro_hash_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_hash_macro_name'and len(config['accepted'])==29
    a,b,proof=checkpoint(original,native,'rc_define_macro',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c24634 and a['words'][2:4]==[3,0xfffffffe]
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_open_preprocessor\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();forwarded=list(struct.unpack('<4I',raw_bytes(text,'setup-forwarded')))
    assert forwarded[2]==0x71c344f8+4+97*4 and b['arguments']['setup_iteration']==97
    forwarded[2]=0x71c344f8 # The original reuses its parameter slot for the live list cursor.
    state,parser,text,input_name=parsed_option_state(original,native,b,forwarded);proof.update(state)
    globals=b['arguments']['preprocessor']['globals']
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])==struct.pack('<5H',3,36,37,38,39)
    assert globals['rc_character_tokens[36]']['bytes']=='2800'and globals['rc_character_classes[36]']['bytes']=='1000'
    expected=('CASE_FROM_COMMAND'+'\0').encode('utf-16le')
    assert bytes.fromhex(globals['rc_macro_name']['bytes'])==expected+b'\0'*(1024-len(expected))
    hash_value=sum(ord(c)&0x5f for c in 'CASE_FROM_COMMAND')&255
    assert globals['rc_macro_hash']['bytes']==bytes([hash_value]).hex()and globals['rc_macro_name_length']['bytes']=='12000000'
    assert globals['rc_macro_buckets']['bytes']=='00'*1024
    value=b['arguments']['definition'];old_wide=[int(x,16)for x in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)]
    assert a['words'][1]==old_wide[19]+36
    assert value==dict(value=b['arguments']['wide_vector'][19]['pointer']+36,length=3,parameters=-2,value_bytes='37000000',formal_bytes=dict(value=4,length=4,parameters=4))
    assert raw_bytes(text,'macro-definition-value')==bytes.fromhex(value['value_bytes'])
    assert re.findall(r'^frontier-errors errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert b['crt_errors']==dict(errno=0,last_error=0)
    assert proof['application_trace'][-1]=='rc_hash_macro_name'
    proof.update(trial='rc_hash_macro_name',preprocessor=parser,macro_hash=dict(name='CASE_FROM_COMMAND',hash=hash_value,name_length=18),definition=dict(value='7',length=3,parameters=-2,byte_cleanup=12),definition_cursor=dict(index=97,original_parameter_reused=True,native_local_index=97),class_reference=dict(path='cases/resources-rich/acceptance-0029.json',sha256=sha(HERE/'cases/resources-rich/acceptance-0029.json')))
    return proof


def define_macro_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_define_macro'and len(config['accepted'])==30
    a,b,proof=checkpoint(original,native,'rc_find_macro',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c24c5b
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_define_macro\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();forwarded=list(struct.unpack('<4I',raw_bytes(text,'setup-forwarded')))
    assert forwarded[2]==0x71c344f8+4+97*4 and b['arguments']['setup_iteration']==97
    forwarded[2]=0x71c344f8 # The original reuses its parameter slot for the live list cursor.
    state,parser,text,input_name=parsed_option_state(original,native,b,forwarded);proof.update(state)
    globals=b['arguments']['preprocessor']['globals']
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])==struct.pack('<5H',3,36,37,38,39)
    assert globals['rc_character_tokens[36]']['bytes']=='2800'and globals['rc_character_classes[36]']['bytes']=='1000'
    expected=('CASE_FROM_COMMAND'+'\0').encode('utf-16le')
    assert bytes.fromhex(globals['rc_macro_name']['bytes'])==expected+b'\0'*(1024-len(expected))
    hash_value=sum(ord(c)&0x5f for c in 'CASE_FROM_COMMAND')&255
    assert globals['rc_macro_hash']['bytes']==bytes([hash_value]).hex()and globals['rc_macro_name_length']['bytes']=='12000000'
    assert globals['rc_macro_buckets']['bytes']=='00'*1024 and globals['rc_macro_listing_enabled']['bytes']=='00000000'
    value=b['arguments']['definition'];old_wide=[int(x,16)for x in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)]
    forwarded_definition=list(struct.unpack('<3I',raw_bytes(text,'definition-forwarded')))
    assert forwarded_definition==[old_wide[19]+36,3,0xfffffffe]
    assert value==dict(value=b['arguments']['wide_vector'][19]['pointer']+36,length=3,parameters=-2,value_bytes='37000000',formal_bytes=dict(value=4,length=4,parameters=4))
    assert raw_bytes(text,'macro-definition-value')==bytes.fromhex(value['value_bytes'])
    assert re.findall(r'^frontier-errors errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert b['crt_errors']==dict(errno=0,last_error=0)
    assert proof['application_trace'][-1]=='rc_define_macro'
    proof.update(trial='rc_define_macro',preprocessor=parser,macro_hash=dict(name='CASE_FROM_COMMAND',hash=hash_value,name_length=18),definition=dict(value='7',length=3,parameters=-2,byte_cleanup=12),definition_cursor=dict(index=97,original_parameter_reused=True,native_local_index=97),hash_reference=dict(path='cases/resources-rich/acceptance-0030.json',sha256=sha(HERE/'cases/resources-rich/acceptance-0030.json')))
    return proof


def find_macro_trial(original,native):
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_find_macro'and len(config['accepted'])==31
    a,b,proof=checkpoint(original,native,'rc_copy_macro_text',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][:3]==[0x71c24d2f,0x71c38128,18]
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_define_macro\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();forwarded=list(struct.unpack('<4I',raw_bytes(text,'setup-forwarded')))
    assert forwarded[2]==0x71c344f8+4+97*4 and b['arguments']['setup_iteration']==97
    forwarded[2]=0x71c344f8 # The original reuses its parameter slot for the live list cursor.
    state,parser,text,input_name=parsed_option_state(original,native,b,forwarded,extra_blocks=1);proof.update(state)
    globals=b['arguments']['preprocessor']['globals']
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])==struct.pack('<5H',3,36,37,38,39)
    assert globals['rc_character_tokens[36]']['bytes']=='2800'and globals['rc_character_classes[36]']['bytes']=='1000'
    expected=('CASE_FROM_COMMAND'+'\0').encode('utf-16le')
    assert bytes.fromhex(globals['rc_macro_name']['bytes'])==expected+b'\0'*(1024-len(expected))
    hash_value=sum(ord(c)&0x5f for c in 'CASE_FROM_COMMAND')&255
    assert globals['rc_macro_hash']['bytes']==bytes([hash_value]).hex()and globals['rc_macro_name_length']['bytes']=='12000000'
    assert globals['rc_macro_buckets']['bytes']=='00'*1024 and globals['rc_macro_listing_enabled']['bytes']=='00000000'
    value=b['arguments']['definition'];old_wide=[int(x,16)for x in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)]
    forwarded_definition=list(struct.unpack('<3I',raw_bytes(text,'definition-forwarded')))
    assert forwarded_definition==[old_wide[19]+36,3,0xfffffffe]
    assert value==dict(value=b['arguments']['wide_vector'][19]['pointer']+36,length=3,parameters=-2,value_bytes='37000000',formal_bytes=dict(value=4,length=4,parameters=4))
    assert raw_bytes(text,'macro-definition-value')==bytes.fromhex(value['value_bytes'])
    assert re.findall(r'^frontier-errors errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert b['crt_errors']==dict(errno=0,last_error=0)
    copy=b['arguments']['copy'];assert copy==dict(value=globals['rc_macro_name']['address'],length=18,value_bytes=expected.hex(),formal_bytes=dict(value=4,length=4))
    assert raw_bytes(text,'macro-copy-value')==expected
    record=b['arguments']['pending_macro'];assert record['record_bytes']==16 and bytes.fromhex(record['bytes'])==raw_bytes(text,'pending-macro-block')==b'\0'*24
    original_macro=int(re.findall(r'^macro-record ([0-9a-f]+)$',text,re.M)[0],16)
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M);results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M);payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(requests)==len(results)==len(payloads)==6 and requests[5][1:]==('9','24','00000000')and int(results[5][0],16)+8==original_macro
    assert results[5][1]=='00000000'and payloads[5]==('24','00'*24)
    allocations=[row for row in b['heap']if row['operation']=='allocate'];last=allocations[5]
    assert last['heap']==b['heap_identity']['handle']and last['flags']==9 and last['bytes']==24 and last['result']+8==record['pointer']and last['payload']=='00'*24
    assert last['last_error_before']==last['last_error_after']==0
    block=next(row for row in b['heap_identity']['blocks']if row['payload']+8==record['pointer'])
    assert block['owner']==b['heap_identity']['handle']and block['flags']==9 and block['bytes']==24 and block['contents']=='00'*24
    assert proof['application_trace'][-1]=='rc_find_macro'
    proof.update(trial='rc_find_macro',preprocessor=parser,macro_hash=dict(name='CASE_FROM_COMMAND',hash=hash_value,name_length=18),definition=dict(value='7',length=3,parameters=-2,byte_cleanup=12),definition_cursor=dict(index=97,original_parameter_reused=True,native_local_index=97),definition_reference=dict(path='cases/resources-rich/acceptance-0031.json',sha256=sha(HERE/'cases/resources-rich/acceptance-0031.json')),pending_record=dict(bytes=16,allocation_bytes=24,header_bytes=8,zeroed=True),copy=dict(value='CASE_FROM_COMMAND',length=18,cleanup=8))
    return proof


def _macro_setup_trial(original,native,stage):
    trial,accepted,frontier,caller={'copy':('rc_copy_macro_text',32,'rc_open_source',0x71c24767),'open':('rc_open_source',33,'rc_refill_source',0x71c23eee),'refill':('rc_refill_source',34,'rc_duplicate_source_name',0x71c23ef9),'duplicate':('rc_duplicate_source_name',35,'rc_emit_source_line',0x71c23f27),'emit':('rc_emit_source_line',36,'rc_write_preprocessed',0x71c24275),'write':('rc_write_preprocessed',37,'rc_quote_source_filename',0x71c24281)}[stage]
    source_opened=stage!='copy';refilled=stage in ['refill','duplicate','emit','write']
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')==trial and len(config['accepted'])==accepted
    assert json.loads((original/'receipt.json').read_text())['case']=='resources-rich-trailing-space','no-space source overread is unsupported'
    a,b,proof=checkpoint(original,native,frontier,('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==caller
    if not source_opened:assert a['words'][2]==1
    assert re.search(r'^#1\s+0x[0-9a-f]+ in '+('rc_emit_source_line'if stage in ['emit','write']else'rc_open_source'if source_opened else'rc_open_preprocessor')+r'\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();forwarded=list(struct.unpack('<4I',raw_bytes(text,'setup-forwarded')))
    assert forwarded[2]==0x71c344f8+4+100*4 and b['arguments']['setup_iteration']==100
    forwarded[2]=0x71c344f8
    state,parser,text,input_name=parsed_option_state(original,native,b,forwarded,extra_blocks=11 if stage in ['duplicate','emit','write']else 10 if source_opened else 9,output_open=True);proof.update(state)
    globals=b['arguments']['preprocessor']['globals'];wide=b['arguments']['wide_vector']
    old_wide=[int(x,16)for x in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)]
    if not source_opened:assert a['words'][1]==old_wide[4]
    assert b['arguments']['source_open']==dict(path=wide[4]['pointer'],first=1,path_bytes=wide[4]['bytes'],formal_bytes=dict(path=4,first=4))
    assert raw_bytes(text,'source-open-input')==bytes.fromhex(wide[4]['bytes'])
    assert globals['rc_macro_hash']['bytes']=='04'and globals['rc_macro_name_length']['bytes']=='0b000000'
    if not source_opened:
        assert globals['rc_source_depth']['bytes']=='ffffffff'and globals['rc_source_cursor']['bytes']==globals['rc_source_encoding']['bytes']==globals['rc_source_file']['bytes']=='00000000'
        assert globals['rc_source_frames']['bytes']=='00'*(36*1024)
    assert len(state['macro_records'])==3
    initial=raw_bytes(text,'wide-initial-argument-block');current=raw_bytes(text,'wide-argument-block')
    assert len(initial)==len(current)==state['widened']['allocation_bytes']
    old_trim=old_wide[19]+38;offset=8+old_trim-state['widened']['original_base']
    assert initial[offset:offset+4]==b' \0\0\0'and current[offset:offset+4]==b'\0'*4
    wanted=bytearray(initial);wanted[offset:offset+2]=b'\0'*2;assert bytes(wanted)==current
    assert re.findall(r'^macro-trim-before pointer=([0-9a-f]+) word=([0-9a-f]+)$',text,re.M)==[('%08x'%old_trim,'0020')]
    assert re.findall(r'^macro-trim-after pointer=([0-9a-f]+) word=([0-9a-f]+)$',text,re.M)==[('%08x'%old_trim,'0000')]
    native_initial=json.loads((native/'wide-initial.json').read_text());new_trim=wide[19]['pointer']+38
    assert native_initial['pointer']==state['widened']['native_base']and native_initial['requested_size']==len(initial)
    assert native_initial['last_argument']==('CASE_FROM_COMMAND=7 '+'\0').encode('utf-16le').hex()and native_initial['trim_pointer']==new_trim
    block=next(row for row in b['heap_identity']['blocks']if row['payload']+8==state['widened']['native_base'])
    before=bytearray.fromhex(native_initial['bytes']);new_offset=8+new_trim-state['widened']['native_base']
    assert before[new_offset:new_offset+4]==b' \0\0\0';before[new_offset:new_offset+2]=b'\0'*2;assert before.hex()==block['contents']
    writes=native_initial['writes'];assert len(writes)==1 and writes[0]['function']=='rc_open_preprocessor'and writes[0]['after']=='0000'and writes[0]['eax']==new_trim
    raw_write=re.findall(r'^RCDLL_TRIM_WRITE=([0-9a-f]+) (\w+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',(native/'gdb.stdout').read_text(),re.M)
    assert raw_write==[('%08x'%new_trim,'rc_open_preprocessor','%08x'%writes[0]['pc'],'0000','%08x'%new_trim)]
    import subprocess
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(native/'captured-native.elf')],text=True)
    def symbol(name):
        found=re.findall(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+re.escape(name)+r'$',symbols,re.M);assert len(found)==1;return tuple(int(v,16)for v in found[0])
    frontier_address,_=symbol(frontier);parent_address,parent_bytes=symbol('rc_open_preprocessor')
    runtime=int(re.findall(r'^Symbol "'+frontier+r'" is a function at address (0x[0-9a-f]+)',(native/'gdb.stdout').read_text(),re.M)[0],16)
    store_pc=writes[0]['pc']-(runtime-frontier_address);assert parent_address<store_pc-5<store_pc<=parent_address+parent_bytes
    elf=(native/'captured-native.elf').read_bytes();phoff=struct.unpack_from('<I',elf,28)[0];phbytes,phcount=struct.unpack_from('<HH',elf,42);stores=[]
    for index in range(phcount):
        kind,raw,start,_,size=struct.unpack_from('<IIIII',elf,phoff+phbytes*index)
        if kind==1 and start<=store_pc-5<store_pc<=start+size:stores.append(elf[raw+store_pc-5-start:raw+store_pc-start])
    assert stores==[bytes.fromhex('66c7000000')],'watchpoint PC must follow actual WORD[EAX]=0 store'
    calls=re.findall(r'^macro-copy-call (\d+) site=([0-9a-f]+) source=([0-9a-f]+) count=(\d+) esp=([0-9a-f]+)$',text,re.M)
    returns=re.findall(r'^macro-copy-return (\d+) result=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M)
    regions=re.findall(r'^macro-copy-region (\d+) base=([0-9a-f]+) allocation=([0-9a-f]+) bytes=(\d+) state=([0-9a-f]+) protect=([0-9a-f]+) type=([0-9a-f]+)$',text,re.M)
    copies=b['arguments']['macro_copies'];assert copies==json.loads((native/'macro-copies.json').read_text())and len(calls)==len(returns)==len(regions)==len(copies)==6
    names=['CASE_FROM_COMMAND','_WIN32','RC_INVOKED'];observations=[]
    for index,(call,returned,region,new)in enumerate(zip(calls,returns,regions,copies)):
        number,site,pointer,count,sp=int(call[0]),int(call[1],16),int(call[2],16),int(call[3]),int(call[4],16)
        assert number==int(returned[0])==int(region[0])==index
        expected=((names[index//2]+'\0').encode('utf-16le')if not index%2 else ('7'if index==1 else'1').encode('utf-16le')+b'\0'*4)
        assert count==new['length']==len(expected)//2
        assert raw_bytes(text,'macro-copy-source-%d'%index)==bytes.fromhex(new['source_bytes'])==expected
        assert site==(0x4d2a if not index%2 else 0x4d63)
        assert int(returned[2],16)==sp+8 and new['return_esp']==new['entry_esp']+12
        result=raw_bytes(text,'macro-copy-result-%d'%index)
        assert result==bytes.fromhex(new['result_block'])==b'\0'*8+expected+b'\0'*2
        base,length=int(region[1],16),int(region[3]);protect=int(region[5],16)
        assert int(region[4],16)==0x1000 and not protect&0x100 and protect&255 in [2,4,8,0x20,0x40,0x80]
        assert base<=pointer and pointer+len(expected)<=base+length
        bounds=new['source_mapping'].split()[0].split('-');assert int(bounds[0],16)<=new['source']and new['source']+len(expected)<=int(bounds[1],16)and new['source_mapping'].split()[1].startswith('r')
        if not index%2:
            assert pointer==0x71c38128 and new['source']==globals['rc_macro_name']['address']and len(expected)<=1024
            source_scope='initialized1024-byte global name object'
        elif index==1:
            assert pointer==old_wide[19]+36 and new['source']==wide[19]['pointer']+36
            assert pointer+len(expected)==state['widened']['original_base']+state['widened']['allocation_bytes']-8
            assert new['source_contract']=='requested-private-heap'
            source_scope='all6bytes inside requested initialized packed vector:7,actualtrim-writeNUL,preservedoriginal terminalNUL'
        else:
            assert pointer==0x71c21268 and new['source']==copies[3]['source']
            assert (original/'relocated-text.bin').read_bytes()[0x268:0x26e]==expected
            source_scope='encountered initialized6-byte literal span, represented by L1 with an explicit secondNUL'
        assert new['errno_before']==new['errno_after']==new['last_error_before']==new['last_error_after']==0
        block=next(row for row in b['heap_identity']['blocks']if row['payload']+8==new['result'])
        assert block['owner']==b['heap_identity']['handle']and block['flags']==9 and block['bytes']==len(result)
        observations.append(dict(characters=count,source_sha256=hashlib.sha256(expected).hexdigest(),source_scope=source_scope,cleanup=8,returned=True))
    assert re.findall(r'^frontier-errors errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('0','00000000')]
    assert b['crt_errors']==dict(errno=0,last_error=0)
    assert proof['application_trace'][-1]==trial
    from importlib.util import spec_from_file_location,module_from_spec
    spec=spec_from_file_location('copy_seh',HERE/'scripts/check-copy-seh.py');module=module_from_spec(spec);spec.loader.exec_module(module);seh=module.verify()
    proof.update(trial=trial,preprocessor=parser,trim=dict(original_word_before=32,word_after=0,owned_terminal_preserved=True,all_other_packed_bytes_unchanged=True,native_store_elf_pc=hex(store_pc-5),native_store_opcode='66c7000000'),copies=observations,source_open=dict(input=input_name,first=1,stack_bytes=8,hidden=dict(depth=-1,cursor=None,encoding=0,current_file=None,frame_count=1024,frame_bytes=36,frames_zero=True)),seh=seh,exception_scope='Native normal readable-memory path only; native faults stop. Windows source retains actual filter/handler; no native fault recovery equivalence.',lookup_reference=dict(path='cases/resources-rich/acceptance-0032.json',sha256=sha(HERE/'cases/resources-rich/acceptance-0032.json')))
    if source_opened:proof['source_open']=source_open_state(original,native,b,state,input_name,refilled=refilled)
    if stage in ['emit','write']:proof['source_line']=source_line_state(original,native,a,b)
    if stage=='write':proof['output_write']=output_write_state(original,native,b)
    return proof


def source_open_state(original,native,b,state,input_name,refilled=False):
    text=(original/'events.txt').read_text();globals=b['arguments']['preprocessor']['globals']
    entry=b['arguments']['source_entry'];assert entry==json.loads((native/'source-entry.json').read_text())
    assert entry==dict(path=b['arguments']['wide_vector'][4]['pointer'],first=1,depth=-1,frames='00'*(36*1024))
    old_path=state['widened']['original_base']+(entry['path']-state['widened']['native_base'])
    assert re.findall(r'^source-open-entry caller=([0-9a-f]+) path=([0-9a-f]+) first=(-?\d+) depth=(-?\d+)$',text,re.M)==[('71c24767','%08x'%old_path,'1','-1')]
    assert raw_bytes(text,'source-stack-before')==b'\0'*(36*1024)
    assert globals['rc_source_depth']['bytes']=='00000000'and globals['rc_source_encoding']['bytes']=='02000000'
    old_path_arg,old_buffer=struct.unpack('<2I',raw_bytes(text,'source-open-forwarded'))
    assert old_path_arg==old_path # Original first parameter slot was actually reused for the allocated buffer.
    new_buffer=int.from_bytes(bytes.fromhex(globals['rc_source_cursor']['bytes']),'little')
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(requests)==len(results)==len(payloads)==(16 if b['function']in ['rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename']else 15)
    assert requests[14][1:]==('9','8206','00000000')and int(results[14][0],16)+8==old_buffer and results[14][1]=='00000000'
    assert requests[14][0]==requests[4][0]and payloads[14]==('8206','00'*8206)
    source=(original/'source'/input_name.rsplit('\\',1)[-1]).read_bytes()
    expected_buffer=b'\0'*8+source+b'\0'*(8198-len(source))if refilled else b'\0'*8206
    assert raw_bytes(text,'source-new-buffer')==expected_buffer
    allocations=[row for row in b['heap']if row['operation']=='allocate'];assert len(allocations)==(16 if b['function']in ['rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename']else 15)
    allocation=allocations[14];identity=b['heap_identity'];block=next(row for row in identity['blocks']if row['payload']+8==new_buffer)
    assert allocation['heap']==block['owner']==identity['handle']and allocation['result']+8==new_buffer
    assert allocation['bytes']==block['bytes']==8206 and allocation['flags']==block['flags']==9
    assert allocation['payload']=='00'*8206 and block['contents']==expected_buffer.hex()and allocation['last_error_before']==allocation['last_error_after']==0
    assert b['arguments']['source_host_path']==str(Path(b['case_inputs']['host_cwd'])/input_name.rsplit('\\',1)[-1])
    if refilled:refill_state(original,native,b,old_buffer,new_buffer)
    return dict(refilled=refilled,input=input_name,first=1,entry_depth=-1,depth=0,encoding=2,frame_capacity=4098,frame_bytes=36,frames=1024,source_buffer_requested=8198,private_heap_header=8,allocation_initially_zero=True,refill_formals=[],hidden_inputs='frame0/current FILE/cursor/encoding and all1024 frames checked',original_parameter_slot_reused_for_buffer=True,file_state='exact scalar FILE fields, PTR-base relation, full512 bytes and actual OS position')


def copy_macro_trial(original,native):
    return _macro_setup_trial(original,native,'copy')


def open_source_trial(original,native):
    return _macro_setup_trial(original,native,'open')


def refill_state(original,native,b,old_buffer,new_buffer):
    text=(original/'events.txt').read_text();globals=b['arguments']['preprocessor']['globals'];records=b['arguments']['refills']
    assert records==json.loads((native/'refills.json').read_text())and len(records)==1
    record=records[0];assert record['depth']==0 and record['cursor']==new_buffer and record['encoding']==2 and record['formals']==[] and record['result']==0
    assert re.findall(r'^refill-entry caller=([0-9a-f]+) depth=(-?\d+) cursor=([0-9a-f]+) encoding=(-?\d+)$',text,re.M)==[('71c23eee','0','%08x'%old_buffer,'2')]
    assert re.findall(r'^refill-result (-?\d+)$',text,re.M)==['0']
    old_frame=struct.unpack('<9I',raw_bytes(text,'refill-entry-frame'));new_frame=struct.unpack('<9I',bytes.fromhex(record['frame']))
    old_file=int.from_bytes((original/'globals.bin').read_bytes()[0x21e88-0x14000:0x21e8c-0x14000],'little');new_file=int.from_bytes(bytes.fromhex(globals['rc_source_file']['bytes']),'little')
    assert old_frame==(4098,old_file,0,0,0,old_buffer,0,2,0)and new_frame==(4098,new_file,0,0,0,new_buffer,0,2,0)
    assert raw_bytes(text,'refill-entry-buffer')==bytes.fromhex(record['buffer'])==b'\0'*8206
    left=struct.unpack('<8I',raw_bytes(text,'refill-entry-file'));right=struct.unpack('<8I',bytes.fromhex(record['file']))
    assert left[0]==left[2]and right[0]==right[2]and left[1:2]+left[3:]==right[1:2]+right[3:]==(0,9,5,0,512,0)
    assert raw_bytes(text,'refill-entry-file-buffer')==bytes.fromhex(record['file_buffer'])and len(bytes.fromhex(record['file_buffer']))==512
    if b['function']=='rc_duplicate_source_name':
        duplicate=b['arguments']['duplicate'];path=b['arguments']['filename_pointer']
        assert duplicate==dict(path=path,path_bytes=(b['arguments']['source_open']['path_bytes']),formal_bytes=dict(path=4))
        original_filename=int.from_bytes((original/'globals.bin').read_bytes()[0x149c4-0x14000:0x149c8-0x14000],'little')
        old_frontier=original_raw(original);assert old_frontier['words'][1]==original_filename
    else:source_name_state(original,native,b)
    assert re.search(r'^#1\s+0x[0-9a-f]+ in '+('rc_emit_source_line'if b['function']in ['rc_write_preprocessed','rc_quote_source_filename']else'rc_open_source')+r'\s*\(',(native/'gdb.stdout').read_text(),re.M)


def refill_source_trial(original,native):
    return _macro_setup_trial(original,native,'refill')


def source_name_state(original,native,b):
    text=(original/'events.txt').read_text();records=b['arguments']['source_name_copies'];assert records==json.loads((native/'source-name-copies.json').read_text())and len(records)==1
    record=records[0];value=bytes.fromhex(b['arguments']['source_open']['path_bytes']);assert raw_bytes(text,'source-name-input')==bytes.fromhex(record['bytes'])==value
    original_filename=int.from_bytes((original/'globals.bin').read_bytes()[0x149c4-0x14000:0x149c8-0x14000],'little')
    assert re.findall(r'^source-name-entry caller=([0-9a-f]+) path=([0-9a-f]+)$',text,re.M)==[('71c23ef9','%08x'%original_filename)]
    assert record['path']==b['arguments']['filename_pointer']and record['formal_bytes']==4
    old_result=int(re.findall(r'^source-name-result ([0-9a-f]+)$',text,re.M)[0],16);new_result=record['result'];assert old_result and new_result
    expected=b'\0'*8+value;assert raw_bytes(text,'source-name-result-block')==bytes.fromhex(record['result_block'])==expected
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M);results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M);payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(requests)==len(results)==len(payloads)==16 and requests[15][0]==requests[14][0]and requests[15][1:]==('9',str(len(expected)),'00000000')and int(results[15][0],16)+8==old_result
    assert payloads[15]==(str(len(expected)),'00'*len(expected))
    allocations=[row for row in b['heap']if row['operation']=='allocate'];allocation=allocations[15];identity=b['heap_identity'];block=next(row for row in identity['blocks']if row['payload']+8==new_result)
    assert allocation['result']+8==new_result and allocation['heap']==block['owner']==identity['handle']and allocation['bytes']==block['bytes']==len(expected)and allocation['flags']==block['flags']==9
    assert allocation['payload']=='00'*len(expected)and block['contents']==expected.hex()
    assert allocation['last_error_before']==allocation['last_error_after']==0


def duplicate_source_name_trial(original,native):
    return _macro_setup_trial(original,native,'duplicate')


def source_line_state(original,native,a,b):
    text=(original/'events.txt').read_text();globals=b['arguments']['preprocessor']['globals'];records=b['arguments']['source_formats']
    assert records==json.loads((native/'source-line-format.json').read_text())and len(records)==1
    record=records[0];template=('#line %d '+'\0').encode('utf-16le');wanted=('#line 1 '+'\0').encode('utf-16le')
    assert raw_bytes(text,'source-line-format-template')==bytes.fromhex(record['format'])==template
    assert re.findall(r'^source-line-format-call dest=([0-9a-f]+) format=([0-9a-f]+) number=(-?\d+) errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('71c38128','71c21250','1','0','00000000')]
    assert re.findall(r'^source-line-format-result result=(-?\d+) errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('8','0','00000000')]
    assert record['destination']==globals['rc_macro_name']['address']and record['number']==1 and record['result']==8
    assert record['errno_before']==record['errno_after']==record['last_error_before']==record['last_error_after']==0
    before=raw_bytes(text,'source-line-format-before');after=raw_bytes(text,'source-line-format-after')
    assert len(before)==len(after)==1024 and before==bytes.fromhex(record['before'])and after==bytes.fromhex(record['after'])
    assert after==wanted+before[len(wanted):]and globals['rc_macro_name']['bytes']==after.hex()
    file=int.from_bytes(bytes.fromhex(globals['rc_cpp_output_file']['bytes']),'little');old_file=int.from_bytes((original/'globals.bin').read_bytes()[0x18e74-0x14000:0x18e78-0x14000],'little')
    write=b['arguments']['write']if b['function']=='rc_write_preprocessed'else b['arguments']['output_writes'][0]['args']
    if b['function']=='rc_write_preprocessed':
        assert a['words'][:5]==[0x71c24275,0x71c38128,16,1,old_file]
        assert raw_bytes(text,'source-write-payload')==wanted[:-2]
    else:
        assert re.findall(r'^output-write-call data=([0-9a-f]+) size=(\d+) count=(\d+) file=([0-9a-f]+) errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('71c38128','16','1','%08x'%old_file,'0','00000000')]
        assert raw_bytes(text,'output-write-input')==wanted[:-2]
    assert write==dict(data=globals['rc_macro_name']['address'],size=16,count=1,file=file,formal_bytes=dict(data=4,size=4,count=4,file=4),payload=wanted[:-2].hex())
    return dict(format='#line %d ',argument=1,result=8,output='#line 1 ',scratch_bytes=1024,written_bytes_including_NUL=18,untouched_tail_bytes=1006,errors_preserved=True,write=dict(size=16,count=1,bytes=16,FILE='same open preprocessor output',ABI='stdcall4/ret16'))


def emit_source_line_trial(original,native):
    return _macro_setup_trial(original,native,'emit')


def output_write_state(original,native,b):
    text=(original/'events.txt').read_text();records=b['arguments']['output_writes'];assert records==json.loads((native/'output-writes.json').read_text())and len(records)==1
    record=records[0];globals=b['arguments']['preprocessor']['globals'];wanted='#line 1 '.encode('utf-16le')
    assert record['result']==1 and record['errno_before']==record['errno_after']==record['last_error_before']==record['last_error_after']==0
    assert re.findall(r'^output-write-result result=(\d+) errno=(-?\d+) last-error=([0-9a-f]+)$',text,re.M)==[('1','0','00000000')]
    assert raw_bytes(text,'output-write-before-file')==bytes.fromhex(record['before_file'])==struct.pack('<8I',0,0,0,128,4,0,4096,0)
    assert raw_bytes(text,'output-write-after-file')==raw_bytes(text,'cpp-output-file')
    assert record['after_file']==globals['rc_cpp_output_file']['file_state']
    assert bytes.fromhex(record['written_buffer'])==raw_bytes(text,'cpp-output-buffer')==wanted
    quote=b['arguments']['quote'];assert quote==dict(destination=globals['rc_macro_name']['address'],path=b['arguments']['filename_pointer'],path_bytes=b['arguments']['source_open']['path_bytes'],destination_bytes=globals['rc_macro_name']['bytes'],formal_bytes=dict(destination=4,path=4))
    old_filename=int.from_bytes((original/'globals.bin').read_bytes()[0x149c4-0x14000:0x149c8-0x14000],'little');a=original_raw(original)
    assert a['words'][:3]==[0x71c24281,0x71c38128,old_filename]
    return dict(size=16,count=1,result=1,output_written_prefix_bytes=16,FILE=dict(descriptor=4,flags=138,pointer_offset=16,remaining=4080,capacity=4096,os_position=0),unwritten_tail='not initialized or compared; not an application input at this frontier',errors_preserved=True,next_quote=dict(formals=2,stack_cleanup=8,destination='same1024-byte global scratch window',source='current bounded UTF16 filename'))


def write_preprocessed_trial(original,native):
    return _macro_setup_trial(original,native,'write')
