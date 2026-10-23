"""Verify encountered compiler switch data from COFF relocations and linked bytes."""
import json
import re
import struct
from state import BINARY, ROOT, sha
from binary import COFF, PE, u32


def prepare(checked,destination):
    metadata=dict(schema=1,functions={});proof=dict(schema=1,tables=[])
    if any(row['name']=='pack_module' for row in checked):
        path=BINARY/'build/pack.obj';obj=COFF(path.read_bytes())
        symbols={row['name']:row for row in obj.symbols.values()}
        function=symbols['_pack_module'];following=symbols['_initialize_single_module']
        assert function['section']==following['section'] and function['type']==following['type']==0x20
        section=obj.sections[function['section']-1];code=section['data']
        maps=[];jumps=[]
        for offset,target,kind in section['relocs']:
            if not function['value']<=offset<following['value'] or kind!=6 or target['section']!=function['section']:continue
            if code[offset-2:offset]==b'\x8a\x90':maps.append((offset,target))
            if code[offset-3:offset]==b'\xff\x24\x95':jumps.append((offset,target))
        assert len(maps)==len(jumps)==1,'switch operand COFF proof changed'
        map_operand,classes=maps[0];jump_operand,table=jumps[0]
        assert u32(code,map_operand)==u32(code,jump_operand)==0
        start=table['value'];middle=classes['value'];end=middle+15
        assert start+6*4==middle and end==following['value'] and code[start-1]==0xc3,'switch object boundaries changed'
        rows=[row for row in section['relocs'] if start<=row[0]<middle]
        assert [row[0] for row in rows]==list(range(start,middle,4))
        publics={}
        for line in (BINARY/'build/cvpack.map').read_text().splitlines():
            match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
            if match:publics[match[1]]=int(match[2],16)
        base=publics['_pack_module']-function['value']
        pe=PE(BINARY/'build/CVPACK.EXE');original=PE(ROOT/'toolchains/5.10/bin/CVPACK.EXE')
        assert publics['_initialize_single_module']==base+end
        remap=code[middle:end]
        assert len(remap)==15 and all(index<6 for index in remap)
        assert remap==original.read(0x404ec8,15)==pe.read(base+middle,15),'class map contents changed'
        targets=[]
        for offset,target,kind in rows:
            assert kind==6 and target['section']==function['section'] and function['value']<=target['value']<start
            address=base+target['value']+u32(code,offset)
            assert pe.read(base+offset,4)==struct.pack('<I',address),'linked switch target/COFF relocation differs'
            targets.append(address)
        assert pe.read(base+map_operand,4)==struct.pack('<I',base+middle)
        assert pe.read(base+jump_operand,4)==struct.pack('<I',base+start)
        metadata['functions']['_pack_module']=dict(rebuilt=dict(text_data=[[base+start,base+end]],switches={str(base+jump_operand-3):targets}))
        frozen=destination/'pack-switch.obj';frozen.write_bytes(path.read_bytes())
        proof['tables'].append(dict(owner='_pack_module',object_sha256=sha(frozen),object='pack-switch.obj',
            rebuilt_sha256=sha(BINARY/'build/CVPACK.EXE'),map_sha256=sha(BINARY/'build/cvpack.map'),
            coff_function=function,coff_next_function=following,coff_pointer_table=table,coff_class_map=classes,
            pointer_relocations=[dict(offset=offset,symbol=target,kind=kind) for offset,target,kind in rows],
            rebuilt_data=[base+start,base+end],rebuilt_targets=targets,class_map=remap.hex(),
            original_class_map=[0x404ec8,0x404ed7]))
    (destination/'graph-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    (destination/'graph-metadata-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    return destination/'graph-metadata.json',destination/'graph-metadata-proof.json'
