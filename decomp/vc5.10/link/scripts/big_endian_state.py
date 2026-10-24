"""One mapped DWORD output and its actual full-width byte-swap input."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE
from mapped_seek_state import with_cursor


@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    from mapped_seek_state import local_offsets
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbols={r[3]:int(r[0],16)for r in rows if len(r)==4 and r[2]=='T'}
    address=symbols['read_big_endian_dword']
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(address),'--stop-address='+str(address+112),path],text=True)
    raw=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert raw[:6]==b'\x55\x89\xe5\x83\xec\x28'
    assert raw[93:98]==b'\xe8'+struct.pack('<i',symbols['mapped_read']-address-98)
    assert raw[107:112]==b'\xe8'+struct.pack('<i',symbols['byte_swap_dword']-address-112)
    offsets=local_offsets(path,digest,'read_big_endian_dword',('value',))
    assert offsets==dict(value=-12)
    return offsets['value'],4+40+4


def compare(original,state,trap,events,directory,original_record,native_record):
    from mapped_seek_state import with_cursor
    binary=Path(__file__).resolve().parents[1];pe=PE(binary.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    proof=json.loads((binary/'cases/hello-win32/swap-dword-input-contract.json').read_text())
    body=pe.read(0x45d890,37)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    decoded=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x45d890))
    assert sum(i.size for i in decoded)==37
    assert [[i.address,int(i.op_str,0)]for i in decoded if i.mnemonic=='ret']==proof['returns']==[[0x45d8b2,4]]
    assert proof['formals']==['DWORD value']and proof['hidden_inputs']==[]
    entry,=[e for e in events if e['event']=='big_endian_entry']
    mac,=[e for e in events if e['event']=='mac_resource_entry']
    assert entry==dict(event='big_endian_entry',stack_pointer=mac['stack_pointer']-24,formals=[14],hidden_inputs=[])
    local=entry['stack_pointer']-4;formals=[original_record,local,4]
    call,=[e for e in events if e['event']=='big_endian_read_call']
    returned,=[e for e in events if e['event']=='big_endian_read_return']
    assert call==dict(event='big_endian_read_call',index=0,entry_stack=entry['stack_pointer']-20,formals=formals,hidden_inputs=[])
    assert returned==dict(event='big_endian_read_return',index=0,entry_stack=call['entry_stack'],return_stack=call['entry_stack']+16,result=4)
    assert len((original/'big-endian-read-0-before.bin').read_bytes())==4 # Own uninitialized bytes, not compared across sides.
    master=json.loads((original/'case.json').read_text())['inputs'][0];data=(binary/master['path']).read_bytes()
    assert len(data)==master['bytes']==2262 and hashlib.sha256(data).hexdigest()==master['sha256']
    output=data[:4];value=struct.unpack('<I',output)[0]
    assert (original/'big-endian-read-0-after.bin').read_bytes()==output
    assert (original/'big-endian-read-0-record-after.bin').read_bytes()==with_cursor((original/'file-record-14.bin').read_bytes(),4)
    if state['function']=='byte_swap_dword':
        assert trap['esp']==entry['stack_pointer']-12 and trap['stack'][:2]==[0x44e5f9,value]
        assert trap['swap_dword_formals']==dict(formals=[value],hidden_inputs=[])
        assert trap['frontier_inputs']==state['frontier_inputs']==dict(formals=[value],hidden_register_inputs=[])
    else:
        swap,=[e for e in events if e['event']=='byte_swap_entry']
        assert swap==dict(event='byte_swap_entry',stack_pointer=entry['stack_pointer']-12,formals=[value],hidden_inputs=[])
    receipt=json.loads((directory/'receipt.json').read_text());offset,frame=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nentry=state['big_endian_entry'];nlocal=nentry['stack_pointer']+offset
    nread=state['big_endian_read_entry'];before=state['big_endian_output']
    assert nread==dict(stack_pointer=nentry['stack_pointer']-frame,formals=[native_record,nlocal,4],hidden_inputs=[])
    assert before==dict(address=nlocal,bytes=4,owner_local_address=nlocal,owner_local_bytes=4,before=before['before'],initial_value_contract='uninitialized DWORD output; no incoming bytes consumed')
    assert len(bytes.fromhex(before['before']))==4
    assert len(state['big_endian_read_returns'])==1
    assert state['big_endian_read_returns'][0]==dict(nread,normal_return=True,return_stack=nread['stack_pointer']+16,result=4,output=dict(before,after=output.hex(),record_after=with_cursor(bytes.fromhex(state['file_records'][14]),4).hex()))
    assert state['byte_swap_entry']==dict(stack_pointer=nentry['stack_pointer']-frame,formals=[value],hidden_inputs=[])
    assert struct.unpack_from('<I',bytes.fromhex(state['file_records'][14]),28)[0]==(0 if state['function']=='read_coff_header'else 4)
    # The DWORD is passed by value to the current pending helper. Earlier output
    # locals remain return-time history, with no extra live pointer input range.

    if state['function']in ('read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header'):
        from swap_result_state import compare as compare_result
        compare_result(original,state,trap,events,directory,entry,mac,output)
