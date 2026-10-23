"""Describe only the actual compiler-emitted COFF switch data, not a CFG."""
import hashlib
from pathlib import Path
import re
import struct

HERE=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def verify(object_path=None,rebuilt_path=None,map_path=None):
    path=object_path or HERE/'build/preprocess.obj'
    rebuilt_path=rebuilt_path or HERE/'build/RCDLL.DLL'
    map_path=map_path or HERE/'build/rcdll.map'
    data=path.read_bytes()
    machine,count,_,symbol_at,symbol_count,optional,_=struct.unpack_from('<HHIIIHH',data)
    assert machine==0x14c and optional==0
    strings=symbol_at+18*symbol_count
    def name(raw):
        if raw[:4]==b'\0'*4:
            at=strings+struct.unpack_from('<I',raw,4)[0]
            return data[at:data.index(b'\0',at)].decode('ascii')
        return raw.rstrip(b'\0').decode('ascii')
    sections=[]
    for index in range(count):
        at=20+40*index
        raw_name,_,_,size,raw,relocations,_,number,_,_=struct.unpack_from('<8sIIIIIIHHI',data,at)
        sections.append(dict(name=raw_name.rstrip(b'\0').decode(),size=size,raw=raw,relocations=relocations,count=number))
    symbols={};by_name={};index=0
    while index<symbol_count:
        at=symbol_at+18*index
        raw_name,value,section,kind,storage,aux=struct.unpack_from('<8sIhHBB',data,at)
        row=dict(name=name(raw_name),value=value,section=section,kind=kind,storage=storage)
        symbols[index]=row;by_name[row['name']]=row;index+=1+aux
    owner=by_name['_rc_apply_option@12'];following=by_name['_rc_option_number@4']
    assert owner['section']==following['section']>0 and owner['kind']==following['kind']==0x20
    section=sections[owner['section']-1];assert section['name']=='.text'
    code=data[section['raw']:section['raw']+section['size']]
    relocations={}
    for index in range(section['count']):
        at,symbol,kind=struct.unpack_from('<IIH',data,section['relocations']+10*index)
        assert at not in relocations;relocations[at]=(symbols[symbol],kind)
    begin,end=owner['value'],following['value'];assert begin<end<=len(code)
    jumps=[at for at in range(begin,end-6)if code[at:at+3]==b'\xff\x24\x85'and at+3 in relocations]
    assert len(jumps)==1;jump=jumps[0]
    table,kind=relocations[jump+3]
    assert kind==6 and table['section']==owner['section']and table['storage']==6
    assert code[jump+3:jump+7]==b'\0'*4
    selectors=[at for at in range(begin,jump-5)if code[at:at+2]==b'\x8a\x81'and at+2 in relocations]
    assert len(selectors)==1;selector=selectors[0];remap,kind=relocations[selector+2]
    assert kind==6 and remap['section']==owner['section']and remap['storage']==6
    assert code[selector+2:selector+6]==b'\0'*4 and selector+6==jump
    assert code[selector-2:selector]==b'\x33\xc0' # zero-extend AL's remapped index
    # cmp DWORD[ebp+disp8],imm8; JA default bounds the byte selector.
    guards=[at for at in range(begin,selector-9)if code[at:at+2]==b'\x83\x7d'and code[at+4:at+6]==b'\x0f\x87']
    assert len(guards)==1;guard=guards[0];slots=code[guard+3]+1
    assert slots==39 and remap['value']+slots==end
    indices=code[remap['value']:end];targets_count=max(indices)+1
    assert targets_count==8 and table['value']+4*targets_count==remap['value']
    assert begin<jump<jump+7<=table['value']<remap['value']<end
    mapping=map_path.read_text()
    def address(symbol):
        rows=re.findall(r'^\s*\w+:\w+\s+'+re.escape(symbol)+r'\s+([0-9a-fA-F]+)\s+f\s+preprocess\.obj\s*$',mapping,re.M)
        assert len(rows)==1;return int(rows[0],16)
    base=address('_rc_apply_option@12')-begin
    assert address('_rc_option_number@4')==base+end
    from importlib.util import spec_from_file_location,module_from_spec
    spec=spec_from_file_location('compiler_support',HERE/'scripts/check-compiler-support.py')
    module=module_from_spec(spec);spec.loader.exec_module(module)
    image_base,read=module.image(rebuilt_path)
    targets=[]
    for index in range(targets_count):
        at=table['value']+index*4;target,kind=relocations[at]
        assert kind==6 and target['section']==owner['section']and target['storage']==6
        addend=struct.unpack_from('<I',code,at)[0];value=base+target['value']+addend
        assert base+begin<=value<base+table['value']
        assert struct.unpack('<I',read(base+at-image_base,4))[0]==value
        targets.append(value)
    assert read(base+remap['value']-image_base,slots)==indices
    # Only these two proven tables are data. Rust still audits all code and gaps.
    return dict(object_sha256=sha(path),rebuilt_sha256=sha(rebuilt_path),map_sha256=sha(map_path),
        owner='rc_apply_option',object_section=owner['section'],object_function=[begin,end],
        object_table=[table['value'],remap['value']],object_remap=[remap['value'],end],
        selector_slots=slots,target_slots=targets_count,indices=list(indices),
        text_data=[[base+table['value'],base+end]],switches={str(base+jump):sorted(set(targets))})
