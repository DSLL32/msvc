"""Raw-evidence validation; no tracing/disassembly or subprocess execution."""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[2]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
NATIVE_PROOF_IMAGE=None
def native_image():return NATIVE_PROOF_IMAGE or HERE/'build/native/rcdll-host'
# Naming compatibility only: historical raw captures/receipts retain their names.
HISTORICAL_IDENTITIES={
    'rcdll_main':('DllMain',0x8b29,0x8b46,'stdcall',12),
    'recon_dll_entry':('_DllMainCRTStartup',0x12d20,0x12ddf,'stdcall',12),
    'rcdll_crt_init':('_CRT_INIT',0x12bb0,0x12d1c,'stdcall',12),
    'rc_main':('main',0x7443,0x845c,'cdecl',0),
}
LOGICAL_NAME_ALIASES={name:value[0]for name,value in HISTORICAL_IDENTITIES.items()}
LOGICAL_NAME_ALIASES['rcdll_command_main']='main' # Native target-TU linker alias only.
def logical_name(name):return LOGICAL_NAME_ALIASES.get(name,name)
def native_name(name):return 'rcdll_command_main' if name=='main' else name
def logical_trace(text):return [logical_name(name)for name in text.splitlines()]
def normalize_runtime(value):
    if isinstance(value,list):return [normalize_runtime(item)for item in value]
    if isinstance(value,dict):return {key:(logical_name(item)if key in ['function','caller']and isinstance(item,str)else normalize_runtime(item))for key,item in value.items()}
    return value
def normalize_configuration(raw):
    # A logical view only. Never rewrite immutable historical evidence.
    config=json.loads(json.dumps(raw))
    for function in config['functions']:
        if function['name'] in HISTORICAL_IDENTITIES:
            name,start,end,abi,slots=HISTORICAL_IDENTITIES[function['name']]
            assert (int(function['start'],16),int(function['end'],16),function['abi'],function['stack_bytes'])==(start,end,abi,slots),'unbound historical alias'
        else:assert function['name']!='rcdll_command_main','native linker alias is not an original logical ID'
        function['name']=logical_name(function['name'])
    config['accepted']=[logical_name(name)for name in config['accepted']]
    if config.get('trial_function'):config['trial_function']=logical_name(config['trial_function'])
    return config
def read_configuration(path):return normalize_configuration(json.loads(path.read_text()))

def candidate():
    # Execution identity is separate from checker versions: validator-only edits
    # can recheck retained raw observations without another original invocation.
    paths=[HERE/'functions.json',HERE/'build/RCDLL.DLL',HERE/'build/rcdll.map',HERE/'build/native/rcdll-host',HERE/'build/debug-original.exe',HERE/'build/help-resources.bin',ROOT/'toolchains/5.10/bin/RCDLL.DLL',ROOT/'toolchains/5.10/bin/MSVCRT.DLL',ROOT/'toolchains/5.10/bin/RC.EXE']
    return dict(schema=2,files={str(p.relative_to(ROOT)):sha(p) for p in paths})

def validation_versions():
    paths=sorted((HERE/'scripts').glob('*.py'))
    paths+=[HERE/'data.json']
    return {str(p.relative_to(ROOT)):sha(p) for p in paths if p.is_file()}

def bind_current(snapshot,side,reuse=None):
    saved=json.loads((snapshot/'candidate.json').read_text())
    current=candidate()
    irrelevant={'decomp/vc5.10/rcdll/build/native/rcdll-host','decomp/vc5.10/rcdll/build/help-resources.bin'} if side=='original' else {'decomp/vc5.10/rcdll/build/debug-original.exe'}
    for path,value in current['files'].items():
        if path not in irrelevant:
            if reuse is not None and path in reuse['allowed_changed_files']:
                assert side=='original'and saved['files'][path]==reuse['previous']['files'][path]and value==reuse['current']['files'][path],'unbound original reuse '+path
            else:assert saved['files'].get(path)==value,'stale execution pin '+path
    assert saved['schema']==current['schema']
    receipt=json.loads((snapshot/'receipt.json').read_text())
    assert sha(snapshot/'functions.json')==receipt['candidate_sha256'],'wrong capture config'
    if reuse is None:assert receipt['candidate_sha256']==sha(HERE/'functions.json'),'wrong masked/native config'
    else:assert side=='original'and receipt['candidate_sha256']==reuse['previous']['files']['decomp/vc5.10/rcdll/functions.json']
    for name,info in receipt['files'].items():
        path=snapshot/name
        assert path.is_file() and path.stat().st_size==info['bytes'] and sha(path)==info['sha256'],'changed raw capture '+name
    return receipt

def module_offset_reads(snapshot,text):
    proof={}
    for name in ['loaded-target.bin','loaded-crt.bin','loaded-driver.bin']:
        size=(snapshot/name).stat().st_size
        assert re.findall(r'^module-hfile-size '+re.escape(name)+r' (\d+)$',text,re.M)==[str(size)]
        rows=re.findall(r'^module-read-at '+re.escape(name)+r' offset=(\d+) requested=(\d+) received=(\d+) status=(\d+) error=([0-9a-f]+)$',text,re.M)
        offset=0
        for at,wanted,received,status,error in rows:
            length=min(16384,size-offset)
            assert int(at)==offset and int(wanted)==int(received)==length and length>0 and int(status)==1
            offset+=length
        assert rows and offset==size and re.findall(r'^module-file '+re.escape(name)+r' (\d+)$',text,re.M)==[str(size)]
        proof[name]=dict(bytes=size,reads=len(rows),sha256=sha(snapshot/name),same_actual_hFile=True,explicit_offsets=True)
    return proof

def original_raw(snapshot,complete=False,reuse=None):
    receipt=bind_current(snapshot,'original',reuse)
    assert receipt['mode']=='masked' and not receipt['timed_out'] and receipt['status']==(0 if complete else 124)
    assert receipt['actual_target_verified'] and receipt['actual_crt_verified']
    assert sha(snapshot/'loaded-target.bin')==sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL'),'actual loaded target bytes differ'
    assert sha(snapshot/'loaded-crt.bin')==sha(ROOT/'toolchains/5.10/bin/MSVCRT.DLL')=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210','actual loaded canonical CRT bytes differ'
    text=(snapshot/'events.txt').read_text()
    assert 'snapshot-ready mode=masked' in text and 'entry-guard ' in text
    relocated=(snapshot/'relocated-text.bin').read_bytes()
    masked=(snapshot/'masked-text.bin').read_bytes()
    expected=bytearray(b'\xcc'*len(relocated))
    for start,end in [(0x1000,0x114c),(0x12e2c,0x13371)]:expected[start-0x1000:end-0x1000]=relocated[start-0x1000:end-0x1000]
    active=read_configuration(snapshot/'functions.json')
    if active.get('original_capture_io')=='actual-hFile-overlapped-offset-v1':module_offset_reads(snapshot,text)
    for function in active['functions']:
        if function['name'] in active['accepted']+[active.get('trial_function')]:
            start,end=int(function['start'],16),int(function['end'],16)
            expected[start-0x1000:end-0x1000]=relocated[start-0x1000:end-0x1000]
            for start,end in function.get('text_data',[]):
                start,end=int(start,16),int(end,16)
                expected[start-0x1000:end-0x1000]=relocated[start-0x1000:end-0x1000]
    for function in active.get('support_imports',[]):
        if function['accepted']:
            start,end=int(function['start'],16),int(function['end'],16)
            expected[start-0x1000:end-0x1000]=relocated[start-0x1000:end-0x1000]
    for function in active.get('support_compiler',[]):
        if function.get('accepted')or function.get('trial'):
            start,end=int(function['start'],16),int(function['end'],16)
            assert (start,end)==(0x12df0,0x12e1f)and function['abi']=='custom-eax-esp'
            expected[start-0x1000:end-0x1000]=relocated[start-0x1000:end-0x1000]
    assert masked==bytes(expected),'actual mask readback differs from exact candidate snapshot ranges'
    assert 'error ' not in text and 'exception ' not in text,'unhandled original exception'
    match=re.findall(r'^frontier ([0-9a-f]{8}) .*$',text,re.M)
    if complete:
        assert match==[] and re.findall(r'^exit (\d+)$',text,re.M)==['0'],'missing real original process completion'
        assert (snapshot/'completion-globals.bin').stat().st_size==87148
        return dict(kind='completion',status=0)
    assert len(match)==1,'need exactly one raw-derived original frontier'
    rva=int(match[0],16)
    config=read_configuration(snapshot/'functions.json')
    functions=[f for f in config['functions']+config.get('support_compiler',[]) if int(f['start'],16)==rva]
    assert len(functions)==1,'unindexed original frontier'
    stack=re.findall(r'^stack((?: [0-9a-f]{8}){12})$',text,re.M)
    assert len(stack)==1
    words=[int(v,16)for v in stack[0].split()]
    registers={name:int(value,16)for name,value in re.findall(r'\b(eax|ecx|edx|ebx|esi|edi|ebp|esp)=([0-9a-f]{8})',next(line for line in text.splitlines()if line.startswith('frontier ')))}
    assert len(registers)==8
    return dict(function=functions[0]['name'],rva=rva,words=words,registers=registers)

def native_raw(snapshot):
    receipt=bind_current(snapshot,'native')
    assert receipt['native_sha256']==sha(native_image())
    raw=json.loads((snapshot/'snapshot.json').read_text())
    assert raw['kind']=='frontier' and raw['function'] and raw['executed_byte']=='cc'
    assert raw['config_sha256']==receipt['candidate_sha256']
    gdb=(snapshot/'gdb.stdout').read_text()
    assert 'SIGTRAP' in gdb,'no debugger trap stop'
    pc=re.findall(r'^RCDLL_PC=([0-9a-f]+)$',gdb,re.M)
    assert len(pc)==1 and int(pc[0],16)==raw['raw_pc'],'raw GDB PC differs from snapshot'
    assert re.search(r'^#0\s+(?:0x[0-9a-f]+ in )?'+re.escape(raw['function'])+r'\s*\(',gdb,re.M),'raw GDB frame differs from snapshot'
    runtime=re.findall(r'^Symbol "'+re.escape(raw['function'])+r'" is a function at address (0x[0-9a-f]+)',gdb,re.M)
    assert len(runtime)==1,'missing raw GDB function address'
    runtime=int(runtime[0],16)
    native=native_image()
    symbols=subprocess.check_output(['nm','-n','--defined-only',str(native)],text=True)
    functions=[(int(a,16),n)for a,t,n in re.findall(r'^([0-9a-f]+) ([Tt]) (\S+)$',symbols,re.M)]
    address=[a for a,n in functions if n==raw['function']]
    assert len(address)==1,'ELF function identity is not unique'
    address=address[0];elf_pc=address+raw['raw_pc']-runtime
    ends=[a for a,n in functions if a>address]
    assert ends and address<elf_pc<=min(ends),'PC is outside the indexed ELF function'
    binary=native.read_bytes();assert binary[:6]==b'\x7fELF\x01\x01','native must be ELF32 little endian'
    phoff=struct.unpack_from('<I',binary,28)[0];size,count=struct.unpack_from('<HH',binary,42)
    loaded=[]
    for index in range(count):
        kind,offset,start,_,filesize=struct.unpack_from('<IIIII',binary,phoff+size*index)
        if kind==1 and start<=elf_pc-1<start+filesize:loaded.append(binary[offset+elf_pc-1-start])
    assert loaded==[0xcc],'raw GDB PC does not follow an INT3 in current ELF bytes'
    return normalize_runtime(raw)

def initial_pair(original,native):
    a=original_raw(original);b=native_raw(native)
    assert a['function']==b['function']=='_DllMainCRTStartup'
    assert a['rva']==int(b['rva'],16)==0x12d20
    assert a['words'][1]==b['arguments']['instance']==0x71c20000
    assert a['words'][2]==b['arguments']['reason']==1
    assert bool(a['words'][3]) and bool(b['arguments']['reserved']),'process-startup context must be nonNULL'
    for stream in ['stdout.bin','stderr.bin']:
        assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(matched_frontier=a['function'],rva=hex(a['rva']),accepted_functions=0,scope='initial DLL entry only; opaque startup contexts retained raw, not dereferenced by a reconstructed function yet')

def entry_trial_pair(original,native):
    """First source body: only argument forwarding and two read-only globals."""
    a=original_raw(original);b=native_raw(native)
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='_DllMainCRTStartup' and config['accepted']==[]
    assert a['function']==b['function']=='_CRT_INIT' and a['rva']==int(b['rva'],16)==0x12bb0
    module=b['module']
    assert b['arguments']['instance']==module['token'] and module['token']!=0
    assert module['name'].lower()=='rcdll.dll' and module['flags']==2
    assert module['sha256']==sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL')
    original_events=(original/'events.txt').read_text()
    bases=re.findall(r'^module RCDLL.dll actual=([0-9a-f]+) ',original_events,re.M)
    paths=re.findall(r'^module-path target (.+)$',original_events,re.M)
    assert len(bases)==len(paths)==1 and int(bases[0],16)==a['words'][1]
    assert paths[0]==module['path'] and paths[0].startswith('S:\\'), 'registered module must match actual captured S: path'
    assert a['words'][2]==b['arguments']['reason']==1
    assert a['words'][3] and b['arguments']['reserved'],'opaque process-startup context must remain nonNULL'
    assert 0x71c32d20<=a['words'][0]<0x71c32ddf,'original next call is not from startup wrapper'
    gdb=(native/'gdb.stdout').read_text()
    assert re.search(r'^#1\s+0x[0-9a-f]+ in (?:_DllMainCRTStartup|recon_dll_entry)\s*\(',gdb,re.M),'native next call is not from startup wrapper'
    for stream in ['stdout.bin','stderr.bin']:
        assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    old=(original/'globals.bin').read_bytes()
    declarations=json.loads((HERE/'data.json').read_text())
    assert (native/'data.json').read_bytes()==(HERE/'data.json').read_bytes(),'different native state declaration'
    matched={}
    for item in declarations:
        offset=int(item['original_rva'],16)-0x14000
        value=old[offset:offset+item['bytes']]
        assert len(value)==item['bytes'] and value.hex()==b['globals'][item['name']],'global state differs '+item['name']
        if item['kind']=='null-function-pointer':assert value==b'\0'*4,'first case cannot invoke an unproved raw DLL hook'
        matched[item['name']]=value.hex()
    assert set(matched)=={'dll_attach_count','dll_raw_main'}
    assert matched['dll_attach_count']=='00000000'
    trace=[int(v,16)for v in re.findall(r'^entry ([0-9a-f]+)$',(original/'events.txt').read_text(),re.M)]
    assert trace==[0x12d20],'wrong trial execution prefix'
    return dict(trial='_DllMainCRTStartup',matched_frontier='_CRT_INIT',globals=matched,arguments=dict(original_instance=a['words'][1],native_instance=b['arguments']['instance'],registered_module=module,reason=a['words'][2],original_reserved=a['words'][3],native_reserved=b['arguments']['reserved'],reserved_contract='opaque nonNULL process-startup context; wrapper only forwards, no dereference'),application_functions=0,crt_support_functions=1)

def lifecycle_arguments(original,native,a,b):
    module=b['module']
    assert b['arguments']['instance']==module['token'] and module['token']
    assert module['name'].lower()=='rcdll.dll' and module['flags']==2
    assert module['sha256']==sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL')
    text=(original/'events.txt').read_text()
    actual=re.findall(r'^module RCDLL.dll actual=([0-9a-f]+) ',text,re.M)
    paths=re.findall(r'^module-path target (.+)$',text,re.M)
    assert len(actual)==len(paths)==1
    assert a['words'][1]==int(actual[0],16)
    assert module['path']==paths[0] and paths[0].startswith('S:\\')
    assert a['words'][2]==b['arguments']['reason']==1
    assert a['words'][3] and b['arguments']['reserved']
    return dict(original_module=a['words'][1],registered_native_module=module,reason=1,original_reserved=a['words'][3],native_reserved=b['arguments']['reserved'])

def crt_init_pair(original,native):
    a=original_raw(original);b=native_raw(native)
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='_CRT_INIT'
    assert config['accepted']==['_DllMainCRTStartup']
    assert a['function']==b['function']=='DllMain' and a['rva']==int(b['rva'],16)==0x8b29
    arguments=lifecycle_arguments(original,native,a,b)
    raw=(original/'globals.bin').read_bytes();matched={}
    assert (native/'data.json').read_bytes()==(HERE/'data.json').read_bytes()
    for item in json.loads((HERE/'data.json').read_text()):
        if item['kind']=='exit-array-pointer':continue
        offset=int(item['original_rva'],16)-0x14000
        value=raw[offset:offset+item['bytes']]
        assert len(value)==item['bytes'] and value.hex()==b['globals'][item['name']],item['name']
        matched[item['name']]=value.hex()
    assert matched==dict(dll_attach_count='01000000',dll_raw_main='00000000',dll_platform_kind='ffffffff',dll_fdiv_adjust='00000000',dll_constructors='00000000')
    old_end,old_base=struct.unpack_from('<II',raw,0x15460)
    text=(original/'events.txt').read_text()
    sizes=[int(v)for v in re.findall(r'^crt-malloc-size (\d+)$',text,re.M)]
    pointers=[int(v,16)for v in re.findall(r'^crt-malloc-result ([0-9a-f]+)$',text,re.M)]
    heads=re.findall(r'^crt-exit-head 4 ([0-9a-f]+)$',text,re.M)
    assert sizes==[128] and pointers==[old_base] and old_end==old_base!=0 and heads==['00000000']
    new=b['exit_array']
    assert new['base']==new['end'] and new['first']=='00000000'
    assert b['allocations']==[dict(caller='_CRT_INIT',bytes=128,result=new['base'])]
    assert int.from_bytes(bytes.fromhex(b['globals']['dll_exit_base']),'little')==new['base']
    assert int.from_bytes(bytes.fromhex(b['globals']['dll_exit_end']),'little')==new['end']
    assert re.findall(r'^entry ([0-9a-f]+)$',text,re.M)==['00012d20','00012bb0','00012e26']
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(trial='_CRT_INIT',matched_frontier='DllMain',arguments=arguments,globals=matched,exit_vector=dict(request_bytes=128,logical_cursor=0,first_callback=None,unwritten_tail='unread in this case; no callbacks registered'),application_functions=0,crt_support_functions=2,import_boundaries=1)

def _rc_host_pair(original,native,stage):
    a=original_raw(original);b=native_raw(native)
    config=read_configuration(original/'functions.json')
    prefix=['_DllMainCRTStartup','_CRT_INIT']
    expected='RC' if stage=='DllMain' else 'main'
    expected_rva=0x8b46 if stage=='DllMain' else 0x7443
    assert config.get('trial_function')==stage and config['accepted']==prefix+([] if stage=='DllMain' else ['DllMain'])
    assert a['function']==b['function']==expected and a['rva']==int(b['rva'],16)==expected_rva
    text=(original/'events.txt').read_text();module=b['module'];raw=(original/'globals.bin').read_bytes()
    bases=re.findall(r'^module RCDLL.dll actual=([0-9a-f]+) ',text,re.M)
    paths=re.findall(r'^module-path target (.+)$',text,re.M)
    assert len(bases)==len(paths)==1 and module['token']
    assert module['name'].lower()=='rcdll.dll' and module['flags']==2
    assert module['sha256']==sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL')
    assert module['path']==paths[0] and paths[0].startswith('S:\\')
    assert struct.unpack_from('<I',raw,0x11f64)[0]==int(bases[0],16)
    assert int.from_bytes(bytes.fromhex(b['globals']['dll_instance']),'little')==module['token']
    assert module['thread_calls_disabled'] is True
    flags=re.findall(r'^module-loader-flags ([0-9a-f]+)$',text,re.M)
    assert len(flags)==1 and int(flags[0],16)&0x40000
    assert (native/'data.json').read_bytes()==(HERE/'data.json').read_bytes()
    matched={}
    for item in json.loads((HERE/'data.json').read_text()):
        if item['kind'] in ['exit-array-pointer','module-handle','window-handle','diagnostic-callback','resource-callback']:continue
        offset=int(item['original_rva'],16)-0x14000
        value=raw[offset:offset+item['bytes']]
        assert value.hex()==b['globals'][item['name']],item['name']
        matched[item['name']]=value.hex()
    assert matched==dict(dll_attach_count='01000000',dll_raw_main='00000000',dll_platform_kind='ffffffff',dll_fdiv_adjust='00000000',dll_constructors='00000000')
    old_end,old_base=struct.unpack_from('<II',raw,0x15460);new=b['exit_array']
    assert re.findall(r'^crt-malloc-size (\d+)$',text,re.M)==['128']
    assert [int(v,16)for v in re.findall(r'^crt-malloc-result ([0-9a-f]+)$',text,re.M)]==[old_base]
    assert old_base==old_end!=0 and re.findall(r'^crt-exit-head 4 ([0-9a-f]+)$',text,re.M)==['00000000']
    assert new['base']==new['end']!=0 and new['first']=='00000000'
    assert b['allocations']==[dict(caller='_CRT_INIT',bytes=128,result=new['base'])]
    for field in ['base','end']:
        assert int.from_bytes(bytes.fromhex(b['globals']['dll_exit_'+field]),'little')==new[field]
    args=b['arguments'];words=a['words']
    if stage=='DllMain':
        assert words[0]==0x010011fd and words[1]==args['window']==0 and words[2]==args['unused']==0
        assert words[3]==0x01001100 and args['diagnostic']==args['host_callback']!=0
        assert words[4]==args['resource']==0 and words[5]==args['count']==2
    else:
        assert words[0]==0x71c28b6c and words[1]==args['count']==2
        assert struct.unpack_from('<III',raw,0x11f58)==(0,0x01001100,0)
        assert b['globals']['rc_window']==b['globals']['rc_resource']=='00000000'
        assert int.from_bytes(bytes.fromhex(b['globals']['rc_diagnostic']),'little')==args['host_callback']!=0
        assert re.search(r'^#1\s+0x[0-9a-f]+ in RC\s*\(',(native/'gdb.stdout').read_text(),re.M)
    vectors=re.findall(r'^argv-(\d+) (\d+) ([0-9a-f]+)$',text,re.M)
    assert [int(i)for i,_,_ in vectors]==[0,1]
    argv=[]
    for _,length,value in vectors:
        value=bytes.fromhex(value);assert len(value)==int(length) and value[-1:]==b'\0'
        argv.append(value[:-1].decode('ascii'))
    assert argv==args['argv'] and argv[0]==paths[0].rsplit('\\',1)[0]+'\\RC.EXE' and argv[1]=='/help'
    assert re.findall(r'^argument-count (\d+)$',text,re.M)==['2']
    assert re.findall(r'^argv-terminator ([0-9a-f]+)$',text,re.M)==['00000000'] and args['terminator']==0
    assert re.findall(r'^entry ([0-9a-f]+)$',text,re.M)==['00012d20','00012bb0','00012e26','00008b29']+([] if stage=='DllMain' else ['00008b46'])
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(trial=stage,matched_frontier=expected,module=module,globals=matched,export_arguments=dict(window=0,unused=0,diagnostic='verified RC.EXE stdcall callback / native fixture_diagnostic',resource=None,count=2,argv=argv),application_functions=1 if stage=='DllMain' else 2,crt_support_functions=2,import_boundaries=1)

def dll_main_pair(original,native):return _rc_host_pair(original,native,"DllMain")
def rc_entry_pair(original,native):return _rc_host_pair(original,native,"RC")

def help_state(original,native,a,b,extra_special=()):
    text=(original/'events.txt').read_text();raw=(original/'globals.bin').read_bytes();module=b['module']
    bases=re.findall(r'^module RCDLL.dll actual=([0-9a-f]+) ',text,re.M)
    paths=re.findall(r'^module-path target (.+)$',text,re.M)
    assert len(bases)==len(paths)==1 and module['token']
    assert module['name'].lower()=='rcdll.dll' and module['flags']==2
    assert module['sha256']==sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL') and module['path']==paths[0]
    assert struct.unpack_from('<I',raw,0x11f64)[0]==int(bases[0],16)
    assert int.from_bytes(bytes.fromhex(b['globals']['dll_instance']),'little')==module['token']
    assert module['thread_calls_disabled'] is True
    assert re.findall(r'^module-loader-flags ([0-9a-f]+)$',text,re.M)==['000c0004']
    assert (native/'data.json').read_bytes()==(HERE/'data.json').read_bytes()
    matched={}
    special=['exit-array-pointer','module-handle','diagnostic-callback','opaque-jump-context','private-heap','include-buffer']
    for item in json.loads((HERE/'data.json').read_text()):
        if item['kind'] in special or item['name']in extra_special:continue
        offset=int(item['original_rva'],16)-0x14000
        value=raw[offset:offset+item['bytes']]
        assert value.hex()==b['globals'][item['name']],item['name']
        matched[item['name']]=value.hex()
    old_end,old_base=struct.unpack_from('<II',raw,0x15460);new=b['exit_array']
    assert re.findall(r'^crt-malloc-size (\d+)$',text,re.M)==['128']
    assert [int(v,16)for v in re.findall(r'^crt-malloc-result ([0-9a-f]+)$',text,re.M)]==[old_base]
    assert old_base==old_end!=0 and re.findall(r'^crt-exit-head 4 ([0-9a-f]+)$',text,re.M)==['00000000']
    assert new['base']==new['end']!=0 and new['first']=='00000000'
    assert b['allocations']==[dict(caller='_CRT_INIT',bytes=128,result=new['base'])]
    for field in ['base','end']:assert int.from_bytes(bytes.fromhex(b['globals']['dll_exit_'+field]),'little')==new[field]
    assert struct.unpack_from('<III',raw,0x11f58)==(0,0x01001100,0)
    assert matched['rc_window']==matched['rc_resource']=='00000000'
    callback=int.from_bytes(bytes.fromhex(b['globals']['rc_diagnostic']),'little')
    assert callback==b['diagnostic_callback']!=0
    return matched

def rc_main_pair(original,native):
    a=original_raw(original);b=native_raw(native)
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='main'
    assert config['accepted']==['_DllMainCRTStartup','_CRT_INIT','DllMain','RC']
    assert a['function']==b['function']=='rc_initialize' and a['rva']==int(b['rva'],16)==0x84c4
    assert a['words'][0]==0x71c2747a
    assert re.search(r'^#1\s+0x[0-9a-f]+ in (?:rcdll_command_main|rc_main)\s*\(',(native/'gdb.stdout').read_text(),re.M)
    matched=help_state(original,native,a,b)
    for field in ['rc_heap','rc_include_paths']:
        item=next(v for v in json.loads((HERE/'data.json').read_text())if v['name']==field)
        offset=int(item['original_rva'],16)-0x14000
        assert (original/'globals.bin').read_bytes()[offset:offset+4]==bytes.fromhex(b['globals'][field])==b'\0'*4
    text=(original/'events.txt').read_text()
    calls=re.findall(r'^setjmp-call ([0-9a-f]+) (\d+) registration=([0-9a-f]+) try-level=([0-9a-f]+)$',text,re.M)
    assert len(calls)==1 and calls[0][0]=='71c45998' and calls[0][1]=='0'
    before=re.findall(r'^setjmp-before 64 ([0-9a-f]+)$',text,re.M)
    after=re.findall(r'^setjmp-after 64 ([0-9a-f]+)$',text,re.M)
    assert before==['00'*64] and len(after)==1
    context=bytes.fromhex(after[0]);words=struct.unpack('<16I',context)
    assert words[5]==0x71c2746b and words[6]==int(calls[0][2],16) and words[7]==int(calls[0][3],16)
    assert words[8:]==(0x56433230,0,0,0,0,0,0,0)
    raw=(original/'globals.bin').read_bytes()
    assert raw[0x11998:0x119d8]==context
    assert re.findall(r'^setjmp-result (-?\d+)$',text,re.M)==['0']
    jump=b['jump'];assert jump and jump['legacy'] and jump['opaque_setjmp3']==1
    host=bytes.fromhex(jump['host_bytes']);assert len(host)==156 and any(host[:24])
    assert b['globals']['rc_abort_context']=='00'*64
    assert re.findall(r'^entry ([0-9a-f]+)$',text,re.M)==['00012d20','00012bb0','00012e26','00008b29','00008b46','00007443','00012de0']
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(trial='main',matched_frontier='rc_initialize',globals=matched,jump_context=dict(original_raw=context.hex(),registration=int(calls[0][2],16),try_level=int(calls[0][3],16),native=jump,contract='opaque count0 save; real caller-frame host context; target buffer never read, target longjmp/unwind rejected'),application_functions=3,crt_support_functions=2,import_boundaries=2)

def rc_initialize_pair(original,native):
    a=original_raw(original);b=native_raw(native)
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_initialize'
    assert config['accepted']==['_DllMainCRTStartup','_CRT_INIT','DllMain','RC','main']
    assert a['function']==b['function']=='rc_allocate' and a['rva']==int(b['rva'],16)==0x109b2
    assert a['words'][0]==0x71c274b7 and a['words'][1]==b['arguments']['bytes']==520
    assert re.search(r'^#1\s+0x[0-9a-f]+ in (?:rcdll_command_main|rc_main)\s*\(',(native/'gdb.stdout').read_text(),re.M)
    matched=help_state(original,native,a,b)
    assert matched['rc_language']=='8000'
    text=(original/'events.txt').read_text();raw=(original/'globals.bin').read_bytes()
    assert re.findall(r'^heap-create (\d+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)==[('1','0','0','00000000')]
    result=re.findall(r'^heap-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    assert len(result)==1 and result[0][1]=='00000000'
    original_heap=struct.unpack_from('<I',raw,0x11f54)[0]
    native_heap=int.from_bytes(bytes.fromhex(b['globals']['rc_heap']),'little')
    assert original_heap==int(result[0][0],16)!=0 and native_heap
    assert b['heap_identity']==dict(handle=native_heap,options=1,initial=0,maximum=0,blocks=[])
    assert b['heap']==[dict(operation='create',caller='rc_initialize',last_error_before=0,flags=1,initial=0,maximum=0,result=native_heap,last_error_after=0)]
    assert raw[0x11588:0x1158c]==bytes.fromhex(b['globals']['rc_include_paths'])==b'\0'*4
    assert re.findall(r'^entry ([0-9a-f]+)$',text,re.M)==['00012d20','00012bb0','00012e26','00008b29','00008b46','00007443','00012de0','000084c4']
    assert b['jump']['opaque_setjmp3']==1
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(trial='rc_initialize',matched_frontier='rc_allocate',globals=matched,private_heap=dict(original=original_heap,native=native_heap,flags=1,initial=0,maximum=0,last_error=0),next_allocation_bytes=520,application_functions=4,crt_support_functions=2,import_boundaries=2)

def help_allocation(original,b,global_name='globals.bin'):
    text=(original/'events.txt').read_text();raw=(original/global_name).read_bytes()
    old_heap=struct.unpack_from('<I',raw,0x11f54)[0]
    new_heap=int.from_bytes(bytes.fromhex(b['globals']['rc_heap']),'little')
    creates=re.findall(r'^heap-create (\d+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    assert creates==[('1','0','0','00000000')]
    assert re.findall(r'^heap-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)==[('%08x'%old_heap,'00000000')]
    calls=re.findall(r'^heap-alloc ([0-9a-f]+) (\d+) (\d+) last-error=([0-9a-f]+)$',text,re.M)
    assert calls==[('%08x'%old_heap,'9','528','00000000')]
    pointers=re.findall(r'^heap-alloc-result ([0-9a-f]+) last-error=([0-9a-f]+)$',text,re.M)
    assert len(pointers)==1 and pointers[0][1]=='00000000'
    old_pointer=int(pointers[0][0],16);assert old_pointer
    assert re.findall(r'^heap-alloc-payload 528 ([0-9a-f]+)$',text,re.M)==['00'*528]
    assert struct.unpack_from('<I',raw,0x11588)[0]==old_pointer+8
    identity=b['heap_identity'];assert identity['handle']==new_heap!=0
    assert identity['options']==1 and identity['initial']==identity['maximum']==0
    assert len(identity['blocks'])==1
    block=identity['blocks'][0];assert block['owner']==new_heap and block['bytes']==528 and block['flags']==9
    assert block['contents']=='00'*528 and block['payload']
    assert int.from_bytes(bytes.fromhex(b['globals']['rc_include_paths']),'little')==block['payload']+8
    assert b['heap']==[dict(operation='create',caller='rc_initialize',last_error_before=0,flags=1,initial=0,maximum=0,result=new_heap,last_error_after=0),dict(operation='allocate',caller='rc_allocate',last_error_before=0,heap=new_heap,flags=9,bytes=528,result=block['payload'],last_error_after=0,payload='00'*528)]
    return dict(original_heap=old_heap,native_heap=new_heap,bytes=528,target_header_bytes=8,buffer_bytes=520,flags=9,zeroed=True)

def rc_allocate_pair(original,native):
    a=original_raw(original);b=native_raw(native);config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_allocate'
    assert config['accepted']==['_DllMainCRTStartup','_CRT_INIT','DllMain','RC','main','rc_initialize']
    assert a['function']==b['function']=='rc_is_option' and a['rva']==int(b['rva'],16)==0x10c4c
    assert a['words'][0]==0x71c274dc and a['words'][1]&255==b['arguments']['value']==ord('/')
    assert re.search(r'^#1\s+0x[0-9a-f]+ in (?:rcdll_command_main|rc_main)\s*\(',(native/'gdb.stdout').read_text(),re.M)
    matched=help_state(original,native,a,b);allocation=help_allocation(original,b)
    assert re.findall(r'^entry ([0-9a-f]+)$',(original/'events.txt').read_text(),re.M)==['00012d20','00012bb0','00012e26','00008b29','00008b46','00007443','00012de0','000084c4','000109b2']
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(trial='rc_allocate',matched_frontier='rc_is_option',globals=matched,allocation=allocation,option_character='/',raw_original_char_slot=a['words'][1],application_functions=5,crt_support_functions=2,import_boundaries=2)

HELP_PREFIX=[0x12d20,0x12bb0,0x12e26,0x8b29,0x8b46,0x7443,0x12de0,0x84c4,0x109b2,0x10c4c,0x8bcd,0x2c64,0x2c98]

def rc_is_option_pair(original,native):
    a=original_raw(original);b=native_raw(native);config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_is_option' and len(config['accepted'])==7
    assert a['function']==b['function']=='rc_emit' and a['rva']==int(b['rva'],16)==0x8bcd
    assert a['words'][0]==0x71c27a82 and a['words'][1]==0x71c211fc
    assert re.findall(r'^emit-message 2 ([0-9a-f]+)$',(original/'events.txt').read_text(),re.M)==[b['arguments']['message']]=='0a00'.split()
    matched=help_state(original,native,a,b);allocation=help_allocation(original,b)
    assert [int(v,16)for v in re.findall(r'^entry ([0-9a-f]+)$',(original/'events.txt').read_text(),re.M)]==HELP_PREFIX[:10]
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(trial='rc_is_option',matched_frontier='rc_emit',globals=matched,allocation=allocation,message='0a00',application_functions=6,crt_support_functions=2,import_boundaries=2)

def callbacks(directory):
    text=(directory/'events.txt').read_text()
    records=re.findall(r'^callback (\d+) (\d+)\nbefore (\d+) ([0-9a-f]+)\nafter (\d+) ([0-9a-f]+)\ncallback-return (-?\d+)$',text,re.M)
    assert len(records)==len(re.findall(r'^callback ',text,re.M))
    result=[]
    for first,second,before_size,before,after_size,after,status in records:
        assert int(before_size)==len(bytes.fromhex(before)) and int(after_size)==len(bytes.fromhex(after))
        result.append(dict(first=int(first),second=int(second),before=before,after=after,result=int(status)))
    return result

def rc_emit_pair(original,native):
    a=original_raw(original);b=native_raw(native);config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_emit' and len(config['accepted'])==8
    assert a['function']==b['function']=='rc_message' and a['rva']==int(b['rva'],16)==0x2c64
    assert a['words'][0]==0x71c27a96 and a['words'][1]==b['arguments']['id']==10001
    matched=help_state(original,native,a,b);allocation=help_allocation(original,b)
    assert matched['rc_message_count']=='01000000' and matched['rc_line']==matched['rc_last_line']=='00000000'
    effects=callbacks(original);assert effects==callbacks(native)==[dict(first=0,second=0,before='0a00',after='0a00',result=0)]
    assert [int(v,16)for v in re.findall(r'^entry ([0-9a-f]+)$',(original/'events.txt').read_text(),re.M)]==HELP_PREFIX[:11]
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(trial='rc_emit',matched_frontier='rc_message',globals=matched,allocation=allocation,callbacks=effects,message_id=10001,application_functions=7,crt_support_functions=2,import_boundaries=2)

def rc_message_pair(original,native):
    a=original_raw(original);b=native_raw(native);config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_message' and len(config['accepted'])==9
    assert a['function']==b['function']=='rc_format' and a['rva']==int(b['rva'],16)==0x2c98
    assert a['words'][:6]==[0x71c27aa2,0x71c38660,2048,0x71c35ca0,0x71c214f8,1472]
    args=b['arguments'];assert args['capacity']==2048 and args['build']==1472 and args['version']=='5.00.1472.1'
    assert args['output']==args['expected_output']!=0
    metadata=json.loads((HERE/'cases/help/message-entries.json').read_text())
    entry=next(row for row in metadata['entries']if row['id']==10001)
    template=bytes.fromhex(entry['payload_hex']).split(b'\0',1)[0]
    assert args['template'].encode('ascii')==template
    text=(original/'events.txt').read_text()
    assert re.findall(r'^formatter-template 71 ([0-9a-f]+)$',text,re.M)==[(template+b'\0').hex()]
    assert re.findall(r'^formatter-version 12 ([0-9a-f]+)$',text,re.M)==[b'5.00.1472.1\0'.hex()]
    matched=help_state(original,native,a,b);allocation=help_allocation(original,b)
    assert bytes.fromhex(matched['rc_message_buffer'])==template+b'\0'*(4096-len(template))
    assert matched['rc_error_text']=='00'*2048
    effects=callbacks(original);assert effects==callbacks(native)==[dict(first=0,second=0,before='0a00',after='0a00',result=0)]
    assert [int(v,16)for v in re.findall(r'^entry ([0-9a-f]+)$',text,re.M)]==HELP_PREFIX[:12]
    for stream in ['stdout.bin','stderr.bin']:assert (original/stream).read_bytes()==(native/stream).read_bytes()==b''
    return dict(trial='rc_message',matched_frontier='rc_format',globals=matched,allocation=allocation,callbacks=effects,formatter=dict(capacity=2048,version=args['version'],build=1472,template=template.decode('ascii')),application_functions=8,crt_support_functions=2,import_boundaries=2)

def native_completion(snapshot):
    receipt=bind_current(snapshot,'native');assert receipt['status']==0
    assert receipt['native_sha256']==sha(native_image())
    raw=json.loads((snapshot/'snapshot.json').read_text())
    assert raw['kind']=='completion' and raw['function']=='fixture_complete' and raw['arguments']==dict(result=0)
    assert raw['config_sha256']==receipt['candidate_sha256']
    assert json.loads((snapshot/'exit.json').read_text())==dict(exit_code=0),'missing real native process completion'
    gdb=(snapshot/'gdb.stdout').read_text()
    assert re.search(r'^\[Inferior \d+ \(process \d+\) exited normally\]$',gdb,re.M)
    assert re.search(r'^#0\s+fixture_complete \(result=0\)',gdb,re.M)
    assert re.findall(r'^RCDLL_PC=([0-9a-f]+)$',gdb,re.M)==['%08x'%raw['raw_pc']]
    runtime=re.findall(r'^Symbol "fixture_complete" is a function at address (0x[0-9a-f]+)',gdb,re.M);assert len(runtime)==1
    symbols=subprocess.check_output(['nm','-n','--defined-only',str(native_image())],text=True)
    funcs=[(int(a,16),n)for a,t,n in re.findall(r'^([0-9a-f]+) ([Tt]) (\S+)$',symbols,re.M)]
    start=[a for a,n in funcs if n=='fixture_complete'];assert len(start)==1
    elf_pc=start[0]+raw['raw_pc']-int(runtime[0],16)
    assert start[0]<=elf_pc<min(a for a,n in funcs if a>start[0])
    return normalize_runtime(raw)

def rc_format_pair(original,native):
    original_raw(original,complete=True);b=native_completion(native)
    config=read_configuration(original/'functions.json')
    assert config.get('trial_function')=='rc_format' and len(config['accepted'])==10
    text=(original/'events.txt').read_text();raw=(original/'completion-globals.bin').read_bytes();module=b['module']
    paths=re.findall(r'^module-path target (.+)$',text,re.M)
    bases=re.findall(r'^module RCDLL.dll actual=([0-9a-f]+) ',text,re.M)
    assert len(paths)==len(bases)==1 and module['path']==paths[0] and module['token']
    assert module['sha256']==sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL') and module['name'].lower()=='rcdll.dll' and module['flags']==2 and module['thread_calls_disabled'] is True
    assert struct.unpack_from('<I',raw,0x11f64)[0]==int(bases[0],16)
    assert int.from_bytes(bytes.fromhex(b['globals']['dll_instance']),'little')==module['token']
    lifecycle=re.findall(r'^lifecycle-call (\d+) ([0-9a-f]+) ([0-9a-f]+)$',text,re.M)
    assert len(lifecycle)==2 and lifecycle[0][0]=='1' and lifecycle[1][0]=='0'
    assert lifecycle[0][1]==lifecycle[1][1]==bases[0] and int(lifecycle[0][2],16) and lifecycle[1][2]=='00000001'
    assert re.findall(r'^lifecycle-return (\d+) (-?\d+)$',text,re.M)==[('1','1'),('0','1')]
    assert re.findall(r'^export-result (-?\d+)$',text,re.M)==['0']
    native_events=(native/'events.txt').read_text()
    for marker in ['attach','export-call RC 0 0 callback 0 2','export-return 0','detach','detach-return 1']:
        assert native_events.splitlines().count(marker)==1
    assert (native/'data.json').read_bytes()==(HERE/'data.json').read_bytes()
    matched={}
    for item in json.loads((HERE/'data.json').read_text()):
        if item['kind'] in ['exit-array-pointer','module-handle','diagnostic-callback','opaque-jump-context','private-heap','include-buffer']:continue
        offset=int(item['original_rva'],16)-0x14000;value=raw[offset:offset+item['bytes']]
        assert value.hex()==b['globals'][item['name']],item['name']
        matched[item['name']]=value.hex()
    assert matched['dll_attach_count']==matched['dll_raw_main']==matched['rc_result']=='00000000'
    assert matched['rc_message_count']=='04000000' and matched['rc_line']==matched['rc_last_line']=='00000000'
    assert b['exit_array']['base']==0 and struct.unpack_from('<I',raw,0x15464)[0]==0
    old_allocations=[int(v,16)for v in re.findall(r'^crt-malloc-result ([0-9a-f]+)$',text,re.M)]
    assert len(old_allocations)==1 and struct.unpack_from('<I',raw,0x15460)[0]==old_allocations[0]
    assert len(b['allocations'])==1 and b['allocations'][0]['bytes']==128 and b['exit_array']['end']==b['allocations'][0]['result']
    assert int.from_bytes(bytes.fromhex(b['globals']['dll_exit_base']),'little')==0
    assert int.from_bytes(bytes.fromhex(b['globals']['dll_exit_end']),'little')==b['exit_array']['end']
    effects=callbacks(original);assert effects==callbacks(native) and len(effects)==4
    assert all(row['first']==row['second']==row['result']==0 for row in effects)
    assert effects[0]==effects[3]==dict(first=0,second=0,before='0a00',after='0a00',result=0)
    assert len(bytes.fromhex(effects[1]['before']))==79 and len(bytes.fromhex(effects[2]['before']))==490
    for row in effects[1:3]:
        before=bytes.fromhex(row['before']);after=bytearray(before);assert before[-3:]==b'\r\n\0';after[-3]=0;assert after.hex()==row['after']
    assert struct.unpack_from('<III',raw,0x11f58)==(0,0x01001100,0)
    assert int.from_bytes(bytes.fromhex(b['globals']['rc_diagnostic']),'little')==b['diagnostic_callback']!=0
    allocation=help_allocation(original,b,'completion-globals.bin')
    assert b['jump']['opaque_setjmp3']==1
    trace=[int(v,16)for v in re.findall(r'^entry ([0-9a-f]+)$',text,re.M)];assert trace==HELP_PREFIX
    functions={int(f['start'],16):f for f in config['functions']}
    app=[functions[rva]['name']for rva in trace if rva in functions and functions[rva]['kind']=='application']
    assert logical_trace((native/'trace.txt').read_text())==app and len(app)==9
    assert (original/'stdout.bin').read_bytes()==(native/'stdout.bin').read_bytes()
    assert sha(original/'stdout.bin')=='e5220dc952d908c903a0aae287e9ab3d5a35872aef56f593f3ad2568441423a1'
    assert (original/'stderr.bin').read_bytes()==(native/'stderr.bin').read_bytes()==b''
    return dict(trial='rc_format',matched_completion=dict(original_status=0,native_status=0,stdout_bytes=589,stderr_bytes=0),globals=matched,allocation=allocation,callbacks=effects,application_trace=app,lifecycle=lifecycle,application_functions=9,crt_support_functions=2,import_boundaries=2)
