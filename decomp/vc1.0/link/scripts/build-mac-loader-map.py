#!/usr/bin/env python3
"""Deterministic meaningful Mac COFF fixture, not reconstruction code.

Uses bundled NTIMAGE.H20/40/18-byte COFF layouts, original-proven machine0x268
legacy section flags and CTO/DUPCON relocation types. M68000 words are inspect-
only test input. Distinct long external names deliberately avoid the historical
original shared-short-name-buffer alias; no later artifact bytes are patched.
"""
from pathlib import Path
import hashlib,json,struct

def build():
    sections=[
        ('.text',bytes.fromhex('4ead00004ead000061000000610000004e754e75'),0x60000020,[(2,2,8),(6,3,8),(10,4,41),(14,5,41)]),
        ('.text2',bytes.fromhex('70014e7570024e75'),0x60000020,[]),
        ('.dupa',bytes.fromhex('4eb9000000004e75'),0x60000020,[(2,2,12)]),
        ('.dupb',bytes.fromhex('4eb90000000070034e75'),0x60000020,[(2,3,12)])]
    symbols=[('_main',0,1),('___Init32BitLoadSeg',18,1),('_helper_function_one',0,2),('_helper_function_two',4,2),('_copy_function_one',0,3),('_copy_function_two',0,4)]
    raw_at=20+40*len(sections);raw_offsets=[]
    for _,data,_,_ in sections:raw_offsets.append(raw_at);raw_at+=len(data)
    rel_at=raw_at;rel_offsets=[]
    for _,_,_,records in sections:rel_offsets.append(rel_at if records else 0);rel_at+=10*len(records)
    table=bytearray();strings=bytearray(4)
    for name,value,section in symbols:
        encoded=name.encode('ascii')
        if len(encoded)>8:field=struct.pack('<II',0,len(strings));strings+=encoded+b'\0'
        else:field=encoded.ljust(8,b'\0')
        table+=struct.pack('<8sIhHBB',field,value,section,0x20,2,0)
    struct.pack_into('<I',strings,0,len(strings))
    data=struct.pack('<HHIIIHH',0x268,len(sections),0,rel_at,len(symbols),0,0)
    for i,(name,raw,flags,relocs) in enumerate(sections):
        data+=struct.pack('<8sIIIIIIHHI',name.encode(),0,0,len(raw),raw_offsets[i],rel_offsets[i],0,len(relocs),0,flags)
    data+=b''.join(raw for _,raw,_,_ in sections)
    data+=b''.join(struct.pack('<IIH',*r) for _,_,_,records in sections for r in records)
    data+=table+strings
    proof=dict(machine=0x268,sections=[dict(name=n,data=raw.hex(),flags=hex(f),relocations=r) for n,raw,f,r in sections],symbols=symbols,bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),legacy_bits='Original preflight required; PE flags0x60000020 preserve legacy code/read/execute interpretation',runtime='No Macintosh instruction is executed')
    return data,proof
if __name__=='__main__':
    base=Path('cases/mac-loader-map/inputs');base.mkdir(parents=True,exist_ok=True)
    data,proof=build();(base/'loader-map.obj').write_bytes(data);(base/'layout.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(len(data),proof['sha256'])
