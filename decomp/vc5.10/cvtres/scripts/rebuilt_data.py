"""Explicit content certificate for VC5's two-stage read_pixel switch tables."""
import re,struct
from binary import COFF,u32

def read_pixel_switch(pe,objbytes,maptext):
    start=int(re.search(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+_read_pixel@12\s+([0-9a-f]{8})',maptext,re.M)[1],16)
    obj=COFF(objbytes);symbol=next(s for s in obj.symbols.values() if s['name']=='_read_pixel@12');section=obj.sections[symbol['section']-1]
    table,classes=start+0x115,start+0x12d
    assert pe.read(start+0x1a,4)==b'\x83\x7d\xf8\x17','read_pixel switch bound changed'
    assert pe.read(start+0x1e,2)==b'\x0f\x87' and start+0x24+struct.unpack('<i',pe.read(start+0x20,4))[0]==start+0x10f,'read_pixel default branch changed'
    assert pe.read(start+0x29,2)==b'\x8a\x90' and u32(pe.read(start+0x2b,4),0)==classes,'read_pixel classmap load changed'
    assert pe.read(start+0x2f,3)==b'\xff\x24\x95' and u32(pe.read(start+0x32,4),0)==table,'read_pixel switch jump changed'
    assert pe.read(start+0x112,3)==b'\xc2\x0c\x00','read_pixel terminal RET changed'
    expected_classes=bytes({1:0,2:1,4:2,8:3,24:4}.get(bits,5) for bits in range(1,25))
    assert pe.read(classes,24)==expected_classes,'read_pixel classmap contents changed'
    assert section['data'][symbol['value']+0x12d:symbol['value']+0x145]==expected_classes,'COFF classmap differs'
    targets=list(struct.unpack('<6I',pe.read(table,24)))
    assert targets==[start+offset for offset in (0xe7,0xb2,0x7b,0x6c,0x36,0x10f)],'read_pixel case table contents changed'
    relocations=[]
    for index,target in enumerate(targets):
        offset=symbol['value']+0x115+4*index
        found=[(s,k) for address,s,k in section['relocs'] if address==offset]
        assert len(found)==1
        local,kind=found[0]
        assert kind==6 and local['section']==symbol['section'] and local['storage']==6,'case entry is not a local DIR32 label'
        assert target==start+local['value']-symbol['value'],'COFF case relocation differs from linked table'
        relocations.append(dict(index=index,object_offset=offset,label=local['name'],target=target))
    metadata=dict(text_data=[[table,classes],[classes,classes+24]],switches={str(start+0x2f):targets})
    return metadata,dict(function='_read_pixel@12',start=start,table=table,classmap=classes,classmap_bytes=list(expected_classes),targets=targets,relocations=relocations,scope='Exact fixed opcode/bound/classmap/table/COFF relocation proof; Rust owns CFG traversal.')
