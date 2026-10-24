"""A fifth seek, independent signature output, WORD result and context store."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE
from mapped_seek_state import local_offsets,with_cursor


@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbols={r[3]:int(r[0],16)for r in rows if len(r)==4 and r[2]=='T'}
    def raw(name,size):
        start=symbols[name]
        listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+size),path],text=True)
        return b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    code=raw('detect_input_machine',153);start=symbols['detect_input_machine']
    assert code[:6]==b'\x55\x89\xe5\x83\xec\x38'
    for offset,end,target in[(74,79,'seek_file'),(148,153,'mapped_read')]:
        assert code[offset:end]==b'\xe8'+struct.pack('<i',symbols[target]-start-end)
    offsets=local_offsets(path,digest,'detect_input_machine',('error','signature'))
    assert offsets==dict(error=-16,signature=-12)
    parent=raw('check_input_machine',103)
    assert parent[54:63]==b'\x66\x89\x45\xfe\x66\x83\x7d\xfe\x00'
    assert parent[99:103]==b'\x66\x89\x48\x30' # Actual WORD store, no DWORD widening.
    caller=raw('read_object_file',86);start=symbols['read_object_file']
    assert caller[:8]==b'\x55\x89\xe5\x57\x56\x83\xec\x60'
    assert caller[81:86]==b'\xe8'+struct.pack('<i',symbols['initialize_machine']-start-86)
    return offsets,64,112


def compare(original,state,trap,events,directory):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    entry,=[e for e in events if e['event']=='detect_machine_entry'];stack=entry['stack_pointer']
    record=struct.unpack('<16I',(original/'file-slots.bin').read_bytes())[14]
    nrecord=struct.unpack('<16I',bytes.fromhex(state['file_slots_bytes']))[14]
    current=(original/'file-record-14.bin').read_bytes();ncurrent=bytes.fromhex(state['file_records'][14])
    receipt=json.loads((directory/'receipt.json').read_text());offsets,frame,parent_frame=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nentry=state['detect_machine_entry'];nstack=nentry['stack_pointer'];error=stack-4;nerror=nstack+offsets['error']
    formals=[14,0,0,error];nformals=[14,0,0,nerror]
    call,=[e for e in events if e['event']=='seek_call'and e['index']==4]
    returned,=[e for e in events if e['event']=='seek_return'and e['index']==4]
    assert call==dict(event='seek_call',index=4,site=0x41040b,entry_stack=stack-36,formals=formals,hidden_inputs=[],record=record)
    assert returned==dict(event='seek_return',index=4,site=0x410410,entry_stack=stack-36,formals=formals,hidden_inputs=[],record=record,return_stack=stack-16,result=0)
    inner,=[e for e in events if e['event']=='mapped_seek_call'and e['index']==4]
    inner_return,=[e for e in events if e['event']=='mapped_seek_return'and e['index']==4]
    assert inner==dict(event='mapped_seek_call',index=4,site=0x40c061,entry_stack=stack-56,formals=[record,0,0,error],hidden_inputs=[])
    assert inner_return==dict(event='mapped_seek_return',index=4,site=0x40c066,entry_stack=stack-56,formals=[record,0,0,error],hidden_inputs=[],return_stack=stack-36,result=0)
    for prefix in('seek','mapped-seek'):
        for phase,cursor in[('before',4),('after',0)]:
            assert (original/('%s-4-record-%s.bin'%(prefix,phase))).read_bytes()==with_cursor(current,cursor)
            assert (original/('%s-4-error-%s.bin'%(prefix,phase))).read_bytes()==bytes(4)
    output=dict(address=nerror,bytes=4,owner_local_address=nerror,owner_local_bytes=4,before='00000000',initial_value_contract='initialized DWORD error scratch; no incoming-value assumption at pending seek',owner_kind='detect_input_machine.error')
    assert state['seek_file_calls'][4]==dict(stack_pointer=nstack-frame,formals=nformals,hidden_inputs=[],caller='detect_input_machine',record=nrecord,record_before=with_cursor(ncurrent,4).hex(),output=output,normal_return=True,return_stack=nstack-frame+20,result=0,record_after=with_cursor(ncurrent,0).hex(),error_after='00000000')
    assert state['mapped_seek_calls'][4]==dict(stack_pointer=nstack-frame-48,formals=[nrecord,0,0,nerror],hidden_inputs=[],record_before=with_cursor(ncurrent,4).hex(),error_before='00000000',normal_return=True,return_stack=nstack-frame-28,result=0,record_after=with_cursor(ncurrent,0).hex(),error_after='00000000')
    read,=[e for e in events if e['event']=='detect_read_call'];finished,=[e for e in events if e['event']=='detect_read_return']
    assert read==dict(event='detect_read_call',index=0,entry_stack=stack-32,formals=[record,stack-8,4],hidden_inputs=[])
    assert finished==dict(event='detect_read_return',index=0,entry_stack=stack-32,return_stack=stack-16,result=4)
    assert len((original/'detect-machine-read-0-before.bin').read_bytes())==4
    case=json.loads((original/'case.json').read_text());master=case['inputs'][0];data=(root/master['path']).read_bytes()
    assert len(data)==master['bytes']==2262 and hashlib.sha256(data).hexdigest()==master['sha256']
    signature=data[:4];machine=int.from_bytes(signature[:2],'little');assert machine==0x14c
    assert (original/'detect-machine-read-0-after.bin').read_bytes()==signature
    assert (original/'detect-machine-read-0-record-after.bin').read_bytes()==current
    assert struct.unpack_from('<I',current,28)[0]==struct.unpack_from('<I',ncurrent,28)[0]==4
    nread=state['detect_read_entry'];before=state['detect_output'];local=nstack+offsets['signature']
    assert nread==dict(stack_pointer=nstack-frame,formals=[nrecord,local,4],hidden_inputs=[])
    assert before==dict(address=local,bytes=4,owner_local_address=local,owner_local_bytes=4,before=before['before'],initial_value_contract='uninitialized DWORD signature output; no incoming bytes consumed')
    assert len(bytes.fromhex(before['before']))==4 and local==nerror+4
    assert state['detect_read_returns']==[dict(nread,normal_return=True,return_stack=nread['stack_pointer']+16,result=4,output=dict(before,after=signature.hex(),record_after=ncurrent.hex()))]
    word_return,=[e for e in events if e['event']=='detect_machine_return'];nword,=state['detect_machine_returns']
    assert word_return==dict(event='detect_machine_return',entry_stack=stack,return_stack=stack+12,raw_result=word_return['raw_result'],result=machine)
    assert nword==dict(nentry,normal_return=True,return_stack=nstack+12,raw_result=nword['raw_result'],result=machine)
    for value in(word_return['raw_result'],nword['raw_result']):assert 0<=value<=0xffffffff and value&0xffff==machine
    assert pe.read(0x410541,3).hex()=='6685c0'and pe.read(0x41055e,4).hex()=='66894630'
    for filename in('input.h','input-machine.cpp'):
        source=(directory/'source/project'/filename).read_bytes();assert hashlib.sha256(source).hexdigest()==receipt['source_files']['src/'+filename]
        assert b'WORD __stdcall detect_input_machine('in source
    parent,=[e for e in events if e['event']=='input_machine_entry'];done,=[e for e in events if e['event']=='input_machine_return']
    assert done==dict(event='input_machine_return',entry_stack=parent['stack_pointer'],return_stack=parent['stack_pointer']+12,raw_result=done['raw_result'])
    ndone,=state['input_machine_returns'];nparent=state['input_machine_entry']
    assert ndone==dict(nparent,normal_return=True,return_stack=nparent['stack_pointer']+12,raw_result=ndone['raw_result'],context_after=state['link_context'])
    obefore=(original/'input-machine-context-before.bin').read_bytes();oafter=(original/'input-machine-context-after.bin').read_bytes()
    nbefore=bytes.fromhex(state['input_machine_context_before']);nafter=bytes.fromhex(ndone['context_after'])
    for left,right in[(obefore,oafter),(nbefore,nafter)]:
        assert len(left)==len(right)==664 and left[48:50]==b'\0\0'
        assert right==left[:48]+struct.pack('<H',machine)+left[50:]
    assert oafter==(original/'link-context.bin').read_bytes()
    proof=json.loads((root/'cases/hello-win32/initialize-machine-input-contract.json').read_text());body=pe.read(0x449630,376)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    assert proof['formals']==['LinkContext *context']and proof['hidden_inputs']==[]and proof['returns']==[[0x4497a5,4]]
    obj,=[e for e in events if e['event']=='object_read_entry'];context=obj['formals'][1]
    assert trap['esp']==obj['stack_pointer']-40 and trap['stack'][:2]==[0x449804,context]
    assert trap['initialize_machine_formals']==dict(formals=[context],hidden_inputs=[])
    assert trap['frontier_inputs']==dict(formals=[context],hidden_register_inputs=[])
    nobject=state['object_reader_entry'];context=nobject['formals'][1]
    assert state['initialize_machine_entry']==dict(stack_pointer=nobject['stack_pointer']-parent_frame,formals=[context],hidden_inputs=[])
    assert state['frontier_inputs']==dict(formals=[context],hidden_register_inputs=[])
    # No returned detector stack local is a live pointer input of this helper.
