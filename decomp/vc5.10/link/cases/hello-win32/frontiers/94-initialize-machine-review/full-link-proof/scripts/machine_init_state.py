"""Machine initializer's first call, live BYTE and unchanged typed context."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE
from mapped_seek_state import local_offsets


def check_ownership(config):
    function,=[r for r in config['driver']if r['name']=='_initialize_machine@4']
    assert (function['start'],function['end'])==(0x449630,0x4497a8)
    literal,=[r for r in config['text_data']if r['start']<=0x4019d4<r['end']and r.get('accepted')]
    assert (literal['start'],literal['end'])==(0x4019d4,0x4019d9)
    assert literal['sha256']==hashlib.sha256(b'IX86\0').hexdigest()


@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbols={r[3]:int(r[0],16)for r in rows if len(r)==4 and r[2]=='T'}
    start=symbols['initialize_machine']
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+97),path],text=True)
    code=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert code[:6]==b'\x55\x89\xe5\x83\xec\x18'
    assert code[30:34]==b'\xc6\x45\xff\x00'
    assert code[92:97]==b'\xe8'+struct.pack('<i',symbols['validate_delay_load_options']-start-97)
    assert local_offsets(path,digest,'initialize_machine',('incremental_supported',))==dict(incremental_supported=-5)
    return 32,-5


def compare(original,state,trap,events,directory):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    check_ownership(json.loads((directory/'functions.json').read_text()))
    obj,=[e for e in events if e['event']=='object_read_entry'];context=obj['formals'][1]
    entry,=[e for e in events if e['event']=='initialize_machine_entry'];stack=entry['stack_pointer']
    assert entry==dict(event='initialize_machine_entry',stack_pointer=obj['stack_pointer']-40,formals=[context],hidden_inputs=[])
    assert trap['esp']==stack-20 and trap['stack'][:2]==[0x449669,context]
    assert trap['initialize_machine_formals']==dict(formals=[context],hidden_inputs=[])
    assert trap['validate_delay_formals']==dict(formals=[context],hidden_inputs=[])
    assert trap['frontier_inputs']==dict(formals=[context],hidden_register_inputs=[])
    receipt=json.loads((directory/'receipt.json').read_text());frame,offset=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nentry=state['initialize_machine_entry'];nstack=nentry['stack_pointer'];ncontext=state['object_reader_entry']['formals'][1]
    assert nentry['formals']==[ncontext]and nentry['hidden_inputs']==[]
    assert state['validate_delay_entry']==dict(stack_pointer=nstack-frame,formals=[ncontext],hidden_inputs=[])
    assert state['frontier_inputs']==dict(formals=[ncontext],hidden_register_inputs=[])
    assert state['incremental_support_output']==dict(address=nstack+offset,bytes=1,value=0)
    assert (original/'incremental-supported.bin').read_bytes()==b'\0'
    assert pe.read(0x44963d,5).hex()=='c644240b00' # Actual initialized entryESP-1 BYTE.
    original_before=(original/'initialize-machine-context-before.bin').read_bytes()
    assert len(original_before)==664 and original_before==(original/'link-context.bin').read_bytes()==(original/'input-machine-context-after.bin').read_bytes()
    assert state['initialize_machine_context_before']==state['link_context']==state['input_machine_returns'][0]['context_after']
    assert original_before[48:50]==bytes.fromhex('4c01')
    # Existing typed heap comparison owns every pointer role in the whole664B.
    assert state['machine_init_globals']==dict(delay_libraries=bytes(12).hex(),small_section_alignment=0,entry_symbol=0,entry_point_name=0)
    for name,size in [('delay-libraries.bin',12),('small-section-alignment.bin',1),('entry-symbol.bin',4),('entry-point-name.bin',4)]:
        assert (original/name).read_bytes()==bytes(size)
    assert (original/'object-read-mode.bin').read_bytes()==b'\0'and state['object_read_mode']['value']==0
    proof=json.loads((root/'cases/hello-win32/validate-delay-input-contract.json').read_text());body=pe.read(0x41f9b0,241)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x41f9b0))
    assert sum(i.size for i in instructions)==241
    assert [[i.address,int(i.op_str,0)]for i in instructions if i.mnemonic=='ret']==proof['returns']==[[0x41fa0d,4],[0x41fa9e,4]]
    assert proof['formals']==['LinkContext *context']and proof['hidden_inputs']==[]
    assert body[:8].hex()=='a1900848005685c0' # Reads list count; ECX is not an input.
