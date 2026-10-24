"""The lookup's real two-formal, output-only module-enumerator initializer."""
import functools,hashlib,json,re,struct,subprocess
from pathlib import Path
from binary import PE
@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    symbols={r[3]:int(r[0],16)for r in[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]if len(r)==4 and r[2]=='T'}
    name='_ZN13OutputLibrary10FindModuleEPKcPKv';start=symbols[name];listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+67),path],text=True)
    body=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert body[:9].hex()=='5589e583ec28894de4'
    assert body[62:67]==b'\xe8'+struct.pack('<i',symbols['initialize_module_enumerator']-start-67)
    assert body[52:62].hex()=='8d4dec890c2489442404'
    info=subprocess.check_output(['readelf','--debug-dump=info',path],text=True)
    blocks=re.split(r'(?=^ <1><)',info,flags=re.M)
    block,=[b for b in blocks if 'DW_TAG_subprogram'in b.splitlines()[0]and re.search(r'DW_AT_low_pc[^\n]*:\s*'+hex(start)+r'\s*$',b,re.M)]
    assert 'DW_OP_reg5 (ebp)'in block
    specification=re.search(r'DW_AT_specification:\s*<0x([0-9a-f]+)>',block).group(1)
    declaration,=[b for b in re.split(r'(?=^ <[0-9]+><)',info,flags=re.M)if b.startswith(' <2><'+specification+'>:')]
    assert re.search(r'DW_AT_linkage_name[^\n]*:\s*'+name+r'\s*$',declaration,re.M)
    variable,=[b for b in re.split(r'(?=^ <2><)',block,flags=re.M)if 'DW_TAG_variable'in b.splitlines()[0]and re.search(r'DW_AT_name[^\n]*:\s*enumerator\s*$',b,re.M)]
    assert int(re.search(r'DW_OP_fbreg: (-?\d+)',variable).group(1))-4==-24
    # Follow this one concrete typedef to its complete16B POD record.
    type_id=re.search(r'DW_AT_type\s*:\s*<0x([0-9a-f]+)>',variable).group(1)
    typedef,=[b for b in blocks if b.startswith(' <1><'+type_id+'>:')];assert 'DW_TAG_typedef'in typedef.splitlines()[0]
    record_id=re.search(r'DW_AT_type\s*:\s*<0x([0-9a-f]+)>',typedef).group(1)
    record,=[b for b in blocks if b.startswith(' <1><'+record_id+'>:')];assert 'DW_TAG_structure_type'in record.splitlines()[0]
    assert int(re.search(r'DW_AT_byte_size\s*:\s*(\d+)',record).group(1))==16
    return 48,-24

def compare(original,state,trap,events,directory,entry,nmethod):
    # Original SUB16, four saved registers and two pushed actual parameters.
    enumerator=entry-16;formals=[enumerator,trap['module_find_formals']['this']]
    assert trap['esp']==entry-44 and trap['stack'][:3]==[0x40f7b4]+formals
    assert trap['module_enum_formals']==dict(formals=formals,hidden_inputs=[])
    assert trap['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
    assert len((original/'module-enumerator-before.bin').read_bytes())==16
    root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE');proof=json.loads((root/'cases/hello-win32/module-enumerator-input-contract.json').read_text());body=pe.read(0x40fb60,34)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    assert proof['formals']==['ModuleEnumerator *output','OutputLibrary *library']and proof['hidden_inputs']==[]and proof['bytes']==16
    assert body.hex()=='8b4424048b4c2408c70090fb4000c7400490fd4000c7400c00000000894808c20800'
    assert proof['initialized_ranges']==[[0,4],[4,8],[12,16],[8,12]]and proof['returns']==[[0x40fb7f,8]]
    receipt=json.loads((directory/'receipt.json').read_text());frame,offset=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    nenum=nmethod['stack_pointer']+offset;nentry=dict(stack_pointer=nmethod['stack_pointer']-frame,formals=[nenum,nmethod['this']],hidden_inputs=[])
    assert state['module_enum_entry']==nentry and state['frontier_inputs']==dict(formals=nentry['formals'],hidden_register_inputs=[])
    output=state['module_enum_output'];assert output==dict(address=nenum,bytes=16,owner_local_address=nenum,owner_local_bytes=16,before=output['before'],initial_value_contract='Output-only16B enumerator; uninitialized own bytes retained, not cross-compared')
    assert len(bytes.fromhex(output['before']))==16
    record,=[r for r in json.loads((directory/'functions.json').read_text())['driver']if r['name']=='_initialize_module_enumerator@8'];assert record['pending_extent']and(record['start'],record['end'])==(0x40fb60,0x40fb61)
