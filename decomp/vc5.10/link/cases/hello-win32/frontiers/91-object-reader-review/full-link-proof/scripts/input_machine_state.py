"""Actual two-pointer machine-check frontier and encountered layout roles."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE


@functools.lru_cache(maxsize=8)
def caller_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbols={r[3]:int(r[0],16)for r in rows if len(r)==4 and r[2]=='T'}
    address=symbols['read_object_file']
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(address),'--stop-address='+str(address+63),path],text=True)
    raw=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert raw[:8]==b'\x55\x89\xe5\x57\x56\x83\xec\x60'
    assert raw[58:63]==b'\xe8'+struct.pack('<i',symbols['check_input_machine']-address-63)
    return 4+8+96+4


def compare(original,state,trap,events,directory):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    proof=json.loads((root/'cases/hello-win32/input-machine-input-contract.json').read_text())
    body=pe.read(0x410530,71);assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    decoded=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x410530))
    assert sum(i.size for i in decoded)==71
    assert [[i.address,int(i.op_str,0)]for i in decoded if i.mnemonic=='ret']==proof['returns']==[[0x410564,8],[0x410574,8]]
    assert proof['formals']==['LinkContext *context','InputArgument *argument']and proof['hidden_inputs']==[]
    # The initial child's first descendant establishes ECX before its first
    # call; the observed caller ECX is not a hidden receiver of this boundary.
    # Explicit bytes stop before the relative call operand; keep actual roles.
    assert pe.read(0x410530,12).hex()=='568b742408578b7c24105756'
    assert pe.read(0x4103f0,12).hex()=='83ec0ca1f8ee47008d4c2408'
    entry,=[e for e in events if e['event']=='object_read_entry'];args=entry['formals'];formals=[args[1],args[0]]
    assert trap['esp']==entry['stack_pointer']-44 and trap['stack'][:3]==[0x4497f5]+formals
    assert trap['input_machine_formals']==dict(formals=formals,hidden_inputs=[])
    assert trap['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    receipt=json.loads((directory/'receipt.json').read_text());frame=caller_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nentry=state['object_reader_entry'];args=nentry['formals'];nformals=[args[1],args[0]]
    assert state['input_machine_entry']==dict(stack_pointer=nentry['stack_pointer']-frame,formals=nformals,hidden_inputs=[])
    assert state['frontier_inputs']==dict(formals=nformals,hidden_register_inputs=[])
    mode=proof['mode_refinement'];assert pe.read(0x44979c,7).hex()==mode['store']['raw']=='c60530f0470001'
    section,=[s for s in pe.sections if s['name']=='.text'];raw=pe.read(pe.base+section['rva'],section['virtual_size']);needle=struct.pack('<I',0x47f030)
    assert [pe.base+section['rva']+i for i in range(len(raw))if raw.startswith(needle,i)]==mode['all_literal_address_references']==[0x44979e,0x4497f6]
    # Name refinement and field splitting change no existing record layout.
    header=(directory/'source/project/input.h').read_text()
    for name,offset in [('input_tag','34'),('timestamp','38'),('file_size','3c')]:
        assert 'offsetof(OutputModule,%s)==0x%s'%(name,offset)in header
    assert proof['module_layout']==dict(bytes=156,unchanged_name=4,unchanged_library=100,new_fields=dict(input_tag=52,timestamp=56,file_size=60))
