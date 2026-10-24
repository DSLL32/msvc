"""Fresh pointer-formal boundary before machine detection does any work."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE


@functools.lru_cache(maxsize=8)
def native_caller(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbols={r[3]:int(r[0],16)for r in rows if len(r)==4 and r[2]=='T'}
    address=symbols['check_input_machine']
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(address),'--stop-address='+str(address+51),path],text=True)
    raw=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert raw[:6]==b'\x55\x89\xe5\x83\xec\x18'
    assert raw[46:51]==b'\xe8'+struct.pack('<i',symbols['detect_input_machine']-address-51)
    return 4+24+4


def compare(original,state,trap,events,directory):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    proof=json.loads((root/'cases/hello-win32/detect-machine-input-contract.json').read_text());body=pe.read(0x4103f0,306)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    decoded=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x4103f0))
    assert sum(i.size for i in decoded)==306
    assert [[i.address,int(i.op_str,0)]for i in decoded if i.mnemonic=='ret']==proof['returns']==[[0x41046d,8],[0x410491,8],[0x41051f,8]]
    assert proof['formals']==['LinkContext *context','InputArgument *argument']and proof['hidden_inputs']==[]
    entry,=[e for e in events if e['event']=='input_machine_entry'];formals=entry['formals']
    if state['function']=='detect_input_machine':
        assert trap['esp']==entry['stack_pointer']-20 and trap['stack'][:3]==[0x410541]+formals
        assert trap['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    else:
        detected,=[e for e in events if e['event']=='detect_machine_entry']
        assert detected==dict(event='detect_machine_entry',stack_pointer=entry['stack_pointer']-20,formals=formals,hidden_inputs=[])
    assert trap['detect_machine_formals']==dict(formals=formals,hidden_inputs=[])
    receipt=json.loads((directory/'receipt.json').read_text());frame=native_caller(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    native=state['input_machine_entry'];formals=native['formals']
    assert state['detect_machine_entry']==dict(stack_pointer=native['stack_pointer']-frame,formals=formals,hidden_inputs=[])
    if state['function']=='detect_input_machine':assert state['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    # Both pointees are already owned completely by the inherited typed
    # context/argument contracts. No pending detector body or output credit.

    if state['function']in ('initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule','initialize_module_enumerator','next_module','initialize_module_enumerator','next_module'):
        from machine_result_state import compare as compare_result
        compare_result(original,state,trap,events,directory)
