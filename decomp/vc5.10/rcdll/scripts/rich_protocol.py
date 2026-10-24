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
    current_line=1 if b['function']in ['rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else 0
    assert arguments['prepared_lines']==len(source.splitlines())==126 and arguments['current_line']==current_line
    raw=(original/'globals.bin').read_bytes()
    assert struct.unpack_from('<I',raw,0x29458-0x14000)[0]==126 and struct.unpack_from('<I',raw,0x16ce8-0x14000)[0]==current_line
    strict_files=filename is not None
    filename=filename or 'resources.rc';filename_bytes=filename.encode('utf-16le');assert len(filename_bytes)<256
    assert raw_bytes(text,'prepared-filename')==bytes.fromhex(arguments['filename'])==filename_bytes+b'\0'*(514-len(filename_bytes))
    old=struct.unpack('<8I',raw_bytes(text,'prepared-closed-input'));new=struct.unpack('<8I',bytes.fromhex(arguments['closed_input']))
    buffered_output=b['function']in ['rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type','rc_read_string','rc_expect_end']
    if buffered_output:
        length=len(buffered_output_prefix(b));assert old[0]-old[2]==new[0]-new[2]==length and old[1:2]+old[3:]==new[1:2]+new[3:]==(4096-length,138,4,0,4096,0)
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
    return dict(lines=126,current_line=current_line,artifact=artifact,input_was_closed=True,current_FILE_state="reused for preprocessor output"if output_open else"closed")


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
                if b['function']in ['rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                    prefix=buffered_output_prefix(b);length=len(prefix)
                    assert left[2]and right[2]and left[0]-left[2]==right[0]-right[2]==length
                    assert left[1:2]+left[3:]==right[1:2]+right[3:]==(4096-length,138,4,0,4096,0)
                    assert raw_bytes(events,'cpp-output-buffer')==bytes.fromhex(actual['file_buffer'])[:length]==prefix
                    assert re.findall(r'^cpp-output-position (\d+)$',events,re.M)==re.findall(r'^pos:\s*(\d+)$',b['arguments']['output_fdinfo'],re.M)==['0']
                else:assert left==right
            else:assert old==new
        elif row['kind']=='source-buffer':
            left=int.from_bytes(old,'little');right=int.from_bytes(new,'little')
            if left:
                original_frame=struct.unpack('<9I',read(0x18e80,36));native_frame=struct.unpack('<9I',bytes.fromhex(observed['globals']['rc_source_frames']['bytes'])[:36])
                cursor_offset=14 if b['function']in ['rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else 10 if b['function']in ['rc_directive_code','rc_next_token']else 6 if b['function']=='rc_character_class'else 4 if b['function']=='rc_read_identifier'else 2 if b['function']in ['rc_handle_directive','rc_read_nonspace']else 0
                assert left==original_frame[5]+cursor_offset and right==native_frame[5]+cursor_offset and right>0
            else:assert old==new
        elif row['kind']=='source-stack':
            if any(old):
                left=struct.unpack('<9I',old[:36]);right=struct.unpack('<9I',new[:36])
                assert old[36:]==new[36:]==b'\0'*(36*1023)
                old_file=int.from_bytes(read(0x21e88,4),'little');new_file=int.from_bytes(bytes.fromhex(observed['globals']['rc_source_file']['bytes']),'little')
                old_buffer=struct.unpack('<9I',old[:36])[5];new_buffer=struct.unpack('<9I',new[:36])[5]
                old_name=new_name=0
                if b['function']in ['rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                    old_name=int(re.findall(r'^source-name-result ([0-9a-f]+)$',(original/'events.txt').read_text(),re.M)[0],16)
                    new_name=b['arguments']['source_name_copies'][0]['result']
                assert left==(4098,old_file,0,old_name,0,old_buffer,3381 if b['function']in ['rc_duplicate_source_name','rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else 0,2,0)
                assert right==(4098,new_file,0,new_name,0,new_buffer,3381 if b['function']in ['rc_duplicate_source_name','rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else 0,2,0)
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
        elif row['kind']=='address-identity':
            assert int.from_bytes(old,'little')==0x71c20000+int(row['pointee_rva'],16)
            assert int.from_bytes(new,'little')==actual['identity_address']and actual['identity_address']!=observed['globals']['rc_macro_name']['address']
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
    assert globals['rc_cpp_active']['bytes']==('00000000'if b['function']in ['rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else'01000000')and globals['rc_cpp_E']['bytes']=='01000000'and globals['rc_cpp_P']['bytes']=='00000000'
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
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])[:10]==struct.pack('<5H',3,36,37,38,39)
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
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])[:10]==struct.pack('<5H',3,36,37,38,39)
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
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])[:10]==struct.pack('<5H',3,36,37,38,39)
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
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])[:10]==struct.pack('<5H',3,36,37,38,39)
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
    assert bytes.fromhex(globals['rc_character_tokens']['bytes'])[:10]==struct.pack('<5H',3,36,37,38,39)
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
    trial,accepted,frontier,caller={'copy':('rc_copy_macro_text',32,'rc_open_source',0x71c24767),'open':('rc_open_source',33,'rc_refill_source',0x71c23eee),'refill':('rc_refill_source',34,'rc_duplicate_source_name',0x71c23ef9),'duplicate':('rc_duplicate_source_name',35,'rc_emit_source_line',0x71c23f27),'emit':('rc_emit_source_line',36,'rc_write_preprocessed',0x71c24275),'write':('rc_write_preprocessed',37,'rc_quote_source_filename',0x71c24281),'quote':('rc_quote_source_filename',38,'rc_character_token',0x71c24220),'token':('rc_character_token',39,'rc_list_macro',0x71c23f6d),'listing':('rc_list_macro',40,'rc_scan_line_start',0x71c23fa3),'scanline':('rc_scan_line_start',41,'rc_handle_directive',0x71c320ad),'directive':('rc_handle_directive',42,'rc_read_nonspace',0x71c2635d),'nonspace':('rc_read_nonspace',43,'rc_read_identifier',0x71c263b6),'identifier':('rc_read_identifier',44,'rc_character_class',0x71c31b76),'classlookup':('rc_character_class',45,'rc_directive_code',0x71c263cd),'dictionary':('rc_directive_code',46,'rc_next_token',0x71c26533),'lexer':('rc_next_token',47,'rc_read_number',0x71c2381e),'number':('rc_read_number',48,'rc_get_logical_character',0x71c32296),'logical':('rc_get_logical_character',49,'rc_parse_integer_value',0x71c32356),'integer':('rc_parse_integer_value',50,'rc_digit_value',0x71c325d7),'digit':('rc_digit_value',51,'rc_decimal_integer_kind',0x71c32384),'kind':('rc_decimal_integer_kind',52,'rc_store_integer',0x71c3239a),'store':('rc_store_integer',53,'rc_intern_type',0x71c27437)}[stage]
    source_opened=stage!='copy';refilled=stage in ['refill','duplicate','emit','write','quote','token','listing','scanline','directive','nonspace','identifier','classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')==trial and len(config['accepted'])==accepted
    assert json.loads((original/'receipt.json').read_text())['case']=='resources-rich-trailing-space','no-space source overread is unsupported'
    a,b,proof=checkpoint(original,native,frontier,('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==caller
    if not source_opened:assert a['words'][2]==1
    assert re.search(r'^#1\s+0x[0-9a-f]+ in '+('rc_store_integer'if stage=='store'else'rc_parse_integer_value'if stage=='integer'else'rc_read_number'if stage in ['number','logical','digit','kind','store']else'rc_next_token'if stage=='lexer'else'rc_read_identifier'if stage=='identifier'else'rc_handle_directive'if stage in ['directive','nonspace','classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']else'rc_scan_line_start'if stage=='scanline'else'rc_quote_source_filename'if stage=='quote'else'rc_emit_source_line'if stage in ['emit','write']else'rc_open_source'if source_opened else'rc_open_preprocessor')+r'\s*\(',(native/'gdb.stdout').read_text(),re.M)
    text=(original/'events.txt').read_text();forwarded=list(struct.unpack('<4I',raw_bytes(text,'setup-forwarded')))
    assert forwarded[2]==0x71c344f8+4+100*4 and b['arguments']['setup_iteration']==100
    forwarded[2]=0x71c344f8
    state,parser,text,input_name=parsed_option_state(original,native,b,forwarded,extra_blocks=13 if stage=='store'else 11 if stage in ['duplicate','emit','write','quote','token','listing','scanline','directive','nonspace','identifier','classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']else 10 if source_opened else 9,output_open=True);proof.update(state)
    globals=b['arguments']['preprocessor']['globals'];wide=b['arguments']['wide_vector']
    old_wide=[int(x,16)for x in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)]
    if not source_opened:assert a['words'][1]==old_wide[4]
    assert b['arguments']['source_open']==dict(path=wide[4]['pointer'],first=1,path_bytes=wide[4]['bytes'],formal_bytes=dict(path=4,first=4))
    assert raw_bytes(text,'source-open-input')==bytes.fromhex(wide[4]['bytes'])
    assert globals['rc_macro_hash']['bytes']==('28'if stage in ['classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']else'04')and globals['rc_macro_name_length']['bytes']==('05000000'if stage in ['classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']else'0b000000')
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
    if stage in ['emit','write','quote','token','listing','scanline','directive','nonspace','identifier','classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']:proof['source_line']=source_line_state(original,native,a,b)
    if stage in ['write','quote','token','listing','scanline','directive','nonspace','identifier','classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']:proof['output_write']=output_write_state(original,native,b)
    if stage=='quote':proof['quote']=quote_source_state(original,native,a,b)
    if stage=='token':proof['character_lookup']=listing_marker_state(original,native,a,b)
    if stage in ['listing','scanline','directive','nonspace','identifier','classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']:proof['macro_listing']=listing_marker_state(original,native,a,b)
    if stage in ['classlookup','dictionary','lexer','number','logical','integer','digit','kind','store']:proof['class_lookup']=completed_class_reads(original,native,a,b)
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
    new_buffer=struct.unpack('<9I',bytes.fromhex(globals['rc_source_frames']['bytes'])[:36])[5]
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(requests)==len(results)==len(payloads)==(18 if b['function']=='rc_intern_type'else 16 if b['function']in ['rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else 15)
    assert requests[14][1:]==('9','8206','00000000')and int(results[14][0],16)+8==old_buffer and results[14][1]=='00000000'
    assert requests[14][0]==requests[4][0]and payloads[14]==('8206','00'*8206)
    source=(original/'source'/input_name.rsplit('\\',1)[-1]).read_bytes()
    expected_buffer=b'\0'*8+source+b'\0'*(8198-len(source))if refilled else b'\0'*8206
    assert raw_bytes(text,'source-new-buffer')==expected_buffer
    allocations=[row for row in b['heap']if row['operation']=='allocate'];assert len(allocations)==(18 if b['function']=='rc_intern_type'else 16 if b['function']in ['rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else 15)
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
    assert re.search(r'^#1\s+0x[0-9a-f]+ in '+('rc_store_integer'if b['function']=='rc_intern_type'else'rc_parse_integer_value'if b['function']=='rc_digit_value'else'rc_read_number'if b['function']in ['rc_get_logical_character','rc_parse_integer_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else'rc_next_token'if b['function']=='rc_read_number'else'rc_read_identifier'if b['function']=='rc_character_class'else'rc_handle_directive'if b['function']in ['rc_read_nonspace','rc_read_identifier','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else'rc_scan_line_start'if b['function']=='rc_handle_directive'else'rc_quote_source_filename'if b['function']=='rc_character_token'else'rc_emit_source_line'if b['function']in ['rc_write_preprocessed','rc_quote_source_filename']else'rc_open_source')+r'\s*\(',(native/'gdb.stdout').read_text(),re.M)


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
    assert len(requests)==len(results)==len(payloads)==(18 if b['function']=='rc_intern_type'else 16)and requests[15][0]==requests[14][0]and requests[15][1:]==('9',str(len(expected)),'00000000')and int(results[15][0],16)+8==old_result
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
    assert after==wanted+before[len(wanted):]
    if b['function']not in ['rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:assert globals['rc_macro_name']['bytes']==after.hex()
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
    left=struct.unpack('<8I',raw_bytes(text,'output-write-after-file'));right=struct.unpack('<8I',bytes.fromhex(record['after_file']))
    assert left[2]and right[2]and left[0]-left[2]==right[0]-right[2]==16
    assert left[1:2]+left[3:]==right[1:2]+right[3:]==(4080,138,4,0,4096,0)
    assert bytes.fromhex(record['written_buffer'])==wanted
    if b['function']not in ['rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
        assert raw_bytes(text,'output-write-after-file')==raw_bytes(text,'cpp-output-file')
        assert record['after_file']==globals['rc_cpp_output_file']['file_state']
        assert raw_bytes(text,'cpp-output-buffer')==wanted
        quote=b['arguments']['quote'];assert quote==dict(destination=globals['rc_macro_name']['address'],path=b['arguments']['filename_pointer'],path_bytes=b['arguments']['source_open']['path_bytes'],destination_bytes=globals['rc_macro_name']['bytes'],formal_bytes=dict(destination=4,path=4))
        old_filename=int.from_bytes((original/'globals.bin').read_bytes()[0x149c4-0x14000:0x149c8-0x14000],'little');a=original_raw(original)
        if b['function']!='rc_character_token':assert a['words'][:3]==[0x71c24281,0x71c38128,old_filename]
    return dict(size=16,count=1,result=1,output_written_prefix_bytes=16,FILE=dict(descriptor=4,flags=138,pointer_offset=16,remaining=4080,capacity=4096,os_position=0),unwritten_tail='not initialized or compared; not an application input at this frontier',errors_preserved=True,next_quote=dict(formals=2,stack_cleanup=8,destination='same1024-byte global scratch window',source='current bounded UTF16 filename'))


def write_preprocessed_trial(original,native):
    return _macro_setup_trial(original,native,'write')


def quote_source_state(original,native,a,b):
    text=(original/'events.txt').read_text();globals=b['arguments']['preprocessor']['globals'];quote=b['arguments']['quote'];token=b['arguments']['token']
    assert a['words'][:2]==[0x71c24220,ord('S')]
    old_filename=int.from_bytes((original/'globals.bin').read_bytes()[0x149c4-0x14000:0x149c8-0x14000],'little')
    assert raw_bytes(text,'quote-forwarded')==struct.pack('<2I',0x71c38128,old_filename)
    assert token==dict(character=ord('S'),formal_bytes=2,stack_slot_bytes=4,table_entry=40,quote_cursor=quote['destination']+2,quote_source=quote['path'])
    assert re.findall(r'^quote-position destination=([0-9a-f]+) source=([0-9a-f]+) token=(\d+)$',text,re.M)==[('71c3812a','%08x'%old_filename,'40')]
    assert raw_bytes(text,'quote-source-input')==bytes.fromhex(quote['path_bytes'])
    before=bytes.fromhex(b['arguments']['source_formats'][0]['after']);after=bytearray(before);after[:4]=('"S').encode('utf-16le')
    assert raw_bytes(text,'quote-destination')==bytes.fromhex(quote['destination_bytes'])==bytes(after)
    assert globals['rc_macro_name']['bytes']==after.hex()
    assert len(bytes.fromhex(globals['rc_character_tokens']['bytes']))==512 and struct.unpack_from('<H',bytes.fromhex(globals['rc_character_tokens']['bytes']),ord('S')*2)[0]==40
    assert globals['rc_character_classes[0]']['bytes']=='8000'
    return dict(first_character='S',character_value=83,formal_bytes=2,stack_slot_bytes=4,table_entry=40,initialized_destination_prefix='doublequote then S, all remaining scratch bytes unchanged',destination_bytes=1024,path='same input filename, not yet advanced',source_cursor='input base',destination_cursor='global scratch+oneWCHAR',implicit_inputs='complete identified256-WORD token table and its adjacent class0 alias checked',scope='other class words remain unknown')


def quote_source_filename_trial(original,native):
    return _macro_setup_trial(original,native,'quote')


def buffered_output_prefix(b):
    if b['function']not in ['rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type','rc_read_string','rc_expect_end']:return '#line 1 '.encode('utf-16le')
    path=bytes.fromhex(b['arguments']['source_open']['path_bytes']).decode('utf-16le')
    assert path.endswith('\0')and '\0'not in path[:-1]
    result=('#line 1 "'+path[:-1].replace('\\','\\\\')+'"\r\n').encode('utf-16le')
    if b['function']=='rc_expect_end':
        filename=bytes.fromhex(b['arguments']['filename']).decode('utf-16le').rstrip('\0')
        result+=('#line 1 "'+filename.replace('\\','\\\\')+'"').encode('utf-16le')
    return result


def listing_marker_state(original,native,a,b):
    text=(original/'events.txt').read_text();arguments=b['arguments'];globals=arguments['preprocessor']['globals']
    completed=b['function']in ['rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']
    if completed:
        records=arguments['macro_listings'];assert records==json.loads((native/'macro-listings.json').read_text())and len(records)==1
        marker=records[0]
        calls=re.findall(r'^listing-entry caller=([0-9a-f]+) marker=([0-9a-f]+) esp=([0-9a-f]+) enabled=(\d+)$',text,re.M);assert len(calls)==1
        caller,pointer,sp,enabled=calls[0];pointer=int(pointer,16);sp=int(sp,16)
        assert caller=='71c23f6d'and enabled=='0'
        assert re.findall(r'^listing-return esp=([0-9a-f]+) enabled=(\d+)$',text,re.M)==[('%08x'%(sp+8),'0')]
        assert marker['return_esp']==marker['entry_esp']+8 and marker['enabled_before']==marker['enabled_after']==0
        assert marker['record']==marker['after']and raw_bytes(text,'listing-record-before')==raw_bytes(text,'listing-record-after')
        assert arguments['scan_line']==dict(formals=[],cursor=int.from_bytes(bytes.fromhex(globals['rc_source_cursor']['bytes']),'little'),current_line=1 if b['function']in ['rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else 0)
        left=raw_bytes(text,'listing-record-before');name=raw_bytes(text,'listing-name');value=raw_bytes(text,'listing-text')
    else:
        marker=arguments['marker'];pointer=a['words'][1]
        assert re.findall(r'^source-marker pointer=([0-9a-f]+) parent-bp=([0-9a-f]+)$',text,re.M)==[('%08x'%pointer,'%08x'%a['registers']['ebp'])]
        assert marker['frame_pointer']-128<marker['pointer']<marker['frame_pointer']
        left=raw_bytes(text,'source-marker-record');name=raw_bytes(text,'source-marker-name');value=raw_bytes(text,'source-marker-text')
    source_bp=a['words'][4]if b['function']=='rc_character_class'else int(re.findall(r'^source-owner-bp ([0-9a-f]+)$',text,re.M)[0],16)if b['function']in ['rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else a['words'][3]if b['function']=='rc_next_token'else a['registers']['ebp']
    assert pointer==source_bp-16
    if b['function']in ['rc_character_class','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:assert re.findall(r'^source-owner-bp ([0-9a-f]+)$',text,re.M)==['%08x'%source_bp]
    assert marker['pointer']==marker['object']and marker['object_bytes']==16 and marker['formal_bytes']==4 and marker['parent']=='rc_open_source'
    right=bytes.fromhex(marker['record']);assert len(left)==len(right)==16
    assert struct.unpack_from('<III',left)==(0,0x71c2124c,0x71c36cf8)
    assert struct.unpack_from('<III',right)==(0,marker['name'],marker['text'])
    assert left[12:14]==right[12:14]==b'\0\0'
    assert marker['text']==marker['marker_global']and marker['marker_global_bytes']==2
    assert name==bytes.fromhex(marker['name_bytes'])=='!\0'.encode('utf-16le')
    assert value==bytes.fromhex(marker['text_bytes'])==b'\0\0'
    assert globals['rc_macro_listing_enabled']['bytes']=='00000000'
    if b['function']in ['rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
        assert arguments['directive']==dict(formals=[],scanner_character=ord('#'),scanner_character_bytes=2)
        if b['function']in ['rc_read_nonspace','rc_read_identifier','rc_character_class']:
            if b['function']=='rc_read_nonspace':assert arguments['nonspace']==dict(formals=[])
            else:
                if b['function']=='rc_read_identifier':assert a['words'][1]==ord('l')
                else:assert a['words'][7]==ord('l')and a['words'][6]==0x71c263b6
                assert arguments['identifier']==dict(character=ord('l'),formal_bytes=2,stack_slot_bytes=4,parent_character=ord('l'))
            assert globals['rc_expanding_macro']['bytes']==globals['rc_in_directive']['bytes']=='00000000'
        frame=struct.unpack('<9I',bytes.fromhex(globals['rc_source_frames']['bytes'])[:36])
        assert arguments['scan_line']['cursor']==frame[5]+(14 if b['function']in ['rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else 10 if b['function']in ['rc_directive_code','rc_next_token']else 6 if b['function']=='rc_character_class'else 4 if b['function']=='rc_read_identifier'else 2)
        assert globals['rc_cpp_active']['bytes']==('00000000'if b['function']in ['rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']else'01000000')
        assert bytes.fromhex(globals['rc_character_tokens']['bytes'])[ord('#')*2:ord('#')*2+2]==b'\x0b\x00'and arguments['current_line']==1
    path=bytes.fromhex(arguments['source_open']['path_bytes']).decode('utf-16le')[:-1]
    quoted=('"'+path.replace('\\','\\\\')+'"').encode('utf-16le')
    before=bytes.fromhex(arguments['source_formats'][0]['after']);wanted=quoted+before[len(quoted):]
    if b['function']=='rc_character_class':
        partial=bytearray(wanted);partial[:4]='li'.encode('utf-16le');wanted=bytes(partial)
        assert a['words'][1]&0xffff==ord('i')and a['registers']['ebp']==0x71c3812a and a['registers']['ebx']==76
        assert arguments['class']==dict(character=ord('i'),formal_bytes=2,stack_slot_bytes=4,table_entry=16,identifier_cursor=globals['rc_macro_name']['address']+2,hash=76)
        table=bytes.fromhex(globals['rc_character_classes']['bytes']);assert len(table)==512 and struct.unpack_from('<H',table,ord('i')*2)[0]==16
        assert struct.unpack_from('<H',table,ord('$')*2)[0]==16
    if b['function']in ['rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
        complete=bytearray(wanted);complete[:10]='line\0'.encode('utf-16le');wanted=bytes(complete)
        if b['function']in ['rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
            changed=bytearray(wanted);changed[:2]='1'.encode('utf-16le');wanted=bytes(changed)
            if b['function']in ['rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                changed[:4]='1\0'.encode('utf-16le');wanted=bytes(changed)
    assert len(wanted)==1024 and globals['rc_macro_name']['bytes']==wanted.hex()
    prefix=buffered_output_prefix(b)
    assert raw_bytes(text,'cpp-output-buffer')==prefix
    assert bytes.fromhex(globals['rc_cpp_output_file']['file_buffer'])[:len(prefix)]==prefix
    return dict(next_formal=dict(type='RcMacro pointer',bytes=4,object_bytes=16,owner='actual rc_open_source local',next=None,name='!',text='same mutable empty WCHAR global',parameters=0,flags=0,reserved='two raw uninitialized bytes retained independently, not compared or read'),hidden_inputs=dict(macro_listing_enabled=0,source_depth=0,current_line=arguments['current_line']),completed_quote=dict(ABI='unsigned16 input/return, stack slot4/ret4',path=path,backslashes_doubled=True,scratch_bytes=1024,quote_bytes=len(quoted),appended_NUL=False,output_initialized_prefix_bytes=len(prefix),output_file_position=0),scope='complete identified token table plus class0 alias; other class words unknown')


def character_token_lookup_trial(original,native):
    return _macro_setup_trial(original,native,'token')


def list_macro_trial(original,native):
    return _macro_setup_trial(original,native,'listing')


def scan_line_start_trial(original,native):
    return _macro_setup_trial(original,native,'scanline')


def handle_directive_trial(original,native):
    return _macro_setup_trial(original,native,'directive')


def read_nonspace_trial(original,native):
    return _macro_setup_trial(original,native,'nonspace')


def read_identifier_trial(original,native):
    return _macro_setup_trial(original,native,'identifier')


def completed_class_reads(original,native,a,b):
    text=(original/'events.txt').read_text();arguments=b['arguments'];globals=arguments['preprocessor']['globals']
    records=arguments['class_reads'];assert records==json.loads((native/'class-reads.json').read_text())and len(records)==4
    calls=re.findall(r'^class-read-call (\d+) character=(\d+) raw=([0-9a-f]+) table=(\d+) hash=(\d+) cursor=([0-9a-f]+) first=(\d+) esp=([0-9a-f]+)$',text,re.M)
    returns=re.findall(r'^class-read-return (\d+) result=(\d+) raw=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(calls)==len(returns)==4
    observations=[]
    for index,(call,returned,new,character,value,hash_value)in enumerate(zip(calls,returns,records,[105,110,101,32],[16,16,24,64],[76,149,227,296])):
        number,ch,raw,entry,hash_before,cursor,first,sp=call
        assert int(number)==int(returned[0])==index and int(ch)==int(raw,16)&65535==new['character']==character
        assert int(entry)==new['table_entry']==new['result']==int(returned[1])==int(returned[2],16)&65535==new['raw_eax']&65535==value
        assert int(hash_before)==new['hash']==hash_value and int(first)==new['identifier_first']==108
        assert int(cursor,16)==0x71c3812a+index*2 and new['identifier_cursor']==globals['rc_macro_name']['address']+2+index*2
        assert int(returned[3],16)==int(sp,16)+4 and new['return_esp']==new['entry_esp']+8
        assert new['formal_bytes']==2 and new['stack_slot_bytes']==4
        observations.append(dict(character=character,class_value=value,hash_before=hash_value,argument_and_return_bytes=2,stack_slot_bytes=4,cleanup=4))
    if b['function']=='rc_directive_code':
        assert arguments['directive_word']==dict(pointer=globals['rc_macro_name']['address'],bytes='line\0'.encode('utf-16le').hex(),formal_bytes=4)
        assert a['words'][1]==0x71c38128
    assert globals['rc_macro_name_length']['bytes']=='05000000'and globals['rc_macro_hash']['bytes']=='28'
    assert globals['rc_expanding_macro']['bytes']=='00000000'and globals['rc_in_directive']['bytes']=='01000000'
    return dict(calls=observations,complete_identifier='line',count_including_NUL=5,hash=40,cursor_offset=10,in_directive=1,next_formal=dict(type='const WCHAR pointer',bytes=4,cleanup=4,contents='line',owner='same1024-byte global scratch'),return_scope='Only AX is the16-bit return; raw upper EAX bits differ and are not a value.',boundary_scope='Native source INT3 holds index256 before unavailable read; Win32 boundary-word/keyword alias remains unclaimed. No universal alias claim.')


def character_class_lookup_trial(original,native):
    return _macro_setup_trial(original,native,'classlookup')


def _directive_lookup_trial(original,native,stage):
    proof=_macro_setup_trial(original,native,stage)
    a=original_raw(original);b=native_raw(native);text=(original/'events.txt').read_text()
    arguments=b['arguments'];globals=arguments['preprocessor']['globals']
    rows=arguments['directive_lookups'];assert rows==json.loads((native/'directive-lookups.json').read_text())and len(rows)==1
    row=rows[0]
    assert row['pointer']==globals['rc_macro_name']['address']and row['bytes']=='line\0'.encode('utf-16le').hex()and row['formal_bytes']==4
    assert row['result']==row['raw_eax']&255==10 and row['return_esp']==row['entry_esp']+8
    calls=re.findall(r'^directive-lookup-entry caller=([0-9a-f]+) word=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(calls)==1
    caller,pointer,sp=calls[0];assert caller=='71c263cd'and pointer=='71c38128'
    returned=re.findall(r'^directive-lookup-return value=(\d+) raw=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(returned)==1
    result,raw,after=returned[0];assert int(result)==int(raw,16)&255==10 and int(after,16)==int(sp,16)+8
    assert raw_bytes(text,'directive-lookup-word')==bytes.fromhex(row['bytes'])
    assert arguments['tokenizer']==dict(formals=[],previous_kind=1,previous_value=0,header_mode=0,condition_mode=0)
    assert globals['rc_token_kind']['bytes']=='01'and globals['rc_token_value']['bytes']==globals['rc_header_mode']['bytes']==globals['rc_in_condition_expression']['bytes']==globals['rc_cpp_active']['bytes']=='00000000'
    proof['directive_lookup']=dict(word='line',formal_bytes=4,return_bytes=1,code=10,cleanup=4,owner='same1024-byte global scratch',next_formals=[],hidden_inputs=arguments['tokenizer'],return_scope='Only AL is the unsigned8 result; upper EAX is unclaimed.',data_scope='29 group bytes,13 bounded suffixes/result bytes; native source holds first character above z before unavailable read.')
    return proof


def directive_code_trial(original,native):
    return _directive_lookup_trial(original,native,'dictionary')


def next_token_trial(original,native):
    proof=_directive_lookup_trial(original,native,'lexer')
    a=original_raw(original);b=native_raw(native);args=b['arguments'];globals=args['preprocessor']['globals']
    assert a['words'][1]&65535==49
    assert args['numeric_token']==dict(first=49,formal_bytes=2,stack_slot_bytes=4,parent_character=49,parent_class=29,buffer_active=0)
    assert globals['rc_number_buffer_active']['bytes']=='00000000'
    proof['numeric_token']=dict(first=49,formal_bytes=2,stack_slot_bytes=4,cleanup=4,parent_class=29,cursor_offset=14,hidden_inputs=dict(buffer_active=0,alternate_buffer='not read while disabled',token_kind=1,token_value=None,output_active=0,source_encoding=2,source_depth=0,source_file='verified prepared UTF16FILE'),descriptor_scope='44 named pointer+BYTE kind rows verified statically; unconsumed keywords/uninitialized padding unclaimed. Native variable lookup holds unavailable rows. No lexer return yet; next callee remains trapped.')
    return proof


def source_cursor_word(original,b):
    text=(original/'events.txt').read_text();data=(original/'globals.bin').read_bytes()
    old_cursor=int.from_bytes(data[0x18120-0x14000:0x18124-0x14000],'little')
    old_buffer=struct.unpack_from('<9I',data,0x18e80-0x14000)[5]
    globals=b['arguments']['preprocessor']['globals'];new_cursor=int.from_bytes(bytes.fromhex(globals['rc_source_cursor']['bytes']),'little');new_buffer=struct.unpack_from('<9I',bytes.fromhex(globals['rc_source_frames']['bytes']))[5]
    block=next(row for row in b['heap_identity']['blocks']if row['payload']+8==new_buffer)
    old=raw_bytes(text,'source-new-buffer');new=bytes.fromhex(block['contents'])
    old_at=old_cursor-old_buffer+8;new_at=new_cursor-new_buffer+8
    assert 8<=old_at<len(old)-1 and 8<=new_at<len(new)-1
    assert old[old_at:old_at+2]==new[new_at:new_at+2]
    return int.from_bytes(old[old_at:old_at+2],'little')


def read_number_trial(original,native):
    proof=_directive_lookup_trial(original,native,'number')
    a=original_raw(original);b=native_raw(native);args=b['arguments'];globals=args['preprocessor']['globals'];text=(original/'events.txt').read_text()
    assert args['numeric_token']==dict(first=49,formal_bytes=2,stack_slot_bytes=4,parent_character=49,parent_class=29,buffer_active=0)
    assert args['logical_character']==dict(formals=[],begin=globals['rc_macro_name']['address'],cursor=globals['rc_macro_name']['address']+2,radix=10,token=7)
    assert re.findall(r'^number-parser-state first=(\d+) radix=(\d+) token=(\d+) begin=([0-9a-f]+) cursor=([0-9a-f]+)$',text,re.M)==[('49','10','7','71c38128','71c3812a')]
    assert globals['rc_number_buffer_active']['bytes']=='00000000'
    word=source_cursor_word(original,b)
    proof['number_parser']=dict(first=49,first_formal_bytes=2,stack_slot_bytes=4,radix=10,provisional_token=7,scratch_initialized_word=49,cursor_offset=2,next_formals=[],next_hidden_inputs=dict(source_cursor_offset=14,current_word=word,source_depth=0,alternate_mode=0),alternate_buffer_scope='Address identity only: distinct original17708/global native anchor, equality/copy only before source guard; no pointee bytes/capacity/layout or data extent credit.',return_scope='AL unsigned8; no number-parser return reached yet.')
    return proof


def _logical_character_trial(original,native,stage):
    proof=_directive_lookup_trial(original,native,stage)
    a=original_raw(original);b=native_raw(native);args=b['arguments'];globals=args['preprocessor']['globals'];text=(original/'events.txt').read_text()
    word=source_cursor_word(original,b)
    assert args['numeric_token']==dict(first=word,formal_bytes=2,stack_slot_bytes=4,parent_character=49,parent_class=29,buffer_active=0)
    assert args['number_state']==dict(begin=globals['rc_macro_name']['address'],cursor=globals['rc_macro_name']['address']+2,radix=10,token=7)
    assert re.findall(r'^number-parser-state first=(\d+) radix=(\d+) token=(\d+) begin=([0-9a-f]+) cursor=([0-9a-f]+)$',text,re.M)==[(str(word),'10','7','71c38128','71c3812a')]
    if b['function']=='rc_parse_integer_value':assert a['words'][1:3]==[0x71c38128,10]
    assert args['integer_conversion']==dict(text=globals['rc_macro_name']['address'],text_bytes='1\0'.encode('utf-16le').hex(),radix=10,formal_bytes=dict(text=4,radix=4))
    records=args['logical_reads'];assert records==json.loads((native/'logical-reads.json').read_text())and len(records)==1;row=records[0]
    calls=re.findall(r'^logical-character-entry caller=([0-9a-f]+) cursor=([0-9a-f]+) word=(\d+) esp=([0-9a-f]+)$',text,re.M);returns=re.findall(r'^logical-character-return value=(\d+) raw=([0-9a-f]+) cursor=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(calls)==len(returns)==1
    caller,cursor,raw_word,sp=calls[0];value,raw,after,returned_sp=returns[0]
    assert caller=='71c32296'and int(raw_word)==int(value)==int(raw,16)&65535==row['word']==row['result']==row['raw_eax']&65535==word
    assert row['formals']==[]and row['depth']==row['alternate_mode']==0
    original_cursor=int.from_bytes((original/'globals.bin').read_bytes()[0x18120-0x14000:0x18124-0x14000],'little')
    assert int(cursor,16)==original_cursor and int(after,16)==original_cursor+2 and int(returned_sp,16)==int(sp,16)+4
    assert row['cursor_before']==args['scan_line']['cursor']and row['cursor_after']==row['cursor_before']+2 and row['return_esp']==row['entry_esp']+4
    proof['logical_character']=dict(formals=[],character=word,return_bytes=2,cleanup=0,source_cursor_advance_bytes=2,caller_rewinds_bytes=2,source_depth=0,alternate_mode=0,return_scope='OnlyAX; upperEAX unclaimed',next_formals=dict(text='1',text_pointer='same1024-byte global scratch',radix=10,stack_bytes=8),scratch='initialized1/NUL then retained independent tail',scope='CR/source-boundary/splice paths remain behind existing named helpers; actual call returns the source quote character.')
    return proof


def logical_character_trial(original,native):
    return _logical_character_trial(original,native,'logical')


def _integer_value_trial(original,native,stage):
    proof=_logical_character_trial(original,native,stage)
    a=original_raw(original);b=native_raw(native);args=b['arguments'];globals=args['preprocessor']['globals'];text=(original/'events.txt').read_text()
    if b['function']=='rc_digit_value':assert a['words'][1]==49
    assert args['digit']==dict(character=49,formal_bytes=4,parent_cursor=globals['rc_macro_name']['address'],accumulator=0,previous=0)
    assert re.findall(r'^integer-conversion-state cursor=([0-9a-f]+) accumulator=(\d+) previous=(\d+) radix=(\d+)$',text,re.M)==[('71c38128','0','0','10')]
    assert raw_bytes(text,'integer-conversion-input')=='1\0'.encode('utf-16le')
    proof['integer_conversion']=dict(text='1',radix=10,accumulator=0,previous=0,cursor_offset=0,next_formal=dict(character=49,type='signed32 int',bytes=4,cleanup=4),hidden_inputs='Actual complete contextual character class table and source/CRT state checked; no hidden register argument identified.',scope='First digit conversion is still trapped; integer overflow/diagnostic branches are retained behind named helpers and unclaimed message resources.')
    return proof


def integer_value_trial(original,native):
    return _integer_value_trial(original,native,'integer')


def _digit_value_trial(original,native,stage):
    proof=_integer_value_trial(original,native,stage)
    a=original_raw(original);b=native_raw(native);args=b['arguments'];text=(original/'events.txt').read_text()
    digits=args['digit_values'];integers=args['integer_returns']
    assert digits==json.loads((native/'digit-values.json').read_text())and integers==json.loads((native/'integer-conversions.json').read_text())and len(digits)==len(integers)==1
    digit=digits[0];integer=integers[0]
    assert {key:digit[key]for key in ['character','formal_bytes','parent_cursor','accumulator','previous']}==args['digit']
    assert {key:integer[key]for key in ['text','text_bytes','radix','formal_bytes']}==args['integer_conversion']
    assert digit['result']==integer['result']==1 and digit['return_esp']==digit['entry_esp']+8 and integer['return_esp']==integer['entry_esp']+12
    dc=re.findall(r'^digit-value-entry character=(\d+) esp=([0-9a-f]+)$',text,re.M);dr=re.findall(r'^digit-value-return value=(\d+) esp=([0-9a-f]+)$',text,re.M)
    ic=re.findall(r'^integer-entry text=([0-9a-f]+) radix=(\d+) esp=([0-9a-f]+)$',text,re.M);ir=re.findall(r'^integer-return value=(\d+) esp=([0-9a-f]+)$',text,re.M)
    assert len(dc)==len(dr)==len(ic)==len(ir)==1 and dc[0][0]=='49'and dr[0][0]==ir[0][0]=='1'
    assert int(dr[0][1],16)==int(dc[0][1],16)+8 and int(ir[0][1],16)==int(ic[0][2],16)+12
    assert ic[0][:2]==('71c38128','10')and raw_bytes(text,'integer-conversion-entry-input')=='1\0'.encode('utf-16le')
    if b['function']=='rc_decimal_integer_kind':assert a['words'][1]==1
    assert args['integer_kind']==dict(value=1,formal_bytes=4,parent_value=1,parent_object_bytes=8,initialized_bytes=4)
    assert re.findall(r'^number-value value=(\d+) storage=([0-9a-f]+) initialized=(\d+)$',text,re.M)==[('1','%08x'%((int(re.findall(r'^store-frontier .* number-bp=([0-9a-f]+)$',text,re.M)[0],16)if b['function']=='rc_intern_type'else a['registers']['ebp'])-20),'4')]
    proof['digit_value']=dict(character=49,result=1,argument_bytes=4,return_bytes=4,cleanup=4,integer_result=1,integer_cleanup=8,next_formal=dict(type='signed32 int',value=1,bytes=4,cleanup=4),parent_storage=dict(object_bytes=8,initialized_bytes=4,other4bytes='uninitialized and unclaimed, not read by this boundary'))
    return proof


def digit_value_trial(original,native):
    return _digit_value_trial(original,native,'digit')


def _decimal_kind_trial(original,native,stage):
    proof=_digit_value_trial(original,native,stage)
    a=original_raw(original);b=native_raw(native);args=b['arguments'];text=(original/'events.txt').read_text()
    kinds=args['integer_kinds'];assert kinds==json.loads((native/'integer-kinds.json').read_text())and len(kinds)==1;row=kinds[0]
    assert row['value']==1 and row['formal_bytes']==4 and row['result']==row['raw_eax']&255==7 and row['return_esp']==row['entry_esp']+8
    calls=re.findall(r'^integer-kind-entry value=(\d+) esp=([0-9a-f]+)$',text,re.M);returns=re.findall(r'^integer-kind-return value=(\d+) raw=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M)
    assert len(calls)==len(returns)==1 and calls[0][0]=='1'and returns[0][0]=='7'and int(returns[0][1],16)&255==7
    assert int(returns[0][2],16)==int(calls[0][1],16)+8
    store=args['store'];assert store['formal_bytes']==dict(token=4,value=4)
    if b['function']=='rc_intern_type':
        old_store=re.findall(r'^store-entry token=([0-9a-f]+) value=([0-9a-f]+) number-bp=([0-9a-f]+) cursor=([0-9a-f]+)$',text,re.M);assert len(old_store)==1
        old_token,old_value,number_bp,_=[int(v,16)for v in old_store[0]]
        assert raw_bytes(text,'store-integer-entry-input')==b'\x01\0\0\0'
    else:old_token,old_value,number_bp=a['words'][1],a['words'][2],a['registers']['ebp']
    assert old_token&255==store['token']&255==7
    assert old_value==number_bp-20
    assert store['value']==store['owner']and store['object_bytes']==8 and store['initialized_bytes']==4
    assert raw_bytes(text,'store-integer-input')==bytes.fromhex(store['payload'])==b'\x01\0\0\0'
    proof['decimal_kind']=dict(value=1,result=7,formal_bytes=4,return_bytes=1,cleanup=4,return_scope='OnlyAL is the result; upperEAX unclaimed',next_formals=dict(token_slot_bytes=4,token_consumed_bits=8,token_value=7,original_raw_token=old_token,native_raw_token=store['token'],raw_token_scope='Both full DWORDs retained independently. Numeric codes4..10 overwrite type before raw fallback; upper24 caller spill bits are unclaimed and not seeded/normalized.',value_pointer_bytes=4,pointee_initialized_bytes=4,pointee_value=1,owner_bytes=8,other4bytes='uninitialized/unclaimed, not read by this boundary'),storage_scope='Store function still traps. Raw unsigned32 formal preserves fallback input; future Win32 body must preserve full original fallback, with native hold outside characterized code range.')
    return proof


def decimal_kind_trial(original,native):
    return _decimal_kind_trial(original,native,'kind')


def store_integer_trial(original,native):
    proof=_decimal_kind_trial(original,native,'store')
    a=original_raw(original);b=native_raw(native);args=b['arguments'];text=(original/'events.txt').read_text()
    old=re.findall(r'^store-frontier token=([0-9a-f]+) value=([0-9a-f]+) node=([0-9a-f]+) type=([0-9a-f]+) type-code=([0-9a-f]+) number-bp=([0-9a-f]+)$',text,re.M);assert len(old)==1
    token,value,node,type_pointer,type_code,number_bp=[int(v,16)for v in old[0]]
    front=args['intern_type'];assert front['formal_bytes']==4 and front['object_bytes']==20 and front['pointer']==front['parent_type']and front['parent']=='rc_store_integer'
    assert a['words'][1]==type_pointer and token&255==args['store']['token']&255==7 and value==number_bp-20 and type_code==front['type_code']|0x1000==0x1003
    node_bytes=bytes.fromhex('0701')+b'\0'*10+b'\x01'+b'\0'*7
    type_bytes=b'\x03\x10'+b'\0'*18
    assert raw_bytes(text,'integer-node')==b'\0'*8+bytes.fromhex(front['node_bytes'])==b'\0'*8+node_bytes
    assert raw_bytes(text,'integer-type')==b'\0'*8+bytes.fromhex(front['payload'])==b'\0'*8+type_bytes
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M);results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M);payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    allocations=[row for row in b['heap']if row['operation']=='allocate'];assert len(requests)==len(results)==len(payloads)==len(allocations)==18
    for index,old_pointer,new_pointer,contents in [(16,node,front['node'],node_bytes),(17,type_pointer,front['pointer'],type_bytes)]:
        assert requests[index][0]==requests[0][0]and requests[index][1:]==('9','28','00000000')and results[index]==('%08x'%(old_pointer-8),'00000000')and payloads[index]==('28','00'*28)
        row=allocations[index];assert row['heap']==b['heap_identity']['handle']and row['flags']==9 and row['bytes']==28 and row['result']+8==new_pointer and row['payload']=='00'*28 and row['last_error_before']==row['last_error_after']==0
        block=next(row for row in b['heap_identity']['blocks']if row['payload']+8==new_pointer)
        assert block['owner']==b['heap_identity']['handle']and block['flags']==9 and block['bytes']==28 and bytes.fromhex(block['contents'])==b'\0'*8+contents
    buckets=args['type_buckets'];assert buckets['count']==256 and buckets['bytes']==1024 and buckets['pointer_bytes']==4 and buckets['address']>0
    old_table=(original/'globals.bin').read_bytes()[0x22830-0x14000:0x22c30-0x14000]
    assert old_table==bytes.fromhex(buckets['payload'])==b'\0'*1024
    pe=(ROOT/'toolchains/5.10/bin/RCDLL.DLL').read_bytes();pe_at=struct.unpack_from('<I',pe,60)[0];section_count=struct.unpack_from('<H',pe,pe_at+6)[0];optional_bytes=struct.unpack_from('<H',pe,pe_at+20)[0]
    sections=[struct.unpack_from('<8sIIII',pe,pe_at+24+optional_bytes+40*i)for i in range(section_count)]
    assert any(name.rstrip(b'\0')==b'.data'and rva+raw_length<=0x22830<0x22c30<=rva+virtual_bytes for name,virtual_bytes,rva,raw_length,raw_offset in sections),'bucket defaults must be inside actual loader-zeroed PE data'
    proof['integer_storage']=dict(physical_token_bytes=4,consumed_token_bits=8,token=7,value=1,node_bytes=20,type_bytes=20,private_headers=8,node_fields=dict(token=7,flags=1,type_pointer=None,value=1),type_flags=0x1003,initialization='Both actual28-byte HeapAlloc9 results are entirely zero; target writes token/flags/value and type WORD, with untouched bytes checked.',next_formal=dict(type='RcIntegerType pointer',bytes=4,object_bytes=20,cleanup=4),hidden_inputs=dict(type_bucket_count=256,pointer_bytes=4,all_initial_buckets_null=True,extent='original22830..22c30, loader-zeroed virtual .data'),scope='Type interner remains trapped; no completed token/type return yet. Native source traps before unsupported raw-token fallback; no upper caller-spill initialization claim.')
    return proof


def _intern_type_trial(original,native,stage):
    trial,accepted,frontier,caller=('rc_intern_type',54,'rc_read_string',0x71c23a8d)if stage=='intern'else('rc_read_string',55,'rc_expect_end',0x71c26a6a)
    config=read_configuration(original/'functions.json');assert config.get('trial_function')==trial and len(config['accepted'])==accepted
    a,b,proof=checkpoint(original,native,frontier,('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==caller
    assert re.search(r'^#1\s+0x[0-9a-f]+ in '+('rc_next_token'if stage=='intern'else'rc_handle_directive')+r'\s*\(',(native/'gdb.stdout').read_text(),re.M)
    assert proof['application_trace'][-1]==trial
    text=(original/'events.txt').read_text();args=b['arguments'];globals=args['preprocessor']['globals'];old_globals=(original/'globals.bin').read_bytes()
    input_name=raw_bytes(text,'input-temporary')[:-1].decode('ascii')
    proof.update(wide_argument_state(original,native,a,b,filename=input_name if stage=='intern'else'resources.rc',extra_blocks=14,output_open=True))
    rows=args['interned_types'];assert rows==json.loads((native/'interned-types.json').read_text())and len(rows)==1;row=rows[0]
    calls=re.findall(r'^intern-entry type=([0-9a-f]+) node=([0-9a-f]+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returns=re.findall(r'^intern-return result=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(calls)==len(returns)==1
    old_type,old_node,old_sp,old_caller=[int(v,16)for v in calls[0]];old_result,old_after=[int(v,16)for v in returns[0]]
    assert old_caller==0x71c27437 and old_after==old_sp+8 and row['return_esp']==row['entry_esp']+8 and row['formal_bytes']==4 and row['object_bytes']==20 and row['parent']=='rc_store_integer'
    type_bytes=b'\x03\x10'+b'\0'*18;node_before=b'\x07\x01'+b'\0'*10+b'\x01'+b'\0'*7
    assert raw_bytes(text,'intern-type-input')==bytes.fromhex(row['payload'])==type_bytes
    assert raw_bytes(text,'intern-node-input')==bytes.fromhex(row['node_bytes'])==node_before
    assert raw_bytes(text,'intern-buckets-before')==bytes.fromhex(row['buckets_before'])==b'\0'*1024 and row['buckets_bytes']==1024 and row['buckets_address']>0
    entry_bytes=b'\0'*12+type_bytes
    assert raw_bytes(text,'intern-result-block')==raw_bytes(text,'intern-current-entry')==bytes.fromhex(row['result_block'])==entry_bytes
    old_buckets=raw_bytes(text,'intern-buckets-after');new_buckets=bytes.fromhex(row['buckets_after']);assert len(old_buckets)==len(new_buckets)==1024
    old_expected=bytearray(1024);new_expected=bytearray(1024);struct.pack_into('<I',old_expected,12,old_result-4);struct.pack_into('<I',new_expected,12,row['result']-4)
    assert old_buckets==bytes(old_expected)and new_buckets==bytes(new_expected)
    assert old_globals[0x22830-0x14000:0x22c30-0x14000]==old_buckets
    buckets=args['type_buckets'];assert buckets['count']==256 and buckets['bytes']==1024 and buckets['pointer_bytes']==4 and buckets['payload']==row['buckets_after']and buckets['address']==row['buckets_address']
    allocations=[v for v in b['heap']if v['operation']=='allocate'];requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M);results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M);payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(allocations)==len(requests)==len(results)==len(payloads)==19
    assert requests[18][0]==requests[0][0]and requests[18][1:]==('9','32','00000000')and results[18]==('%08x'%(old_result-12),'00000000')and payloads[18]==('32','00'*32)
    identity=b['heap_identity'];last=allocations[18];assert last['heap']==identity['handle']and last['flags']==9 and last['bytes']==32 and last['result']+12==row['result']and last['payload']=='00'*32 and last['last_error_before']==last['last_error_after']==0
    for pointer,size,payload in [(row['result']-4,32,entry_bytes),(row['pointer'],28,b'\0'*8+type_bytes)]:
        block=next(v for v in identity['blocks']if v['payload']+8==pointer);assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==size and bytes.fromhex(block['contents'])==payload
    assert int(results[16][0],16)+8==old_node and int(results[17][0],16)+8==old_type
    assert allocations[16]['result']+8==row['node']and allocations[17]['result']+8==row['pointer']
    current=bytearray(node_before);struct.pack_into('<I',current,4,row['result']);old_current=bytearray(node_before);struct.pack_into('<I',old_current,4,old_result)
    assert raw_bytes(text,'intern-current-node')==b'\0'*8+bytes(old_current)
    node=args['integer_node'];assert node['pointer']==row['node']and bytes.fromhex(node['payload'])==bytes(current)
    block=next(v for v in identity['blocks']if v['payload']+8==node['pointer']);assert block['owner']==identity['handle']and block['flags']==9 and block['bytes']==28 and bytes.fromhex(block['contents'])==b'\0'*8+bytes(current)
    reader=args['reader'];assert reader['formals']==[]and reader['caller']=='rc_next_token'and reader['token_kind']==7 and reader['token_value']==node['pointer']and reader['header_mode']==reader['output_active']==0 and reader['in_directive']==1
    if stage=='intern':
        assert old_globals[0x144f4-0x14000]==int.from_bytes(bytes.fromhex(globals['rc_token_kind']['bytes']),'little')==7
        assert struct.unpack_from('<I',old_globals,0x16cd0-0x14000)[0]==old_node and int.from_bytes(bytes.fromhex(globals['rc_token_value']['bytes']),'little')==node['pointer']
    for name,rva,value in [('rc_header_mode',0x16efc,0),('rc_cpp_active',0x1811c,0 if stage=='intern'else 1),('rc_in_directive',0x18530,1)]:
        assert struct.unpack_from('<I',old_globals,rva-0x14000)[0]==int.from_bytes(bytes.fromhex(globals[name]['bytes']),'little')==value
    old_frame=struct.unpack_from('<9I',old_globals,0x18e80-0x14000);new_frame=struct.unpack('<9I',bytes.fromhex(globals['rc_source_frames']['bytes'])[:36])
    old_cursor=struct.unpack_from('<I',old_globals,0x18120-0x14000)[0];new_cursor=int.from_bytes(bytes.fromhex(globals['rc_source_cursor']['bytes']),'little')
    if stage!='intern':
        entries=re.findall(r'^string-entry cursor=([0-9a-f]+) word=(\d+) token=([0-9a-f]+) header=(\d+) active=(\d+) directive=(\d+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);assert len(entries)==1
        old_cursor=int(entries[0][0],16);new_cursor=reader['cursor']
        assert int(entries[0][2],16)==old_node and entries[0][3:6]==('0','0','1')and entries[0][7]=='71c23a8d'
    assert old_cursor==old_frame[5]+16 and new_cursor==new_frame[5]+16==reader['cursor']
    source=raw_bytes(text,'string-source-block');new_source=next(v for v in identity['blocks']if v['payload']+8==new_frame[5]);assert new_source['owner']==identity['handle']and new_source['flags']==9 and new_source['bytes']==8206
    assert source==bytes.fromhex(new_source['contents'])and len(source)==8206
    word=struct.unpack_from('<H',source,8+old_cursor-old_frame[5])[0];assert word==reader['current_word']
    assert old_frame[0]==new_frame[0]==4098 and old_frame[7]==new_frame[7]==2
    file=struct.unpack('<8I',raw_bytes(text,'cpp-source-file'));new_file=struct.unpack('<8I',bytes.fromhex(globals['rc_source_file']['file_state']))
    assert file[0]==file[2]and new_file[0]==new_file[2]and file[1:2]+file[3:]==new_file[1:2]+new_file[3:]==(0,25,5,0,512,0)
    assert raw_bytes(text,'cpp-source-buffer')==bytes.fromhex(globals['rc_source_file']['file_buffer'])and len(bytes.fromhex(globals['rc_source_file']['file_buffer']))==512
    assert re.findall(r'^cpp-source-position (\d+)$',text,re.M)==['6762']and re.search(r'^pos:\s*6762$',args['source_fdinfo'],re.M)
    assert b['crt_errors']==dict(errno=0,last_error=0)
    proof['type_interning']=dict(input_bytes=20,argument_bytes=4,cleanup=4,result_alias='owned24-byte entry.type at offset4; current integer node.type and returned type are identical',bucket_count=256,hash=3,before_all_null=True,changed_buckets=[3],entry_bytes=24,heap_header_bytes=8,allocation_bytes=32,defined_payload='Full allocation initially zero; next null, copied20-byte type and cleared WORD at type+16; all bytes/tails exact.',next_formals=[],next_hidden_inputs=dict(source_cursor_offset=16,current_word=word,header_mode=0,output_active=0,in_directive=1,token_kind=7,token_value='actual owned integer node7/value1/type alias'),retired_frame_scope='Previous number-parser stack is no longer live; its raw proof remains in integer-storage-verification.json. No stale stack contents asserted.',comparison_scope='No list comparison call occurs in this first empty-bucket insertion; named comparator remains trapped.')
    proof['trial']=trial
    return proof


def intern_type_trial(original,native):
    return _intern_type_trial(original,native,'intern')


def read_string_trial(original,native):
    proof=_intern_type_trial(original,native,'string')
    a=original_raw(original);b=native_raw(native);args=b['arguments'];text=(original/'events.txt').read_text();old_globals=(original/'globals.bin').read_bytes();globals=args['preprocessor']['globals']
    reads=args['string_reads'];assert reads==json.loads((native/'string-reads.json').read_text())and len(reads)==1;row=reads[0]
    assert row['layout']==dict(buffer_bytes=4090,element_bytes=2,buffer_alignment=0,number_offset=2048,extra_bytes=2,extra_alignment=0,hash_offset=0,length_offset=1,string_length_offset=0)
    before=raw_bytes(text,'string-buffer-before');after=raw_bytes(text,'string-buffer-after');expected='resources.rc\0'.encode('utf-16le')
    assert before==bytes.fromhex(row['buffer_before'])==b'\0'*4090
    assert after==raw_bytes(text,'string-buffer-current')==bytes.fromhex(row['buffer_after'])==expected+before[len(expected):]
    assert row['return_esp']==row['entry_esp']+4 and row['cursor_after']==row['cursor']+len(expected)and row['token_value_after']==row['buffer']
    entries=re.findall(r'^string-entry cursor=([0-9a-f]+) word=(\d+) token=([0-9a-f]+) header=(\d+) active=(\d+) directive=(\d+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returns=re.findall(r'^string-return cursor=([0-9a-f]+) token=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M)
    assert len(entries)==len(returns)==1 and int(returns[0][0],16)==int(entries[0][0],16)+len(expected)and int(returns[0][2],16)==int(entries[0][6],16)+4 and int(returns[0][1],16)==0x71c36f08
    assert raw_bytes(text,'string-extra-before')==bytes.fromhex(row['extra_before'])==b'\0'*2
    assert raw_bytes(text,'string-extra-after')==raw_bytes(text,'string-extra-current')==bytes.fromhex(row['extra_after'])==struct.pack('<H',len(expected)//2)
    current=args['string_current'];assert current==dict(pointer=row['buffer'],payload=after.hex(),extra=struct.pack('<H',len(expected)//2).hex())
    assert old_globals[0x16f08-0x14000:0x17f02-0x14000]==after and old_globals[0x16cd4-0x14000:0x16cd6-0x14000]==bytes.fromhex(current['extra'])==bytes.fromhex(globals['rc_token_extra']['bytes'])
    assert struct.unpack_from('<I',old_globals,0x16cd0-0x14000)[0]==0x71c36f08 and int.from_bytes(bytes.fromhex(globals['rc_token_value']['bytes']),'little')==row['buffer']
    assert old_globals[0x144f4-0x14000]==int.from_bytes(bytes.fromhex(globals['rc_token_kind']['bytes']),'little')==3
    assert struct.unpack_from('<I',old_globals,0x14a80-0x14000)[0]==0x71c36f08+2048 and int.from_bytes(bytes.fromhex(globals['rc_number_buffer']['bytes']),'little')==globals['rc_number_buffer']['identity_address']==row['buffer']+2048
    assert struct.unpack_from('<I',old_globals,0x21e90-0x14000)[0]==int.from_bytes(bytes.fromhex(globals['rc_number_buffer_active']['bytes']),'little')==0
    assert raw_bytes(text,'expect-end-word')==bytes.fromhex(args['expect_end']['bytes'])=='#line\0'.encode('utf-16le')and a['words'][1]==0x71c21350
    end=args['expect_end'];assert end['formal_bytes']==4 and end['pointer']>0 and end['output_active']==end['in_directive']==1
    old_cursor=struct.unpack_from('<I',old_globals,0x18120-0x14000)[0];new_cursor=int.from_bytes(bytes.fromhex(globals['rc_source_cursor']['bytes']),'little')
    assert old_cursor==int(returns[0][0],16)and new_cursor==row['cursor_after']==end['cursor']
    source=raw_bytes(text,'string-source-block');old_frame=struct.unpack_from('<9I',old_globals,0x18e80-0x14000);word=struct.unpack_from('<H',source,8+old_cursor-old_frame[5])[0]
    assert end['current_word']==word
    assert old_globals[0x18128-0x14000:0x18528-0x14000]==bytes.fromhex(globals['rc_macro_name']['bytes'])
    output=raw_bytes(text,'cpp-output-buffer');expected_output=buffered_output_prefix(b);assert output==expected_output and bytes.fromhex(globals['rc_cpp_output_file']['file_buffer'])[:len(output)]==output
    # Exact encountered original writers establish both aliases and inclusive bound.
    code=(original/'relocated-text.bin').read_bytes()
    checks={0x11f40:'bf086fc371',0x11f53:'8bdf',0x11fad:'8bc32bc724fe3df60f00007e44',0x11ffe:'6689334343',0x12042:'66832300',0x12053:'66891dd46cc371',0x3b70:'a2d46cc371',0x3b7a:'a2d56cc371'}
    for rva,opcodes in checks.items():assert code[rva-0x1000:rva-0x1000+len(bytes.fromhex(opcodes))]==bytes.fromhex(opcodes)
    pe=(ROOT/'toolchains/5.10/bin/RCDLL.DLL').read_bytes();at=struct.unpack_from('<I',pe,60)[0];count=struct.unpack_from('<H',pe,at+6)[0];optional=struct.unpack_from('<H',pe,at+20)[0]
    sections=[struct.unpack_from('<8sIIII',pe,at+24+optional+40*i)for i in range(count)]
    assert any(name.rstrip(b'\0')==b'.data'and rva+raw<=0x16f08<0x17f02<=rva+virtual for name,virtual,rva,raw,offset in sections),'string area must be wholly loader-zeroed, no initialized adjacent image bytes'
    proof['string_reading']=dict(formals=[],cleanup=0,return_scope='void; EAX unclaimed',text='resources.rc',characters_including_NUL=len(expected)//2,source_advance_bytes=len(expected),buffer_bytes=4090,defined_write_bytes=len(expected),untouched_tail_bytes=4090-len(expected),bound=dict(last_payload_WCHAR_byte_offset=4086,last_cursor_byte_offset=4088,terminator_inclusive_last_byte=4089,storage_WCHARs=2045,derivation='Even cursor starts at base; <=0xff6 byte offset stores WORD then adds2. Overflow path does not advance cursor; final WORD at4088 requires4090 bytes.'),union=dict(bytes=2,hash_byte_offset=0,length_byte_offset=1,string_length_WORD_offset=0,after_length=13,writer_instructions={hex(k):v for k,v in checks.items()}),number_alias=dict(byte_offset=2048,element=1024,alternate_mode=0,scope='Interior address identity now uses actual encountered string array. Existing opaque-anchor raw/history unchanged; alternate numeric mode still traps, no numeric content/extent behavior accepted.'),initialization='Actual whole4090-byte original/native entry arrays zero and entire span in loader-zeroed PE.data; no adjacent initialized bytes included.',next_formal=dict(type='const WCHAR pointer',bytes=4,contents='#line',cleanup=4),next_hidden_inputs=dict(source_cursor_offset=42,current_word=word,output_active=1,in_directive=1,token_kind=3,string_length=13,token_value='same actual initialized string buffer'),output_buffer_bytes=len(output))
    return proof


def expect_end_trial(original,native):
    config=read_configuration(original/'functions.json');assert config.get('trial_function')=='rc_expect_end'and len(config['accepted'])==56
    a,b,proof=checkpoint(original,native,'rc_equal_type',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c272d5 and re.search(r'^#1\s+0x[0-9a-f]+ in rc_intern_type\s*\(',(native/'gdb.stdout').read_text(),re.M)
    assert proof['application_trace'][-1]=='rc_expect_end'
    text=(original/'events.txt').read_text();args=b['arguments'];old_globals=(original/'globals.bin').read_bytes();globals=args['preprocessor']['globals']
    rows=args['expected_ends'];assert rows==json.loads((native/'expected-ends.json').read_text())and len(rows)==1;row=rows[0]
    calls=re.findall(r'^expect-entry word=([0-9a-f]+) cursor=([0-9a-f]+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returns=re.findall(r'^expect-return cursor=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(calls)==len(returns)==1
    old_word,old_cursor,old_sp,old_caller=[int(v,16)for v in calls[0]];old_after,old_return_sp=[int(v,16)for v in returns[0]]
    assert old_caller==0x71c26a6a and old_word==0x71c21350 and row['word']>0 and row['formal_bytes']==4
    assert raw_bytes(text,'expect-input-word')==bytes.fromhex(row['word_bytes'])=='#line\0'.encode('utf-16le')
    assert old_return_sp==old_sp+8 and row['return_esp']==row['entry_esp']+8 and old_after==old_cursor+2 and row['cursor_after']==row['cursor']+2
    old_frame=struct.unpack_from('<9I',old_globals,0x18e80-0x14000);new_frame=struct.unpack('<9I',bytes.fromhex(globals['rc_source_frames']['bytes'])[:36])
    source=raw_bytes(text,'compare-source-block');block=next(v for v in b['heap_identity']['blocks']if v['payload']+8==new_frame[5])
    assert block['owner']==b['heap_identity']['handle']and block['flags']==9 and block['bytes']==8206 and bytes.fromhex(block['contents'])==source
    old_offset=old_cursor-old_frame[5];new_offset=row['cursor']-new_frame[5];assert old_offset==new_offset and 0<=old_offset<len(source)-12
    assert struct.unpack_from('<2H',source,8+old_offset)==(13,10)and row['current_word']==13
    compare=args['type_compare'];assert compare['formal_bytes']==dict(left=4,right=4)and compare['object_bytes']==dict(left=20,right=20)
    type_bytes=b'\x03\x10'+b'\0'*18
    assert raw_bytes(text,'type-compare-left')==raw_bytes(text,'type-compare-right')==bytes.fromhex(compare['left_bytes'])==bytes.fromhex(compare['right_bytes'])==type_bytes
    assert a['words'][1]!=a['words'][2]and compare['left']!=compare['right']and compare['right']==compare['right_member']==compare['entry']+4 and compare['entry_bytes']==24 and compare['hash']==3
    assert a['words'][2]==a['registers']['ebx']+4 and a['registers']['edi']==0x71c42830+12
    old_buckets=old_globals[0x22830-0x14000:0x22c30-0x14000];new_buckets=bytes.fromhex(compare['buckets']);old_expected=bytearray(1024);new_expected=bytearray(1024)
    struct.pack_into('<I',old_expected,12,a['words'][2]-4);struct.pack_into('<I',new_expected,12,compare['entry'])
    assert old_buckets==bytes(old_expected)and new_buckets==bytes(new_expected)and compare['buckets_bytes']==1024 and compare['buckets_address']>0
    allocations=[v for v in b['heap']if v['operation']=='allocate'];requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M);results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M);payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(allocations)==len(requests)==len(results)==len(payloads)==21
    assert requests[20][0]==requests[0][0]and requests[20][1:]==('9','28','00000000')and results[20]==('%08x'%(a['words'][1]-8),'00000000')and payloads[20]==('28','00'*28)
    assert int(results[18][0],16)+12==a['words'][2]and allocations[18]['result']+12==compare['right']and allocations[20]['result']+8==compare['left']
    assert allocations[20]['heap']==b['heap_identity']['handle']and allocations[20]['flags']==9 and allocations[20]['bytes']==28 and allocations[20]['payload']=='00'*28 and allocations[20]['last_error_before']==allocations[20]['last_error_after']==0
    assert raw_bytes(text,'type-compare-left-allocation')==b'\0'*8+type_bytes and raw_bytes(text,'type-compare-right-entry')==b'\0'*12+type_bytes
    for pointer,size,expected in [(compare['left'],28,b'\0'*8+type_bytes),(compare['entry'],32,b'\0'*12+type_bytes)]:
        owner=next(v for v in b['heap_identity']['blocks']if v['payload']+8==pointer);assert owner['owner']==b['heap_identity']['handle']and owner['flags']==9 and owner['bytes']==size and bytes.fromhex(owner['contents'])==expected
    old_current=struct.unpack_from('<I',old_globals,0x18120-0x14000)[0];new_current=int.from_bytes(bytes.fromhex(globals['rc_source_cursor']['bytes']),'little')
    current_offset=old_current-old_frame[5];assert current_offset==new_current-new_frame[5]and 0<=current_offset<len(source)-10
    word=struct.unpack_from('<H',source,8+current_offset)[0]
    for name,rva in [('rc_cpp_active',0x1811c),('rc_in_directive',0x18530),('rc_header_mode',0x16efc)]:
        assert struct.unpack_from('<I',old_globals,rva-0x14000)[0]==int.from_bytes(bytes.fromhex(globals[name]['bytes']),'little')
    assert struct.unpack_from('<I',old_globals,0x16ce8-0x14000)[0]==args['current_line']and struct.unpack_from('<I',old_globals,0x29458-0x14000)[0]==args['prepared_lines']
    old_output=struct.unpack('<8I',raw_bytes(text,'cpp-output-file'));new_output=struct.unpack('<8I',bytes.fromhex(globals['rc_cpp_output_file']['file_state']));defined=old_output[0]-old_output[2]
    assert old_output[1:2]+old_output[3:]==new_output[1:2]+new_output[3:]and defined==new_output[0]-new_output[2]and 0<=defined<=4096
    assert raw_bytes(text,'cpp-output-buffer')==bytes.fromhex(globals['rc_cpp_output_file']['file_buffer'])[:defined]
    assert b['crt_errors']==dict(errno=0,last_error=0)
    scanner=next(v for v in config['functions']if v['name']=='rc_scan_line_start');assert ['0x2028','0x202c']in scanner['text_data']
    relocated=(original/'relocated-text.bin').read_bytes();masked=(original/'masked-text.bin').read_bytes()
    assert relocated[0x1028:0x102c]==masked[0x1028:0x102c]==b'\r\0\0\0'
    assert relocated[0x11064:0x11067]==bytes.fromhex('6a025e')and relocated[0x110e5:0x110ea]==bytes.fromhex('682820c271')
    proof['directive_end']=dict(argument='#line',argument_bytes=4,cleanup=4,return_scope='void; EAX unclaimed',input_source_offset=old_offset,input_words=[13,10],cursor_advance_bytes=2,behavior='Nonspace reader skips CR and returns LF; directive helper rewinds one WCHAR and returns. Following scanner consumes that newline and starts the next #line marker.',next_formals=dict(left_pointer_bytes=4,right_pointer_bytes=4,pointees_bytes=20,contents='flags1003 and eighteen zero bytes',distinct_owners=True,right_alias='owned24B interned entry.type at+4',left_owner='actual fresh20B type in28B private allocation',cleanup=8),next_hidden_inputs='Whole comparator body reads only WORD0/DWORD8/DWORD4 from the two observed formal pointees; no global/register input identified.',current_source_offset=current_offset,current_word=word,pending_output_defined_bytes=defined)
    proof['trial']='rc_expect_end'
    return proof


def equal_type_trial(original,native):
    config=read_configuration(original/'functions.json');assert config.get('trial_function')=='rc_equal_type'and len(config['accepted'])==57
    a,b,proof=checkpoint(original,native,'rc_parse_angle_name',('rc_input_temporary','rc_output_temporary'))
    assert re.search(r'^#1\s+0x[0-9a-f]+ in rc_handle_directive\s*\(',(native/'gdb.stdout').read_text(),re.M)
    assert proof['application_trace'][-1]=='rc_equal_type'and a['words'][0]==0x71c2668c
    text=(original/'events.txt').read_text();args=b['arguments'];old_globals=(original/'globals.bin').read_bytes();globals=args['preprocessor']['globals']
    rows=args['type_comparisons'];assert rows==json.loads((native/'type-comparisons.json').read_text())and len(rows)==1;row=rows[0]
    calls=re.findall(r'^compare-entry left=([0-9a-f]+) right=([0-9a-f]+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returns=re.findall(r'^compare-return result=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(calls)==len(returns)==1
    old_left,old_right,old_sp,old_caller=[int(v,16)for v in calls[0]];result,old_return_sp=[int(v,16)for v in returns[0]]
    type_bytes=b'\x03\x10'+b'\0'*18
    assert old_caller==0x71c272d5 and old_left!=old_right and row['left']!=row['right']and row['caller']=='rc_intern_type'
    assert row['formal_bytes']==dict(left=4,right=4)and old_return_sp==old_sp+12 and row['return_esp']==row['entry_esp']+12 and result==row['result']==1
    assert raw_bytes(text,'compare-input-left')==raw_bytes(text,'compare-input-right')==bytes.fromhex(row['left_bytes'])==bytes.fromhex(row['right_bytes'])==type_bytes
    blocks=b['heap_identity']['blocks']
    requests=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M);results=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M);payloads=re.findall(r'^heap-alloc-payload (\d+) ([0-9a-f]*)$',text,re.M)
    assert len(requests)==len(results)==len(payloads)==21
    assert requests[20]==(requests[0][0],'9','28','00000000')and results[20]==('%08x'%(old_left-8),'00000000')and payloads[20]==('28','00'*28)
    assert int(results[18][0],16)+12==old_right and requests[18]==(requests[0][0],'9','32','00000000')and payloads[18]==('32','00'*32)
    old_buckets=old_globals[0x22830-0x14000:0x22c30-0x14000];expected=bytearray(1024);struct.pack_into('<I',expected,12,old_right-4);assert old_buckets==bytes(expected)
    for pointer,size,offset in [(row['left'],28,8),(row['right'],32,12)]:
        block=next(v for v in blocks if v['payload']+offset==pointer);assert block['owner']==b['heap_identity']['handle']and block['flags']==9 and block['bytes']==size
        assert bytes.fromhex(block['contents'])[offset:offset+20]==type_bytes
    angle=args['angle_name'];assert angle['formals']==[]and angle['caller']=='rc_handle_directive'and angle['macro_bytes']==1024
    assert angle['header_mode']==0 and angle['output_active']==angle['in_directive']==1 and angle['token_kind']==old_globals[0x144f4-0x14000] ==63
    assert raw_bytes(text,'angle-macro-before')==bytes.fromhex(angle['macro_before'])==bytes.fromhex(globals['rc_macro_name']['bytes'])
    assert angle['macro']==globals['rc_macro_name']['address']and raw_bytes(text,'angle-marker-before')==bytes.fromhex(angle['marker_before'])==b'\0\0'
    old_frame=struct.unpack_from('<9I',old_globals,0x18e80-0x14000);new_frame=struct.unpack('<9I',bytes.fromhex(globals['rc_source_frames']['bytes'])[:36])
    source=raw_bytes(text,'angle-source-block');block=next(v for v in blocks if v['payload']+8==new_frame[5])
    assert block['owner']==b['heap_identity']['handle']and block['flags']==9 and block['bytes']==8206 and bytes.fromhex(block['contents'])==source
    old_cursor=struct.unpack_from('<I',old_globals,0x18120-0x14000)[0];offset=old_cursor-old_frame[5]
    assert offset==angle['cursor']-new_frame[5]and 0<=offset<len(source)-10
    word=struct.unpack_from('<H',source,8+offset)[0];assert angle['current_word']==word and source[8+offset:8+offset+24]=='windows.h>\r\n'.encode('utf-16le')
    assert angle['cursor']==int.from_bytes(bytes.fromhex(globals['rc_source_cursor']['bytes']),'little')
    for name,rva in [('rc_cpp_active',0x1811c),('rc_in_directive',0x18530),('rc_header_mode',0x16efc)]:
        assert struct.unpack_from('<I',old_globals,rva-0x14000)[0]==angle[dict(rc_cpp_active='output_active',rc_in_directive='in_directive',rc_header_mode='header_mode')[name]]==int.from_bytes(bytes.fromhex(globals[name]['bytes']),'little')
    assert struct.unpack_from('<I',old_globals,0x16ce8-0x14000)[0]==args['current_line']and struct.unpack_from('<I',old_globals,0x29458-0x14000)[0]==args['prepared_lines']
    old_output=struct.unpack('<8I',raw_bytes(text,'cpp-output-file'));new_output=struct.unpack('<8I',bytes.fromhex(globals['rc_cpp_output_file']['file_state']));defined=old_output[0]-old_output[2]
    assert old_output[1:2]+old_output[3:]==new_output[1:2]+new_output[3:]and defined==new_output[0]-new_output[2]and 0<=defined<=4096
    assert raw_bytes(text,'cpp-output-buffer')==bytes.fromhex(globals['rc_cpp_output_file']['file_buffer'])[:defined]and b['crt_errors']==dict(errno=0,last_error=0)
    proof['type_comparison']=dict(formals=dict(left_pointer_bytes=4,right_pointer_bytes=4,pointee_bytes=20),field_order=['flagsWORD0','keyDWORD8','keyDWORD4'],equal=True,result=1,cleanup=8,distinct_owned_objects=True,next_formals=[],next_hidden_inputs=dict(source_cursor_offset=offset,current_word=word,source_block_bytes=8206,macro_scratch_bytes=1024,header_mode=0,output_active=1,in_directive=1,token_kind=63),next_source='windows.h>CRLF',marker_scope='Only empty firstWCHAR currently modeled; next angle-name stub traps before any include marker copy.',pending_output_defined_bytes=defined)
    proof['trial']='rc_equal_type';return proof


def angle_name_trial(original,native):
    config=read_configuration(original/'functions.json');assert config.get('trial_function')=='rc_parse_angle_name'and len(config['accepted'])==58
    a,b,proof=checkpoint(original,native,'rc_search_include_directories',('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==0x71c26717 and re.search(r'^#1\s+0x[0-9a-f]+ in rc_handle_directive\s*\(',(native/'gdb.stdout').read_text(),re.M)
    assert proof['application_trace'][-1]=='rc_parse_angle_name'
    text=(original/'events.txt').read_text();args=b['arguments'];old_globals=(original/'globals.bin').read_bytes();globals=args['preprocessor']['globals']
    rows=args['angle_names'];assert rows==json.loads((native/'angle-names.json').read_text())and len(rows)==1;row=rows[0]
    calls=re.findall(r'^angle-entry cursor=([0-9a-f]+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returns=re.findall(r'^angle-return result=([0-9a-f]+) cursor=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(calls)==len(returns)==1
    old_cursor,old_sp,old_caller=[int(v,16)for v in calls[0]];result,old_after,old_return_sp=[int(v,16)for v in returns[0]]
    name='windows.h\0'.encode('utf-16le');assert len(name)==20
    assert old_caller==0x71c2668c and row['formals']==[]and row['caller']=='rc_handle_directive'and old_return_sp==old_sp+4 and row['return_esp']==row['entry_esp']+4
    assert result==0x71c38128 and row['result']==row['macro']and old_after==old_cursor+20 and row['cursor_after']==row['cursor']+20
    assert row['layout']==dict(marker_bytes=514,element_bytes=2,alignment=0)
    before=raw_bytes(text,'angle-input-macro');expected=bytearray(before);expected[:20]=name
    assert len(before)==1024 and bytes.fromhex(row['macro_before'])==before and raw_bytes(text,'angle-result-macro')==bytes.fromhex(row['macro_after'])==bytes(expected)
    assert raw_bytes(text,'angle-input-marker')==raw_bytes(text,'angle-result-marker')==bytes.fromhex(row['marker_before'])==bytes.fromhex(row['marker_after'])==bytes(514)
    copies=args['include_copies'];assert copies==json.loads((native/'include-copies.json').read_text())and len(copies)==1;copy=copies[0]
    old_copy=re.findall(r'^include-copy destination=([0-9a-f]+) source=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);old_copy_return=re.findall(r'^include-copy-return result=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(old_copy)==len(old_copy_return)==1
    dest,source,copy_sp=[int(v,16)for v in old_copy[0]];copy_result,copy_return_sp=[int(v,16)for v in old_copy_return[0]]
    assert (dest,source,copy_result)==(0x71c36cf8,0x71c38128,0x71c36cf8)and copy_return_sp==copy_sp
    assert copy['caller']=='rc_handle_directive'and copy['destination']==row['marker']==copy['result']and copy['source']==row['macro']and copy['formal_bytes']==dict(destination=4,source=4)and copy['return_esp']==copy['entry_esp']+4
    assert raw_bytes(text,'include-copy-source')==bytes.fromhex(copy['source_bytes'])==name
    assert raw_bytes(text,'include-copy-before')==bytes.fromhex(copy['before'])==bytes(514)
    marker_after=name+bytes(494);assert raw_bytes(text,'include-copy-after')==bytes.fromhex(copy['after'])==marker_after
    search=args['include_search'];assert search['formals']==[]and search['caller']=='rc_handle_directive'and search['marker_bytes']==514
    assert search['marker']==row['marker']and search['macro']==search['token_value']==row['macro']and search['macro_bytes']==1024
    assert raw_bytes(text,'include-marker-current')==bytes.fromhex(search['marker_current'])==marker_after and raw_bytes(text,'include-macro-current')==bytes.fromhex(search['macro_current'])==bytes(expected)
    assert old_globals[0x16cf8-0x14000:0x16efa-0x14000]==marker_after
    # Physical loader-zero span only; neither the original C array bound nor the
    # two bytes at16efa..16efc are inferred from this514B native model.
    target=(ROOT/'toolchains/5.10/bin/RCDLL.DLL').read_bytes();pe=struct.unpack_from('<I',target,0x3c)[0];sections=pe+24+struct.unpack_from('<H',target,pe+20)[0]
    data=next(struct.unpack_from('<4I',target,sections+40*i+8)for i in range(struct.unpack_from('<H',target,pe+6)[0])if target[sections+40*i:sections+40*i+8].rstrip(b'\0')==b'.data')
    virtual_bytes,rva,raw_bytes_count,raw_offset=data;assert rva+raw_bytes_count<=0x16cf8<0x16efa<0x16efc<rva+virtual_bytes
    old_frame=struct.unpack_from('<9I',old_globals,0x18e80-0x14000);new_frame=struct.unpack('<9I',bytes.fromhex(globals['rc_source_frames']['bytes'])[:36]);source_block=raw_bytes(text,'angle-input-source')
    assert raw_bytes(text,'include-source-block')==source_block
    block=next(v for v in b['heap_identity']['blocks']if v['payload']+8==new_frame[5]);assert block['owner']==b['heap_identity']['handle']and block['flags']==9 and block['bytes']==8206 and bytes.fromhex(block['contents'])==source_block
    entry_offset=old_cursor-old_frame[5];assert entry_offset==row['cursor']-new_frame[5]and source_block[8+entry_offset:8+entry_offset+24]=='windows.h>\r\n'.encode('utf-16le')
    current=struct.unpack_from('<I',old_globals,0x18120-0x14000)[0];current_offset=current-old_frame[5];assert current_offset==search['cursor']-new_frame[5]==entry_offset+22
    assert struct.unpack_from('<H',source_block,8+current_offset)[0]==search['current_word']==10
    assert search['header_mode']==0 and search['output_active']==search['in_directive']==1 and search['token_kind']==63
    for name,rva,key in [('rc_cpp_active',0x1811c,'output_active'),('rc_in_directive',0x18530,'in_directive'),('rc_header_mode',0x16efc,'header_mode')]:assert struct.unpack_from('<I',old_globals,rva-0x14000)[0]==search[key]==int.from_bytes(bytes.fromhex(globals[name]['bytes']),'little')
    assert struct.unpack_from('<I',old_globals,0x16cd0-0x14000)[0]==0x71c38128 and old_globals[0x144f4-0x14000]==search['token_kind']
    listing=raw_bytes(text,'include-path-list');assert listing==old_globals[0x14828-0x14000:0x149bc-0x14000]and len(listing)==404
    old_list=struct.unpack('<101I',listing);new_list=struct.unpack('<101I',bytes.fromhex(globals['rc_cpp_includes']['bytes']));assert old_list[0]==new_list[0]==search['include_count']==96
    old_pointers=[int(v,16)for v in re.findall(r'^wide-pointer \d+ ([0-9a-f]+)$',text,re.M)];wide=args['wide_vector'];assert len(old_pointers)==len(wide)==20 and args['wide_terminator']==0
    wanted_old=[0]*100;wanted_new=[0]*100;directories=search['directories'];assert [v['index']for v in directories]==[96,97,98,99]
    for slot,index,offset in [(99,11,4),(98,13,0),(97,15,0),(96,17,0)]:
        wanted_old[slot]=old_pointers[index]+offset;wanted_new[slot]=wide[index]['pointer']+offset
        item=next(v for v in directories if v['index']==slot);assert item['pointer']==wanted_new[slot]and bytes.fromhex(item['bytes'])==raw_bytes(text,'include-directory-%d'%slot)==bytes.fromhex(wide[index]['bytes'])[offset:]
    assert list(old_list[1:])==wanted_old and list(new_list[1:])==wanted_new
    assert search['path_characters']==wide[9]['pointer']+6 and raw_bytes(text,'include-path-characters')==bytes.fromhex(wide[9]['bytes'])[6:]
    storage=search['string_storage'];assert storage['bytes']==4090 and storage['number_offset']==2048 and search['number_buffer']==storage['address']+2048
    assert bytes.fromhex(storage['payload'])==old_globals[0x16f08-0x14000:0x17f02-0x14000]=='resources.rc\0'.encode('utf-16le')+bytes(4064)
    for name,rva in [('current_line',0x16ce8),('prepared_lines',0x29458)]:assert struct.unpack_from('<I',old_globals,rva-0x14000)[0]==args[name]
    old_output=struct.unpack('<8I',raw_bytes(text,'cpp-output-file'));new_output=struct.unpack('<8I',bytes.fromhex(globals['rc_cpp_output_file']['file_state']));defined=old_output[0]-old_output[2]
    assert old_output[1:2]+old_output[3:]==new_output[1:2]+new_output[3:]and defined==new_output[0]-new_output[2]and 0<=defined<=4096
    assert raw_bytes(text,'cpp-output-buffer')==bytes.fromhex(globals['rc_cpp_output_file']['file_buffer'])[:defined]and b['crt_errors']==dict(errno=0,last_error=0)
    boundary_path=HERE/'cases/resources-rich-trailing-space/include-marker-boundary.json';boundary=json.loads(boundary_path.read_text());boundary_raw=Path(boundary['raw_directory']);guard=boundary_raw/'receipt.json'
    assert sha(guard)==boundary['receipt_sha256']and json.loads(guard.read_text())==boundary['proof']
    guard_record=boundary['proof'];assert guard_record['status']=='pass'and guard_record['kind']=='include-marker'and guard_record['files']['production.elf']==sha(HERE/'build/native/rcdll-host')and guard_record['files']['target.o']==sha(HERE/'build/native/directives.o')
    assert guard_record['supported_boundary']['name_characters']==256 and guard_record['supported_boundary']['next_frontier']=='rc_search_current_directory'and guard_record['gdb']['argument_value']==258
    for filename,digest in guard_record['files'].items():assert sha(boundary_raw/filename)==digest
    proof['include_marker_guard']=dict(record=str(boundary_path.relative_to(HERE)),sha256=sha(boundary_path),raw_receipt_sha256=sha(guard),production_object_bound=True,scope='Only selected native257-WCHAR model is bounded; original C capacity and larger storage remain unknown.')
    proof['angle_name']=dict(formals=[],return_pointer_alias='rc_macro_name',cleanup=0,entry_source_offset=entry_offset,cursor_advance_bytes=20,parsed_name='windows.h',scratch_write_bytes=20,scratch_untouched_bytes=1004,marker_model=dict(elements=257,element_bytes=2,bytes=514,alignment=0,original_C_size='unknown',original_span=['0x16cf8','0x16efa'],unclaimed_gap=['0x16efa','0x16efc'],next_field='rc_header_mode at16efc',copy_bytes=20,untouched_bytes=494,return_alias=True,code_credit=0,native_guard='production source traps before length>=257 copy'),next_formals=[],next_hidden_inputs=dict(source_cursor_offset=current_offset,current_word=10,include_remaining=96,include_indices=[96,97,98,99],all100_slots_checked=True,directory_pointees_checked=True,token_value_alias='rc_macro_name',scratch_alias='rc_string_buffer+2048',scratch_owned_bytes_remaining=2042,path_characters='actual packed -pc argument',header_mode=0,output_active=1,in_directive=1,token_kind=63),pending_output_defined_bytes=defined)
    proof['trial']='rc_parse_angle_name';return proof


def _include_search_checkpoint(original,native,comment=False,matcher=False,block=False,push=False,define=False,replacement=False,bounds=False,lookup=False,evaluate=False,conditional=False,logical_or=False,logical_and=False,bitwise_or=False,bitwise_xor=False,bitwise_and=False,equality=False,relational=False,shift=False,additive=False,multiplicative=False,unary=False,lexer_correction=False,expression_match=False):
    trial='rc_accept_expression_token'if expression_match else'rc_parse_unary'if unary else'rc_parse_multiplicative'if multiplicative else'rc_parse_additive'if additive else'rc_parse_shift'if shift else'rc_parse_relational'if relational else'rc_parse_equality'if equality else'rc_parse_bitwise_and'if bitwise_and else'rc_parse_bitwise_xor'if bitwise_xor else'rc_parse_bitwise_or'if bitwise_or else'rc_parse_logical_and'if logical_and else'rc_parse_logical_or'if logical_or else'rc_parse_conditional_expression'if conditional else'rc_evaluate_condition'if evaluate else'rc_find_macro_parameter'if lookup else'rc_check_macro_space'if bounds else'rc_read_macro_replacement'if replacement else'rc_parse_define'if define else'rc_push_condition'if push else'rc_skip_block_comment'if block else'rc_match_logical_character'if matcher else'rc_scan_comment'if comment else'rc_search_include_directories';frontier='rc_evaluate_defined'if expression_match else'rc_accept_expression_token'if unary else'rc_parse_unary'if multiplicative else'rc_parse_multiplicative'if additive else'rc_parse_additive'if shift else'rc_parse_shift'if relational else'rc_parse_relational'if equality else'rc_parse_equality'if bitwise_and else'rc_parse_bitwise_and'if bitwise_xor else'rc_parse_bitwise_xor'if bitwise_or else'rc_parse_bitwise_or'if logical_and else'rc_parse_logical_and'if logical_or else'rc_parse_logical_or'if conditional else'rc_parse_conditional_expression'if evaluate else'rc_evaluate_condition'if lookup else'rc_find_macro_parameter'if bounds else'rc_check_macro_space'if replacement else'rc_read_macro_replacement'if define else'rc_parse_define'if push else'rc_push_condition'if block else'rc_skip_block_comment'if matcher else'rc_match_logical_character'if comment else'rc_scan_comment';caller='rc_accept_expression_token'if expression_match else'rc_parse_unary'if unary else'rc_parse_multiplicative'if multiplicative else'rc_parse_additive'if additive else'rc_parse_shift'if shift else'rc_parse_relational'if relational else'rc_parse_equality'if equality else'rc_parse_bitwise_and'if bitwise_and else'rc_parse_bitwise_xor'if bitwise_xor else'rc_parse_bitwise_or'if bitwise_or else'rc_parse_logical_and'if logical_and else'rc_parse_logical_or'if logical_or else'rc_parse_conditional_expression'if conditional else'rc_evaluate_condition'if evaluate else'rc_handle_directive'if lookup else'rc_read_macro_replacement'if bounds or replacement else'rc_parse_define'if define else'rc_handle_directive'if block or push else'rc_scan_comment'if comment or matcher else'rc_scan_line_start';source_offset=4678 if expression_match else 4662 if lookup else 592 if bounds else 580 if replacement else 578 if define else 492 if push else 476 if block else 4 if matcher else 2;source_word=40 if expression_match else 32 if lookup else 35 if bounds else 120 if replacement else 48 if define else 32 if push else 10 if block else 43 if matcher else 42
    config=read_configuration(original/'functions.json')
    if lexer_correction:
        assert unary and not config.get('trial_function')and len(config['accepted'])==81 and config['accepted'][-1]=='rc_parse_unary'
    else:assert config.get('trial_function')==trial and len(config['accepted'])==(81 if expression_match else 80 if unary else 79 if multiplicative else 78 if additive else 77 if shift else 76 if relational else 75 if equality else 74 if bitwise_and else 73 if bitwise_xor else 72 if bitwise_or else 71 if logical_and else 70 if logical_or else 69 if conditional else 68 if evaluate else 67 if lookup else 66 if bounds else 65 if replacement else 64 if define else 63 if push else 62 if block else 61 if matcher else 60 if comment else 59)
    a,b,proof=checkpoint(original,native,frontier,('rc_input_temporary','rc_output_temporary'))
    assert a['words'][0]==(0x71c27073 if expression_match else 0x71c22ff4 if unary else 0x71c22f9e if multiplicative else 0x71c22f6a if additive else 0x71c22f34 if shift else 0x71c22ec2 if relational else 0x71c22e82 if equality else 0x71c22e62 if bitwise_and else 0x71c22e42 if bitwise_xor else 0x71c22e22 if bitwise_or else 0x71c22df5 if logical_and else 0x71c22dcc if logical_or else 0x71c22d8d if conditional else 0x71c22cd7 if evaluate else 0x71c264a8 if lookup else 0x71c250a4 if bounds else 0x71c24e3c if replacement else 0x71c249eb if define else 0x71c2651c if push else 0x71c2674b if block else 0x71c32a12 if matcher else 0x71c32a09 if comment else 0x71c3216d)and re.search(r'^#1\s+0x[0-9a-f]+ in '+caller+r'\s*\(',(native/'gdb.stdout').read_text(),re.M)
    assert proof['application_trace'][-1]==trial
    text=(original/'events.txt').read_text();args=b['arguments'];old_globals=(original/'globals.bin').read_bytes();globals=args['preprocessor']['globals'];calls=args['include_opens']
    assert calls==json.loads((native/'include-opens.json').read_text())and len(calls)==4
    entries=re.findall(r'^search-open attempt=(\d+) index=(\d+) path=([0-9a-f]+) first=(\d+) esp=([0-9a-f]+) errno=(\d+) last-error=([0-9a-f]+)$',text,re.M)
    returns=re.findall(r'^search-open-return attempt=(\d+) result=([0-9a-f]+) esp=([0-9a-f]+) errno=(\d+) last-error=([0-9a-f]+)$',text,re.M);assert len(entries)==4 and len(returns)==3
    expected_paths=['./windows.h','./windows.h','include/windows.h','R:\\toolchains\\5.10\\include/windows.h']
    for index,(entry,row,path)in enumerate(zip(entries,calls,expected_paths)):
        attempt,slot,pointer,first,sp,errno_value,error=entry;assert int(attempt)==index and int(slot)==row['index']==99-index and row['remaining']==96
        assert row['formal_bytes']==dict(path=4,first=4)and int(first)==row['first']==0 and int(pointer,16)==0x71c37708
        assert raw_bytes(text,'search-open-path-%d'%index)==bytes.fromhex(row['path_bytes'])==(path+'\0').encode('utf-16le')
        assert int(errno_value)==row['errno_before']==(0 if index==0 else 2)and int(error,16)==row['last_error_before']==(0 if index==0 else 2)
        if index<3:
            attempt,result,after,errno_value,error=returns[index];assert int(attempt)==index and int(result,16)==row['result']==0 and int(after,16)==int(sp,16)+8 and row['return_esp']==row['entry_esp']+12
            assert int(errno_value)==row['errno_after']==2 and int(error,16)==row['last_error_after']==2
        else:assert not any(key in row for key in ['result','return_esp','errno_after','last_error_after']),'fourth open is genuinely pending at a child frontier, not a matching unchecked result'
    c=args['comment'];assert c['formals']==([dict(name='name',value=args['defined_operator']['name'],bytes=4)]if expression_match else[dict(name='token',value=55,bytes=1)]if unary else[dict(name='name',value=args['macro_parameter']['pointer'],bytes=4)]if bounds else[dict(name='character',value=42,bytes=4)]if comment else[])and c['caller']==caller and c['depth']==1
    if comment:assert args['character_match']==dict(character=42,formal_bytes=4,stack_slot_bytes=4,comparison_bits=32)and a['words'][1]==42
    assert re.findall(r'^comment-depth (\d+)$',text,re.M)==['1']
    old=struct.unpack('<9I',raw_bytes(text,'comment-frame'));new=struct.unpack('<9I',bytes.fromhex(c['frame']));old_parent=struct.unpack('<9I',raw_bytes(text,'comment-parent-frame'));new_parent=struct.unpack('<9I',bytes.fromhex(c['parent_frame']))
    assert tuple(old[i]for i in [0,2,6,7,8])==tuple(new[i]for i in [0,2,6,7,8])==(4098,1,4098,1,0)
    assert old[4]-old_parent[5]==new[4]-new_parent[5]==106 and old[4]>old_parent[5]>0
    current=struct.unpack_from('<I',old_globals,0x18120-0x14000)[0];assert current-old[5]==c['cursor']-new[5]==source_offset and c['current_word']==source_word and c['encoding']==1
    master=(ROOT/'toolchains/5.10/include/WINDOWS.H').read_bytes();expected_source=bytes(8)+master[:4098].decode('cp1252').encode('utf-16le')+bytes(2)
    assert raw_bytes(text,'comment-source-block')==bytes.fromhex(c['source_block'])==expected_source and len(expected_source)==8206 and struct.unpack_from('<H',expected_source,8+source_offset)[0]==c['current_word']
    blocks=b['heap_identity']['blocks'];buffer=next(v for v in blocks if v['payload']+8==new[5]);assert buffer['owner']==b['heap_identity']['handle']and buffer['flags']==9 and buffer['bytes']==8206 and bytes.fromhex(buffer['contents'])==expected_source
    name=next(v for v in blocks if v['payload']+8==new[3]);assert name['owner']==b['heap_identity']['handle']and name['flags']==9 and name['bytes']==34 and bytes.fromhex(name['contents'])==bytes(8)+'resources.rc\0'.encode('utf-16le')
    assert raw_bytes(text,'comment-filename')==bytes.fromhex(args['filename'])==bytes.fromhex(calls[3]['path_bytes'])+bytes(514-len(bytes.fromhex(calls[3]['path_bytes'])))
    marker='windows.h\0'.encode('utf-16le')+bytes(494);assert raw_bytes(text,'comment-marker')==bytes.fromhex(c['marker'])==marker
    storage=bytearray(4090);storage[:26]='resources.rc\0'.encode('utf-16le');path=bytes.fromhex(calls[3]['path_bytes']);storage[2048:2048+len(path)]=path
    if bounds:storage[:14]='0x0400\0'.encode('utf-16le')
    if lookup:storage[:16]='0x0400\0D'.encode('utf-16le')
    assert raw_bytes(text,'comment-work-buffer')==bytes.fromhex(c['work_buffer'])==bytes(storage)
    old_file=struct.unpack('<8I',raw_bytes(text,'comment-source-file'));new_file=struct.unpack('<8I',bytes.fromhex(c['source_file']))
    assert old_file[1:2]+old_file[3:]==new_file[1:2]+new_file[3:]==(510,9,6,0,4096,0)and old_file[0]-old_file[2]==new_file[0]-new_file[2]==2
    assert raw_bytes(text,'comment-source-buffer')[:512]==bytes.fromhex(c['file_buffer'])[:512]==master[4096:4608]
    assert re.findall(r'^comment-source-position (\d+)$',text,re.M)==['4608']and re.findall(r'^pos:\s*(\d+)$',c['file_fdinfo'],re.M)==['4608']and Path(c['file_host_path']).resolve()==(ROOT/'toolchains/5.10/include/WINDOWS.H').resolve()
    assert struct.unpack_from('<I',old_globals,0x14a10-0x14000)[0]==1 and struct.unpack_from('<I',old_globals,0x16ce8-0x14000)[0]==args['current_line']==(71 if lookup else 21 if define or replacement or bounds else 17 if push else 16 if block else 1)and struct.unpack_from('<I',old_globals,0x29458-0x14000)[0]==args['prepared_lines']==(143 if lookup else 135 if define or replacement or bounds else 131 if push else 130 if block else 128)
    old_output=struct.unpack('<8I',raw_bytes(text,'cpp-output-file'));new_output=struct.unpack('<8I',bytes.fromhex(globals['rc_cpp_output_file']['file_state']));defined=old_output[0]-old_output[2]
    assert old_output[1:2]+old_output[3:]==new_output[1:2]+new_output[3:]and defined==new_output[0]-new_output[2]and 0<=defined<=4096 and raw_bytes(text,'cpp-output-buffer')==bytes.fromhex(globals['rc_cpp_output_file']['file_buffer'])[:defined]
    assert b['crt_errors']==dict(errno=2,last_error=0)
    masked=(original/'masked-text.bin').read_bytes();relocated=(original/'relocated-text.bin').read_bytes();assert masked[0x384:0x38a]==relocated[0x384:0x38a]=='./\0'.encode('utf-16le')
    literal=next(v for v in config['functions']if v['name']=='rc_search_include_directories');assert ['0x1384','0x138a']in literal['text_data']
    searches=json.loads((native/'wide-searches.json').read_text());assert len(searches)==6
    for row,character in zip(searches,[119,58,46,46,101,101]):
        assert row['character']&65535==character and row['terminated_input']==row['input_after']=='\\:/\0'.encode('utf-16le').hex()and row['result']==(row['input']+2 if character==58 else 0)
        assert row['errno_before']==row['errno_after']and row['last_error_before']==row['last_error_after']and row['fp_before']==row['fp_after']
    # Reuse the unchanged marker length guard proof with current source identity
    # and the actual current ELF guard bytes; no repeated auxiliary launch.
    boundary=json.loads((HERE/'cases/resources-rich-trailing-space/include-marker-boundary.json').read_text());guard=boundary['proof'];old_source=json.loads((Path(boundary['raw_directory'])/'production-compiled-source.json').read_text());new_source=json.loads((native/'compiled-source.json').read_text());key='decomp/vc5.10/rcdll/src/directives.c'
    if evaluate:
        # A new evaluator is appended after the complete prior translation unit;
        # the marker owner itself remains byte-for-byte source-identical.
        prior_text=old_source['contents'][key]['text']
        # Explicit zero-credit alias correction removes only this unrelated
        # duplicate declaration; the entire marker owner is still identical.
        if 'int rc_keep_internal_marker;\n'not in new_source['contents'][key]['text']:
            assert prior_text.count('int rc_keep_internal_marker;\n')==1
            prior_text=prior_text.replace('int rc_keep_internal_marker;\n','',1)
        assert new_source['contents'][key]['text'].startswith(prior_text)
        assert new_source['contents'][key]['text'][len(prior_text):].startswith('\n/* Balance is reset before each preprocessing constant expression. */')
    else:assert old_source['contents'][key]==new_source['contents'][key]
    import subprocess
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(native/'captured-native.elf')],text=True);symbol=re.findall(r'^([0-9a-f]+) ([0-9a-f]+) T rc_handle_directive$',symbols,re.M);assert len(symbol)==1
    address,size=[int(v,16)for v in symbol[0]];offset=guard['production_guard']['offset'];expected_guard=bytes.fromhex(guard['production_guard']['guard_bytes']);elf=(native/'captured-native.elf').read_bytes();phoff=struct.unpack_from('<I',elf,28)[0];phsize,count=struct.unpack_from('<HH',elf,42);matches=[]
    for index in range(count):
        kind,file_offset,start,_,length=struct.unpack_from('<IIIII',elf,phoff+phsize*index)
        guard_end=address+offset+1
        if kind==1 and start<=guard_end-len(expected_guard)<guard_end<=start+length:matches.append(elf[file_offset+guard_end-len(expected_guard)-start:file_offset+guard_end-start])
    assert offset<size and matches==[expected_guard]and expected_guard[-1]==0xcc
    proof['reused_marker_guard']=dict(record='cases/resources-rich-trailing-space/include-marker-boundary.json',sha256=sha(HERE/'cases/resources-rich-trailing-space/include-marker-boundary.json'),compiled_source_sha256=new_source['contents'][key]['sha256'],guard_owner_source_sha256=old_source['contents'][key]['sha256'],guard_owner_prefix_unchanged=True,current_ELF_guard_offset=offset,current_ELF_guard_bytes=expected_guard.hex(),scope='Unchanged marker-length premise; original guard plain/GDB tests reused, current source and actual production ELF guard independently checked.')
    proof['include_search']=dict(paths=expected_paths,indices=[99,98,97,96],failed_returns=3,successful_open_pending_at_verified_child=True,physical_open_formals=dict(path_pointer_bytes=4,first_bytes=4,cleanup=8),initialized_path_span_bytes=len(path),source_depth=1,decoded_source_bytes=8196,source_terminator_bytes=2,current_source_offset=2,current_word=42,source_file=dict(descriptor=6,capacity=4096,initialized_buffer_bytes=512,consumed_bytes=2,remaining_bytes=510,position=4608,unwritten_tail_bytes=3584,unwritten_tail_scope='Retained raw, not compared or claimed initialized'),next_formals=[],next_hidden_inputs='Actual source cursor points to star42 in owned initialized8198B UTF16 header buffer; source frame/encoding/FILE and parser globals retained. Whole scan-comment body delegates through the existing cursor; no additional register/out-pointer input identified.',return_scope='Search and fourth rc_open_source have not returned; actual matching nonempty rc_scan_comment child frontier proves progress. No None result is accepted.',pending_output_defined_bytes=defined,shared_boundary='Canonical WCHAR16 wcschr; only needle lowWORD is consumed from4B slot, original upper16 bits unobserved/unclaimed.',literal_bytes=6,literal_code_credit=0)
    if comment:
        proof['include_search']['next_formals']=[dict(name='character',value=42,bytes=4)]
        proof['include_search']['return_scope']='Search and fourth open remain pending; their progress is verified at the nested matcher child. Neither a search nor dispatcher return is claimed.'
        proof['comment_dispatch']=dict(formals=[],return_scope='Not yet returned: first matcher call is the actual nonempty child frontier.',next_formals=[dict(name='character',value=42,formal_bytes=4,stack_slot_bytes=4,comparison_bits=32)],next_hidden_inputs=dict(source_cursor_offset=2,current_word=struct.unpack_from('<H',expected_source,10)[0],ownership='Same initialized source frame and decoded storage; matcher reads complete32-bit formal and existing cursor, no additional register/out-pointer inputs found.'),return_to_original='0x71c32a09',native_caller='rc_scan_comment')
    if matcher:
        rows=args['character_matches'];assert rows==json.loads((native/'character-matches.json').read_text())and len(rows)==1
        entry=re.findall(r'^character-match-entry character=(\d+) cursor=([0-9a-f]+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returned=re.findall(r'^character-match-return result=(\d+) cursor=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(entry)==len(returned)==1
        character,cursor,sp,return_to=entry[0];result,after,end_sp=returned[0];row=rows[0]
        assert int(character)==row['character']==42 and row['formal_bytes']==4 and row['caller']=='rc_scan_comment'and int(return_to,16)==0x71c32a09
        assert int(cursor,16)-old[5]==row['cursor']-new[5]==2 and int(after,16)-old[5]==row['cursor_after']-new[5]==4
        assert int(result)==row['result']==1 and int(end_sp,16)==int(sp,16)+8 and row['return_esp']==row['entry_esp']+8
        proof['include_search'].update(current_source_offset=source_offset,current_word=source_word,next_hidden_inputs='Actual cursor4/plus43 in the same initialized SDK source; block-comment scanner has no formals or additional register/out-pointer inputs, and consumes existing source/line/output globals.',return_scope='Fourth open and comment dispatcher remain pending; matcher has actually returned1 with ret4 and source advance2.')
        proof['character_match']=dict(character=42,formal_bytes=4,comparison_bits=32,result=1,cleanup=4,source_before_offset=2,source_after_offset=4,next_formals=[],next_current_word=source_word,next_caller='rc_scan_comment',next_original_return_to='0x71c32a12',state_scope='Full source frame and initialized source bytes plus line/FILE/output parser globals checked; block-comment helper has no additional register/out-pointer inputs identified.')
    if block or push or define or replacement or bounds:
        rows=args['block_comments'];assert rows==json.loads((native/'block-comments.json').read_text())and len(rows)==1;row=rows[0]
        entry=re.findall(r'^block-comment-entry cursor=([0-9a-f]+) line=(\d+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returned=re.findall(r'^block-comment-return cursor=([0-9a-f]+) line=(\d+) esp=([0-9a-f]+)$',text,re.M);assert len(entry)==len(returned)==1
        cursor,line,sp,return_to=entry[0];after,line_after,end_sp=returned[0];end=master.index(b'*/')+2;line_count=master[2:end].count(b'\n');assert end==219 and line_count==13
        assert row['formals']==[]and row['caller']=='rc_scan_comment'and int(return_to,16)==0x71c32a12 and int(sp,16)+4==int(end_sp,16)and row['return_esp']==row['entry_esp']+4
        assert int(cursor,16)-old[5]==row['cursor']-new[5]==4 and int(after,16)-old[5]==row['cursor_after']-new[5]==end*2
        assert int(line)==row['line']==1 and int(line_after)==row['line_after']==1+line_count
        buffers=[]
        for phase in ['before','after']:
            old_state=struct.unpack('<8I',raw_bytes(text,'block-'+phase+'-file'));new_state=struct.unpack('<8I',bytes.fromhex(row['output_'+phase]['file_state']));written=old_state[0]-old_state[2]
            assert old_state[1:2]+old_state[3:]==new_state[1:2]+new_state[3:]and written==new_state[0]-new_state[2]==row['output_'+phase]['defined_bytes']and 0<written<=4096
            payload=raw_bytes(text,'block-'+phase+'-buffer');assert payload==bytes.fromhex(row['output_'+phase]['payload'])and len(payload)==written;buffers.append(payload)
        assert buffers[1]==buffers[0]+'\r\n'.encode('utf-16le')*line_count
        condition=args['condition_entry'];assert condition['formals']==[]and condition['nesting']==struct.unpack('<i',raw_bytes(text,'condition-nesting'))[0]==(1 if define or replacement or bounds else 0 if push else -1)
        assert condition['caller']=='rc_handle_directive'and condition['directive']==(6 if push or define or replacement or bounds else 5)
        if not(push or define or replacement or bounds):assert condition['condition']==1 and a['registers']['esi']==1
        assert condition['word']==condition['macro']and raw_bytes(text,'condition-word')==bytes.fromhex(condition['macro_bytes'])and raw_bytes(text,'condition-word')[:(14 if push or define or replacement or bounds else 20)]==('WINVER\0'if define or replacement or bounds else'define\0'if push else'_WINDOWS_\0').encode('utf-16le')
        assert raw_bytes(text,'condition-state')==bytes.fromhex(condition['state'])==(struct.pack('<II',1,1)+bytes(120)if define or replacement or bounds else struct.pack('<I',1)+bytes(124)if push else bytes(128))
        assert masked[0x230:0x234]==relocated[0x230:0x234]=='\r\n'.encode('utf-16le')
        literal=next(v for v in config['functions']if v['name']=='rc_skip_block_comment');assert ['0x1230','0x1234']in literal['text_data']
        proof['include_search'].update(current_source_offset=source_offset,current_word=source_word,next_hidden_inputs='Actual nesting=-1 is the conditional push helper input; caller has #ifndef directive5, condition1 and current _WINDOWS_ identifier. Full source/FILE/current-line globals remain captured.',return_scope='Fourth source open remains pending; block-comment helper has returned normally, conditional push is an actual nonempty frontier.')
        proof['block_comment']=dict(formals=[],return_cleanup=0,source_before_offset=4,source_after_offset=end*2,initialized_read_bytes=end*2-4,line_before=1,line_after=14,LFs=line_count,output_append_bytes=line_count*4,output_defined_before=len(buffers[0]),output_defined_after=len(buffers[1]),literal_rva='0x1230',literal_bytes=4,literal_code_credit=0,next_formals=[],next_hidden_inputs=dict(nesting=-1,caller='rc_handle_directive',directive=5,condition=1,word='_WINDOWS_',current_source_offset=source_offset,current_word=source_word),scope='Only actual initialized header span and normal return traversed; EOF/resume branch remains named trapped dependency.')
    if push:
        rows=args['condition_pushes'];assert rows==json.loads((native/'condition-pushes.json').read_text())and len(rows)==1;row=rows[0]
        entry=re.findall(r'^condition-push-entry level=(-?\d+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returned=re.findall(r'^condition-push-return result=(-?\d+) level=(-?\d+) esp=([0-9a-f]+)$',text,re.M);assert len(entry)==len(returned)==1
        level,sp,return_to=entry[0];result,after,end_sp=returned[0]
        assert int(level)==row['level']==-1 and int(result)==row['result']==0 and int(after)==row['level_after']==0
        assert int(return_to,16)==0x71c2674b and row['caller']=='rc_handle_directive'and row['formals']==[]and int(end_sp,16)==int(sp,16)+4 and row['return_esp']==row['entry_esp']+4
        assert raw_bytes(text,'condition-before-caller-store')==bytes.fromhex(row['state_before_caller_store'])==bytes(128)
        assert 'condition'not in condition,'the next define branch has no initialized condition local to claim'
        proof['block_comment']['next_hidden_inputs']=dict(nesting=0,caller='rc_handle_directive',directive=6,word='define',current_source_offset=source_offset,current_word=source_word)
        proof['include_search']['next_hidden_inputs']='Actual define dispatcher has no formals. Source cursor492/space32, current directive6, parser/macro globals and initialized SDK span are captured; no additional register/out-pointer inputs identified.'
        proof['include_search']['return_scope']='Source open remains pending; block-comment and conditional push have returned, caller has stored condition_state[0]=1 before actual define parser child.'
        proof['condition_push']=dict(formals=[],entry_level=-1,return_level=0,result=0,cleanup=0,caller_store=dict(index=0,value=1,original_store_rva='0x674b',state_before_bytes=128,state_after='first DWORD1 followed124 unchanged zero bytes'),next_formals=[],next_source_offset=source_offset,next_current_word=source_word,next_directive=6,next_identifier='define',next_hidden_inputs='Current initialized SDK source/frame/cursor and parser/macro globals; no additional register or out-pointer input found. Any upcoming named stub must receive its own actual typed frontier proof.')
    if define or replacement or bounds:
        rows=args['define_parsers'];assert rows==json.loads((native/'define-parsers.json').read_text())and len(rows)==2
        entries=re.findall(r'^define-parser-entry index=(\d+) cursor=([0-9a-f]+) line=(\d+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returned=re.findall(r'^define-parser-return index=(\d+) cursor=([0-9a-f]+) line=(\d+) esp=([0-9a-f]+)$',text,re.M);assert len(entries)==2 and len(returned)==1
        for index,(entry,row,offset,line)in enumerate(zip(entries,rows,[492,2*(master.index(b'#define WINVER')+len(b'#define'))],[17,21])):
            number,cursor,current_line,sp,return_to=entry
            assert int(number)==index and int(cursor,16)-old[5]==row['cursor']-new[5]==offset and int(current_line)==row['line']==line
            assert int(return_to,16)==0x71c2651c and row['caller']=='rc_handle_directive'and row['formals']==[]
            if index==0:
                number,after,after_line,end_sp=returned[0];assert int(number)==0 and int(after,16)-old[5]==row['cursor_after']-new[5]==512 and int(after_line)==row['line_after']==17
                assert int(end_sp,16)==int(sp,16)+4 and row['return_esp']==row['entry_esp']+4
                assert raw_bytes(text,'defined-first-name')==bytes.fromhex(row['name_after'])and bytes.fromhex(row['name_after'])[:20]=='_WINDOWS_\0'.encode('utf-16le')
            else:assert not any(key in row for key in ['return_esp','cursor_after','line_after','name_after']),'second define is actually pending in its replacement reader'
        state=args['macro_replacement'];assert state['formals']==[]and state['caller']=='rc_parse_define'and state['parameter_count']==struct.unpack('<i',raw_bytes(text,'macro-parameter-count'))[0]==0
        assert struct.unpack('<I',raw_bytes(text,'macro-replacement-pointer'))[0]==0x71c36f08 and state['pointer']==state['buffer']and state['buffer_bytes']==4090 and state['saved_parameters']==(0 if replacement or bounds else a['words'][1])==0
        nodes=args['preprocessor']['macros'];old_nodes=re.findall(r'^define-macro-node index=(\d+) bucket=(\d+) pointer=([0-9a-f]+) next=([0-9a-f]+) name=([0-9a-f]+) text=([0-9a-f]+) kind=([0-9a-f]+)$',text,re.M)
        assert re.findall(r'^define-macro-count (\d+)$',text,re.M)==['4']and len(old_nodes)==len(nodes)==4
        expected_names=['RC_INVOKED','CASE_FROM_COMMAND','_WIN32','_WINDOWS_'];assert [int(row[1])for row in old_nodes]==[v['bucket']for v in nodes]==[4,13,114,233]
        for index,(old_row,row,name)in enumerate(zip(old_nodes,nodes,expected_names)):
            number,bucket,pointer,next_pointer,name_pointer,text_pointer,kind=old_row;record=raw_bytes(text,'define-macro-record-%d'%index);native_record=bytes.fromhex(row['record']);name_block=raw_bytes(text,'define-macro-name-%d'%index)
            assert int(number)==index and record[:8]==native_record[:8]==bytes(8)and struct.unpack('<4I',record[8:])==tuple(int(v,16)for v in [next_pointer,name_pointer,text_pointer,kind])
            assert struct.unpack('<4I',native_record[8:])==(row['next'],row['name'],row['text'],row['parameters']&255|row['flags']<<8|row['reserved']<<16)and int(kind,16)==row['parameters']==row['flags']==row['reserved']==0
            assert int(next_pointer,16)==row['next']==0 and name_block==bytes.fromhex(row['name_block'])==bytes(8)+(name+'\0\0').encode('utf-16le')
            allocation=next(v for v in blocks if v['payload']+8==row['pointer']);assert allocation['owner']==b['heap_identity']['handle']and allocation['flags']==9 and allocation['bytes']==24 and bytes.fromhex(allocation['contents'])==native_record
            allocation=next(v for v in blocks if v['payload']+8==row['name']);assert allocation['owner']==b['heap_identity']['handle']and allocation['flags']==9 and allocation['bytes']==len(name_block)and bytes.fromhex(allocation['contents'])==name_block
            if index==3:assert int(text_pointer,16)==row['text']==0 and row['text_block']is None
            else:assert int(text_pointer,16)>0 and row['text']>0 and raw_bytes(text,'define-macro-text-%d'%index)==bytes.fromhex(row['text_block'])
        copies=json.loads((native/'macro-copies.json').read_text());assert len(copies)==7;copy=copies[-1]
        copy_calls=re.findall(r'^macro-copy-call (\d+) site=([0-9a-f]+) source=([0-9a-f]+) count=(\d+) esp=([0-9a-f]+)$',text,re.M);copy_returns=re.findall(r'^macro-copy-return (\d+) result=([0-9a-f]+) esp=([0-9a-f]+)$',text,re.M);assert len(copy_calls)==len(copy_returns)==7
        number,site,pointer,count,sp=copy_calls[-1];returned_number,result,end_sp=copy_returns[-1];assert number==returned_number=='6'and int(count)==copy['length']==10 and int(pointer,16)==0x71c38128
        assert copy['source_contract']=='initialized-global-name'and raw_bytes(text,'macro-copy-source-6')==bytes.fromhex(copy['source_bytes'])=='_WINDOWS_\0'.encode('utf-16le')
        assert int(end_sp,16)==int(sp,16)+8 and copy['return_esp']==copy['entry_esp']+12 and int(result,16)==int(old_nodes[-1][4],16)and copy['result']==nodes[-1]['name']
        assert raw_bytes(text,'macro-copy-result-6')==bytes.fromhex(copy['result_block'])==bytes(8)+'_WINDOWS_\0\0'.encode('utf-16le')
        assert struct.unpack_from('<I',old_globals,0x16ce0-0x14000)[0]==struct.unpack('<I',bytes.fromhex(globals['rc_macro_name_length']['bytes']))[0]==7 and old_globals[0x16cec-0x14000]==bytes.fromhex(globals['rc_macro_hash']['bytes'])[0]==219
        proof['block_comment']['next_hidden_inputs']=dict(nesting=1,caller='rc_parse_define',directive=6,word='WINVER',current_source_offset=source_offset,current_word=source_word)
        proof['include_search']['next_hidden_inputs']='Replacement reader has no formals; actual parameter_count0/replacement==4090-byte string buffer, cursor578/digit48/current WINVER identifier and parser globals captured. Pending caller parameter slot0 is retained separately, not invented as a callee formal.'
        proof['include_search']['return_scope']='Fourth source open remains pending. First empty define returned; second actual define call is pending in replacement reader with no invented return fields.'
        proof['define_parser']=dict(formals=[],first_call=dict(entry_source_offset=492,return_source_offset=512,line=17,cleanup=0,defined_name='_WINDOWS_',name_copy_bytes=20,name_allocation_bytes=30,node_allocation_bytes=24,text_pointer_NULL=True,parameter_count=0,bucket=233),second_call=dict(entry_source_offset=2*(master.index(b'#define WINVER')+len(b'#define')),line=21,returned=False),next_formals=[],next_hidden_inputs=dict(source_offset=578,current_word=48,parameter_count=0,replacement_buffer_bytes=4090,replacement_alias='rc_string_buffer',pending_caller_parameter_value=0,current_identifier='WINVER',identifier_length_including_NUL=7,identifier_hash=219,nesting=1),macro_records=4,scope='Fourth macro is real owned node/name storage; empty replacement is NULL, not an invented buffer. Function-like helper and replacement reader remain named traps.')
    if replacement:
        space=args['macro_space'];assert space['cursor']==space['out']==state['pointer']==state['buffer']and a['words'][1]==0x71c36f08
        assert space['formal_bytes']==space['stack_slot_bytes']==4 and space['caller']=='rc_read_macro_replacement'
        assert space['character']==a['registers']['ebx']&65535==48 and space['character_bytes']==2 and space['stringify']==a['words'][6]==0 and space['charize']==a['words'][7]==0
        assert raw_bytes(text,'comment-work-buffer')==bytes.fromhex(c['work_buffer']), 'replacement has not written yet'
        proof['define_parser']['next_hidden_inputs'].update(source_offset=source_offset,current_word=source_word)
        proof['define_parser']['next_formals']=[dict(name='cursor',bytes=4,alias='rc_string_buffer')]
        proof['define_parser']['scope']='First empty define returned; second define and replacement reader are genuinely pending at the first bounds check, before any replacement write. Fourth owned macro and seven-copy proofs remain positive.'
        proof['replacement_reader']=dict(formals=[],returned=False,source_before_offset=578,source_after_offset=source_offset,character=48,current_source_word=source_word,read_bytes=2,replacement_pointer_alias='rc_string_buffer',replacement_storage_bytes=4090,no_replacement_write=True,stringify=0,charize=0,next_formals=[dict(name='cursor',bytes=4,stack_slot_bytes=4,alias='rc_macro_replacement==rc_string_buffer')],next_hidden_inputs='Bounds helper consumes complete32-bit cursor address and the current WINVER macro name only on its diagnostic path; no register/out-pointer input identified. Source/frame/macro/conditional state and actual parent flags retained.',return_scope='No replacement length or completed second definition is invented.')
    if bounds:
        checks=args['macro_space_checks'];assert checks==json.loads((native/'macro-space-checks.json').read_text())and len(checks)==7
        entries=re.findall(r'^macro-space-entry index=(\d+) cursor=([0-9a-f]+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returned=re.findall(r'^macro-space-return index=(\d+) esp=([0-9a-f]+)$',text,re.M);assert len(entries)==len(returned)==7
        for index,(entry,ret,row,offset)in enumerate(zip(entries,returned,checks,[0,2,2,4,6,8,10])):
            number,cursor,sp,return_to=entry;return_number,end_sp=ret
            assert int(number)==int(return_number)==index and int(cursor,16)-0x71c36f08==row['cursor']-state['buffer']==offset
            assert int(return_to,16)==(0x71c24e3c if index<2 else 0x71c25065)and row['formal_bytes']==4 and row['caller']=='rc_read_macro_replacement'
            assert int(end_sp,16)==int(sp,16)+8 and row['return_esp']==row['entry_esp']+8 and 0<=offset<4086
        parameter=args['macro_parameter'];assert parameter['pointer']==parameter['start']==state['buffer']+2 and a['words'][1]==a['registers']['esi']==0x71c36f0a
        assert parameter['formal_bytes']==parameter['stack_slot_bytes']==4 and bytes.fromhex(parameter['bytes'])=='x0400\0'.encode('utf-16le')and parameter['caller']=='rc_read_macro_replacement'
        assert parameter['out']==state['buffer']+12 and a['registers']['ebp']==0x71c36f14 and parameter['character']==a['registers']['ebx']&65535==10 and parameter['stringify']==parameter['charize']==0
        assert bytes.fromhex(c['work_buffer'])[2:14]==bytes.fromhex(parameter['bytes'])
        from protocol import module_offset_reads
        proof['module_offset_io']=module_offset_reads(original,text)
        proof['define_parser']['next_hidden_inputs'].update(source_offset=source_offset,current_word=source_word)
        proof['define_parser']['next_formals']=[dict(name='name',bytes=4,alias='rc_macro_replacement+2',input_bytes=12)]
        proof['include_search']['next_hidden_inputs']='Parameter lookup has actual12-byte initialized UTF16 name x0400 at buffer+2, physical pointer slot4 and parameter_count0. Source/frame/cursor, buffer prefix/tail and parser state are retained; no additional register/out-pointer input identified.'
        proof['macro_space']=dict(formal_bytes=4,stack_slot_bytes=4,cleanup=4,normal_returns=7,cursor_offsets=[0,2,2,4,6,8,10],bound_original='0x71c37efe',bound_buffer_offset=4086,bound_element_index=2043,buffer_bytes=4090,buffer_writes=dict(bytes=14,payload='0x0400\0'.encode('utf-16le').hex(),untouched_bytes=4076),next_formals=[dict(name='name',bytes=4,alias='rc_string_buffer+2',initialized_name='x0400',name_bytes_including_NUL=12)],next_parameter_count=0,source_offset=source_offset,current_source_word=source_word,scope='All seven actual cursor comparisons are below the bound and return with ret4. Fatal overflow branch is present in faithful Win32 source but untraversed. Whole reader and second define remain pending at parameter lookup.')
    if lookup:
        rows=args['parameter_lookups'];assert rows==json.loads((native/'parameter-lookups.json').read_text())and len(rows)==1;row=rows[0]
        entry=re.findall(r'^parameter-lookup-entry name=([0-9a-f]+) esp=([0-9a-f]+) caller=([0-9a-f]+)$',text,re.M);returned=re.findall(r'^parameter-lookup-return result=(-?\d+) esp=([0-9a-f]+)$',text,re.M);assert len(entry)==len(returned)==1
        pointer,sp,return_to=entry[0];result,end_sp=returned[0]
        assert int(pointer,16)==0x71c36f0a and row['pointer']==row['buffer']+2 and int(return_to,16)==0x71c250a4 and row['formal_bytes']==4 and row['caller']=='rc_read_macro_replacement'and row['parameter_count']==0
        assert int(result)==row['result']==-1 and int(end_sp,16)==int(sp,16)+8 and row['return_esp']==row['entry_esp']+8
        assert raw_bytes(text,'parameter-lookup-name')==raw_bytes(text,'parameter-lookup-name-after')==bytes.fromhex(row['bytes'])==bytes.fromhex(row['name_after'])=='x0400\0'.encode('utf-16le')
        expression=args['expression_entry'];assert expression==dict(formals=[],saved_output=1,output_active=0,in_expression=1,in_directive=1)and a['registers']['esi']==1
        for name,rva,value in [('rc_cpp_active',0x1811c,0),('rc_in_condition_expression',0x18e6c,1),('rc_in_directive',0x18530,1)]:assert struct.unpack_from('<I',old_globals,rva-0x14000)[0]==value
        condition=args['condition_entry'];assert condition['formals']==[]and condition['caller']=='rc_handle_directive'and condition['nesting']==struct.unpack('<i',raw_bytes(text,'condition-nesting'))[0]==1 and condition['directive']==0 and condition['word']==condition['macro']
        identifier='defined'if expression_match else'if';identifier_bytes=(identifier+'\0').encode('utf-16le');identifier_hash=sum(ord(c)&0x5f for c in identifier)&255
        assert raw_bytes(text,'condition-word')==bytes.fromhex(condition['macro_bytes'])and raw_bytes(text,'condition-word')[:len(identifier_bytes)]==identifier_bytes
        assert raw_bytes(text,'condition-state')==bytes.fromhex(condition['state'])==struct.pack('<II',1,1)+bytes(120)
        assert struct.unpack_from('<I',old_globals,0x16ce0-0x14000)[0]==struct.unpack('<I',bytes.fromhex(globals['rc_macro_name_length']['bytes']))[0]==len(identifier)+1 and old_globals[0x16cec-0x14000]==bytes.fromhex(globals['rc_macro_hash']['bytes'])[0]==identifier_hash
        state=args['macro_replacement'];assert set(state)=={'parameter_count','pointer','buffer','buffer_bytes'}and state['parameter_count']==struct.unpack('<i',raw_bytes(text,'macro-parameter-count'))[0]==0 and state['pointer']==state['buffer']and state['buffer_bytes']==4090 and struct.unpack('<I',raw_bytes(text,'macro-replacement-pointer'))[0]==0x71c36f08
        # The define parser's three caller frames have really returned. Preserve
        # their recorded effects; never read retired locals as current input.
        parsers=args['define_parsers'];assert parsers==json.loads((native/'define-parsers.json').read_text())and len(parsers)==3 and all(v['return_esp']==v['entry_esp']+4 for v in parsers)
        returned=re.findall(r'^define-parser-return index=(\d+) cursor=([0-9a-f]+) line=(\d+) esp=([0-9a-f]+)$',text,re.M);assert len(returned)==3 and [int(v[0])for v in returned]==[0,1,2]
        for old_row,row in zip(returned,parsers):assert int(old_row[1],16)-old[5]==row['cursor_after']-new[5]and int(old_row[2])==row['line_after']
        names=['RC_INVOKED','CASE_FROM_COMMAND','_WIN32','_INC_WINDOWS','WINVER','_WINDOWS_'];buckets=[4,13,114,195,219,233]
        nodes=args['preprocessor']['macros'];old_nodes=re.findall(r'^define-macro-node index=(\d+) bucket=(\d+) pointer=([0-9a-f]+) next=([0-9a-f]+) name=([0-9a-f]+) text=([0-9a-f]+) kind=([0-9a-f]+)$',text,re.M)
        assert re.findall(r'^define-macro-count (\d+)$',text,re.M)==['6']and len(old_nodes)==len(nodes)==6 and [int(v[1])for v in old_nodes]==[v['bucket']for v in nodes]==buckets
        for index,(old_row,row,name)in enumerate(zip(old_nodes,nodes,names)):
            number,bucket,pointer,next_pointer,name_pointer,text_pointer,kind=old_row;record=raw_bytes(text,'define-macro-record-%d'%index);native_record=bytes.fromhex(row['record']);name_block=raw_bytes(text,'define-macro-name-%d'%index)
            assert int(number)==index and record[:8]==native_record[:8]==bytes(8)and struct.unpack('<4I',record[8:])==tuple(int(v,16)for v in [next_pointer,name_pointer,text_pointer,kind])
            assert struct.unpack('<4I',native_record[8:])==(row['next'],row['name'],row['text'],row['parameters']&255|row['flags']<<8|row['reserved']<<16)and int(kind,16)==row['parameters']==row['flags']==row['reserved']==0
            assert int(next_pointer,16)==row['next']==0 and name_block==bytes.fromhex(row['name_block'])==bytes(8)+(name+'\0\0').encode('utf-16le')
            for target,payload,size in [(row['pointer'],native_record,24),(row['name'],name_block,len(name_block))]:
                allocation=next(v for v in blocks if v['payload']+8==target);assert allocation['owner']==b['heap_identity']['handle']and allocation['flags']==9 and allocation['bytes']==size and bytes.fromhex(allocation['contents'])==payload
            if name in ['_INC_WINDOWS','_WINDOWS_']:assert int(text_pointer,16)==row['text']==0 and row['text_block']is None
            else:
                text_block=raw_bytes(text,'define-macro-text-%d'%index);expected=bytes(8)+({'RC_INVOKED':'1','CASE_FROM_COMMAND':'7','_WIN32':'1','WINVER':'0x0400'}[name]+'\0D\0').encode('utf-16le')
                assert text_block==bytes.fromhex(row['text_block'])==expected
                allocation=next(v for v in blocks if v['payload']+8==row['text']);assert allocation['owner']==b['heap_identity']['handle']and allocation['flags']==9 and allocation['bytes']==len(expected)and bytes.fromhex(allocation['contents'])==expected
        old_bucket_words=struct.unpack_from('<256I',old_globals,0x21ea0-0x14000);new_bucket_words=struct.unpack('<256I',bytes.fromhex(globals['rc_macro_buckets']['bytes']));expected_old=[0]*256;expected_new=[0]*256
        for old_row,row in zip(old_nodes,nodes):expected_old[int(old_row[1])]=int(old_row[2],16);expected_new[row['bucket']]=row['pointer']
        assert list(old_bucket_words)==expected_old and list(new_bucket_words)==expected_new
        copies=json.loads((native/'macro-copies.json').read_text());assert len(copies)==10 and [v['length']for v in copies[7:]]==[7,8,13]
        assert copies[8]['source_contract']=='initialized-global-string-buffer'and copies[8]['source']==state['buffer']and bytes.fromhex(copies[8]['source_bytes'])=='0x0400\0D'.encode('utf-16le')and bytes.fromhex(copies[8]['result_block'])==bytes(8)+'0x0400\0D\0'.encode('utf-16le')
        assert raw_bytes(text,'macro-copy-source-8')==bytes.fromhex(copies[8]['source_bytes'])and raw_bytes(text,'macro-copy-result-8')==bytes.fromhex(copies[8]['result_block'])and copies[8]['return_esp']==copies[8]['entry_esp']+12
        current_output=raw_bytes(text,'cpp-output-buffer');first_prefix=raw_bytes(text,'block-before-buffer');before_marker=master[:master.index(b'#endif')].count(b'\n');total_lines=master[:source_offset//2].count(b'\n');line_marker=('#line %d \"'%(before_marker+2)+expected_paths[3].replace('\\','\\\\')+'\"\r\n').encode('utf-16le');assert before_marker==21 and total_lines==70 and current_output==first_prefix+'\r\n'.encode('utf-16le')*before_marker+line_marker+'\r\n'.encode('utf-16le')*(total_lines-before_marker-1)
        assert struct.unpack('<I',bytes.fromhex(globals['rc_cpp_E']['bytes']))[0]==1
        from protocol import module_offset_reads
        proof['module_offset_io']=module_offset_reads(original,text)
        proof['include_search'].update(current_source_offset=source_offset,current_word=source_word,next_hidden_inputs='Actual #if expression entry has no formals; source/frame/cursor, six complete macro owners, output-disabled/expression/directive flags and saved output1 checked.',return_scope='SDK source open remains pending; all three defines have returned. Actual expression helper is a nonempty child frontier.')
        proof['parameter_lookup']=dict(formal_bytes=4,cleanup=4,name='x0400',name_bytes=12,name_alias='rc_string_buffer+2',parameter_count=0,result=-1,no_parameter_table_read=True,subsequent_definitions=['WINVER','_INC_WINDOWS'],complete_macro_nodes=6,WINVER_text_allocation_bytes=26,string_buffer_prefix_bytes=16,string_buffer_untouched_bytes=4074,SDK_line=71,SDK_source_offset=source_offset,next_formals=[],next_hidden_inputs=dict(saved_output=1,output_active=0,in_expression=1,in_directive=1,identifier=identifier,identifier_hash=identifier_hash,nesting=1),retired_define_locals='Not reread; all three returns are actual prior snapshots.',line_marker=line_marker.decode('utf-16le'),scope='Count-zero lookup returns-1/ret4, numeric macro replacement completes with initialized16-byte serialization and exact requested26-byte copied owner. #if has not been evaluated.')
    if evaluate:
        state=args['condition_evaluation'];assert set(state)=={'formals','caller','parentheses','token_kind','expression_token_address','lexer_token_address','token_bytes','lexer_token'}
        assert state['formals']==([dict(name='name',value=args['defined_operator']['name'],bytes=4)]if expression_match else[dict(name='token',value=55,bytes=1)]if unary else[])and state['caller']==caller and state['parentheses']==0 and state['token_kind']==1
        assert state['expression_token_address']!=state['lexer_token_address']and state['token_bytes']==1
        assert state['lexer_token_address']==globals['rc_token_kind']['address']
        def data_symbol(name):
            rows=re.findall(r'^([0-9a-f]+) ([0-9a-f]+) [BbDd] '+re.escape(name)+r'$',symbols,re.M);assert len(rows)==1
            return tuple(int(value,16)for value in rows[0])
        lookahead_elf,lookahead_width=data_symbol('rc_expression_token');lexer_elf,lexer_width=data_symbol('rc_token_kind');assert lookahead_width==lexer_width==1
        frontier_elf=re.findall(r'^([0-9a-f]+) [0-9a-f]+ T '+re.escape(b['function'])+r'$',symbols,re.M);assert len(frontier_elf)==1
        frontier_runtime=re.findall(r'^Symbol "'+re.escape(b['function'])+r'" is a function at address (0x[0-9a-f]+)',(native/'gdb.stdout').read_text(),re.M);assert len(frontier_runtime)==1
        assert state['expression_token_address']-lookahead_elf==state['lexer_token_address']-lexer_elf==int(frontier_runtime[0],16)-int(frontier_elf[0],16)
        listing_alias=next(row for row in json.loads((native/'preprocess-data.json').read_text())['globals']if row['name']=='rc_macro_listing_enabled')
        if listing_alias.get('native_expression')=='rc_apstudio':
            apstudio_elf,apstudio_width=data_symbol('rc_apstudio');assert apstudio_width==4
            assert globals['rc_macro_listing_enabled']['address']-apstudio_elf==int(frontier_runtime[0],16)-int(frontier_elf[0],16)
            assert globals['rc_macro_listing_enabled']['bytes']==b['globals']['rc_apstudio']==old_globals[0x25058-0x14000:0x2505c-0x14000].hex()
            for control,rva in [('rc_expanding_macro',0x16f00),('rc_source_depth',0x14a10)]:
                control_elf,control_width=data_symbol(control);assert control_width==4
                assert globals[control]['address']-control_elf==int(frontier_runtime[0],16)-int(frontier_elf[0],16)
                assert globals[control]['bytes']==old_globals[rva-0x14000:rva-0x14000+4].hex()
        assert state['lexer_token']==old_globals[0x144f4-0x14000]==bytes.fromhex(globals['rc_token_kind']['bytes'])[0]
        assert raw_bytes(text,'expression-parentheses')==struct.pack('<i',0)and raw_bytes(text,'expression-token')==bytes([1])
        assert struct.unpack_from('<i',old_globals,0x18e7c-0x14000)[0]==state['parentheses']and old_globals[0x149dc-0x14000]==state['token_kind']
        proof['condition_evaluation']=dict(formals=[],return_pending=True,parentheses=0,token_kind=1,counter_original_rva='0x18e7c',token_original_rva='0x149dc',lexer_token_original_rva='0x144f4',distinct_token_storage=True,token_bytes=1,next_formals=[],next_hidden_inputs=proof['parameter_lookup']['next_hidden_inputs'],scope='Wrapper resets parenthesis count0 and the distinct expression lookahead byte1 before calling the conditional-expression parser. Parser entry is the actual named INT3; no expression result or diagnostic branch traversal claimed.')
        if conditional:proof['condition_evaluation']['scope']='Conditional-expression parser enters logical-OR with no formals and unchanged source/counter/token/flags/macros. First child INT3 is before any expression result or ternary dispatch; uninitialized alternate local is not consumed or observed.'
        if logical_or:proof['condition_evaluation']['scope']='Logical-OR parser enters logical-AND with no formals and unchanged source/counter/token/flags/macros. No result or operator matcher return claimed; both operands remain explicit calls, not host short-circuit parsing.'
        if logical_and:proof['condition_evaluation']['scope']='Logical-AND parser enters bitwise-OR with no formals and unchanged live source/counter/token/flags/macros. Both operands remain explicit parser calls; no operator or expression result claimed.'
        if bitwise_or:proof['condition_evaluation']['scope']='Bitwise-OR parser enters bitwise-XOR with no formals and unchanged live source/counter/token/flags/macros. No operator or expression result claimed.'
        if bitwise_xor:proof['condition_evaluation']['scope']='Bitwise-XOR parser enters bitwise-AND with no formals and unchanged live source/counter/token/flags/macros. No operator or expression result claimed.'
        if bitwise_and:proof['condition_evaluation']['scope']='Bitwise-AND parser enters equality with no formals and unchanged live source/counter/token/flags/macros. No operator or expression result claimed.'
        if equality:proof['condition_evaluation']['scope']='Equality parser enters relational parser with no formals and unchanged live source/counter/token/flags/macros. Original one-comparison control flow retained; no operator or expression result claimed.'
        if relational:proof['condition_evaluation']['scope']='Relational parser enters shift parser with no formals and unchanged live source/counter/token/flags/macros. Original signed single-comparison flow retained; no operator or expression result claimed.'
        if shift:proof['condition_evaluation']['scope']='Shift parser enters additive parser with no formals and unchanged live source/counter/token/flags/macros. Original signed i386 shift dispatch retained; no operand, shift-count, operator return or expression result claimed.'
        if additive:proof['condition_evaluation']['scope']='Additive parser enters multiplicative parser with no formals and unchanged live source/counter/distinct lookahead/lexer/flags/macros. No operand, arithmetic/operator return or expression result claimed.'
        if multiplicative:proof['condition_evaluation']['scope']='Multiplicative parser enters unary parser with no formals and unchanged live source/counter/distinct lookahead/lexer/flags/macros. Original signed i386 multiply/divide/remainder loop retained; no arithmetic operand/operator return, division exception or expression result claimed.'
        if unary and not expression_match:
            match=args['expression_match'];assert match['token']==55 and match['formal_bytes']==1 and match['physical_slot_bytes']==4 and match['formal_address']==match['physical_slot_address']and match['caller']==caller
            assert raw_bytes(text,'expression-match-slot')==bytes.fromhex(match['physical_slot'])==struct.pack('<I',a['words'][1])==struct.pack('<I',55)
            proof['condition_evaluation'].update(next_formals=[dict(name='token',formal_bytes=1,physical_slot_bytes=4,consumed_bits=8,physical_raw=match['physical_slot'],value=match['token'])],scope='Unary parser reaches token matcher55 before any operand. Actual one-byte formal and full physical4-byte slot captured, with distinct expression lookahead149dc/lexer144f4 and initialized source/frame/macros. No matcher result, prefetch or unary result claimed.')
        if expression_match:
            operator=args['defined_operator'];assert operator['name']==globals['rc_macro_name']['address']==condition['macro']and a['words'][1]==0x71c38128
            assert operator['formal_bytes']==operator['physical_slot_bytes']==4 and operator['caller']==caller
            assert raw_bytes(text,'defined-name-slot')==struct.pack('<I',a['words'][1])
            assert raw_bytes(text,'defined-name')==bytes.fromhex(operator['name_bytes'])==identifier_bytes and bytes.fromhex(globals['rc_macro_name']['bytes'])[:16]==identifier_bytes
            assert operator['matcher_token']==55 and operator['matcher_formal_bytes']==1 and operator['matcher_formal_address']==operator['matcher_slot_address']
            assert bytes.fromhex(operator['matcher_slot'])==struct.pack('<I',a['words'][6])==struct.pack('<I',55)and a['words'][5]==0x71c22ff4
            assert operator['lexer_result']==2
            value_elf,value_width=data_symbol('rc_expression_value');assert value_width==operator['expression_value_bytes']==4
            assert operator['expression_value_address']-value_elf==int(frontier_runtime[0],16)-int(frontier_elf[0],16)
            assert raw_bytes(text,'expression-value')==old_globals[0x18e78-0x14000:0x18e7c-0x14000]==struct.pack('<i',operator['expression_value'])==bytes(4)
            proof['condition_evaluation'].update(next_formals=[dict(name='name',formal_bytes=4,physical_slot_bytes=4,alias='rc_macro_name',initialized_input_bytes=16,text='defined')],scope='Matcher55 prefetches an identifier through the corrected ordinary lexer branch and reaches the real defined-operator helper. Actual UTF16 input defined+NUL, pointer alias, caller/source4678/left-parenthesis40, saved matcher BYTE55/full4B slot, distinct token storage and expression-value DWORD18e78 are bound. No defined-helper result, matcher return or expression result claimed.')
            proof['expression_match']=dict(token_consumed_bits=8,token=55,physical_slot_raw=operator['matcher_slot'],lexer_kind=2,original_lexer_kind_derivation='Actual return70.73 is the helper call reached only by matcher case2; raw EAX0 here is the dispatch subtraction result, not a claimed lexer-return register.',source_offset=source_offset,current_word=source_word,identifier_length=len(identifier)+1,identifier_hash=identifier_hash,next_name_bytes=16,expression_value_original_rva='0x18e78',expression_value_native_address=operator['expression_value_address'],expression_value=0,return_pending=True)

    proof['trial']=trial;return proof

def include_search_trial(original,native):
    return _include_search_checkpoint(original,native)

def scan_comment_trial(original,native):
    return _include_search_checkpoint(original,native,comment=True)

def character_match_trial(original,native):
    return _include_search_checkpoint(original,native,matcher=True)

def block_comment_trial(original,native):
    return _include_search_checkpoint(original,native,block=True)

def push_condition_trial(original,native):
    return _include_search_checkpoint(original,native,push=True)

def parse_define_trial(original,native):
    return _include_search_checkpoint(original,native,define=True)

def replacement_reader_trial(original,native):
    return _include_search_checkpoint(original,native,replacement=True)

def macro_space_trial(original,native):
    return _include_search_checkpoint(original,native,bounds=True)

def parameter_lookup_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True)

def evaluate_condition_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True)

def conditional_expression_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True)

def logical_or_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True)

def logical_and_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True)

def bitwise_or_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True)

def bitwise_xor_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True)

def bitwise_and_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True)

def equality_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True,equality=True)

def relational_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True,equality=True,relational=True)

def shift_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True,equality=True,relational=True,shift=True)

def additive_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True,equality=True,relational=True,shift=True,additive=True)

def multiplicative_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True,equality=True,relational=True,shift=True,additive=True,multiplicative=True)

def unary_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True,equality=True,relational=True,shift=True,additive=True,multiplicative=True,unary=True)


def expression_match_trial(original,native):
    return _include_search_checkpoint(original,native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True,equality=True,relational=True,shift=True,additive=True,multiplicative=True,unary=True,expression_match=True)
