"""Constructor state before the first module-index allocation (no callee credit)."""
import functools,hashlib,json,re,struct,subprocess
from mapped_seek_state import local_offsets
from pathlib import Path
from binary import PE


@functools.lru_cache(maxsize=8)
def native_contract(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    offsets=local_offsets(path,digest,'create_input_module',('module','section_count','symbol_count'))
    assert offsets==dict(module=-36,section_count=-12,symbol_count=-16)
    rows=[r.split()for r in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()]
    table={r[3]:r for r in rows if len(r)==4}
    slot=table['contribution_arena'];assert slot[2]=='B'and int(slot[1],16)==4
    start=int(table['create_input_module'][0],16);size=int(table['create_input_module'][1],16);index=int(table['allocate_module_index'][0],16)
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(start),'--stop-address='+str(start+size),path],text=True)
    body=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    calls=[i for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,start)if i.mnemonic=='call'and i.op_str==hex(index)]
    call,=calls
    listing=subprocess.check_output(['objdump','-w','-d','--start-address='+str(index),'--stop-address='+str(index+28),path],text=True)
    stub=b''.join(bytes.fromhex(m.group(1))for m in re.finditer(r'^\s*[0-9a-f]+:\s*((?:[0-9a-f]{2}\s+)+)',listing,re.M))
    assert stub[:6].hex()=='5589e583ec18'and stub[27:]==b'\xcc'
    return int(slot[0],16),call.address+call.size,offsets['module'],index+28

def current_record(original,state,trap):
    current=state['module_creation_current']
    left=(original/'input-module-record.bin').read_bytes();right=bytes.fromhex(current['record'])
    assert len(left)==len(right)==156
    lw=struct.unpack('<39I',left);rw=struct.unpack('<39I',right)
    header=(original/'input-module-header-current.bin').read_bytes()
    assert header==bytes.fromhex(current['header'])and len(header)==20
    timestamp=struct.unpack_from('<I',header,4)[0]
    assert struct.unpack_from('<H',header,2)[0]==current['section_count']==7
    assert struct.unpack_from('<I',header,12)[0]==current['symbol_count']==37
    for raw,words in((left,lw),(right,rw)):
        expected=bytearray(156)
        for offset,value in((0,words[0]),(4,words[1]),(0x38,timestamp),(0x68,words[26])):
            assert value;struct.pack_into('<I',expected,offset,value)
        assert raw==expected,'new module differs outside four written fields'
    assert current['names']==[dict(address=rw[i],text='hello.obj')for i in(0,1)]
    assert current['contributions']==rw[26]
    return left,right


def compare(original,state,trap,events,directory):
    left,right=current_record(original,state,trap);lw=struct.unpack('<39I',left);rw=struct.unpack('<39I',right)
    current=state['module_creation_current']
    assert trap['frontier_inputs']==state['frontier_inputs']==dict(formals=[],hidden_register_inputs=[])
    assert trap['module_index_formals']==dict(formals=[],hidden_inputs=[])
    entry,=[e for e in events if e['event']=='input_module_entry']
    assert trap['esp']==entry['stack_pointer']-24 and trap['stack'][0]==0x40ebd6
    assert trap['ebp']==trap['constructor_local_module']
    nentry=state['input_module_entry'];nindex=state['module_index_entry']
    receipt=json.loads((directory/'receipt.json').read_text());slot,return_address,local_offset,trap_pc=native_contract(str(directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    assert state['pc']==trap_pc and state['stack'][7]==return_address
    assert state['stack'][6]==nentry['stack_pointer']-4
    assert current['contribution_arena']['slot']==slot
    record,=[r for r in json.loads((directory/'functions.json').read_text())['driver']if r['name']=='_allocate_module_index']
    assert record['pending_extent']and(record['start'],record['end'])==(0x431e50,0x431e51)
    assert nindex==dict(stack_pointer=nentry['stack_pointer']-64,formals=[],hidden_inputs=[])
    assert current['module_local_bytes']==4 and current['module_local_address']==nentry['stack_pointer']+local_offset
    assert current['header_address']==nentry['formals'][3]and current['created_address']==nentry['formals'][5]
    assert current['library']==nentry['formals'][4]
    assert (original/'input-module-created-current.bin').read_bytes()==bytes.fromhex(current['created'])==b'\1'
    assert (original/'input-module-header-current.bin').read_bytes()==(original/'module-find-header.bin').read_bytes()
    # Complete constructor already bound by its input contract. These exact
    # reached stores and calls establish widths, order and both arena owners.
    pe=PE(Path(__file__).resolve().parents[4]/'toolchains/5.10/bin/LINK.EXE')
    anchors={0x40ea33:'33d285ed0f94c28810',0x40ea49:'689c000000',0x40ea6e:'6890754600e8680b0400',0x40ea7e:'8bcbc1e106516894754600e8520b0400',0x40ea92:'894568',0x40ea9b:'c7452c00000000',0x40eaa4:'50e8d6120400894500',0x40eab6:'8b460485ff894538',0x40eac0:'57e8ba120400894504',0x40ebd1:'e87a320200'}
    for address,hexdata in anchors.items():assert pe.read(address,len(bytes.fromhex(hexdata)))==bytes.fromhex(hexdata),(hex(address),'constructor writer span')
    names=[];copies=[e for e in events if e['event']=='input_module_name_copy']
    assert len(copies)==4 and len(state['link_string_calls'])==21 and len(state['duplicate_calls'])==28
    for i in range(2):
        b,a=copies[2*i:2*i+2];wrapper=state['link_string_calls'][19+i];duplicate=state['duplicate_calls'][26+i]
        assert b['kind']==a['kind']==i and(b['before'],a['before'])==(1,0)
        assert(b['site'],a['site'])==[(0x40eaa5,0x40eaaa),(0x40eac1,0x40eac6)][i]
        assert b['entry_stack']==a['entry_stack']==entry['stack_pointer']-28
        assert b['stack']==entry['stack_pointer']-24 and a['stack']==entry['stack_pointer']-20
        assert b['argument']==a['argument']==entry['formals'][i]and a['result']==lw[i]
        assert b['text']==a['text']=='hello.obj'
        assert (original/('input-module-copy-%d.bin'%i)).read_bytes()==(original/('input-module-name-%d.bin'%i)).read_bytes()==b'hello.obj\0'
        assert wrapper==dict(input=nentry['formals'][i],text='hello.obj',entry_stack=nentry['stack_pointer']-64,result=rw[i],return_stack=nentry['stack_pointer']-56)
        assert duplicate==dict(text='hello.obj',input=nentry['formals'][i],result=rw[i])
        names.append(dict(original_base=lw[i],native_base=rw[i],bytes=10))
    # Distinct allocation generations: never reuse the borrowed input or each other.
    for values,input_address in((lw,entry['formals'][0]),(rw,nentry['formals'][0])):
        assert all(a+10<=b or b+10<=a for a,b in((values[0],values[1]),(values[0],input_address),(values[1],input_address)))
    arena=struct.unpack('<I',(original/'contribution-arena-pointer.bin').read_bytes())[0]
    header=struct.unpack('<4I',(original/'contribution-arena.bin').read_bytes());native=current['contribution_arena'];nheader=struct.unpack('<4I',bytes.fromhex(native['raw']))
    assert arena and native['address']and native['slot']
    assert header[:3]==nheader[:3]==(0,448,3648)
    assert header[3]==lw[26]and nheader[3]==rw[26]
    assert current['contribution_bytes']==448
    assert (original/'input-module-contributions.bin').read_bytes()==bytes.fromhex(current['contribution_data'])==bytes(448)
    assert (original/'contribution-page.bin').read_bytes()==bytes.fromhex(native['data'])==bytes(4096)
    # The whole zero page, its header, and both heap strings enter the inherited
    # allocation-generation/disjointness and exact VirtualAlloc ordering checks.
    return dict(arena=dict(original_arena=arena,native_arena=native['address'],original_base=header[3],native_base=nheader[3],bytes=4096),ranges=names)
