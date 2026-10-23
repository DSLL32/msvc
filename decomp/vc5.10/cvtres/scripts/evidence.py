"""Raw frontier/completion comparison and immutable candidate evidence binding."""
import json
import re
from pathlib import Path
from mask import BINARY,ROOT,TARGET,sha,build
from inputs import fingerprints
import tempfile

PROVIDER='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'

def original_raw(directory):
    text=(directory/'events.txt').read_text()
    assert not re.search(r'^exception ',text,re.M),'original exception, not completion'
    traps=re.findall(r'^trap ([0-9a-f]+) eip ([0-9a-f]+)',text,re.M)
    exits=re.findall(r'^exit ([0-9]+)$',text,re.M)
    assert (len(traps)==1 and not exits) or (len(exits)==1 and not traps),'missing/ambiguous raw original frontier or exit'
    entries=[int(v,16) for v in re.findall(r'^entry ([0-9a-f]+)$',text,re.M)]
    if traps:
        a,eip=(int(x,16) for x in traps[0]);assert eip==a+1
        return dict(kind='trap',address=a,entries=entries)
    return dict(kind='exit',status=int(exits[0]),entries=entries)

def native_raw(directory):
    state=json.loads((directory/'state.json').read_text())
    raw=(directory/'gdb.stdout').read_text()
    if state.get('stop_signal')==5:
        assert 'Program received signal SIGTRAP' in raw
        assert isinstance(state.get('function'),str) and state['function']
        assert isinstance(state.get('pc'),int) and state['pc']
        verify_native_int3(state,(directory/'native.elf') if (directory/'native.elf').exists() else (BINARY/'build/native/CVTRES'))
        assert re.search(r'\b'+re.escape(state['function'])+r'\s*\(',raw),'raw GDB frame/symbol differs'
        return dict(kind='trap',function=state['function'],state=state)
    status=state.get('exit_status')
    assert isinstance(status,int) and 'exited' in raw,'missing real native frontier or completion'
    return dict(kind='exit',status=status,state=state)

def compare_frontier(config,original,native,initial=False):
    old=original_raw(original);new=native_raw(native)
    assert old['kind']==new['kind'],'frontier/completion differs'
    if old['kind']=='trap':
        if initial:
            assert old['address']==config['target']['entry'] and new['function']=='original_crt_entry_barrier'
            name='mainCRTStartup'
        else:
            matches=[f for f in config['functions'] if f['start']==old['address']]
            assert len(matches)==1,'original frontier is not a named function'
            f=matches[0];assert f['status']=='stub','frontier not still unimplemented'
            assert new['function']==f['native'],'native frontier differs'
            name=f['name'];state=new['state'];text=(original/'events.txt').read_text()
            words=re.search(r'^stack ([0-9a-f ]+)$',text,re.M)
            assert words,'missing original ABI stack'
            words=[int(x,16) for x in words[1].split()]
            if name=='main':
                argc=re.search(r'^argc ([0-9]+)$',text,re.M)
                argv=[bytes.fromhex(x).decode('latin1') for x in re.findall(r'^argv [0-9]+ ([0-9a-f]*)$',text,re.M)]
                assert argc and int(argc[1])==state['argc']==len(config['case']['arguments'])+1 and len(argv)==state['argc']
                assert argv==state['argv'] and argv[1:]==config['case']['arguments'],'complete argv differs'
            if name=='usage':assert words[1]==state['status']
            if name=='load_message':assert words[1]==state['id']
            compare_context(config,text,state)
            if f.get('parameters'):compare_parameters(config,f,text,state,original,native)
        result=dict(kind='trap',frontier=name,original=old['address'],native_function=new['function'])
    else:
        assert not initial
        assert old['status']==new['status']==config['case']['canonical_reference']['status'],'unexpected completed case status'
        reference=config['case']['canonical_reference']
        for label in ['stdout','stderr']:
            master=BINARY/'cases'/config['case']['id']/'reference'/(label+'.bin')
            assert sha(master.read_bytes())==reference[label+'_sha256'] and len(master.read_bytes())==reference[label+'_size']
            assert (original/(label+'.bin')).read_bytes()==master.read_bytes(),'completed output differs from untouched canonical master'
        assert reference['target_sha256']==config['target']['sha256'] and reference['provider_sha256']==PROVIDER
        result=dict(kind='exit',status=old['status'])
    if old['kind']=='exit':
        for name,record in config['case']['canonical_reference'].get('artifacts',{}).items():
            master=BINARY/'cases'/config['case']['id']/'reference'/name
            assert sha(master.read_bytes())==record['sha256']
            assert (original/name).read_bytes()==(native/name).read_bytes()==master.read_bytes(),'completed artifact differs: '+name
    for stream in ['stdout.bin','stderr.bin']:
        assert (original/stream).read_bytes()==(native/stream).read_bytes(),stream+' differs'
    if not initial:
        by_address={f['start']:f for f in config['functions']}
        expected=[by_address[a]['native'] for a in old['entries'] if a in by_address]
        if old['kind']=='trap':expected.append(new['function'])
        assert expected==(native/'trace.txt').read_text().splitlines(),'application first-entry trace differs'
    return result

def bind(config,original,native,frozen=False):
    current=(BINARY/'functions.json').read_bytes();expected=sha(current)
    if frozen:
        def semantic(value):
            copy=json.loads(json.dumps(value));copy.pop('accepted',None)
            for function in copy['functions']:
                function.pop('status',None);function.pop('evidence',None)
            return copy
        assert semantic(json.loads(current))==semantic(config),'frozen trial differs beyond accepted ledger'
        expected=sha((original/'configuration.json').read_bytes())
        current=(original/'configuration.json').read_bytes()
    assert json.loads(current)==config
    old=json.loads((original/'receipt.json').read_text());new=json.loads((native/'receipt.json').read_text())
    for directory,receipt in [(original,old),(native,new)]:
        snapshot=(directory/'configuration.json').read_bytes()
        assert receipt['configuration_sha256']==sha(snapshot)==expected,'stale/different candidate snapshot'
        assert snapshot==current,'candidate snapshot contents changed'
        assert receipt['original_sha256']==config['target']['sha256']==sha(TARGET.read_bytes()),'target changed'
        for name,value in receipt['raw_sha256'].items():assert sha((directory/name).read_bytes())==value,'raw observation changed: '+name
        assert receipt['argv']==config['case']['arguments'] and receipt['case']==config['case']['id'],'case invocation differs'
        for item in config['case'].get('inputs',[]):
            assert receipt['case_inputs'][item['name']]==sha((directory/item['name']).read_bytes())==item['sha256'],'case input differs'
        for name,value in receipt.get('artifacts',{}).items():assert sha((directory/name).read_bytes())==value,'case artifact changed'
    for key,name,current in [('native_sha256','native.elf',BINARY/'build/native/CVTRES'),('rebuilt_sha256','rebuilt.exe',BINARY/'build/CVTRES.EXE'),('map_sha256','rebuilt.map',BINARY/'build/cvtres.map')]:
        pinned=native/name
        if pinned.exists():assert new[key]==sha(pinned.read_bytes()),'pinned compiled artifact changed: '+name
        if not frozen or not pinned.exists():assert new[key]==sha(current.read_bytes()),'current compiled artifact changed: '+name
    verify_build_snapshot(native/'native-build.json',new['native_sha256'],new['build_snapshot_sha256'])
    if (native/'help.resources').exists():
        import subprocess
        with tempfile.TemporaryDirectory(prefix='provider-check-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres') as temporary:
            provider=Path(temporary)/'help.resources'
            subprocess.run(['python3',str(BINARY/'scripts/prepare-resources.py'),'--output',str(provider)],cwd=BINARY,check=True,stdout=subprocess.DEVNULL)
            assert provider.read_bytes()==(native/'help.resources').read_bytes(),'native resource provider differs from verified cache'
    assert old['helper_status']==0 and not old['timed_out']
    if (original/'capture-host-build.json').exists():verify_build_snapshot(original/'capture-host-build.json',old['helper_sha256'],old.get('compiled_snapshot_sha256',{}).get('capture-host-build.json'))
    if (original/'capture-host.exe').exists():assert sha((original/'capture-host.exe').read_bytes())==old['helper_sha256'],'immutable capture helper changed'
    assert old['masked_sha256']==sha((original/'CVTRES.EXE').read_bytes())==old['loaded_image_hfile_sha256']==sha((original/'loaded-image.exe').read_bytes())
    assert old['provider_verified'] and old['provider_actual_hfile_sha256']==sha((original/'loaded-msvcrt.dll').read_bytes())==PROVIDER
    assert not old['unmasked'],'unmasked observation cannot accept reconstruction'
    with tempfile.TemporaryDirectory(prefix='mask-check-',dir=str(Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres')) as directory:
        rebuilt=Path(directory)/'masked.exe'
        build(rebuilt,initial=old['initial'],candidates=old['candidates'],configuration=config)
        assert rebuilt.read_bytes()==(original/'CVTRES.EXE').read_bytes(),'raw original mask is not exact current trial'
    return old,new

def verify_native_int3(state,executable):
    """Bind a raw stop PC to this ELF's actual function and INT3 byte."""
    import struct
    import subprocess
    names=subprocess.check_output(['nm','-S','--defined-only',str(executable)],text=True)
    symbol=state['function']
    found=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+re.escape(symbol)+r'$',names,re.M)
    assert found,'GDB frontier has no actual ELF function'
    start,size=(int(v,16) for v in found.groups());pc=state['pc']
    assert start<pc<=start+size,'GDB PC outside declared native function'
    data=executable.read_bytes();assert data[:6]==b'\x7fELF\x01\x01'
    phoff=struct.unpack_from('<I',data,28)[0]
    entsize,count=struct.unpack_from('<HH',data,42)
    for i in range(count):
        kind,offset,vaddr,_,filesz,_,flags,_=struct.unpack_from('<8I',data,phoff+i*entsize)
        if kind==1 and flags&1 and vaddr<=pc-1<vaddr+filesz:
            assert data[offset+pc-1-vaddr]==0xcc,'native PC is not after actual ELF INT3'
            return
    raise AssertionError('native stop PC is not mapped executable file data')


def compare_context(config,text,state,prefix='',loaded_messages=False):
    memory={int(a,16):bytes.fromhex(raw) for a,raw in re.findall(r'^'+re.escape(prefix)+r'memory ([0-9a-f]+) ([0-9a-f]+)$',text,re.M)}
    if config.get('heap_state'):
        from resource_state import original as original_heap,canonical
        assert canonical(original_heap(text,prefix))==canonical(state['globals']['__heap__']),'typed resource heap/alias/directory state differs'
    for item in config.get('data',[]):
        assert item['original'] in memory,'missing original contextual state'
        actual=bytes.fromhex(state['globals'][item['name']]['raw_hex']);expected=memory[item['original']]
        if item['kind']=='cstring-or-null':
            assert bool(int.from_bytes(expected,'little'))==bool(int.from_bytes(actual,'little')),'global string pointer nullness differs'
            if int.from_bytes(expected,'little'):
                match=re.search(r'^'+re.escape(prefix)+r'pointer-string '+format(item['original'],'08x')+r' ([0-9a-f]*)$',text,re.M)
                assert match and bytes.fromhex(match[1]).decode('latin1')==state['globals'][item['name']]['cstring'],'global string differs'
        elif loaded_messages and item['name'] in ('message_library','message_loader'):
            assert int.from_bytes(expected,'little') and int.from_bytes(actual,'little'),'missing resolved message binding'
        elif item['kind'] in ('resource-heap','resource-directory'):
            assert bool(int.from_bytes(expected,'little'))==bool(int.from_bytes(actual,'little')),'resource root nullness differs'
        elif item['kind']=='nullable-pointer':assert int.from_bytes(expected,'little')==int.from_bytes(actual,'little')==0,'unexpected nonnull pointer requires typed identity proof'
        else:assert actual==expected,('contextual state differs',item['name'],sha(expected),sha(actual))
    crt_memory={int(a,16):bytes.fromhex(raw) for a,raw in re.findall(r'^'+re.escape(prefix)+r'crt-memory ([0-9a-f]+) ([0-9a-f]+)$',text,re.M)}
    strings={int(a,16):bytes.fromhex(raw).decode('latin1') for a,raw in re.findall(r'^'+re.escape(prefix)+r'crt-string ([0-9a-f]+) ([0-9a-f]*)$',text,re.M)}
    for item in config.get('crt_data',[]):
        expected=crt_memory[item['rva']];record=state['crt'][item['name']];actual=bytes.fromhex(record['raw_hex'])
        if item['kind']=='cstring':
            assert int.from_bytes(expected,'little') and int.from_bytes(actual,'little')
            assert strings[item['rva']]==record['cstring'],'CRT program path differs'
        elif item['kind']=='file':
            import struct
            left=struct.unpack('<8I',expected);right=struct.unpack('<8I',actual)
            assert left[1]==right[1] and left[3:]==right[3:],'CRT FILE scalar state differs'
            assert bool(left[0])==bool(right[0]) and bool(left[2])==bool(right[2]),'CRT FILE pointer nullness differs'
            used=left[0]-left[2] if left[2] else 0
            assert used==(right[0]-right[2] if right[2] else 0)==record['used'],'CRT pending byte count differs'
            pending=re.search(r'^'+re.escape(prefix)+r'crt-buffer '+format(item['rva'],'08x')+r' ([0-9a-f]*)$',text,re.M)
            if used:assert pending and pending[1]==record['buffer_hex'],'CRT pending buffer differs'
        else:assert expected==actual,('CRT scalar differs',item['name'],expected.hex(),actual.hex())

def compare_candidate_entry(config,function,original,native):
    f=next(f for f in config['functions'] if f['name']==function)
    text=(original/'events.txt').read_text()
    match=re.search(r'^entry '+format(f['start'],'08x')+r'\n(.*?)(?=^entry [0-9a-f]{8}$|^trap |\Z)',text,re.M|re.S)
    assert match,'candidate missing raw original entry context'
    capsule=match[1]
    state=json.loads((native/'state.json').read_text())
    entries=[e for e in state.get('entries',[]) if e['function']==f['native']]
    assert entries,'candidate missing native debugger entry context'
    entry=entries[0]
    if function=='main':
        words=re.search(r'^entry-stack ([0-9a-f ]+)$',capsule,re.M)
        assert words;words=[int(x,16) for x in words[1].split()]
        argc=re.search(r'^entry-argc ([0-9]+)$',capsule,re.M)
        argv=[bytes.fromhex(x).decode('latin1') for x in re.findall(r'^entry-argv [0-9]+ ([0-9a-f]*)$',capsule,re.M)]
        assert argc and int(argc[1])==entry['argc']==len(config['case']['arguments'])+1 and argv==entry['argv']
        assert argv[1:]==config['case']['arguments']
        assert (words[3]==0)==entry['envp_null']
    elif function in ('usage','load_message'):
        words=re.search(r'^entry-stack ([0-9a-f ]+)$',capsule,re.M);assert words
        words=[int(x,16) for x in words[1].split()]
        assert words[1]==entry['status' if function=='usage' else 'id']
    compare_context(config,capsule,entry,prefix='entry-')
    if f.get('parameters'):compare_parameters(config,f,capsule,entry,original,native,prefix='entry-')
    return dict(function=function,original_entry=f['start'],native_entry=f['native'],contextual_ranges=len(config.get('data',[])),crt_ranges=len(config.get('crt_data',[])))


def verify_build_snapshot(path,binary_hash,snapshot_hash=None):
    import base64
    raw=path.read_bytes()
    if snapshot_hash:assert sha(raw)==snapshot_hash,'compiled-source snapshot changed'
    record=json.loads(raw);assert record['binary_sha256']==binary_hash
    assert record['files'],'compiled-source snapshot empty'
    for item in record['files'].values():assert sha(base64.b64decode(item['content_base64']))==item['sha256'],'compiled-source content/hash differs'


def compare_message_observations(config,original,native):
    """Typed callback/return observations; dynamic pointers use proved identities."""
    text=(original/'events.txt').read_text();state=json.loads((native/'state.json').read_text())
    observations=re.findall(r'^observation ([0-9a-f]{8})\n(.*?)(?=^observation |^entry |^exit |\Z)',text,re.M|re.S)
    assert [int(a,16) for a,_ in observations]==[0x1003163,0x1001470,0x1003163,0x1001483],'missing resource-call/return observations'
    assert len(state.get('resource_calls',[]))==len(state.get('message_returns',[]))==2
    user32=re.findall(r'^module ([0-9a-f]+) size [0-9]+ path .*?[\\/]user32\.dll$',text,re.I|re.M)
    assert len(user32)==1,'missing/ambiguous actual USER32 load identity'
    module=int(user32[0],16)
    for index,id in enumerate([8003,8004]):
        calltext=observations[index*2][1];returntext=observations[index*2+1][1]
        call=state['resource_calls'][index];returned=state['message_returns'][index]
        words=re.search(r'^observation-stack ([0-9a-f ]+)$',calltext,re.M);assert words
        words=[int(x,16) for x in words[1].split()]
        assert words[:4]==[0,id,0x100457c,1024],'original LoadStringA argument ABI differs'
        assert call['handle']==0 and call['id']==id and call['capacity']==1024,'native resource call scalar ABI differs'
        assert call['output']==call['globals']['message_buffer']['address'],'native resource output pointer differs'
        memory={int(a,16):bytes.fromhex(raw) for a,raw in re.findall(r'^observation-memory ([0-9a-f]+) ([0-9a-f]+)$',calltext,re.M)}
        assert int.from_bytes(memory[0x100497c],'little')==module,'original cache is not actually loaded USER32'
        eax=int(re.search(r'^observation-trap .*? eax ([0-9a-f]+)',calltext,re.M)[1],16)
        assert eax==int.from_bytes(memory[0x1004980],'little')!=0,'original callback is not cached GetProcAddress result'
        assert int.from_bytes(bytes.fromhex(call['globals']['message_library']['raw_hex']),'little')==2,'native USER32 token differs'
        assert int.from_bytes(bytes.fromhex(call['globals']['message_loader']['raw_hex']),'little')==call['expected_loader'],'native callback is not shared LoadStringA'
        assert memory[0x100457c].hex()==call['globals']['message_buffer']['raw_hex'],'callback incoming buffer differs'
        assert int(re.search(r'^observation-trap .*? eax ([0-9a-f]+)',returntext,re.M)[1],16)==0x100457c,'original return pointer differs'
        assert returned['id']==id and returned['return_value']==returned['globals']['message_buffer']['address'],'native return pointer differs'
        compare_context(config,returntext,returned,prefix='observation-',loaded_messages=True)
    return dict(ids=[8003,8004],buffer_bytes=1024,callback_abi='LoadStringA(NULL,id,message_buffer,1024)',module='USER32.DLL',lookup='LoadLibraryA/GetProcAddress exact audited dataflow',observed_calls=2,observed_returns=2)


def compare_parameters(config,function,text,state,original,native,prefix=''):
    import struct
    words=re.search(r'^'+re.escape(prefix)+r'stack ([0-9a-f ]+)$',text,re.M);assert words
    words=[int(x,16) for x in words[1].split()]
    for index,item in enumerate(function['parameters']):
        record=state['parameters'][str(index)]
        if item['kind'] in ('identifier','resource-header'):
            kind='i' if item['kind']=='identifier' else 'h'
            observed=re.search(r'^'+re.escape(prefix)+r'typed '+str(index)+r' kind '+kind+r' pointer ([0-9a-f]+) raw ([0-9a-f]+)$',text,re.M)
            assert observed and int(observed[1],16)==words[index+1] and record['pointer'],'missing typed resource pointer'
            left=bytes.fromhex(observed[2]);right=bytes.fromhex(record['raw_hex'])
            if config.get('heap_state'):
                from resource_state import original as original_heap,canonical
                _,old_ids=canonical(original_heap(text,prefix),with_ids=True)
                _,new_ids=canonical(state['globals']['__heap__'],with_ids=True)
                node_kind=1 if kind=='i' else 4
                assert old_ids.get((node_kind,words[index+1]),0)==new_ids.get((node_kind,record['pointer']),0),'resource argument alias identity differs'
            if kind=='h':assert left==right and len(left)==24,'resource header differs'
            else:
                assert len(left)==len(right)==20
                original_kind=struct.unpack_from('<I',left)[0]
                assert original_kind==struct.unpack_from('<I',right)[0] and original_kind in (1,2),'resource identifier kind differs'
                if original_kind==2:assert left==right,'ordinal resource identifier differs'
                else:
                    assert left[:4]+left[8:]==right[:4]+right[8:],'named resource identifier fields differ'
                    assert bool(int.from_bytes(left[4:8],'little'))==bool(int.from_bytes(right[4:8],'little')),'identifier next nullness differs'
                    named=re.search(r'^'+re.escape(prefix)+r'typed-text '+str(index)+r' ([0-9a-f]+)$',text,re.M)
                    assert named and named[1]==record['text_hex'],'resource identifier UTF16 bytes differ'
            continue
        if item['kind'] in ('out-buffer','in-buffer'):
            size=item.get('size') or words[item['length_arg']+1]
            observed=re.search(r'^'+re.escape(prefix)+r'buffer '+str(index)+r' pointer ([0-9a-f]+) size ([0-9]+) state ([0-9a-f]+) protection ([0-9a-f]+)$',text,re.M)
            assert observed and int(observed[1],16)==words[index+1] and record['pointer'],'missing typed buffer pointer'
            assert int(observed[2])==record['size']==size,'buffer size differs'
            if item.get('base'):
                base=next(v for v in config['data'] if v['name']==item['base'])
                source=re.search(r'^'+re.escape(prefix)+r'memory '+format(base['original'],'08x')+r' ([0-9a-f]+)$',text,re.M);assert source
                original_base=int.from_bytes(bytes.fromhex(source[1]),'little')
                native_base=int.from_bytes(bytes.fromhex(state['globals'][item['base']]['raw_hex']),'little')
                offset=words[index+1]-original_base
                assert offset==record['pointer']-native_base and offset>=0 and offset+size<=state['globals']['__heap__']['directory_size'],'directory output pointer offset differs'

            assert int(observed[3],16)==0x1000 and 'r' in record['permissions'],'buffer mapping is not committed/readable'
            if item['kind']=='out-buffer':assert int(observed[4],16)&0xcc and 'w' in record['permissions'],'output buffer not writable'
            else:
                contents=re.search(r'^'+re.escape(prefix)+r'buffer-bytes '+str(index)+r' ([0-9a-f]*)$',text,re.M)
                assert contents and contents[1]==record['bytes_hex'],'input buffer bytes differ'
            continue
        if item['kind']!='file':
            assert record['value']==words[index+1],('scalar argument differs',function['name'],item['name']);continue
        raw=re.search(r'^'+re.escape(prefix)+r'file '+str(index)+r' pointer ([0-9a-f]+) raw ([0-9a-f]+)$',text,re.M);assert raw,'missing original FILE argument'
        assert int(raw[1],16)==words[index+1] and record['pointer'],'FILE argument identity differs'
        left=struct.unpack('<8I',bytes.fromhex(raw[2]));right=struct.unpack('<8I',bytes.fromhex(record['raw_hex']))
        assert left[1]==right[1] and left[3:]==right[3:],('FILE scalar state differs',item['name'],left,right)
        assert bool(left[0])==bool(right[0]) and bool(left[2])==bool(right[2]),'FILE buffer nullness differs'
        assert (left[0]-left[2] if left[2] else 0)==(right[0]-right[2] if right[2] else 0),'FILE buffer cursor differs'
        pending=re.search(r'^'+re.escape(prefix)+r'file-buffer '+str(index)+r' ([0-9a-f]+)$',text,re.M)
        assert (pending[1] if pending else '')==record['buffer_hex'],'FILE valid buffer contents differ'
        osstate=re.search(r'^'+re.escape(prefix)+r'file-os '+str(index)+r' fd ([0-9]+) flags ([0-9a-f]+) handle ([0-9a-f]+)$',text,re.M);assert osstate
        assert int(osstate[1])==right[4] and int(osstate[2],16)==record['os_flags'],'FILE descriptor flags differ'
        assert int(osstate[3],16) not in (0,0xffffffff) and record['os_handle'] not in (0,-1),'invalid FILE OS handle'
        position=re.search(r'^'+re.escape(prefix)+r'file-position '+str(index)+r' ([0-9]+)$',text,re.M);assert position
        assert int(position[1])==record['position'],'FILE physical position differs'
        path=re.search(r'^'+re.escape(prefix)+r'file-path '+str(index)+r' ([0-9a-f]+)$',text,re.M);assert path
        path=bytes.fromhex(path[1]).decode('latin1');name=path.replace('\\','/').rsplit('/',1)[1]
        assert name in [v['name'] for v in config['case'].get('inputs',[])]+config['case'].get('artifacts',[]),'unknown FILE identity'
        assert Path(record['path']).resolve()==(native/name).resolve(),'native FILE is not exact case file'
        scratch=Path.home()/'.ai-tmp/msvc2';expected='S:\\'+str((original/name).relative_to(scratch)).replace('/','\\')
        assert path.removeprefix('\\\\?\\').lower()==expected.lower(),'original FILE is not exact case file'
