"""Exact archive identity and observed internal CRT/public printf seam proofs."""
from pathlib import Path
import hashlib,json
from binary import PE,COFF,archive,u32

CONTRACTS={
 '__stbuf':'int __cdecl _stbuf(FILE *); actual disk stdout returns0, FILE unchanged',
 '__isatty':'int __cdecl _isatty(int); actual fd1 disk returns0',
 '__output':'int __cdecl _output(FILE *,const char *,va_list); actual narrow output returns24',
 '__ftbuf':'void __cdecl _ftbuf(int,FILE *); actual flag0, FILE/buffer/descriptor unchanged',
 '_write_char':'void __cdecl write_char(int,FILE *,int *); actual tab advances counter0 to1',
 '_write_multi_char':'void __cdecl write_multi_char(int,int,FILE *,int *); actual signed count-9 leaves counter unchanged',
 '_write_string':'void __cdecl write_string(char *,int,FILE *,int *); actual first call count0, FILE/counter unchanged',
 '_get_int_arg':'int __cdecl get_int_arg(va_list *); actual source pointer returned, cursor advances4'}

def sha(data):return hashlib.sha256(data).hexdigest()

def identity(record):
    library=Path('../../../toolchains/1.0-nt/lib')/record['library'];objdata=next(d for n,d in archive(library)if n==record['object']);obj=COFF(objdata)
    sym=next(s for s in obj.symbols.values()if s['name']==record['name']);text=next(s for s in obj.sections if s['index']==sym['section'])
    lo=sym['value'];size=record['end']-record['start'];body=text['data'][lo:lo+size];assert len(body)==size
    pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');linked=pe.read(record['start'],size);compared=bytearray(linked);relocations=[];bindings={}
    known={r['name']:r['start']for r in json.loads(Path('runtime.json').read_text())}
    # C2's earlier CRT variants (__filbuf, fseek, ftell, ...) have individually
    # reconstructed original records rather than falsely matching this archive.
    # Library callers still bind their named relocations to those exact entries.
    for r in json.loads(Path('functions.json').read_text())['driver']:
        assert known.setdefault(r['name'],r['start'])==r['start'],r['name']
    sectionbase=record['start']-lo
    for offset,symbol,kind in text['relocs']:
        if not lo<=offset<lo+size:continue
        local=offset-lo;addend=u32(body,local);actual=u32(linked,local);name=symbol['name']
        if kind==20:
            actual=(record['start']+local+4+actual)&0xffffffff
            expected=(sectionbase+symbol['value']+addend)&0xffffffff if symbol['section']==text['index'] else (known[name]+addend)&0xffffffff
            assert actual==expected,(record['name'],name,hex(actual),hex(expected));binding='named code relocation'
        else:
            assert kind==6
            if symbol['section']==text['index']:
                expected=(sectionbase+symbol['value']+addend)&0xffffffff;assert actual==expected;binding='same archive text/table'
            elif symbol['section']:
                base=(actual-symbol['value']-addend)&0xffffffff
                section=next(s for s in obj.sections if s['index']==symbol['section'])
                assert name in ['.data','.bss'],(record['name'],name)
                linked_data=pe.read(base,len(section['data']));normalized_data=bytearray(linked_data)
                for data_offset,data_symbol,data_kind in section['relocs']:
                    assert data_kind==6 and data_symbol['section']==section['index']
                    expected_data=(base+data_symbol['value']+u32(section['data'],data_offset))&0xffffffff
                    assert u32(linked_data,data_offset)==expected_data
                    normalized_data[data_offset:data_offset+4]=section['data'][data_offset:data_offset+4]
                assert bytes(normalized_data)==section['data'],(record['name'],'archive data mismatch')
                expected=actual;binding='archive data section exact bytes'
                assert bindings.setdefault(name,base)==base
            else:
                base=(actual-addend)&0xffffffff
                anchors={'__iob':0x86198,'__nhandle':0x86770,'__osfile':0x86778,'__cflush':0x8709c}
                if name in anchors:assert base==anchors[name],(name,hex(base))
                assert bindings.setdefault(name,base)==base
                assert any(pe.base+s['rva']<=base<pe.base+s['rva']+s['virtual_size']for s in pe.sections),name
                expected=actual;binding='linked external data binding; known anchor where available'
            assert actual==expected
        relocations.append(dict(offset=local,symbol=name,kind=kind,target=hex(actual),binding=binding))
        compared[local:local+4]=body[local:local+4]
    assert bytes(compared)==body,'Archive function body differs outside proven relocations'
    return dict(record=record,library_sha256=sha(library.read_bytes()),object_sha256=sha(objdata),function_offset=lo,exact_text_bytes=size,text_sha256=sha(body),linked_text_sha256=sha(linked),relocations=relocations,external_bindings={n:hex(a)for n,a in bindings.items()})

def validate_observations(name,receipt):
    assert name in CONTRACTS and receipt['matched_public_seam'] and receipt['public_result']==24
    assert receipt['public_original_file']==receipt['public_native_file']
    assert receipt['public_native_file']==[dict(flags=2,count=0,fd=1,size=0,null=1,used=0,position=0,buffer_hex=''),dict(flags=10,count=4072,fd=1,size=4096,null=0,used=24,position=0,buffer_hex=b'\tTITLE\tdbglist.c\n\t.386P\n'.hex())]
    assert bytes.fromhex(receipt['public_native_file'][-1]['buffer_hex'])==b'\tTITLE\tdbglist.c\n\t.386P\n'
    rows=[r for r in receipt['observations']if r['name']==name];assert [r['stage']for r in rows]==['enter','return']
    before,after=rows;public=next(r for r in receipt['observations']if r['name']=='_printf')
    assert before['args']==after['args']
    args=before['args'];left,right=before['file'],after['file']
    if name in ['__stbuf','__isatty']:
        assert after['eax']==0 and left==right
        assert args==([0x861b8]if name=='__stbuf'else[1])
    elif name=='__output':
        assert args[0]==0x861b8 and args[1]==public['args'][0] and after['eax']==24
        assert {k:v for k,v in left.items()if k not in ['counter','cursor','value']}==receipt['public_original_file'][0]
        assert {k:v for k,v in right.items()if k not in ['counter','cursor','value']}==receipt['public_original_file'][1]
    elif name=='__ftbuf':assert args==[0,0x861b8]and left==right
    elif name=='_write_char':
        assert args[0]==9 and args[1]==0x861b8 and left['counter']==0 and right['counter']==1
        assert right['buffer_hex']=='09'
    elif name=='_write_multi_char':assert args[:3]==[32,0xfffffff7,0x861b8] and left==right
    elif name=='_write_string':
        assert args[1]==0 and args[2]==0x861b8 and left==right
        # This observed first call is the empty prefix, not the later source
        # string loop. The full public printf buffer proves the complete text;
        # do not describe this breakpoint as exercising the write loop.
    elif name=='_get_int_arg':
        assert after['eax']==public['args'][1]==left['value']and right['cursor']==left['cursor']+4
    return rows

def validate(evidence,record,before):
    assert evidence['identity']==identity(record)
    path=Path(evidence['observation_path']);assert sha(path.read_bytes())==evidence['observation_sha256']
    observation=json.loads(path.read_text());assert evidence['internal_observations']==validate_observations(record['name'],observation)
    assert evidence['contract']==CONTRACTS[record['name']]
    assert evidence['native_boundary']=='Public printf seam only; internal CRT native ABIs not independently matched/reconstructed; zero application credit'
