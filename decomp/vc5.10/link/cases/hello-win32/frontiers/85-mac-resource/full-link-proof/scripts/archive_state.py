"""Actual mapped-read inputs and its owned eight-byte caller output array."""
import functools
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path
from binary import PE


@functools.lru_cache(maxsize=1)
def original_contract():
    binary=Path(__file__).resolve().parents[1]
    target=binary.parents[2]/'toolchains/5.10/bin/LINK.EXE'
    assert hashlib.sha256(target.read_bytes()).hexdigest()=='04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579'
    proof=json.loads((binary/'cases/hello-win32/archive-signature-literal.json').read_text())
    pe=PE(target)
    assert pe.read(0x402e68,8)==b'!<arch>\n'
    assert hashlib.sha256(pe.read(0x402e68,8)).hexdigest()==proof['sha256']
    assert hashlib.sha256(pe.read(0x45cd40,185)).hexdigest()==proof['body_sha256']
    assert proof['bytes']==proof['comparison_count']==8
    assert pe.read(0x4498e5,2).hex()=='84c0' # Actual caller consumes AL, not upper EAX.
    assert pe.read(0x45cdc7,10).hex()=='b908000000bf682e4000'
    assert pe.read(0x45cdd1,8).hex()=='8d74240833d2f3a6'
    output=json.loads((binary/'cases/hello-win32/mapped-read-output-contract.json').read_text())
    body=pe.read(0x40cab0,135)
    assert hashlib.sha256(body).hexdigest()==output['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x40cab0))
    archive=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(0x45cd40,185),0x45cd40))
    assert sum(i.size for i in archive)==185
    assert [(i.address,int(i.op_str,0))for i in archive if i.mnemonic=='ret']==[(0x45cdec,8),(0x45cdf6,8)]
    assert sum(i.size for i in instructions)==135
    assert [(i.address,int(i.op_str,0))for i in instructions if i.mnemonic=='ret']==[(0x40cb2a,12),(0x40cb34,12)]
    # The destination argument is loaded only AFTER either resize/error call.
    # Its two uses are REP stores; incoming output bytes are never read.
    assert pe.read(0x40cb05,22).hex()==output['destination_load_through_copy_hex']
    assert output['destination_consumption']=='write-only; no incoming output bytes consumed'
    data,=[s for s in pe.sections if s['name']=='.data']
    for address,size in ((0x47eed8,4),(0x47f0b0,1)):
        rva=address-pe.base
        assert data['rva']+data['size']<=rva and rva+size<=data['rva']+data['virtual_size']
    return proof


@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    """Bind this local to frozen DWARF and its actual EBP frame prologue.

    This reads a variable location, not a second CFG/dataflow implementation.
    Unsupported compiler frame expressions deliberately require a new witness.
    """
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    owner,=[r for r in rows if len(r)==4 and r[3]=='is_archive']
    assert owner[2]=='T'
    address=int(owner[0],16)
    info=subprocess.check_output(['readelf','--debug-dump=info',path],text=True)
    blocks=re.split(r'(?=^ <1><)',info,flags=re.M)
    block,=[b for b in blocks if 'DW_TAG_subprogram'in b.splitlines()[0]and re.search(r'DW_AT_name\s*:.*:\s*is_archive\s*$',b,re.M)]
    assert int(re.search(r'DW_AT_low_pc[^\n]*:\s*(0x[0-9a-f]+)',block).group(1),16)==address
    assert 'DW_OP_reg5 (ebp)'in block
    variable,=[b for b in re.split(r'(?=^ <2><)',block,flags=re.M)if 'DW_TAG_variable'in b.splitlines()[0]and re.search(r'DW_AT_name\s*:.*:\s*signature\s*$',b,re.M)]
    location=int(re.search(r'DW_OP_fbreg: (-?\d+)',variable).group(1))
    prologue=subprocess.check_output(['objdump','-d','--start-address='+str(address),'--stop-address='+str(address+3),path],text=True)
    raw=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',prologue,re.M))
    assert raw==b'\x55\x89\xe5' # EBP = true entry ESP - 4.
    globals_={}
    for name,size in (('archive_member_offset',4),('file_access_statistics',1)):
        row,=[r for r in rows if len(r)==4 and r[3]==name]
        assert row[2]=='B'and int(row[1],16)==size
        globals_[name]=int(row[0],16)
    return location-4,globals_


def compare(original,state,trap,events,native_directory,oslots,nslots):
    literal=original_contract()
    config=json.loads((original/'functions.json').read_text())
    owned,=[r for r in config['text_data']if r['start']==0x402e68]
    assert owned['end']==0x402e70 and owned['accepted']is True and owned['sha256']==literal['sha256']
    mask=(original/'mask.bin').read_bytes();begin=struct.unpack_from('<I',mask)[0]
    position=8+0x2e68-begin
    assert mask[position:position+8]==b'!<arch>\n'and mask[position+8:position+10]==b'\xcc\xcc'
    directory=Path(native_directory);receipt=json.loads((directory/'receipt.json').read_text())
    offset,globals_=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    reader,=[e for e in events if e['event']=='read_input_entry']
    entry,=[e for e in events if e['event']=='archive_entry']
    lists={e['name']:e for e in events if e['event']=='argument_list'}
    assert entry==dict(event='archive_entry',stack_pointer=reader['stack_pointer']-24,
        formals=[lists['link_files']['nodes'][0]['words'][0],14],hidden_inputs=[])
    completed=state['function']in ('is_mac_resource_file','get_file_length')
    if not completed:
        assert trap['archive_entry_stack']==entry['stack_pointer']
        assert trap['esp']==entry['stack_pointer']-32 and trap['stack'][0]==0x45cd7a
    buffer=entry['stack_pointer']-8
    formals=[oslots[14],buffer,8]
    if not completed:
        assert trap['stack'][1:4]==formals
        assert trap['mapped_read_formals']==dict(formals=formals,hidden_inputs=[])
        assert trap['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    assert len((original/('archive-read-0-before.bin'if completed else'archive-output-before.bin')).read_bytes())==8
    nentry=state['archive_entry'];read=state['mapped_read_entry'];output=state['archive_output']
    nbuffer=nentry['stack_pointer']+offset
    assert output==dict(address=nbuffer,bytes=8,owner_local_address=nbuffer,owner_local_bytes=8,
        before=output['before'],initial_value_contract='uninitialized output array; no incoming bytes consumed')
    assert len(bytes.fromhex(output['before']))==8 # Own uninitialized bytes; never cross-equated.
    assert read['stack_pointer']<nbuffer and nbuffer+8<nentry['stack_pointer']
    assert read['formals']==[nslots[14],nbuffer,8]and read['hidden_inputs']==[]
    if not completed:assert state['frontier_inputs']==dict(formals=read['formals'],hidden_register_inputs=[])
    assert state['archive_member_offset']==0 and (original/'archive-member-offset.bin').read_bytes()==bytes(4)
    # The original snapshot reads this actual one-byte global at the current
    # trap even though the surrounding caller-local metadata is historical.
    assert trap['file_record_formals']['file_access_statistics']==state['archive_statistics_enabled']==0
    assert globals_['archive_member_offset']!=globals_['file_access_statistics']
    if completed:
        compare_completed(original,state,trap,events,directory,reader,entry,formals,read,output)
        return None # This array is now expired; only its actual earlier observations are used.
    return dict(original_base=buffer,native_base=nbuffer,bytes=8)


def compare_completed(original,state,trap,events,directory,reader,entry,formals,read,output):
    """Return-time output is historical here; never read the expired array."""
    call,=[e for e in events if e['event']=='archive_read_call']
    returned,=[e for e in events if e['event']=='archive_read_return']
    assert call==dict(event='archive_read_call',index=0,entry_stack=entry['stack_pointer']-32,
        formals=formals,hidden_inputs=[])
    assert returned==dict(event='archive_read_return',index=0,entry_stack=call['entry_stack'],
        return_stack=call['entry_stack']+16,result=8)
    binary=Path(__file__).resolve().parents[1]
    pe=PE(binary.parents[2]/'toolchains/5.10/bin/LINK.EXE')
    probe=json.loads((binary/'cases/hello-win32/mac-resource-input-contract.json').read_text())
    assert hashlib.sha256(pe.read(0x44e620,138)).hexdigest()==probe['body_sha256']
    assert probe['formals']==['int file']and probe['hidden_inputs']==[]
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(0x44e620,138),0x44e620))
    assert sum(i.size for i in instructions)==138
    assert [[i.address,int(i.op_str,0)]for i in instructions if i.mnemonic=='ret']==probe['returns']
    assert all(cleanup==4 for address,cleanup in probe['returns'])and len(probe['returns'])==4
    master=json.loads((original/'case.json').read_text())['inputs'][0]
    data=(binary/master['path']).read_bytes()
    assert len(data)==master['bytes']==2262 and hashlib.sha256(data).hexdigest()==master['sha256']
    signature=data[:8];assert signature!=b'!<arch>\n'
    assert (original/'archive-read-0-after.bin').read_bytes()==signature
    assert (original/'archive-read-0-record-after.bin').read_bytes()==(original/'file-record-14.bin').read_bytes()
    assert len(state['archive_read_returns'])==1
    nreturn,=state['archive_read_returns']
    assert nreturn==dict(read,normal_return=True,return_stack=read['stack_pointer']+16,result=8,
        output=dict(output,after=signature.hex(),record_after=state['file_records'][14]))
    predicate,=[e for e in events if e['event']=='archive_predicate_call']
    finished,=[e for e in events if e['event']=='archive_predicate_return']
    assert predicate==dict(event='archive_predicate_call',entry_stack=entry['stack_pointer'],
        formals=entry['formals'],hidden_inputs=[])
    assert finished==dict(event='archive_predicate_return',entry_stack=entry['stack_pointer'],
        return_stack=entry['stack_pointer']+12,raw_result=finished['raw_result'],result=0)
    assert len(state['archive_returns'])==1
    nfinished,=state['archive_returns']
    assert nfinished==dict(state['archive_entry'],normal_return=True,
        return_stack=state['archive_entry']['stack_pointer']+12,raw_result=nfinished['raw_result'],result=0)
    for raw in (finished['raw_result'],nfinished['raw_result']):
        assert 0<=raw<=0xffffffff and raw&255==0
    # Only AL belongs to this return ABI. The original caller's TEST AL and
    # both original RET8 sites were bound above; verify the compiled-source
    # BYTE declaration/definition too. Retain each complete observed EAX.
    receipt=json.loads((directory/'receipt.json').read_text())
    for filename in ('input.c','input.h'):
        source=(directory/'source/project'/filename).read_bytes()
        assert hashlib.sha256(source).hexdigest()==receipt['source_files']['src/'+filename]
        assert b'BYTE __stdcall is_archive('in source
    if state['function']=='is_mac_resource_file':
        assert trap['esp']==reader['stack_pointer']-20 and trap['stack'][:2]==[0x44994e,14]
        assert trap['mac_resource_formals']==dict(formals=[14],hidden_inputs=[])
    else:
        assert state['function']=='get_file_length'
        current,=[e for e in events if e['event']=='mac_resource_entry']
        assert current==dict(event='mac_resource_entry',stack_pointer=reader['stack_pointer']-20,formals=[14],hidden_inputs=[])
        assert trap['esp']==current['stack_pointer']-24 and trap['stack'][:2]==[0x44e62e,14]
        assert trap['file_length_formals']==dict(formals=[14],hidden_inputs=[])
        assert state['file_length_entry']==dict(stack_pointer=state['file_length_entry']['stack_pointer'],formals=[14],hidden_inputs=[])
        delta=native_classifier_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
        assert state['file_length_entry']['stack_pointer']==state['mac_resource_entry']['stack_pointer']-delta
        length=json.loads((binary/'cases/hello-win32/file-length-input-contract.json').read_text())
        body=pe.read(0x40c080,107)
        assert hashlib.sha256(body).hexdigest()==length['body_sha256']
        decoded=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x40c080))
        assert sum(i.size for i in decoded)==107
        assert [[i.address,int(i.op_str,0)]for i in decoded if i.mnemonic=='ret']==length['returns']==[[0x40c0e8,4]]
        assert length['formals']==['int file']and length['hidden_inputs']==[]
        assert pe.read(0x44e629,5)==b'\xe8'+struct.pack('<i',0x40c080-0x44e62e)
    assert state['mac_resource_entry']==dict(stack_pointer=state['read_input_entry']['stack_pointer']-48,
        formals=[14],hidden_inputs=[])
    assert trap['frontier_inputs']==state['frontier_inputs']==dict(formals=[14],hidden_register_inputs=[])


@functools.lru_cache(maxsize=8)
def native_classifier_contract(path,digest):
    """Finite emitted caller/frame anchor for this genuine entry observation."""
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    caller,=[r for r in rows if len(r)==4 and r[3]=='is_mac_resource_file']
    callee,=[r for r in rows if len(r)==4 and r[3]=='get_file_length']
    assert caller[2]==callee[2]=='T'
    address=int(caller[0],16);target=int(callee[0],16)
    listing=subprocess.check_output(['objdump','-d','--start-address='+str(address),'--stop-address='+str(address+41),path],text=True)
    raw=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert raw[:6]==b'\x55\x89\xe5\x83\xec\x38' # Saved EBP4 + fixed56B caller frame.
    assert raw[30:36]==b'\x8b\x45\x08\x89\x04\x24' # Actual source file argument passed unchanged.
    assert raw[36:]==b'\xe8'+struct.pack('<i',target-address-41)
    return 4+56+4 # Saved EBP, caller local frame, actual CALL return word.
