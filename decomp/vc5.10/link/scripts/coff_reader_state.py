"""Complete COFF-header output and twelve explicit module-constructor inputs."""
import functools
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from binary import PE,u32
from mapped_seek_state import local_offsets,with_cursor


def check_ownership(config):
    record,=[r for r in config['driver']if r['name']=='_read_coff_header@16']
    assert (record['start'],record['end'])==(0x4610c0,0x4611d1)


@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[r.split()for r in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    symbols={r[3]:int(r[0],16)for r in rows if len(r)==4 and r[2]=='T'}
    def raw(name,size):
        start=symbols[name];listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+size),path],text=True)
        return b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    body=raw('read_coff_header',191);start=symbols['read_coff_header']
    assert body[:6].hex()=='5589e583ec28'
    assert body[186:191]==b'\xe8'+struct.pack('<i',symbols['mapped_read']-start-191)
    caller=raw('read_object_file',275);start=symbols['read_object_file']
    assert caller[270:275]==b'\xe8'+struct.pack('<i',symbols['create_input_module']-start-275)
    assert caller[192:195].hex()=='8d45d7'
    assert local_offsets(path,digest,'read_object_file',('header','created'))==dict(header=-36,created=-45)
    return 48,112,-36,-45


def unused_position_contract(root,pe):
    proof=json.loads((root/'cases/hello-win32/coff-reader-unused-position.json').read_text())
    assert hashlib.sha256(pe.data).hexdigest()==proof['target_sha256']
    for key in('wrapper','common'):
        item=proof[key];assert hashlib.sha256(pe.read(item['start'],item['bytes'])).hexdigest()==item['sha256']
    assert pe.read(0x430243,5)==b'\xe8'+struct.pack('<i',0x430160-0x430248)
    assert pe.read(0x43021c,6).hex()=='ff1584114000'
    directory=pe.base+u32(pe.data,pe.optional+104);found=[]
    while pe.read(directory,20)!=bytes(20):
        lookup,_,_,name,iat=struct.unpack('<5I',pe.read(directory,20));i=0
        while u32(pe.read(pe.base+lookup+4*i,4),0):
            value=u32(pe.read(pe.base+lookup+4*i,4),0)
            if pe.base+iat+4*i==0x401184:found.append((pe.string(pe.base+name),pe.string(pe.base+value+2)))
            i+=1
        directory+=20
    assert found==[('MSVCRT.dll','exit')]
    provider=proof['provider_reference']
    for field,digest in [('path',provider['sha256']),('receipt',provider['receipt_sha256'])]:
        assert hashlib.sha256((root/provider[field]).read_bytes()).hexdigest()==digest
    # No entry-to-RET path in this exact common body avoids public CRT exit.
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(0x430160,204),0x430160)}
    pending=[0x430160];seen=set()
    while pending:
        address=pending.pop()
        if address in seen or address==0x43021c:continue
        seen.add(address);i=instructions[address];assert i.mnemonic!='ret'
        if i.mnemonic.startswith('j'):
            pending.append(int(i.op_str,0))
            if i.mnemonic=='jmp':continue
        pending.append(address+i.size)
    return proof


def compare(original,state,trap,events,directory):
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE');unused_position_contract(root,pe)
    check_ownership(json.loads((directory/'functions.json').read_text()))
    obj,=[e for e in events if e['event']=='object_read_entry'];stack=obj['stack_pointer'];args=obj['formals']
    reader,=[e for e in events if e['event']=='coff_reader_entry'];header=stack-20
    assert reader==dict(event='coff_reader_entry',stack_pointer=stack-52,formals=[14,header,0,0],hidden_inputs=[])
    read,=[e for e in events if e['event']=='coff_read_call'];returned,=[e for e in events if e['event']=='coff_read_return']
    record=struct.unpack('<16I',(original/'file-slots.bin').read_bytes())[14]
    assert read==dict(event='coff_read_call',index=0,entry_stack=reader['stack_pointer']-28,formals=[record,header,20],hidden_inputs=[])
    assert returned==dict(event='coff_read_return',index=0,entry_stack=read['entry_stack'],return_stack=read['entry_stack']+16,result=20)
    master=json.loads((original/'case.json').read_text())['inputs'][0];data=(root/master['path']).read_bytes()
    assert len(data)==master['bytes']==2262 and hashlib.sha256(data).hexdigest()==master['sha256'];expected=data[:20]
    assert (original/'coff-header-before.bin').read_bytes()==(original/'coff-header-read-0-before.bin').read_bytes()
    assert len((original/'coff-header-before.bin').read_bytes())==20
    assert (original/'coff-header-after.bin').read_bytes()==(original/'coff-header-read-0-after.bin').read_bytes()==expected
    current=(original/'file-record-14.bin').read_bytes();ncurrent=bytes.fromhex(state['file_records'][14])
    assert struct.unpack_from('<I',current,28)[0]==struct.unpack_from('<I',ncurrent,28)[0]==20
    assert (original/'coff-header-read-0-record-after.bin').read_bytes()==current
    receipt=json.loads((directory/'receipt.json').read_text());rframe,oframe,hoffset,coffset=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nobj=state['object_reader_entry'];nstack=nobj['stack_pointer'];nargs=nobj['formals'];nheader=nstack+hoffset
    nreader=state['coff_header_entry'];nread=state['coff_read_entry'];out=state['coff_read_output'];nrecord=struct.unpack('<16I',bytes.fromhex(state['file_slots_bytes']))[14]
    assert nread==dict(stack_pointer=nreader['stack_pointer']-rframe,formals=[nrecord,nheader,20],hidden_inputs=[])
    assert out==state['coff_header_output']
    assert state['coff_read_returns']==[dict(nread,normal_return=True,return_stack=nread['stack_pointer']+16,result=20,output=dict(out,after=expected.hex(),record_after=ncurrent.hex()))]
    done,=[e for e in events if e['event']=='coff_reader_return'];ndone,=state['coff_header_returns']
    assert done==dict(event='coff_reader_return',entry_stack=reader['stack_pointer'],return_stack=reader['stack_pointer']+20,raw_result=done['raw_result'],result=0)
    assert ndone==dict(nreader,normal_return=True,return_stack=nreader['stack_pointer']+20,raw_result=ndone['raw_result'],result=0,header_after=expected.hex())
    for value in(done['raw_result'],ndone['raw_result']):assert 0<=value<=0xffffffff and value&255==0
    # Exact source BYTE signature plus original AL-setting body define the return;
    # this particular caller ignores it. Retain each physical EAX separately.
    assert pe.read(0x4611c2,2).hex()=='32c0'and pe.read(0x4611ce,3).hex()=='c21000'
    for filename in('input.c','input.h'):
        source=(directory/'source/project'/filename).read_bytes();assert hashlib.sha256(source).hexdigest()==receipt['source_files']['src/'+filename]
        assert b'BYTE __stdcall read_coff_header('in source
    # The complete argument record and library already belong to inherited typed
    # contracts. Recover their actual member identities rather than equate strings.
    nodes=next(e for e in events if e['event']=='argument_list'and e['name']=='link_files')['nodes'];argument=nodes[0]['words']
    name,original_name=argument[0:2];library=args[2];created=stack+4
    formals=[original_name,name,0,header,library,created,0,20,0,0,0,0]
    completed=state['function']in ('OutputLibrary::FindModule','initialize_module_enumerator','next_module','allocate_module_index','allocate_module_section_headers')
    if completed:
        module,=[e for e in events if e['event']=='input_module_entry']
        assert module==dict(event='input_module_entry',stack_pointer=stack-84,formals=formals,hidden_inputs=[])
    else:assert trap['esp']==stack-84 and trap['stack'][:13]==[0x44985d]+formals
    assert trap['input_module_formals']==dict(formals=formals,hidden_inputs=[])
    if not completed:assert trap['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    assert pe.read(0x4497e9,4).hex()=='8b742420'and pe.read(0x449842,4).hex()=='8d4c2438'
    assert (original/'input-module-created-before.bin').read_bytes()==bytes([args[0]&255])
    nname=state['canonicalized_arguments'][0]['result'];ncreated=nstack+coffset
    nformals=[nname,nname,0,nheader,nargs[2],ncreated,0,20,0,0,0,0]
    assert state['input_module_entry']==dict(stack_pointer=nstack-oframe,formals=nformals,hidden_inputs=[])
    if not completed:assert state['frontier_inputs']==dict(formals=nformals,hidden_register_inputs=[])
    cout=state['input_module_created_output'];assert cout==dict(address=ncreated,bytes=1,owner_local_address=ncreated,owner_local_bytes=1,before=cout['before'],initial_value_contract='BYTE created output; own incoming byte retained, writes before consumption qualified separately')
    assert len(bytes.fromhex(cout['before']))==1 and ncreated+1<=nheader
    proof=json.loads((root/'cases/hello-win32/create-module-input-contract.json').read_text());body=pe.read(0x40ea00,972)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    assert len(proof['formals'])==proof['physical_slots']==12 and proof['hidden_inputs']==[]
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x40ea00))
    assert sum(i.size for i in instructions)==972
    assert [[i.address,int(i.op_str,0)]for i in instructions if i.mnemonic=='ret']==proof['returns']==[[0x40eda9,48],[0x40edc9,48]]
    assert proof['created_output']['first_store']==dict(address=0x40ea3a,raw='8810')
    assert (proof['created_output']['formal_index'],proof['created_output']['bytes'])==(5,1)
    assert pe.read(0x40ea2b,17).hex()=='8b44242c85c0740933d285ed0f94c28810'
    # Both header20 and createdBYTE are actual current pointer owners. Returned
    # reader/error/initializer locals stay historical and add no current ranges.

    if completed:
        from module_constructor_state import compare as compare_constructor
        compare_constructor(original,state,trap,events,directory,formals,nformals,expected)
