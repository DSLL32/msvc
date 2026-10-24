"""Raw-derived masked object-only MRE prefix; no native success/acceptance."""
import hashlib,json,re,struct
from pathlib import Path
from binary import PE,u32
from case_inputs import load
from dependency_packet import exports,TARGET_SHA
from dependency_response import phase_snapshot
from mask import construct
from mktemp_state import CRT_SHA,original_states,original as mktemp,original_entry
from mre_packet import target_objects,MSPDB_SHA
from mre_file import file_info
from import_residuals import qualified_slots

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
PHASES=tuple(p for i in range(1,4)for p in('mre-%d-before'%i,'mre-%d-after'%i,'mre-%d-after-copy-after'%i))+('mre-frontier',)
sha=lambda raw:hashlib.sha256(raw).hexdigest()


def decode(directory):
    directory=Path(directory);capture_bytes=(directory/'capture.json').read_bytes();capture=json.loads(capture_bytes)
    assert capture['mre_response']is True and capture['baseline']is False and capture['case']=='mre-object'
    assert not any(capture.get(n)for n in('mre_analysis','dependency_analysis','dependency_complete','dependency_response','link_response','clock_analysis','clock_response'))
    assert capture['controller_error']is None and capture['helper_status']==0 and capture['command'][-2]=='1'
    def read(name):
        assert name in capture['files'];path=directory/name
        assert path.is_file()
        if path.is_symlink():
            assert not path.readlink().is_absolute()and path.resolve().is_relative_to(ROOT/'toolchains/5.10'),'unowned evidence symlink'
        raw=path.read_bytes();assert capture['files'][name]=={'bytes':len(raw),'sha256':sha(raw)},'changed raw '+name
        return raw
    for name in capture['files']:read(name)
    assert read('mre-observer.txt')==b'CL_MRE_PREFIX_RESPONSE_V2\n'
    config_bytes=read('config.json');config=json.loads(config_bytes)
    assert sha(config_bytes)==capture['config_sha256']
    assert '_prepare_jobs'in config['accepted']and '_option_signature'not in config['accepted']
    masked=construct(config)
    assert sha(masked)==sha(read('CL.EXE'))==sha(read('loaded-target.bin'))==capture['original_mask_sha256']
    assert capture['target_sha256']==config['target']['sha256']==TARGET_SHA
    assert PE(directory/'loaded-target.bin').read(0x40393a,1)==b'\xcc'
    assert sha(read('MSVCRT.DLL'))==sha(read('loaded-msvcrt.bin'))==capture['loaded_msvcrt_sha256']==CRT_SHA
    assert sha(read('MSPDB50.DLL'))==sha(read('loaded-mspdb50.bin'))==MSPDB_SHA
    assert sha(read('reference-debugger.exe'))==capture['helper_sha256']==capture['helper_build']['binary']['sha256']
    for name,digest in capture['helper_sources'].items():assert sha(read(name))==digest==capture['helper_build']['sources']['scripts/'+name]
    for field in('controller_source','helper_source','mre_transport_source'):
        item=capture[field];suffix='.c'if field=='helper_source'else'.py'
        owned=directory.parent/'sources'/(field+suffix)
        source=owned if owned.exists()else Path(item['path'])
        content=source.read_bytes();assert len(content)==item['bytes']and sha(content)==item['sha256'],'original execution source identity changed'
    case=load('mre-object');assert all(capture[k]==case[k]for k in('case','arguments','inputs'))
    for item in case['inputs']:assert read(item['name'])==(PROJECT/item['source']).read_bytes()
    assert read('case-inputs.json')==json.dumps(case,indent=2).encode()+b'\n'
    events=read('observer/mre-events.txt').decode('ascii');target_events=read('events.txt').decode('ascii')
    assert re.findall(r'^snapshot phase=(\S+)',events,re.M)==list(PHASES)
    assert re.findall(r'^exit .*status=(\d+)$',target_events,re.M)==['28673']
    assert re.findall(r'^trap .*address=([0-9a-f]{8})',target_events,re.M)==['0040393a']
    assert not re.search(r'^exception |mre-unexpected-call',target_events,re.M)
    assert read('mre-calls.txt')==b'3\n'
    trace=read('trace.txt').decode('ascii').splitlines()
    assert 'execute_pass'not in trace and 'final_pass'not in trace and trace[-1]=='prepare_jobs'
    assert read('stdout.bin')==b'' and read('stderr.bin')==b"Command line warning D4027 : source file 'test.obj' ignored\r\n"
    errors=original_states(directory,PHASES,'observer/mre-events.txt');phases={};contexts={};objects={}
    for phase in PHASES:
        phases[phase],contexts[phase]=phase_snapshot(directory,read,phase,errors[phase],mre_prefix=True)
        if not phase.endswith('-copy-after'):objects[phase]=target_objects(read,phase)
    before=phases['mre-1-before'];assert objects['mre-1-before']['jobs']==[]
    assert errors['mre-1-before']=={'errno':0,'doserrno':2,'last_error':2}
    for phase in PHASES[1:]:assert errors[phase]=={'errno':2,'doserrno':2,'last_error':0}
    for phase in PHASES:
        value=phases[phase]
        assert value['mb_codepage']==value['lc_codepage']==0 and value['lc_handles']==[0]*6 and value['x87_control_word']==639
        for field in('crt_environment_sha256','os_environment_sha256','mb_state_sha256','ctype_sha256','pctype_address','file0_2_words','streams'):
            assert value[field]==before[field],'changed original parent state '+field
        assert contexts[phase][28:140]==contexts['mre-1-before'][28:140],'changed actual full x87 state'
        if phase!='mre-1-before':
            assert value['open_descriptors']==phases['mre-1-after']['open_descriptors']
            assert value['open_descriptors'][3]['query']['position_low']==1024 and value['open_descriptors'][3]['query']['position_high']==0
            assert read('observer/'+phase+'-lowio-0.bin')[108:144]==read('observer/mre-1-after-lowio-0.bin')[108:144]
        if phase in objects:
            initial=objects['mre-1-before'];actual=objects[phase]
            for field in('prepare_jobs_arguments','options','jobs'):assert actual[field]==initial[field]
            expected=list(initial['driver_words'])
            if phase=='mre-frontier':expected[5]=0;expected[6]=initial['prepare_jobs_arguments'][0]+20
            assert actual['driver_words']==expected
            assert len(read('observer/'+phase+'-shared-scratch.bin'))==1024
            assert read('observer/'+phase+'-shared-scratch.bin')==read('observer/mre-1-before-shared-scratch.bin')
    provider=PE(directory/'loaded-mspdb50.bin');table=exports(provider);slots=qualified_slots(PE(directory/'loaded-target.bin'));calls=[];engine=driver=None;frame=None
    for i,(site,slot,cleanup,name,ordinal)in enumerate([(0x403597,0x40a300,24,'_MREFOpenByName@24',75),(0x4035c1,0x40a314,8,'_MREQueryMreDrv@8',79),(0x4035ca,0x40a304,4,'_MREDrvOneTimeInit@4',70)],1):
        bphase='mre-%d-before'%i;aphase='mre-%d-after'%i;b=contexts[bphase];a=contexts[aphase]
        identity=struct.unpack('<4I',read('observer/'+bphase+'-identity.bin'))
        assert read('observer/'+bphase+'-identity.bin')==read('observer/'+aphase+'-identity.bin')
        base,function,ebp,esp=identity
        assert slots[slot]=={'dll':'mspdb50.dll','name':name}
        assert base==provider.base and table[name]['ordinal']==ordinal and function==base+table[name]['rva']
        assert u32(b,180)==u32(a,180)==ebp and u32(b,196)==esp and u32(a,196)==esp+cleanup
        nonvolatiles={name:u32(b,offset)for name,offset in(('edi',156),('esi',160),('ebx',164),('ebp',180))}
        assert all(u32(a,offset)==nonvolatiles[name]for name,offset in(('edi',156),('esi',160),('ebx',164),('ebp',180))),'MRE stdcall nonvolatile register changed'
        if frame is None:frame=ebp
        else:assert frame==ebp
        assert u32(b,184)==site+1 and u32(a,184)==site+7
        arguments=struct.unpack('<%dI'%(cleanup//4),read('observer/'+bphase+'-arguments.bin'))
        assert read('observer/'+bphase+'-arguments.bin')==read('observer/'+aphase+'-arguments.bin')==read('observer/'+bphase+'-stack.bin')[:cleanup]
        rows=re.findall(r'^mre sequence=%d phase=(before|after) call=([0-9a-f]{8}) return=([0-9a-f]{8}) function=([0-9a-f]{8}) esp=([0-9a-f]{8}) eax=([0-9a-f]{8}) event=(\d+)$'%i,events,re.M)
        assert len(rows)==2
        for row,phase,context in zip(rows,(bphase,aphase),(b,a)):
            assert row[0]==phase.rsplit('-',1)[1] and tuple(int(x,16)for x in row[1:6])==(site,site+6,function,u32(context,196),u32(context,176))and int(row[6])==phases[phase]['event']
        record={'normal_return_nonvolatiles':nonvolatiles,'name':name,'ordinal':ordinal,'qualified_IAT':slot,'function':function,'callsite':site,'stdcall_bytes':cleanup,'arguments':list(arguments),'result_word':u32(a,176),'before':phases[bphase],'after':phases[aphase]}
        if i==1:
            assert arguments[0]==ebp-12 and arguments[2]==ebp-20 and arguments[3]==0x4091b8 and arguments[4:]==(0,1)
            assert read('observer/'+bphase+'-pdb-path.bin')==read('observer/'+aphase+'-pdb-path.bin')==b'case.idb\0'
            assert u32(a,176)==1 and u32(read('observer/'+aphase+'-error-out.bin'),0)==0
            engine=u32(read('observer/'+aphase+'-engine-out.bin'),0);assert engine
            assert read('observer/'+bphase+'-message.bin')==read('observer/'+aphase+'-message.bin')==read('observer/'+bphase+'-shared-scratch.bin')
        elif i==2:
            assert arguments[0]==engine and arguments[1]==ebp-4
            driver=u32(read('observer/'+aphase+'-driver-out.bin'),0);assert driver==engine+4
        else:assert arguments==(driver,)
        record['initial_out_payload']='Uninitialized caller locals before the relevant first write; raw retained, not compared as initialized input'
        record['return_qualification']='Open result1 consumed; Query/Init EAX ignored by target, raw retained without semantic return-type claim'
        calls.append(record)
    copies=[]
    for i in range(1,4):
        phase='mre-%d-after'%i;after=phase+'-copy-after';prefix='observer/mre-files-idb-'+phase+'-'
        header=read(prefix+'request.bin');assert len(header)==32
        schema,event,tid,slot,handle,position,high,pid=struct.unpack('<8I',header)
        assert schema==1 and event==phases[phase]['event']and tid==phases[phase]['thread']and pid==u32(read('streams-'+phase+'-teb.bin'),0x20)and high==0 and position==1024
        assert read(prefix+'ack.bin')==header+phase.encode()+b'\0'
        info_raw=read(prefix+'descriptor.bin');assert info_raw==read(prefix+'descriptor-initial.bin')
        for suffix in('path-initial.bin','path.bin','descriptor-after.bin','path-after.bin','path-reopened.bin'):assert read(prefix+suffix)==info_raw
        info=file_info(info_raw);assert info['bytes']==25600 and not info['attributes']&0x410
        lowio=read(prefix+'lowio.bin');assert len(lowio)==36 and lowio==read(prefix+'lowio-after.bin')==read('observer/'+phase+'-lowio-0.bin')[108:144]
        assert u32(lowio,0)==handle and lowio[4]==0x11
        assert slot==u32(read('observer/'+phase+'-pioinfo-pointers.bin'),0)
        assert read(prefix+'position-after.bin')==struct.pack('<2I',position,high)
        assert read(prefix+'context-before.bin')==read(prefix+'context-after.bin')==contexts[phase]==contexts[after]
        opened=struct.unpack('<6I',read(prefix+'path-open.bin'));assert opened[0]not in(0,0xffffffff)and opened[2]==1 and opened[4:]==(0,7)
        obj=read(prefix+'object-basic.bin');assert 8<=len(obj)<=128
        access=u32(obj,4);assert access&3==3
        dos=read(prefix+'dos-path.bin');assert dos.endswith(b'\0')and b'\0'not in dos[:-1]
        assert dos[:-1].decode('ascii')==phases[phase]['cwd'].rstrip('\\')+'\\case.idb'
        cwd=read(prefix+'cwd.bin').decode('utf-16le');assert cwd[:-1].rstrip('\\')==phases[phase]['cwd'].rstrip('\\')and cwd.endswith('\0')
        for key in phases[phase]:
            if key!='directory':assert phases[phase][key]==phases[after][key],'copy changed target state '+key
        query=phases[phase]['open_descriptors'][3]['query']
        assert (query['handle'],query['volume'],query['index_high'],query['index_low'])==(handle,info['volume'],info['index_high'],info['index_low'])
        host=json.loads(read(prefix+'host.json'));content=read(prefix+'contents.bin')
        assert host['phase']==phase and host['event']==event and host['tid']==tid and host['debuggee_pid']==pid
        assert host['handle']==handle and host['position']==position and host['granted_access']==access and host['windows_information']==info
        assert host['path_identity_access']==0 and host['windows_data_read_sharing_observed']is False
        assert host['request_sha256']==sha(header)and host['windows_path']==dos[:-1].decode('ascii')
        assert host['bytes']==len(content)==info['bytes']and host['sha256']==sha(content)
        assert host['posix_before']==host['posix_after']==host['posix_path_after']
        assert host['posix_before']['size']==info['bytes']and host['posix_before']['mode']&0o170000==0o100000
        assert host['fixed_drive']=={'link':'/root/.ai-tmp/vc1.0-capture/wine/dosdevices/s:','target':'/root/.ai-tmp/msvc2'}
        assert re.findall(r'^file-copy phase='+phase+r' event=(\d+) tid=(\d+) all-threads-held=debug-event position=0:1024$',events,re.M)==[(str(event),str(tid))]
        copies.append({'phase':phase,'request_sha256':sha(header),'windows_file_information':info,'posix_identity_and_metadata':host['posix_before'],'actual_granted_access':access,'lowio_flags':0x11,'position':1024,'bytes':len(content),'sha256':sha(content),'owned_contents':prefix+'contents.bin','postcopy_context_and_target_state_unchanged':True,'identity_qualification':'Windows handles match each other, POSIX descriptor/path match each other; fixed owned S: path bridges them. No cross-platform inode/FileId equality or Windows data-sharing claim.'})
    assert len({record['sha256']for record in copies})==1
    frontier=struct.unpack('<16I',read('observer/mre-frontier-stack.bin'))
    caller=objects['mre-frontier']['prepare_jobs_arguments']
    assert frontier[:2]==(0x4035f2,caller[0])and read('observer/mre-frontier-formal-arguments.bin')==struct.pack('<I',caller[0])
    assert read('observer/mre-frontier-pending.bin')==bytes(20)
    assert re.findall(r'^response-frontier kind=option-signature address=0040393a calls=3 event=(\d+)$',events,re.M)==[str(phases['mre-frontier']['event'])]
    return {'kind':'Fresh masked original-only MRE-object prefix with intermediate IDB preservation; native success/acceptance held','case':'mre-object','capture_sha256':sha(capture_bytes),'config_sha256':sha(config_bytes),'mask_sha256':sha(masked),'provider_sha256':MSPDB_SHA,'entry_profile':original_entry(directory),'entry_and_mktemp':mktemp(directory),'calls':calls,'intermediate_files':copies,'target_objects':objects,'signature_frontier':{'address':0x40393a,'caller_return':frontier[0],'state':frontier[1],'pending_defined_zero':True,'full_state':phases['mre-frontier']},'opaque_roles':{'engine':engine,'driver':driver,'driver_offset':4,'object_contents_and_extent':'Unobserved/unclaimed; target holds opaque roles only'},'file_lifetime':'Closed before Open, genuine provider-owned fd3 remains binary/noinherit/readwrite atposition1024 through all3 returns and real signature frontier; later normal close/release effects unobserved and denied','remaining_native_gaps':['Genuine native fd3 creation/handle/position/exclusive sharing/noinherit/lifetime and exactfile effects need shared-owner contract','Current local MRE native dependency adapter still fails closed; no successful response implemented','Later MRE calls/file changes/close lifecycle unobserved'],'normal_completion':False,'application_credit':0,'decoder_sources':{name:sha((PROJECT/'scripts'/name).read_bytes())for name in('mre_object_packet.py','mre_packet.py','dependency_response.py','mktemp_state.py','mre_file.py','mask.py','case_inputs.py')}}
