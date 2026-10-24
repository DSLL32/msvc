"""Two independent file lifetimes at the first object-reader boundary.

Entry traces remain first-seen. Every repeated caller-site invocation, live
out-local and normal return is checked independently here.
"""
import functools
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from file_open_state import slot_symbols
from file_mapping_state import mapping_symbol
from mapped_input_contract import verify


@functools.lru_cache(maxsize=1)
def loader_contract():
    from binary import PE
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    from capstone.x86 import X86_OP_IMM
    binary=Path(__file__).resolve().parents[1]
    target=binary.parents[2]/'toolchains/5.10/bin/LINK.EXE'
    proof=json.loads((binary/'cases/hello-win32/input-loader-contract.json').read_text())
    assert hashlib.sha256(target.read_bytes()).hexdigest()==proof['target_sha256']=='04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579'
    pe=PE(target);body=pe.read(0x449a50,147)
    assert hashlib.sha256(body).hexdigest()==proof['loader']['sha256']
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    instructions=list(decoder.disasm(body,0x449a50));assert sum(x.size for x in instructions)==147
    assert [[x.address,int(x.op_str,0)]for x in instructions if x.mnemonic=='ret']==proof['loader']['returns']==[[0x449ae0,8]]
    assert [[x.address,x.operands[0].imm]for x in instructions if x.mnemonic=='call'and x.operands[0].type==X86_OP_IMM]==proof['loader']['calls']
    assert pe.read(0x449a73,5).hex()=='bfc4704600' # String-copy destination, no total capacity inference.
    for address,raw in [(0x449a94,'a3f8ee4700'),(0x449aad,'a090f14700'),(0x449ab6,'a198f14700'),(0x449acf,'a1f8ee4700')]:
        assert pe.read(address,5).hex()==raw # DWORD file/status and BYTE reuse accesses stay distinct.
    assert pe.read(0x4670c4,32)==bytes(32)
    data,=[s for s in pe.sections if s['name']=='.data']
    for address,size in [(0x47eef8,4),(0x47f190,1),(0x47f198,4)]:
        rva=address-pe.base
        assert data['rva']+data['size']<=rva and rva+size<=data['rva']+data['virtual_size']
    assert proof['globals'][0]['original_total_capacity']=='unknown'
    assert proof['read_frontier']==dict(address=0x4498d0,stack_formals=['InputArgument *','OutputLibrary *','LinkContext *'],hidden_inputs=[],return_cleanup_bytes=12)
    return proof


@functools.lru_cache(maxsize=1)
def reader_contract():
    from binary import PE
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    binary=Path(__file__).resolve().parents[1]
    target=binary.parents[2]/'toolchains/5.10/bin/LINK.EXE'
    proof=json.loads((binary/'cases/hello-win32/input-reader-contract.json').read_text())
    assert hashlib.sha256(target.read_bytes()).hexdigest()==proof['target_sha256']
    pe=PE(target);body=pe.read(0x4498d0,177)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x4498d0))
    assert sum(i.size for i in instructions)==177
    assert [[i.address,int(i.op_str,0)]for i in instructions if i.mnemonic=='call']==proof['calls']
    assert [[i.address,int(i.op_str,0)]for i in instructions if i.mnemonic=='ret']==proof['returns']==[[0x449940,12],[0x449968,12],[0x44997e,12]]
    assert pe.read(0x449935,6).hex()=='8b530c895634' # Opaque DWORD tag: argument+12 →library+52.
    assert proof['formals']==['InputArgument *','OutputLibrary *','LinkContext *']and proof['hidden_inputs']==[]
    return proof


def words(data):
    assert len(data)==40
    return struct.unpack('<10I',data)


def cache(slots,used):
    return dict(file_newest=slots[15-used],pending_files=slots[16-used]if used else 0,
                cached_files=0,cached_file_tail=0,cached_file_count=0)


def pool(slots,used,active):
    result=[(0,0,i,0,0,0,0,0,0,slots[i-1]if i else 0)for i in range(16)]
    for i,value in active.items():result[i]=value
    for i in range(16-used,16):
        if i not in active:result[i]=(0,0,i,0,0,0,0,0,0,slots[i+1]if i<15 else 0)
    return result


@functools.lru_cache(maxsize=8)
def input_symbols(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    wanted={'current_phase':(32,'B'),'current_input_file':(4,'B'),
            'incremental_status_code':(4,'B'),'incremental_reuse':(1,'B')}
    found={}
    for line in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines():
        row=line.split()
        if len(row)==4 and row[3]in wanted:
            assert (int(row[1],16),row[2])==wanted[row[3]]
            found[row[3]]=int(row[0],16)
    assert found.keys()==wanted.keys()
    return found


def pair(before,after,sites,cleanup):
    assert (before['before'],after['before'])==(1,0)
    assert (before['site'],after['site'])==sites
    assert before['stack']==before['entry_stack']==after['entry_stack']
    assert after['stack']==before['stack']+cleanup


def normal(call,cleanup):
    assert call['normal_return']is True
    assert call['return_stack']==call['entry_stack']+cleanup+4


def compare(original,state,trap,events,native_directory=None):
    verify() # The mapped opener consumes only the low BYTE of physical arg3.
    loader_contract()
    if state['function']in ('is_archive','mapped_read','is_mac_resource_file','get_file_length'):reader_contract()
    from collision_state import compare as compare_collision
    compare_collision(original,state,trap,events)
    directory=Path(native_directory)
    receipt=json.loads((directory/'receipt.json').read_text())
    elf=str(directory/'compiled/native-LINK.EXE')
    symbols=slot_symbols(elf,receipt['native_sha256'])
    assert state['file_cache_slots']==symbols
    oslots=list(struct.unpack('<16I',(original/'file-slots.bin').read_bytes()))
    nslots=list(struct.unpack('<16I',bytes.fromhex(state['file_slots_bytes'])))
    assert len(set(oslots))==len(set(nslots))==16 and all(oslots)and all(nslots)
    assert state['file_globals']['file_record_count']==16
    assert state['file_globals']['file_newest']==nslots[13]
    assert struct.unpack('<3I',(original/'file-pointers.bin').read_bytes())[1:]==(oslots[13],16)
    assert trap['file_cache_lists']=={k:v for k,v in cache(oslots,2).items()if k!='file_newest'}
    assert state['file_cache_lists']=={k:v for k,v in cache(nslots,2).items()if k!='file_newest'}
    assert state['mapped_file_slots_address']==mapping_symbol(elf,receipt['native_sha256'])
    assert (original/'mapped-file-slots.bin').read_bytes()==struct.pack('<16I',oslots[15],*([0]*15))
    assert state['mapped_file_slots']==[nslots[15]]+[0]*15
    assert bytes.fromhex(state['mapped_file_slots_bytes'])==struct.pack('<16I',*state['mapped_file_slots'])

    lists={e['name']:e for e in events if e['event']=='argument_list'}
    argument=lists['link_files']['nodes'][0];nargument=state['canonicalized_arguments'][0]
    names=[trap['output_name_pointer'],argument['words'][1]]
    nnames=[state['output_name_pointer'],nargument['result']]
    assert argument['words'][0]==argument['words'][1]and argument['text']=='hello.obj'
    expected_names=['hello.exe','hello.obj']
    opens=[next(e for e in events if e['event']=='open_file_entry'),
           next(e for e in events if e['event']=='file_open_call'and e['kind']=='input')]
    nopens=state['file_open_entries'];assert len(nopens)==2
    assert state['open_file_entry']==nopens[1]
    for i in range(2):
        assert opens[i]['formals']==[names[i],0x8302 if i==0 else 0x8000,0x180 if i==0 else 0]
        assert nopens[i]['formals']==[nnames[i],0x8302 if i==0 else 0x8000,0x180 if i==0 else 0]
        assert opens[i]['hidden_inputs']==nopens[i]['hidden_inputs']==[]
    oentry=[opens[0]['stack_pointer'],opens[1]['stack']-4]
    load,=[e for e in events if e['event']=='load_input_entry']
    assert oentry==[load['stack_pointer']-4,load['stack_pointer']-32]
    assert [e['stack_pointer']for e in nopens]==[state['load_input_entry']['stack_pointer'],state['load_input_entry']['stack_pointer']-48]
    returns=[e for e in events if e['event']=='file_open_return']
    assert [e['kind']for e in returns]==['mapping','mapped','output','mapping','mapped','input']
    assert len(state['file_open_returns'])==len(state['mapped_open_returns'])==len(state['mapping_entries'])==len(state['mapping_returns'])==2
    assert state['file_open_returns']==[state['output_open_return'],state['input_open_return']]
    assert state['mapped_open_return']==state['mapped_open_returns'][1]
    assert state['mapping_return']==state['mapping_returns'][1]
    assert state['mapping_entry']==state['mapping_entries'][1]
    assert state['open_file_formals']==dict(name=nnames[1],flags=0x8000,mode=0,text='hello.obj')
    assert trap['open_file_formals']==dict(name=names[1],flags=0x8000,mode=0,text='hello.obj')
    assert state['file_record_entry']==dict(stack_pointer=nopens[1]['stack_pointer']-48,
        formals=[nnames[1],0x8000,0,state['file_record_formals']['created']],hidden_inputs=[])
    assert trap['file_input_lifetime']==state['file_input_lifetime']=='captured at mapped-opener return before caller locals expire'

    flow=[e for e in events if e['event']=='file_flow_observation']
    sequence=[('acquire',1),('available',1),('move',1),('move',0),('available',0),('name',1),('name',0),('acquire',0)]
    assert [(e['kind'],e['before'],e['index'])for e in flow]==[(k,b,i)for i in range(2)for k,b in sequence]
    lookups=[e for e in events if e['event']=='file_lookup_observation'];assert len(lookups)==4
    assert len(state['file_lookup_calls'])==len(state['file_flow_calls']['move'])==len(state['file_flow_calls']['available'])==2
    create=[e for e in events if e['event']=='mapped_create_observation'];assert len(create)==4
    assert len(state['mapped_create_calls'])==2 and state['mapped_create_call']==state['mapped_create_calls'][1]
    apis=[e for e in events if e['event']=='mapping_api_observation'];assert len(apis)==12
    assert [(e['kind'],e['before'])for e in apis]==[(k,b)for i in range(2)for k in range(3)for b in (1,0)]
    assert len(state['mapping_api_calls'])==6
    error_events=[e for e in events if e['event']=='crt_error_state']
    errors={e['phase']:e for e in error_events};assert len(errors)==len(error_events)
    ranges=[];oactive={};nactive={}
    used_names=[];nused_names=[];views=[];nviews=[];handles=[];nhandles=[]
    for i in range(2):
        index=15-i;text=expected_names[i];flags=0x8302 if i==0 else 0x8000;mode=0x180 if i==0 else 0
        record=oslots[index];nrecord=nslots[index]
        acquire_before,available_before,move_before,move_after,available_after,name_before,name_after,acquire_after=flow[i*8:i*8+8]
        created=oentry[i]-8;size_slot=oentry[i]-4
        acquired=[names[i],flags,mode,created]
        pair(acquire_before,acquire_after,(0x40b829,0x40b82e),16)
        assert acquire_before['arguments']==acquire_after['arguments']==acquired
        assert acquire_before['stack']==oentry[i]-40 and acquire_before['created']==0
        assert acquire_after['created']==1 and acquire_after['result']==record
        move_args=[record,0x46753c,0,0x4674d8,0,0]
        nmove_args=[nrecord,symbols['file_newest'],0,symbols['pending_files'],0,0]
        pair(move_before,move_after,(0x40b951,0x40b956),24)
        assert move_before['arguments']==move_after['arguments']==move_args
        move=state['file_flow_calls']['move'][i];normal(move,24);assert move['arguments']==nmove_args
        for post,e in enumerate((move_before,move_after)):
            active=dict(oactive);nact=dict(nactive)
            assert e['cache']==cache(oslots,i+post)
            assert (original/('file-flow-move-%d-%s-slots.bin'%(i,'after'if post else'before'))).read_bytes()==struct.pack('<16I',*oslots)
            observed=[words((original/('file-flow-move-%d-%s-record-%02d.bin'%(i,'after'if post else'before',j))).read_bytes())for j in range(16)]
            assert observed==pool(oslots,i+post,active)
            snap=move['after'if post else'before']
            assert snap['slots']==nslots and snap['table']==state['file_globals']['file_slots']
            assert snap['table_bytes']==state['file_slots_bytes']and snap['global_slots']==symbols
            assert snap['cache']==cache(nslots,i+post)
            assert [words(bytes.fromhex(r))for r in snap['records']]==pool(nslots,i+post,nact)
        pair(available_before,available_after,(0x40b885,0x40b88a),0)
        assert available_before['arguments']==available_after['arguments']==[]and available_after['result']==record
        avail=state['file_flow_calls']['available'][i];normal(avail,0)
        assert avail['arguments']==[]and avail['result']==nrecord and avail['before']==move['before']and avail['after']==move['after']
        lb,la=lookups[i*2:i*2+2];pair(lb,la,(0x40b878,0x40b87d),4)
        assert lb['name']==la['name']==names[i]and la['result']==0
        lookup=state['file_lookup_calls'][i];normal(lookup,4)
        assert lookup['name']==nnames[i]and lookup['result']==0
        pair(name_before,name_after,(0x40b898,0x40b89d),4)
        assert name_before['arguments']==name_after['arguments']==[names[i]]
        assert name_before['text']==name_after['text']==text
        owned=name_after['result'];nowned=state['mapped_create_calls'][i]['arguments'][0]
        assert owned and nowned and owned!=names[i]and nowned!=nnames[i]
        contents=text.encode()+b'\0'
        assert (original/('file-copied-name-%d.bin'%i)).read_bytes()==(original/('file-record-name-%d.bin'%index)).read_bytes()==contents
        assert state['duplicate_calls'][-2+i]==dict(input=nnames[i],text=text,result=nowned)
        used_names.append(owned);nused_names.append(nowned)
        acquired_record=(owned,0x13 if i==0 else 1,index,0,0,0,0,0,0,oslots[15]if i else 0)
        nacquired_record=(nowned,0x13 if i==0 else 1,index,0,0,0,0,0,0,nslots[15]if i else 0)
        assert words((original/('file-acquire-result-%d.bin'%i)).read_bytes())==acquired_record
        cb,ca=create[i*2:i*2+2];pair(cb,ca,(0x40c914,0x40c91a),28)
        access=[0xc0000000,0,0,2,0x80,0]if i==0 else [0x80000000,1,0,3,0,0]
        assert cb['arguments']==ca['arguments']==[owned]+access and cb['text']==ca['text']==text
        cc=state['mapped_create_calls'][i];normal(cc,28)
        assert cc['arguments']==[nowned]+access and cc['text']==text
        handle=ca['result'];nhandle=cc['result'];assert handle not in (0,0xffffffff)and nhandle not in (0,0xffffffff)
        handles.append(handle);nhandles.append(nhandle)
        for phase in ('before','after'):
            key='mapped-create-'+('%d-'%i if i else '')+phase
            expected=dict(errno=2,doserrno=2,last_error=2 if i==0 and phase=='before'else 0)
            assert {k:errors[key][k]for k in expected}==cc[phase+'_errors']==expected
        size=0x80000 if i==0 else 2262;protection=4 if i==0 else 2;view_access=0xf001f if i==0 else 4
        observed=apis[i*6:i*6+6];calls=state['mapping_api_calls'][i*3:i*3+3]
        for j,kind in enumerate(('create','view','close')):
            before,after=observed[j*2:j*2+2];call=calls[j];count=1 if j==2 else 6
            pair(before,after,[(0x40cd44,0x40cd4a),(0x40cd95,0x40cd9b),(0x40cdc9,0x40cdcf)][j],count*4)
            normal(call,count*4);assert call['kind']==kind and call['mapping_index']==i
            assert before['arguments']==after['arguments']
            for phase in ('before','after'):
                key='mapping-'+('%d-'%i if i else '')+kind+'-'+phase
                assert {k:errors[key][k]for k in ('errno','doserrno','last_error')}==call[phase+'_errors']==dict(errno=2,doserrno=2,last_error=0)
        assert observed[0]['arguments']==[handle,0,protection,0,size,0]
        assert calls[0]['arguments']==[nhandle,0,protection,0,size,0]
        mapping=observed[1]['result'];nmapping=calls[0]['result']
        assert mapping and nmapping and mapping!=handle and nmapping!=nhandle
        assert observed[2]['arguments']==[mapping,view_access,0,0,size,0]
        assert calls[1]['arguments']==[nmapping,view_access,0,0,size,0]
        view=observed[3]['result'];nview=calls[1]['result'];assert view and nview
        views.append(view);nviews.append(nview)
        assert observed[4]['arguments']==[mapping]and calls[2]['arguments']==[nmapping]
        assert observed[5]['result']==calls[2]['result']==1
        rmap,rmapped,ropen=returns[i*3:i*3+3]
        for e,call,site,cleanup,result in zip((rmap,rmapped,ropen),(state['mapping_returns'][i],state['mapped_open_returns'][i],state['file_open_returns'][i]),(0x40c9cf,0x40b84a,0x44b8e5 if i==0 else 0x449a8d),(12,16,12),(1,0,index)):
            assert e['site']==site and e['stack']==e['entry_stack']+cleanup+4
            assert (e['result']&255 if e is rmap else e['result'])==result
            normal(call,cleanup);assert call['result']==result
            if e is not ropen:assert e['error']==call['error']==0 and e['created']==1
        assert [e['entry_stack']for e in (rmap,rmapped,ropen)]==[oentry[i]-76,oentry[i]-44,oentry[i]]
        no=nopens[i]['stack_pointer'];nm=state['mapped_open_returns'][i];nr=state['mapping_returns'][i]
        assert state['file_open_returns'][i]['arguments']==nopens[i]['formals']
        assert state['file_open_returns'][i]['entry_stack']==no
        assert nm['entry_stack']==no-48 and nr['entry_stack']==no-128
        caller=nm['caller_formals'];assert [caller[k]for k in ('name','flags','mode')]==nopens[i]['formals']
        assert caller['created_bytes']==1 and caller['initial_created']==1 and caller['initial_size']==0 and caller['file_access_statistics']==0
        assert caller['initial_mapped_result']==-1 and caller['created']==caller['local_created']<no
        assert caller['size_slot']+4<=no and not caller['size_slot']<=caller['created']<caller['size_slot']+4
        assert nm['arguments']==[nrecord,flags,nm['arguments'][2],caller['size_slot']]
        assert 0<=nm['arguments'][2]<=0xffffffff and nm['arguments'][2]&255==nm['created']==1
        assert nr['arguments']==state['mapping_entries'][i]['raw_formals']==[nrecord,0,caller['size_slot']]
        assert state['mapping_entries'][i]['hidden_inputs']==[]
        mapped_call=[e for e in events if e['event']=='file_open_call'and e['kind']=='mapped'][i]
        mapping_call=[e for e in events if e['event']=='file_open_call'and e['kind']=='mapping'][i]
        assert mapped_call['formals']==[record,flags,mapped_call['formals'][2],size_slot]and mapped_call['formals'][2]&255==1
        assert mapping_call['formals']==[record,0,size_slot]
        assert mapped_call['hidden_inputs']==mapping_call['hidden_inputs']==[]
        assert mapped_call['stack']-4==rmapped['entry_stack']and mapping_call['stack']-4==rmap['entry_stack']
        assert available_before['stack']==oentry[i]-56 and lb['stack']==oentry[i]-60
        assert move_before['stack']==oentry[i]-88 and name_before['stack']==oentry[i]-60
        assert avail['entry_stack']==nopens[i]['stack_pointer']-80
        assert move['entry_stack']==nopens[i]['stack_pointer']-128
        assert lookup['entry_stack']==nopens[i]['stack_pointer']-80
        oactive[index]=(owned,0x1b if i==0 else 9,index,0 if i==0 else size,handle,view,size,8 if i==1 and state['function']in ('is_mac_resource_file','get_file_length')else 0,0,oslots[15]if i else 0)
        nactive[index]=(nowned,0x1b if i==0 else 9,index,0 if i==0 else size,nhandle,nview,size,8 if i==1 and state['function']in ('is_mac_resource_file','get_file_length')else 0,0,nslots[15]if i else 0)
        ranges.extend([dict(original_base=owned,native_base=nowned,bytes=len(contents)),dict(original_base=view,native_base=nview,bytes=size)])
        if i==1:
            assert state['file_record_formals']==caller
            assert trap['file_record_formals']==dict(name=names[i],flags=flags,mode=mode,created=created,
                initial_created=1,size_slot=size_slot,initial_size=0,file_access_statistics=0,created_bytes=1)
            for current,slots,physical in ((trap,oslots,mapped_call['formals'][2]),(state,nslots,nm['arguments'][2])):
                size_pointer=size_slot if current is trap else caller['size_slot']
                assert current['mapped_open_formals']==dict(record=slots[index],flags=flags,created=1,
                    raw_created_word=physical,raw_upper_bits=physical>>8,size=size_pointer,initial_size=0)
                assert current['mapping_formals']==dict(record=slots[index],resize=0,raw_resize_word=0,error=size_pointer,initial_error=0)
            assert state['mapped_open_entry']==dict(stack_pointer=nm['entry_stack'],raw_formals=nm['arguments'],hidden_inputs=[])
            assert words(bytes.fromhex(state['file_acquire_return']['raw']))==nacquired_record
            assert state['file_acquire_return']['name']==nowned and bytes.fromhex(state['file_acquire_return']['name_bytes'])==contents
            assert state['file_acquire_return']['arguments']==[nnames[i],flags,mode,caller['created']]
            assert state['file_acquire_return']['before_created']==0 and state['file_acquire_return']['after_created']==1
            normal(state['file_acquire_return'],16)
            assert state['file_acquire_return']['result']==nrecord
            assert state['file_name_call']['arguments']==[nnames[i]]and state['file_name_call']['text']==text and state['file_name_call']['result']==nowned
            normal(state['file_name_call'],4)
            assert bytes.fromhex(state['file_name_call']['name_bytes'])==bytes.fromhex(state['file_record_name_bytes'])==contents
    assert len(set(used_names))==len(set(nused_names))==len(set(handles))==len(set(nhandles))==2
    assert [words((original/('file-record-%02d.bin'%i)).read_bytes())for i in range(16)]==pool(oslots,2,oactive)
    assert [words(bytes.fromhex(r))for r in state['file_records']]==pool(nslots,2,nactive)
    assert state['created_views']==[dict(address=v,bytes=z,mapping_index=i)for i,(v,z)in enumerate(zip(nviews,(0x80000,2262)))]
    assert state['created_view']==dict(address=nviews[0],bytes=0x80000)
    output=b''.join((original/('created-mapping-view-%02d.bin'%i)).read_bytes()for i in range(8))
    assert output==bytes(0x80000)==(directory/'created-mapping-view.bin').read_bytes()==(original/'artifacts/hello.exe').read_bytes()==(directory/'artifacts/hello.exe').read_bytes()
    case=json.loads((original/'case.json').read_text());obj=next(x for x in case['inputs']if x['name']=='hello.obj')
    original_input=(Path(__file__).resolve().parents[1]/obj['path']).read_bytes()
    assert len(original_input)==2262 and hashlib.sha256(original_input).hexdigest()==obj['sha256']
    assert (original/'mapping-01-view-00.bin').read_bytes()==(directory/'mapping-01-view.bin').read_bytes()==original_input
    globals_=input_symbols(elf,receipt['native_sha256'])
    assert state['input_diagnostic_name']==dict(address=globals_['current_phase'],modeled_prefix_bytes=32,raw=(b'hello.obj\0'+bytes(22)).hex())
    assert (original/'input-diagnostic-before.bin').read_bytes()==bytes.fromhex(state['input_diagnostic_before'])==bytes(32)
    assert (original/'input-diagnostic-name.bin').read_bytes()==bytes.fromhex(state['input_diagnostic_name']['raw'])
    assert (original/'current-input-file.bin').read_bytes()==struct.pack('<I',14)and state['input_file_index']==14
    assert (original/'incremental-status-code.bin').read_bytes()==bytes(4)
    assert (original/'incremental-reuse.bin').read_bytes()==bytes(1)
    assert state['incremental_status']==state['incremental_status_entry']==dict(code=0,reuse=0,code_bytes=4,reuse_bytes=1)
    assert state['output_file']==trap['output_file']==15
    ranges.extend(dict(original_base=a,native_base=b,bytes=40)for a,b in zip(oslots,nslots))
    ranges.append(dict(original_base=struct.unpack('<I',(original/'file-pointers.bin').read_bytes()[:4])[0],native_base=state['file_globals']['file_slots'],bytes=64))
    ranges.append(dict(original_base=0x4670c4,native_base=globals_['current_phase'],bytes=32))
    if state['function']in ('mapped_read','is_mac_resource_file','get_file_length'):
        from archive_state import compare as compare_archive
        live_output=compare_archive(original,state,trap,events,native_directory,oslots,nslots)
        if live_output is not None:ranges.append(live_output)
    return ranges
