#!/usr/bin/env python3
"""Explicit COFF fixture metadata edits; executable instructions stay compiler output.

Add one documented weak-external auxiliary record to each of three ordinary
undefined functions (native characteristic1/2/3). Remap relocation symbol indexes;
point each fallback at the same compiled definition. The companion object gives
one external an absolute section/value for public /absolutesfrom input.
"""
from pathlib import Path
import struct,sys

def patch(source,output,weak):
    image=bytearray(source.read_bytes())
    machine,sections,stamp,offset,count,optional,flags=struct.unpack_from('<HHIIIHH',image)
    assert machine==0x14c and not optional
    strings=image[offset+count*18:]
    assert len(strings)==struct.unpack_from('<I',strings)[0]
    entries=[];index=0
    while index<count:
        record=bytearray(image[offset+index*18:offset+(index+1)*18]);aux=record[17]
        short,long=struct.unpack_from('<II',record)
        if short:name=record[:8].split(b'\0')[0].decode('ascii')
        else:name=strings[long:strings.index(0,long)].decode('ascii')
        entries.append(dict(old=index,record=record,name=name,aux=bytes(image[offset+(index+1)*18:offset+(index+1+aux)*18])))
        index+=aux+1
    wanted={'_weak_name':1,'_lazy_name':2,'_alias_name':3}
    assert {item['name'] for item in entries}>=(set(wanted)|{'_default_target'} if weak else {'_absolute_value'})
    mapping={};new_index=0
    for item in entries:
        mapping[item['old']]=new_index;new_index+=1+item['record'][17]+int(weak and item['name'] in wanted)
    target=next((mapping[item['old']] for item in entries if item['name']=='_default_target'),None)
    symbols=bytearray()
    for item in entries:
        record=item['record'];aux=item['aux']
        if weak and item['name'] in wanted:
            assert record[17]==0 and record[16]==2 and struct.unpack_from('<h',record,12)[0]==0
            record[16]=105;record[17]=1
            aux=struct.pack('<II10x',target,wanted[item['name']])
        elif not weak and item['name']=='_absolute_value':
            struct.pack_into('<Ih',record,8,0x1234,-1)
        symbols+=record+aux
    if weak:
        for section in range(sections):
            start=20+40*section;relocs=struct.unpack_from('<I',image,start+24)[0];number=struct.unpack_from('<H',image,start+32)[0]
            for i in range(number):
                where=relocs+10*i+4;old=struct.unpack_from('<I',image,where)[0];struct.pack_into('<I',image,where,mapping[old])
    struct.pack_into('<I',image,12,new_index)
    output.write_bytes(image[:offset]+symbols+strings)

if __name__=='__main__':
    directory=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
    patch(directory/'weak.obj',out/'weak.obj',True)
    patch(directory/'values.obj',out/'values.obj',False)
    if len(sys.argv)>3:
        assert sys.argv[3]=='--unmatched'
        data=(out/'values.obj').read_bytes()
        assert data.count(b'_absolute_value')==1
        (out/'values.obj').write_bytes(data.replace(b'_absolute_value',b'_unmatched_name'))
