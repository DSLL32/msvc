"""Full DWORD helper returns, BYTE classifier result and actual object inputs."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE


@functools.lru_cache(maxsize=8)
def native_reader_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbols={r[3]:(int(r[0],16),int(r[1],16),r[2])for r in rows if len(r)==4}
    address=symbols['read_input_argument'][0]
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(address),'--stop-address='+str(address+311),path],text=True)
    raw=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert raw[:6]==b'\x55\x89\xe5\x83\xec\x28'
    assert raw[230:235]==b'\xe8'+struct.pack('<i',symbols['is_mac_resource_file'][0]-address-235)
    assert raw[235:240]==b'\x83\xec\x04\x3c\x00' # Caller consumes only AL.
    assert raw[306:311]==b'\xe8'+struct.pack('<i',symbols['read_object_file'][0]-address-311)
    mode,size,kind=symbols['machine_initialized'];assert(size,kind)==(1,'B')
    return 48,mode


def compare(original,state,trap,events,directory,entry,mac,output):
    result=int.from_bytes(output,'big')
    swap,=[e for e in events if e['event']=='byte_swap_entry']
    for event,owner in [('byte_swap_return',swap),('big_endian_return',entry)]:
        returned,=[e for e in events if e['event']==event]
        assert returned==dict(event=event,entry_stack=owner['stack_pointer'],return_stack=owner['stack_pointer']+8,result=result)
    for key,owner in [('byte_swap_returns',state['byte_swap_entry']),('big_endian_returns',state['big_endian_entry'])]:
        assert state[key]==[dict(owner,normal_return=True,return_stack=owner['stack_pointer']+8,result=result)]
    returned,=[e for e in events if e['event']=='mac_resource_return']
    assert returned==dict(event='mac_resource_return',entry_stack=mac['stack_pointer'],return_stack=mac['stack_pointer']+8,raw_result=returned['raw_result'],result=0)
    nmac=state['mac_resource_entry'];nreturn,=state['mac_resource_returns']
    assert nreturn==dict(nmac,normal_return=True,return_stack=nmac['stack_pointer']+8,raw_result=nreturn['raw_result'],result=0)
    for raw in(returned['raw_result'],nreturn['raw_result']):assert 0<=raw<=0xffffffff and raw&255==0
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    assert pe.read(0x449949,9).hex()=='e8d24c000084c07419' # CALL classifier; TEST AL; JE.
    receipt=json.loads((directory/'receipt.json').read_text())
    for filename in('input.c','input.h'):
        source=(directory/'source/project'/filename).read_bytes()
        assert hashlib.sha256(source).hexdigest()==receipt['source_files']['src/'+filename]
        assert b'BYTE __stdcall is_mac_resource_file('in source
    proof=json.loads((root/'cases/hello-win32/object-reader-input-contract.json').read_text());body=pe.read(0x4497e0,240)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x4497e0))
    assert sum(i.size for i in instructions)==240
    assert [[i.address,int(i.op_str,0)]for i in instructions if i.mnemonic=='ret']==proof['returns']==[[0x449895,12],[0x4498cd,12]]
    assert proof['formals']==['InputArgument *argument','LinkContext *context','OutputLibrary *library']and proof['hidden_inputs']==[]
    reader,=[e for e in events if e['event']=='read_input_entry'];args=reader['formals']
    formals=[args[0],args[2],args[1]]
    if state['function']=='read_object_file':
        assert trap['esp']==reader['stack_pointer']-28 and trap['stack'][:4]==[0x44997b]+formals
        assert trap['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    else:
        object_entry,=[e for e in events if e['event']=='object_read_entry']
        assert object_entry==dict(event='object_read_entry',stack_pointer=reader['stack_pointer']-28,formals=formals,hidden_inputs=[])
    assert trap['object_read_formals']==dict(formals=formals,hidden_inputs=[])
    frame,mode=native_reader_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nreader=state['read_input_entry'];args=nreader['formals'];nformals=[args[0],args[2],args[1]]
    assert state['object_reader_entry']==dict(stack_pointer=nreader['stack_pointer']-frame,formals=nformals,hidden_inputs=[])
    if state['function']=='read_object_file':assert state['frontier_inputs']==dict(formals=nformals,hidden_register_inputs=[])
    # Existing complete argument/library/context contracts own all three live
    # pointees; historical classifier/read locals are no longer live ranges.
    data,=[s for s in pe.sections if s['name']=='.data']
    assert pe.base+data['rva']+data['size']<=0x47f030<pe.base+data['rva']+data['virtual_size']
    assert pe.read(0x4497f5,5).hex()=='a030f04700'
    assert (original/('object-read-mode-entry.bin'if state['function']in ('read_coff_header','create_input_module','OutputLibrary::FindModule','initialize_module_enumerator','next_module','initialize_module_enumerator','next_module')else 'object-read-mode.bin')).read_bytes()==bytes(1)
    assert state['object_read_mode']==dict(address=mode,bytes=1,value=0)

    if state['function']in ('check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule','initialize_module_enumerator','next_module','initialize_module_enumerator','next_module'):
        from input_machine_state import compare as compare_machine
        compare_machine(original,state,trap,events,directory)
