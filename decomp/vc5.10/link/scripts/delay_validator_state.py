"""Normal void validator return and the next backend's live BYTE output."""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE
from machine_init_state import native_contract


def check_ownership(config,pe):
    record,=[r for r in config['driver']if r['name']=='_validate_delay_load_options@4']
    assert (record['start'],record['end'])==(0x41f9b0,0x41faa1)
    for address in (0x403414,0x40340c,0x40142c,0x402954,0x4033fc):
        text=pe.string(address).encode()+b'\0'
        item,=[r for r in config['text_data']if r.get('accepted')and r['start']<=address<r['end']]
        assert (item['start'],item['end'])==(address,address+len(text))
        assert item['sha256']==hashlib.sha256(text).hexdigest()


def compare(original,state,trap,events,directory):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    check_ownership(json.loads((directory/'functions.json').read_text()),pe)
    init,=[e for e in events if e['event']=='initialize_machine_entry'];stack=init['stack_pointer'];context=init['formals'][0]
    entry,=[e for e in events if e['event']=='validate_delay_entry'];returned,=[e for e in events if e['event']=='validate_delay_return']
    assert returned==dict(event='validate_delay_return',entry_stack=entry['stack_pointer'],return_stack=entry['stack_pointer']+8,raw_result=returned['raw_result'])
    before=(original/'initialize-machine-context-before.bin').read_bytes()
    assert (original/'validate-delay-context-after.bin').read_bytes()==before==(original/'link-context.bin').read_bytes()
    assert (original/'validate-delay-support-after.bin').read_bytes()==b'\0'
    nentry=state['validate_delay_entry'];nreturned,=state['validate_delay_returns']
    assert nreturned==dict(nentry,normal_return=True,return_stack=nentry['stack_pointer']+8,raw_result=nreturned['raw_result'],context_after=state['link_context'])
    for value in(returned['raw_result'],nreturned['raw_result']):assert 0<=value<=0xffffffff # Void: retain each raw EAX only.
    if state['function']=='initialize_x86_backend':
        assert trap['esp']==stack-24 and trap['stack'][:3]==[0x44968e,context,stack-1]
        assert trap['frontier_inputs']==dict(formals=[context,stack-1],hidden_register_inputs=[])
    else:
        backend,=[e for e in events if e['event']=='x86_backend_entry']
        assert backend==dict(event='x86_backend_entry',stack_pointer=stack-24,formals=[context,stack-1],hidden_inputs=[])
    assert trap['x86_backend_formals']==dict(formals=[context,stack-1],hidden_inputs=[])
    ninit=state['initialize_machine_entry'];nstack=ninit['stack_pointer'];ncontext=ninit['formals'][0]
    receipt=json.loads((directory/'receipt.json').read_text());elf=str(directory/'compiled/native-LINK.EXE');frame,offset=native_contract(elf,receipt['native_sha256'])
    formals=[ncontext,nstack+offset]
    assert state['x86_backend_entry']==dict(stack_pointer=nstack-frame,formals=formals,hidden_inputs=[])
    if state['function']=='initialize_x86_backend':assert state['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    assert state['x86_incremental_support_output']==dict(address=nstack+offset,owner_local_address=nstack+offset,bytes=1,owner_local_bytes=1,before='00')
    # Bind the actual C caller's address-of-BYTE and call target in its exact ELF.
    rows=[r.split()for r in subprocess.check_output(['nm','-S','--defined-only',elf],text=True).splitlines()]
    symbols={r[3]:int(r[0],16)for r in rows if len(r)==4 and r[2]=='T'}
    start=symbols['initialize_machine'];listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+248),elf],text=True)
    raw=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert raw[233:236]==bytes.fromhex('8d45ff')
    assert raw[243:248]==b'\xe8'+struct.pack('<i',symbols['initialize_x86_backend']-start-248)
    proof=json.loads((root/'cases/hello-win32/x86-backend-input-contract.json').read_text());body=pe.read(0x431890,133)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x431890))
    assert sum(i.size for i in instructions)==133
    assert [[i.address,int(i.op_str,0)]for i in instructions if i.mnemonic=='ret']==proof['returns']==[[0x431912,8]]
    assert proof['formals']==['LinkContext *context','BYTE *incremental_supported']and proof['hidden_inputs']==[]
    assert proof['output']==dict(pointer_formal=1,bytes=1,store=dict(address=0x4318e3,raw='c60201'))
    assert pe.read(0x449683,6).hex()=='8d4c240b5156' # The original initialized BYTE's address, then context.

    if state['function']in ('read_coff_header','create_input_module','OutputLibrary::FindModule','initialize_module_enumerator','initialize_module_enumerator'):
        from x86_backend_state import compare as compare_backend
        compare_backend(original,state,trap,events,directory)
