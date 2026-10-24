"""Four real seek invocations, normal returns and historical local ownership."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE


def with_cursor(raw,cursor):
    data=bytearray.fromhex(raw)if isinstance(raw,str)else bytearray(raw)
    assert len(data)==40
    struct.pack_into('<I',data,28,cursor)
    return bytes(data)


@functools.lru_cache(maxsize=8)
def local_offsets(path,digest,owner,names):
    """Read finite named EBP-relative locals from the exact frozen ELF's DWARF."""
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbol,=[r for r in rows if len(r)==4 and r[3]==owner]
    assert symbol[2]=='T'
    info=subprocess.check_output(['readelf','--debug-dump=info',path],text=True)
    blocks=re.split(r'(?=^ <1><)',info,flags=re.M)
    block,=[b for b in blocks if 'DW_TAG_subprogram'in b.splitlines()[0]and re.search(r'DW_AT_name\s*:.*:\s*'+re.escape(owner)+r'\s*$',b,re.M)]
    assert int(re.search(r'DW_AT_low_pc[^\n]*:\s*(0x[0-9a-f]+)',block).group(1),16)==int(symbol[0],16)
    assert 'DW_OP_reg5 (ebp)'in block
    result={}
    for name in names:
        variable,=[b for b in re.split(r'(?=^ <2><)',block,flags=re.M)if 'DW_TAG_variable'in b.splitlines()[0]and re.search(r'DW_AT_name\s*:.*:\s*'+re.escape(name)+r'\s*$',b,re.M)]
        result[name]=int(re.search(r'DW_OP_fbreg: (-?\d+)',variable).group(1))-4
    return result


def compare(original,state,trap,events,directory,reader,original_record,native_record):
    from archive_state import native_classifier_contract,native_length_output_contract,native_seek_bridge_contract
    binary=Path(__file__).resolve().parents[1]
    pe=PE(binary.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    proof=json.loads((binary/'cases/hello-win32/big-endian-input-contract.json').read_text())
    body=pe.read(0x44e5c0,86)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    decoded=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x44e5c0))
    assert sum(i.size for i in decoded)==86
    assert [[i.address,int(i.op_str,0)]for i in decoded if i.mnemonic=='ret']==proof['returns']==[[0x44e5fa,4],[0x44e613,4]]
    assert proof['formals']==['int file']and proof['hidden_inputs']==[]
    mac,=[e for e in events if e['event']=='mac_resource_entry']
    length,=[e for e in events if e['event']=='file_length_entry']
    assert mac==dict(event='mac_resource_entry',stack_pointer=reader['stack_pointer']-20,formals=[14],hidden_inputs=[])
    assert length==dict(event='file_length_entry',stack_pointer=mac['stack_pointer']-24,formals=[14],hidden_inputs=[])
    if state['function']=='read_big_endian_dword':
        assert trap['esp']==mac['stack_pointer']-24 and trap['stack'][:2]==[0x44e65e,14]
        assert trap['big_endian_formals']==dict(formals=[14],hidden_inputs=[])
        assert trap['frontier_inputs']==state['frontier_inputs']==dict(formals=[14],hidden_register_inputs=[])
    else:assert state['function']in ('byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine')
    nmac=state['mac_resource_entry'];nlength=state['file_length_entry'];receipt=json.loads((directory/'receipt.json').read_text())
    elf=str(directory/'compiled/native-LINK.EXE');digest=receipt['native_sha256']
    frame=native_classifier_contract(elf,digest)
    assert nmac==dict(stack_pointer=state['read_input_entry']['stack_pointer']-48,formals=[14],hidden_inputs=[])
    assert nlength==dict(stack_pointer=nmac['stack_pointer']-frame,formals=[14],hidden_inputs=[])
    assert state['big_endian_entry']==dict(stack_pointer=nmac['stack_pointer']-frame,formals=[14],hidden_inputs=[])
    offsets=local_offsets(elf,digest,'get_file_length',('current_error','end_error'))
    mac_offsets=local_offsets(elf,digest,'is_mac_resource_file',('error',))
    assert offsets==dict(current_error=-16,end_error=-20)and mac_offsets==dict(error=-16)
    _,length_frame=native_length_output_contract(elf,digest);bridge=native_seek_bridge_contract(elf,digest)
    oerrors=[length['stack_pointer']-4,length['stack_pointer']+4,length['stack_pointer']+4,mac['stack_pointer']+4]
    nerrors=[nlength['stack_pointer']+offsets['current_error'],nlength['stack_pointer']+offsets['end_error'],nlength['stack_pointer']+offsets['end_error'],nmac['stack_pointer']+mac_offsets['error']]
    for pointers in(oerrors,nerrors):
        assert pointers[1]==pointers[2]
        distinct=sorted(set(pointers));assert len(distinct)==3 and all(a+4<=b for a,b in zip(distinct,distinct[1:]))
    ostacks=[length['stack_pointer']-36]*3+[mac['stack_pointer']-36]
    nstacks=[nlength['stack_pointer']-length_frame]*3+[nmac['stack_pointer']-frame]
    calls=[e for e in events if e['event']=='seek_call'];returns=[e for e in events if e['event']=='seek_return']
    mapped=[e for e in events if e['event']=='mapped_seek_call'];mapped_returns=[e for e in events if e['event']=='mapped_seek_return']
    assert len(calls)==len(returns)==len(mapped)==len(mapped_returns)==len(state['seek_file_calls'])==len(state['mapped_seek_calls'])==(5 if state['function']=='initialize_machine'else 4)
    sites=[0x40c0a8,0x40c0c1,0x40c0dd,0x44e653];continuations=[0x40c0ad,0x40c0c6,0x40c0e2,0x44e658]
    positions=[8,8,2262,8,0];arguments=[(0,1),(0,2),(8,0),(0,0)];results=[8,2262,8,0]
    roles=['get_file_length.current_error','get_file_length.end_error','get_file_length.end_error','is_mac_resource_file.error']
    oraw=(original/'file-record-14.bin').read_bytes();nraw=bytes.fromhex(state['file_records'][14])
    assert struct.unpack_from('<I',oraw,28)[0]==struct.unpack_from('<I',nraw,28)[0]==(4 if state['function']in ('byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine')else 0)
    for i,((offset,origin),result)in enumerate(zip(arguments,results)):
        oformals=[14,offset,origin,oerrors[i]];nformals=[14,offset,origin,nerrors[i]]
        omapped=[original_record,offset,origin,oerrors[i]];nmapped=[native_record,offset,origin,nerrors[i]]
        assert calls[i]==dict(event='seek_call',index=i,site=sites[i],entry_stack=ostacks[i],formals=oformals,hidden_inputs=[],record=original_record)
        assert returns[i]==dict(event='seek_return',index=i,site=continuations[i],entry_stack=ostacks[i],formals=oformals,hidden_inputs=[],record=original_record,return_stack=ostacks[i]+20,result=result)
        assert mapped[i]==dict(event='mapped_seek_call',index=i,site=0x40c061,entry_stack=ostacks[i]-20,formals=omapped,hidden_inputs=[])
        assert mapped_returns[i]==dict(event='mapped_seek_return',index=i,site=0x40c066,entry_stack=ostacks[i]-20,formals=omapped,hidden_inputs=[],return_stack=ostacks[i],result=result)
        for prefix in('seek','mapped-seek'):
            for phase,position in(('before',positions[i]),('after',positions[i+1])):
                assert (original/('%s-%d-record-%s.bin'%(prefix,i,phase))).read_bytes()==with_cursor(oraw,position)
                assert (original/('%s-%d-error-%s.bin'%(prefix,i,phase))).read_bytes()==bytes(4)
        output=dict(address=nerrors[i],bytes=4,owner_local_address=nerrors[i],owner_local_bytes=4,before='00000000',initial_value_contract='initialized DWORD error scratch; no incoming-value assumption at pending seek',owner_kind=roles[i])
        assert state['seek_file_calls'][i]==dict(stack_pointer=nstacks[i],formals=nformals,hidden_inputs=[],caller='get_file_length'if i<3 else'is_mac_resource_file',record=native_record,record_before=with_cursor(nraw,positions[i]).hex(),output=output,normal_return=True,return_stack=nstacks[i]+20,result=result,record_after=with_cursor(nraw,positions[i+1]).hex(),error_after='00000000')
        assert state['mapped_seek_calls'][i]==dict(stack_pointer=nstacks[i]-bridge,formals=nmapped,hidden_inputs=[],record_before=with_cursor(nraw,positions[i]).hex(),error_before='00000000',normal_return=True,return_stack=nstacks[i]-bridge+20,result=result,record_after=with_cursor(nraw,positions[i+1]).hex(),error_after='00000000')
    assert state['seek_file_entry']==dict(stack_pointer=nstacks[0],formals=state['seek_file_calls'][0]['formals'],hidden_inputs=[])
    assert state['seek_mapped_entry']==dict(stack_pointer=nstacks[0]-bridge,formals=state['mapped_seek_calls'][0]['formals'],hidden_inputs=[])
    first_output=state['seek_file_calls'][0]['output'].copy();first_output.pop('owner_kind')
    assert state['file_length_output']==first_output
    call,=[e for e in events if e['event']=='file_length_call'];returned,=[e for e in events if e['event']=='file_length_return']
    assert call==dict(event='file_length_call',entry_stack=length['stack_pointer'],formals=[14],hidden_inputs=[])
    assert returned==dict(event='file_length_return',entry_stack=length['stack_pointer'],formals=[14],hidden_inputs=[],return_stack=length['stack_pointer']+8,result=2262)
    assert (original/'file-length-record-after.bin').read_bytes()==with_cursor(oraw,8)
    assert len(state['file_length_returns'])==1
    assert state['file_length_returns'][0]==dict(nlength,normal_return=True,return_stack=nlength['stack_pointer']+8,result=2262,record_after=with_cursor(nraw,8).hex())
    # All error observations above were made at their genuine inner call/return.
    # get_file_length has returned: its locals are historical, never live ranges.

    if state['function']in ('byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine'):
        from big_endian_state import compare as compare_read
        compare_read(original,state,trap,events,directory,original_record,native_record)
