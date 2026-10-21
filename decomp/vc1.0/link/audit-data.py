#!/usr/bin/env python3
"""Check encountered C data definitions, normalizing their named pointers."""
import json,struct
from pathlib import Path
from binary import PE,u16,u32
from compare import publics
from importlib.machinery import SourceFileLoader
wt=SourceFileLoader('wine_tool','wine-tool.py').load_module()
a,b=PE(wt.TC/'bin/LINK.EXE'),PE('build/link.exe')
symbols=json.loads(Path('data-symbols.json').read_text());names=publics()
def read(pe,address,size):
    sec=next(s for s in pe.sections if pe.base+s['rva']<=address<pe.base+s['rva']+s['virtual_size'])
    return pe.read(address,size) if sec['off'] else bytes(size)
def fixups(pe):
    result=set();p=pe.offset(pe.base+u32(pe.data,pe.optional+136));end=p+u32(pe.data,pe.optional+140)
    while p<end:
        page,size=u32(pe.data,p),u32(pe.data,p+4)
        if not size:break
        for q in range(p+8,p+size,2):
            value=u16(pe.data,q)
            if value>>12==3:result.add(pe.base+page+(value&4095))
        p+=size
    return result
fix=fixups(a);count=0
for r in symbols:
    if r['owner'].endswith('.LIB'):continue
    name=r['name'];start=r['address'];size=r['size']
    expected=bytearray(read(a,start,size));actual=read(b,names['_'+name],size)
    for q in sorted(fix):
        if start<=q and q+4<=start+size:
            value=u32(expected,q-start)
            matches=[s for s in symbols if s['address']<=value<s['address']+s['size']]
            assert matches,'name pointed-to data %08x referenced by %s'%(value,name)
            target=min(matches,key=lambda s:s['size'])
            new=names['_'+target['name']]+value-target['address']
            struct.pack_into('<I',expected,q-start,new)
    assert actual==expected,'%s initializer differs: %s != %s'%(name,actual.hex(),expected.hex())
    count+=1
print('%d contextual data definitions match original initial values and named pointers.'%count)
