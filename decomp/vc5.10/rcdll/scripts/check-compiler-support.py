#!/usr/bin/env python3
"""Read-only identity/ownership proof for the provided compiler's stack probe."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from protocol import HERE,ROOT,sha

def image(path):
    b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0];o=pe+24
    assert b[:2]==b'MZ'and b[pe:pe+4]==b'PE\0\0'
    base=struct.unpack_from('<I',b,o+28)[0]
    sections=[struct.unpack_from('<IIII',b,o+224+i*40+8)for i in range(struct.unpack_from('<H',b,pe+6)[0])]
    def read(rva,size):
        found=[b[raw+rva-start:raw+rva-start+size]for _,start,n,raw in sections if start<=rva and rva+size<=start+n]
        assert len(found)==1 and len(found[0])==size
        return found[0]
    return base,read

def verify(rebuilt=True):
    library=ROOT/'toolchains/5.10/lib/MSVCRT.LIB'
    members=subprocess.check_output(['llvm-ar','t',str(library)],text=True).splitlines()
    selected=[name for name in members if name.lower().endswith('chkstk.obj')];assert len(selected)==1
    member=selected[0];obj=subprocess.check_output(['llvm-ar','p',str(library),member])
    machine,count,_,symbol_offset,symbol_count,optional,_=struct.unpack_from('<HHIIIHH',obj,0)
    assert machine==0x14c and optional==0
    sections=[]
    for i in range(count):
        at=20+40*i;name=obj[at:at+8].rstrip(b'\0').decode();size,raw,reloc,_,relocations,_=struct.unpack_from('<IIIIHH',obj,at+16)
        sections.append(dict(name=name,size=size,offset=raw,relocations=relocations))
    text=[row for row in sections if row['name']=='.text'];assert len(text)==1
    text=text[0];assert text['size']==47 and text['relocations']==0
    body=obj[text['offset']:text['offset']+47]
    strings=symbol_offset+18*symbol_count;names=[];i=0
    while i<symbol_count:
        at=symbol_offset+18*i;raw=obj[at:at+8]
        if raw[:4]==b'\0'*4:
            offset=struct.unpack_from('<I',raw,4)[0];start=strings+offset;name=obj[start:obj.index(b'\0',start)].decode()
        else:name=raw.rstrip(b'\0').decode()
        value,section,kind,storage,aux=struct.unpack_from('<IhHBB',obj,at+8)
        if storage==2 and section>0:names.append((name,value,section))
        i+=1+aux
    assert sorted(name for name,value,section in names if value==0)==['__alloca_probe','__chkstk']
    _,old=image(ROOT/'toolchains/5.10/bin/RCDLL.DLL')
    assert old(0x12df0,47)==body
    assert old(0x856c,10)[:5]==b'\xb8\xc4\x42\0\0'
    displacement=struct.unpack_from('<i',old(0x856c,10),6)[0]
    assert 0x8576+displacement==0x12df0
    config=json.loads((HERE/'functions.json').read_text())
    for function in config['functions']:
        start,end=int(function['start'],16),int(function['end'],16)
        assert end<=0x12df0 or start>=0x12e1f,'overlapping compiler/helper ownership'
    report=dict(kind='canonical compiler support, not application C reconstruction',name='__chkstk',aliases=['__alloca_probe'],start='0x12df0',end='0x12e1f',code_bytes=47,body_sha256=hashlib.sha256(body).hexdigest(),relocations=0,library=str(library.relative_to(ROOT)),library_sha256=sha(library),member=member,member_sha256=hashlib.sha256(obj).hexdigest(),abi='CUSTOM EAX=size, ret0 with ESP=entryESP+4-size; ECX/callee-preserved registers restored; EAX=returnPC',original_caller='0x8571',original_return='0x8576',original_entry_eax='0x42c4',corrected_analysis='Earlier0x42c0 was a decompiler local-label inference; actual original prologue and epilogue use0x42c4',scope='only normal non-overflow caller; no Windows stack-overflow or SEH equivalence claimed',native='compiler-owned parent frame allocation; filtered CRT helper, no fake C call/trap/ABI claim')
    if rebuilt:
        base,new=image(HERE/'build/RCDLL.DLL');map_text=(HERE/'build/rcdll.map').read_text()
        addresses=re.findall(r'\s__chkstk\s+([0-9a-fA-F]+)\s',map_text);assert len(addresses)==1
        address=int(addresses[0],16);assert new(address-base,47)==body
        report['rebuilt']=dict(address=hex(address),end=hex(address+47),map_sha256=sha(HERE/'build/rcdll.map'),pe_sha256=sha(HERE/'build/RCDLL.DLL'))
    return report
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path);a=p.parse_args();report=verify()
    if a.output:a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
