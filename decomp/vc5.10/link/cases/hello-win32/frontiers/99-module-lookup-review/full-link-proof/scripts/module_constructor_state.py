"""Constructor's reached header reads and genuine library thiscall boundary."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE

@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    symbols={r[3]:int(r[0],16)for r in[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]if len(r)==4 and r[2]=='T'}
    start=symbols['create_input_module'];listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+103),path],text=True)
    body=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert body[:6].hex()=='5589e583ec38'
    assert body[98:103]==b'\xe8'+struct.pack('<i',symbols['_ZN13OutputLibrary10FindModuleEPKcPKv']-start-103)
    # Actual loads from constructor args5/2/3: ECXreceiver +name/identity, no
    # wrapper or synthesized logical method target.
    assert body[82:98].hex()=='8b4d188b550c8b451089142489442404'
    return 64

def compare(original,state,trap,events,directory,formals,nformals,header):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    config=json.loads((directory/'functions.json').read_text())
    creator,=[r for r in config['driver']if r['name']=='_create_input_module@48']
    assert (creator['start'],creator['end'])==(0x40ea00,0x40edcc)
    method,=[r for r in config['driver']if r['name']=='OutputLibrary::FindModule']
    child=state['function']=='initialize_module_enumerator'
    assert method['start']==0x40f7a0
    if child:assert not method['pending_extent']and method['end']==0x40f82c
    else:assert method['pending_extent']
    original_entry,=[e for e in events if e['event']=='input_module_entry']
    entry=original_entry['stack_pointer'];expected=dict(this=formals[4],formals=[formals[1],formals[2]],other_hidden_inputs=[])
    if child:
        seen,=[e for e in events if e['event']=='module_find_entry']
        assert seen==dict(event='module_find_entry',stack_pointer=entry-32,**expected)
    else:
        assert trap['esp']==entry-32 and trap['stack'][:3]==[0x40ea29]+expected['formals']
        assert trap['ecx']==formals[4]
        assert trap['frontier_inputs']==dict(formals=expected['formals'],hidden_register_inputs=[dict(register='ecx',value=formals[4])])
    assert trap['module_find_formals']==expected
    assert (original/'module-find-header.bin').read_bytes()==header
    assert (original/'module-find-created.bin').read_bytes()==(original/'input-module-created-before.bin').read_bytes()
    assert (original/'module-mac-target.bin').read_bytes()==(original/'module-native-ppc.bin').read_bytes()==b'\0'
    # Complete original method establishes incomingECX and both stack inputs;
    # EAX/EDX/ESI/EDI/EBP/EBX are overwritten before semantic use, flags defined.
    proof=json.loads((root/'cases/hello-win32/find-module-input-contract.json').read_text());body=pe.read(0x40f7a0,140)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    assert proof['physical_stack_slots']==2 and proof['receiver']=='ecx'and proof['other_hidden_inputs']==[]
    assert pe.read(0x40ea00,41).hex()=='518b4c24105355568b742420578b7c241c33db8b460c668b5e02518b4c242c5789442418e8770d0000'
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x40f7a0));assert sum(i.size for i in instructions)==140
    assert [[i.address,int(i.op_str,0)]for i in instructions if i.mnemonic=='ret']==[[0x40f80f,8],[0x40f829,8]]
    receipt=json.loads((directory/'receipt.json').read_text());frame=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nentry=state['input_module_entry']['stack_pointer']
    nmethod=dict(stack_pointer=nentry-frame,this=nformals[4],formals=[nformals[1],nformals[2]],other_hidden_inputs=[])
    assert state['module_find_entry']==nmethod
    if not child:assert state['frontier_inputs']==dict(formals=nmethod['formals'],hidden_register_inputs=[dict(register='ecx',value=nmethod['this'])])
    assert state['module_find_state']==dict(header=header.hex(),created=state['input_module_created_output']['before'],mac_target=0,native_ppc_symbol_maps=0)
    # This lookup receives neither the caller's header nor created-output pointer;
    # both stay live in outer frames but unchanged before its first normal return.

    if child:
        from module_enumerator_state import compare as compare_enumerator
        compare_enumerator(original,state,trap,events,directory,entry-32,nmethod)
