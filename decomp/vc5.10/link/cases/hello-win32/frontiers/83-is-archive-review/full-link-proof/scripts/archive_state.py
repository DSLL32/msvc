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
    assert pe.read(0x45cdc7,10).hex()=='b908000000bf682e4000'
    assert pe.read(0x45cdd1,8).hex()=='8d74240833d2f3a6'
    output=json.loads((binary/'cases/hello-win32/mapped-read-output-contract.json').read_text())
    body=pe.read(0x40cab0,135)
    assert hashlib.sha256(body).hexdigest()==output['body_sha256']
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,0x40cab0))
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
    assert trap['archive_entry_stack']==entry['stack_pointer']
    assert trap['esp']==entry['stack_pointer']-32 and trap['stack'][0]==0x45cd7a
    buffer=entry['stack_pointer']-8
    formals=[oslots[14],buffer,8]
    assert trap['stack'][1:4]==formals
    assert trap['mapped_read_formals']==dict(formals=formals,hidden_inputs=[])
    assert trap['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    assert len((original/'archive-output-before.bin').read_bytes())==8
    nentry=state['archive_entry'];read=state['mapped_read_entry'];output=state['archive_output']
    nbuffer=nentry['stack_pointer']+offset
    assert output==dict(address=nbuffer,bytes=8,owner_local_address=nbuffer,owner_local_bytes=8,
        before=output['before'],initial_value_contract='uninitialized output array; no incoming bytes consumed')
    assert len(bytes.fromhex(output['before']))==8 # Own uninitialized bytes; never cross-equated.
    assert read['stack_pointer']<nbuffer and nbuffer+8<nentry['stack_pointer']
    assert read['formals']==[nslots[14],nbuffer,8]and read['hidden_inputs']==[]
    assert state['frontier_inputs']==dict(formals=read['formals'],hidden_register_inputs=[])
    assert state['archive_member_offset']==0 and (original/'archive-member-offset.bin').read_bytes()==bytes(4)
    # The original snapshot reads this actual one-byte global at the current
    # trap even though the surrounding caller-local metadata is historical.
    assert trap['file_record_formals']['file_access_statistics']==state['archive_statistics_enabled']==0
    assert globals_['archive_member_offset']!=globals_['file_access_statistics']
    return dict(original_base=buffer,native_base=nbuffer,bytes=8)
