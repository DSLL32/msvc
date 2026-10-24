"""Actual first index assignment and the constructor's published module."""
import functools,hashlib,json,re,struct,subprocess
from pathlib import Path
from binary import PE
from mapped_seek_state import local_offsets,with_cursor
from archive_state import native_length_output_contract,native_seek_bridge_contract


def input_contract(directory):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE');proof=json.loads((root/'cases/hello-win32/module-index-contract.json').read_text())
    assert proof['formals']==proof['hidden_inputs']==[]and(proof['start'],proof['end'])==(0x431e50,0x431f26)
    body=pe.read(proof['start'],proof['end']-proof['start']);assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_OP_MEM
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True;instructions=list(cs.disasm(body,proof['start']));assert sum(i.size for i in instructions)==214
    assert [[i.address,int(i.op_str or '0',0)]for i in instructions if i.mnemonic=='ret']==proof['returns']==[[0x431f1f,0],[0x431f25,0]]
    globals=[0x467368,0x494390,0x494394,0x4943a8,0x4943ac,0x4943b0]
    assert [dict(address=i.address,bytes=i.bytes.hex(),text=i.mnemonic+' '+i.op_str)for i in instructions if any(o.type==CS_OP_MEM and o.mem.disp in globals for o in i.operands)]==proof['global_references']
    assert pe.read(0x467368,1)==b'\1'
    section,=[s for s in pe.sections if pe.base+s['rva']+s['size']<=0x494390 and 0x494398<=pe.base+s['rva']+s['virtual_size']]
    for value in(0x494390,0x494394):assert pe.data.count(struct.pack('<I',value))==1
    pending=proof['pending'];assert pending['formals']==['OutputModule *module']and pending['hidden_inputs']==[]and pending['returns']==[[0x462f0d,4]]
    assert hashlib.sha256(pe.read(0x462ef0,32)).hexdigest()==pending['body_sha256']
    assert pe.read(0x462ef0,32).hex()=='568b7424088b46188d0480c1e003506828494900e8d7c6feff8946605ec20400'
    config=json.loads((directory/'functions.json').read_text());actual,=[r for r in config['driver']if r['name']==pending['name']]
    assert actual['pending_extent']and(actual['start'],actual['end'])==(0x462ef0,0x462ef1)
    return proof

def record(words,header,library,file_size):
    data=bytearray(156)
    machine,sections,timestamp,symbol_offset,symbols,optional,characteristics=struct.unpack('<HHIIIHH',header)
    for offset,value in((0,words[0]),(4,words[1]),(12,symbol_offset),(16,symbols),(24,sections),(56,timestamp),(60,file_size),(64,20),(100,library),(104,words[26])):
        struct.pack_into('<I',data,offset,value)
    struct.pack_into('<HH',data,20,optional,characteristics)
    return bytes(data)


def current_record(original,state,trap):
    current=state['module_creation_current'];left=(original/'input-module-record.bin').read_bytes();right=bytes.fromhex(current['record'])
    assert len(left)==len(right)==156
    lw=struct.unpack('<39I',left);rw=struct.unpack('<39I',right);header=(original/'input-module-header-current.bin').read_bytes()
    assert header==bytes.fromhex(current['header'])and len(header)==20
    assert current['section_count']==7 and current['symbol_count']==37
    assert lw[0]and lw[1]and lw[26]and rw[0]and rw[1]and rw[26]
    assert left==record(lw,header,trap['command_line_library']['address'],2262)
    assert right==record(rw,header,state['command_line_library']['address'],2262)
    assert current['names']==[dict(address=rw[i],text='hello.obj')for i in(0,1)]
    assert current['contributions']==rw[26]
    return left,right


@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    offsets=local_offsets(path,digest,'read_object_file',('module','header','created'));assert offsets==dict(module=-40,header=-36,created=-45)
    rows=[r.split()for r in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()];table={r[3]:r for r in rows if len(r)==4}
    start=int(table['read_object_file'][0],16);size=int(table['read_object_file'][1],16);callee=int(table['allocate_module_section_headers'][0],16)
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+size),path],text=True)
    body=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    call,=[i for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,start)if i.mnemonic=='call'and i.op_str==hex(callee)]
    # Actual void-one-pointer stub has12B stack frame and30B traced prologue.
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(callee),'--stop-address='+str(callee+31),path],text=True)
    stub=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert stub[:9].hex()=='5589e583ec088b4508'and stub[30:]==b'\xcc'
    slots={name:int(table[name][0],16)for name in('contribution_arena','reset_module_indices','module_index_value','buffer8')}
    assert int(table['reset_module_indices'][1],16)==1 and int(table['module_index_value'][1],16)==8
    start=int(table['allocate_module_index'][0],16);size=int(table['allocate_module_index'][1],16)
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+size),path],text=True)
    data=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    from capstone import CS_OP_MEM
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True;used={o.mem.disp for i in cs.disasm(data,start)for o in i.operands if o.type==CS_OP_MEM}
    expected={slots['buffer8']+o for o in(0,4,8)}|{slots['reset_module_indices'],slots['module_index_value'],slots['module_index_value']+4}
    assert expected<=used
    assert len({slots['reset_module_indices'],slots['module_index_value'],slots['buffer8']})==3
    return offsets,call.address+call.size,callee+31,slots


def compare(original,state,trap,events,directory):
    input_contract(directory)
    compare_second_length(original,state,trap,events,directory)
    left,right=current_record(original,state,trap);lw=struct.unpack('<39I',left);rw=struct.unpack('<39I',right);current=state['module_creation_current']
    creator,=[e for e in events if e['event']=='input_module_entry'];obj,=[e for e in events if e['event']=='object_read_entry'];entry=creator['stack_pointer'];nentry=state['input_module_entry'];nobj=state['object_reader_entry']
    assert trap['frontier_inputs']==dict(formals=[trap['constructor_local_module']],hidden_register_inputs=[])
    assert trap['module_headers_formals']==dict(formals=trap['frontier_inputs']['formals'],hidden_inputs=[])
    assert trap['esp']==obj['stack_pointer']-40 and trap['stack'][:2]==[0x4498c5,trap['constructor_local_module']]
    assert state['module_headers_entry']==dict(stack_pointer=nobj['stack_pointer']-112,formals=[current['module']],hidden_inputs=[])
    assert state['frontier_inputs']==dict(formals=[current['module']],hidden_register_inputs=[])
    receipt=json.loads((directory/'receipt.json').read_text());offsets,returned,pc,slots=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    assert state['pc']==pc and state['stack'][3]==returned and state['stack'][4]==current['module']
    assert current['contribution_arena']['slot']==slots['contribution_arena']
    assert current['owner_function']=='read_object_file'and current['module_local_address']==nobj['stack_pointer']+offsets['module']and current['module_local_bytes']==4
    assert current['header_address']==nobj['stack_pointer']+offsets['header']==nentry['formals'][3]
    assert current['created_address']==nobj['stack_pointer']+offsets['created']==nentry['formals'][5]
    assert current['library']==nentry['formals'][4]
    assert (original/'input-module-created-current.bin').read_bytes()==bytes.fromhex(current['created'])==b'\1'
    index,=[e for e in events if e['event']=='module_index_entry'];done,=[e for e in events if e['event']=='module_index_return']
    assert index==dict(event='module_index_entry',stack_pointer=entry-24,formals=[],hidden_inputs=[])
    assert done==dict(event='module_index_return',entry_stack=entry-24,return_stack=entry-20,result=0,caller_module=trap['constructor_local_module'])
    nindex=state['module_index_entry'];assert nindex==dict(stack_pointer=nentry['stack_pointer']-64,formals=[],hidden_inputs=[])
    ndone,=state['module_index_returns'];assert ndone==dict(nindex,before=state['module_index_before'],normal_return=True,return_stack=nindex['stack_pointer']+4,result=0,after=ndone['after'])
    before=struct.unpack('<3I',(original/'module-index-before-descriptor.bin').read_bytes());after=struct.unpack('<3I',(original/'module-index-after-descriptor.bin').read_bytes());nbefore=struct.unpack('<3I',bytes.fromhex(ndone['before']['descriptor']));nafter=struct.unpack('<3I',bytes.fromhex(ndone['after']['descriptor']))
    assert before[0]==after[0]and before[1:]==nbefore[1:]==(500,500)and after[1:]==nafter[1:]==(1,500)and nbefore[0]==nafter[0]
    assert (original/'module-index-before-reset.bin').read_bytes()==bytes([ndone['before']['reset']])==b'\1'
    assert (original/'module-index-after-reset.bin').read_bytes()==bytes([ndone['after']['reset']])==b'\0'
    value=(original/'module-index-before-value.bin').read_bytes();assert value==(original/'module-index-after-value.bin').read_bytes()==bytes.fromhex(ndone['before']['value'])==bytes.fromhex(ndone['after']['value'])==bytes(8)
    for before_data,after_data in(((original/'module-index-before-array.bin').read_bytes(),(original/'module-index-after-array.bin').read_bytes()),(bytes.fromhex(ndone['before']['array']),bytes.fromhex(ndone['after']['array']))):
        assert len(before_data)==len(after_data)==4000 and after_data[:8]==value
        assert before_data[8:]==after_data[8:],'unused own allocation tail changed'
    assert (original/'buffer8.bin').read_bytes()==(original/'module-index-after-descriptor.bin').read_bytes()
    assert state['app_globals']['buffer8']==ndone['after']['descriptor']
    returned,=[e for e in events if e['event']=='input_module_return'];assert returned==dict(event='input_module_return',entry_stack=entry,return_stack=entry+52,result=trap['constructor_local_module'])
    original_returned=(original/'input-module-returned-record.bin').read_bytes();assert original_returned==record(lw,bytes.fromhex(current['header']),creator['formals'][4],0)
    assert (original/'input-module-returned-header.bin').read_bytes()==bytes.fromhex(current['header'])
    assert (original/'input-module-returned-created.bin').read_bytes()==b'\1'
    nreturned,=state['input_module_returns'];assert nreturned==dict(nentry,normal_return=True,return_stack=nentry['stack_pointer']+52,result=current['module'],record=record(rw,bytes.fromhex(current['header']),nentry['formals'][4],0).hex(),header=current['header'],created='01',library=nreturned['library'])
    assert (original/'input-module-returned-library.bin').read_bytes()==(original/'command-line-library.bin').read_bytes()
    assert nreturned['library']==state['command_line_library']['record']
    # Reuse the unchanged exact two-copy/arena state proof, explicitly projecting
    # the earlier constructor frame/record. This is historical, not live stack.
    from module_creation_state import compare_initial_allocations
    return compare_initial_allocations(original,state,trap,events,creator,nentry,lw,rw)


def compare_second_length(original,state,trap,events,directory):
    """Derived original second entry/return, witnessed by caller and three seeks."""
    pe=PE(Path(__file__).resolve().parents[4]/'toolchains/5.10/bin/LINK.EXE')
    assert pe.read(0x449866,15).hex()=='8b0df8ee470051e80e28fcff89473c'
    # The executed exact caller passes current_input_file14, then stores EAX as
    # file_size. Reaching the next real trap proves normal continuation/cleanup;
    # nested observed calls establish that this is the distinct20/end/20 visit.
    assert int.from_bytes((original/'current-input-file.bin').read_bytes(),'little')==14
    obj,=[e for e in events if e['event']=='object_read_entry'];length_stack=obj['stack_pointer']-40
    nobj=state['object_reader_entry'];nlength_stack=nobj['stack_pointer']-112
    receipt=json.loads((directory/'receipt.json').read_text());elf=str(directory/'compiled/native-LINK.EXE');digest=receipt['native_sha256']
    offsets=local_offsets(elf,digest,'get_file_length',('current_error','end_error'));assert offsets==dict(current_error=-16,end_error=-20)
    _,frame=native_length_output_contract(elf,digest);bridge=native_seek_bridge_contract(elf,digest)
    calls=[e for e in events if e['event']=='seek_call'];returns=[e for e in events if e['event']=='seek_return'];mapped=[e for e in events if e['event']=='mapped_seek_call'];mdone=[e for e in events if e['event']=='mapped_seek_return']
    assert len(calls)==len(returns)==len(mapped)==len(mdone)==len(state['seek_file_calls'])==len(state['mapped_seek_calls'])==9
    original_record=struct.unpack('<16I',(original/'file-slots.bin').read_bytes())[14];native_record=struct.unpack('<16I',bytes.fromhex(state['file_slots_bytes']))[14]
    oraw=(original/'file-record-14.bin').read_bytes();nraw=bytes.fromhex(state['file_records'][14]);positions=[20,20,2262,20]
    for i,(offset,origin,result,site,continuation,role)in enumerate(((0,1,20,0x40c0a8,0x40c0ad,'current_error'),(0,2,2262,0x40c0c1,0x40c0c6,'end_error'),(20,0,20,0x40c0dd,0x40c0e2,'end_error'))):
        j=6+i;error=length_stack+(-4 if i==0 else 4);nerror=nlength_stack+offsets[role];stack=length_stack-36;nstack=nlength_stack-frame;args=[14,offset,origin,error];nargs=[14,offset,origin,nerror]
        assert calls[j]==dict(event='seek_call',index=j,site=site,entry_stack=stack,formals=args,hidden_inputs=[],record=original_record)
        assert returns[j]==dict(event='seek_return',index=j,site=continuation,entry_stack=stack,formals=args,hidden_inputs=[],record=original_record,return_stack=stack+20,result=result)
        assert mapped[j]==dict(event='mapped_seek_call',index=j,site=0x40c061,entry_stack=stack-20,formals=[original_record]+args[1:],hidden_inputs=[])
        assert mdone[j]==dict(event='mapped_seek_return',index=j,site=0x40c066,entry_stack=stack-20,formals=[original_record]+args[1:],hidden_inputs=[],return_stack=stack,result=result)
        for prefix in('seek','mapped-seek'):
            for phase,position in(('before',positions[i]),('after',positions[i+1])):
                assert (original/('%s-%d-record-%s.bin'%(prefix,j,phase))).read_bytes()==with_cursor(oraw,position)
                assert (original/('%s-%d-error-%s.bin'%(prefix,j,phase))).read_bytes()==bytes(4)
        output=dict(address=nerror,bytes=4,owner_local_address=nerror,owner_local_bytes=4,before='00000000',initial_value_contract='initialized DWORD error scratch; no incoming-value assumption at pending seek',owner_kind='get_file_length.'+role)
        assert state['seek_file_calls'][j]==dict(stack_pointer=nstack,formals=nargs,hidden_inputs=[],caller='get_file_length',record=native_record,record_before=with_cursor(nraw,positions[i]).hex(),output=output,normal_return=True,return_stack=nstack+20,result=result,record_after=with_cursor(nraw,positions[i+1]).hex(),error_after='00000000')
        assert state['mapped_seek_calls'][j]==dict(stack_pointer=nstack-bridge,formals=[native_record]+nargs[1:],hidden_inputs=[],record_before=with_cursor(nraw,positions[i]).hex(),error_before='00000000',normal_return=True,return_stack=nstack-bridge+20,result=result,record_after=with_cursor(nraw,positions[i+1]).hex(),error_after='00000000')
    assert state['file_length_returns'][1]==dict(stack_pointer=nlength_stack,formals=[14],hidden_inputs=[],normal_return=True,return_stack=nlength_stack+8,result=2262,record_after=with_cursor(nraw,20).hex())
