"""Backend completion, genuine callback identities and new COFF output owner."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE
from mapped_seek_state import local_offsets,with_cursor


def check_ownership(config):
    record,=[r for r in config['driver']if r['name']=='_initialize_x86_backend@8']
    assert (record['start'],record['end'])==(0x431890,0x431915)
    literal,=[r for r in config['text_data']if r.get('accepted')and r['start']<=0x401430<r['end']]
    assert (literal['start'],literal['end'])==(0x401430,0x401437)
    assert literal['sha256']==hashlib.sha256(b'GPSIZE\0').hexdigest()


@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[r.split()for r in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbols={r[3]:int(r[0],16)for r in rows if len(r)==4 and r[2]=='T'}
    start=symbols['read_object_file'];listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+170),path],text=True)
    body=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert body[:8].hex()=='5589e5575683ec60'
    for offset,end,name in[(123,128,'seek_file'),(165,170,'read_coff_header')]:
        assert body[offset:end]==b'\xe8'+struct.pack('<i',symbols[name]-start-end)
    assert body[38:45].hex()=='c745d800000000'
    offsets=local_offsets(path,digest,'read_object_file',('error','header'))
    assert offsets==dict(error=-44,header=-36)
    return symbols,112,offsets


def compare(original,state,trap,events,directory):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    obj,=[e for e in events if e['event']=='object_read_entry'];stack=obj['stack_pointer'];context=obj['formals'][1]
    init,=[e for e in events if e['event']=='initialize_machine_entry'];backend,=[e for e in events if e['event']=='x86_backend_entry']
    returned,=[e for e in events if e['event']=='x86_backend_return'];done,=[e for e in events if e['event']=='initialize_machine_return']
    assert returned==dict(event='x86_backend_return',entry_stack=backend['stack_pointer'],return_stack=backend['stack_pointer']+12,raw_result=returned['raw_result'])
    assert done==dict(event='initialize_machine_return',entry_stack=init['stack_pointer'],return_stack=init['stack_pointer']+8,raw_result=done['raw_result'])
    assert (original/'x86-support-before.bin').read_bytes()==b'\0'and(original/'x86-support-after.bin').read_bytes()==b'\1'
    current=(original/'link-context.bin').read_bytes()
    assert current==(original/'x86-context-after.bin').read_bytes()==(original/'initialize-machine-context-after.bin').read_bytes()==(original/'initialize-machine-context-before.bin').read_bytes()
    assert struct.unpack_from('<II',current,0x64)==(4096,512)
    assert (original/'machine-initialized-after.bin').read_bytes()==(original/'object-read-mode.bin').read_bytes()==b'\1'
    assert (original/'object-read-mode-entry.bin').read_bytes()==b'\0'and state['object_read_mode']['value']==0
    receipt=json.loads((directory/'receipt.json').read_text());symbols,frame,offsets=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nobj=state['object_reader_entry'];nstack=nobj['stack_pointer'];ncontext=nobj['formals'][1]
    nbackend=state['x86_backend_entry'];nreturned,=state['x86_backend_returns'];ninit=state['initialize_machine_entry'];ndone,=state['initialize_machine_returns']
    output=dict(state['x86_incremental_support_output'],after='01')
    callbacks=dict(process_relocations=symbols['process_x86_relocations'],classify_relocation=symbols['classify_x86_relocation'])
    assert nreturned==dict(nbackend,normal_return=True,return_stack=nbackend['stack_pointer']+12,raw_result=nreturned['raw_result'],context_after=state['link_context'],output=output,**callbacks)
    assert ndone==dict(ninit,normal_return=True,return_stack=ninit['stack_pointer']+8,raw_result=ndone['raw_result'],context_after=state['link_context'],machine_initialized=1)
    for value in(returned['raw_result'],done['raw_result'],nreturned['raw_result'],ndone['raw_result']):assert 0<=value<=0xffffffff
    assert state['current_machine_state']==dict(machine_initialized=1,small_section_alignment=0,**callbacks)
    assert state['relocation_callbacks_before']==dict(process_relocations=0,classify_relocation=0)
    for filename,address in [('relocation-processor',0x4312b0),('relocation-classifier',0x431230)]:
        assert (original/(filename+'-before.bin')).read_bytes()==bytes(4)
        assert (original/(filename+'-after.bin')).read_bytes()==(original/(filename+'.bin')).read_bytes()==struct.pack('<I',address)
    config=json.loads((directory/'functions.json').read_text())
    check_ownership(config)
    for name,address in [('_process_x86_relocations@28',0x4312b0),('_classify_x86_relocation@16',0x431230)]:
        record,=[r for r in config['driver']if r['name']==name]
        assert (record['start'],record['end'],record['status'],record['pending_extent'])==(address,address+1,'stub',True)
        assert name not in config['accepted']and name not in state['entries']
    # Sixth seek: the original reuses its incoming context-argument slot after
    # saving that context in EBX. Its entry formal remains historical evidence.
    assert pe.read(0x4497e4,4).hex()=='8b5c2420'
    assert pe.read(0x449813,8).hex()=='c744243800000000'
    error=stack+8;nerror=nstack+offsets['error'];record=struct.unpack('<16I',(original/'file-slots.bin').read_bytes())[14]
    nrecord=struct.unpack('<16I',bytes.fromhex(state['file_slots_bytes']))[14]
    call,=[e for e in events if e['event']=='seek_call'and e['index']==5];ret,=[e for e in events if e['event']=='seek_return'and e['index']==5]
    assert call==dict(event='seek_call',index=5,site=0x44981b,entry_stack=stack-52,formals=[14,0,0,error],hidden_inputs=[],record=record)
    assert ret==dict(event='seek_return',index=5,site=0x449820,entry_stack=stack-52,formals=[14,0,0,error],hidden_inputs=[],record=record,return_stack=stack-32,result=0)
    inner,=[e for e in events if e['event']=='mapped_seek_call'and e['index']==5];iret,=[e for e in events if e['event']=='mapped_seek_return'and e['index']==5]
    assert inner==dict(event='mapped_seek_call',index=5,site=0x40c061,entry_stack=stack-72,formals=[record,0,0,error],hidden_inputs=[])
    assert iret==dict(event='mapped_seek_return',index=5,site=0x40c066,entry_stack=stack-72,formals=[record,0,0,error],hidden_inputs=[],return_stack=stack-52,result=0)
    raw=with_cursor((original/'file-record-14.bin').read_bytes(),0);nraw=with_cursor(bytes.fromhex(state['file_records'][14]),0)
    for prefix in('seek','mapped-seek'):
        for phase,cursor in [('before',4),('after',0)]:
            assert (original/('%s-5-record-%s.bin'%(prefix,phase))).read_bytes()==with_cursor(raw,cursor)
            assert (original/('%s-5-error-%s.bin'%(prefix,phase))).read_bytes()==bytes(4)
    noutput=dict(address=nerror,bytes=4,owner_local_address=nerror,owner_local_bytes=4,before='00000000',initial_value_contract='initialized DWORD error scratch; no incoming-value assumption at pending seek',owner_kind='read_object_file.error')
    assert state['seek_file_calls'][5]==dict(stack_pointer=nstack-frame,formals=[14,0,0,nerror],hidden_inputs=[],caller='read_object_file',record=nrecord,record_before=with_cursor(nraw,4).hex(),output=noutput,normal_return=True,return_stack=nstack-frame+20,result=0,record_after=nraw.hex(),error_after='00000000')
    assert state['mapped_seek_calls'][5]==dict(stack_pointer=nstack-frame-48,formals=[nrecord,0,0,nerror],hidden_inputs=[],record_before=with_cursor(nraw,4).hex(),error_before='00000000',normal_return=True,return_stack=nstack-frame-28,result=0,record_after=nraw.hex(),error_after='00000000')
    header=stack-20;nheader=nstack+offsets['header']
    if state['function']=='read_coff_header':
        assert trap['esp']==stack-52 and trap['stack'][:5]==[0x449834,14,header,0,0]
        assert trap['stack'][15]==0 and context # Former contextargument slot now errorDWORD.
    assert trap['coff_header_formals']==dict(formals=[14,header,0,0],hidden_inputs=[])
    if state['function']=='read_coff_header':assert trap['frontier_inputs']==dict(formals=[14,header,0,0],hidden_register_inputs=[])
    assert len((original/'coff-header-before.bin').read_bytes())==20
    assert state['coff_header_entry']==dict(stack_pointer=nstack-frame,formals=[14,nheader,0,0],hidden_inputs=[])
    if state['function']=='read_coff_header':assert state['frontier_inputs']==dict(formals=[14,nheader,0,0],hidden_register_inputs=[])
    out=state['coff_header_output'];assert out==dict(address=nheader,bytes=20,owner_local_address=nheader,owner_local_bytes=20,before=out['before'],initial_value_contract='uninitialized IMAGE_FILE_HEADER output; incoming bytes not compared')
    assert len(bytes.fromhex(out['before']))==20 and nerror+4<=nheader
    proof=json.loads((root/'cases/hello-win32/coff-header-input-contract.json').read_text());body=pe.read(0x4610c0,273)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    assert proof['formals']==['int file','IMAGE_FILE_HEADER *header','void *alternate_header','BYTE *alternate']and proof['hidden_inputs']==[]and proof['returns']==[[0x4611ce,16]]
    # The initializer's returned BYTE owner is historical, not a current range.

    if state['function']in ('create_input_module','OutputLibrary::FindModule','initialize_module_enumerator','next_module','allocate_module_index','allocate_module_section_headers'):
        from coff_reader_state import compare as compare_coff
        compare_coff(original,state,trap,events,directory)
