#!/usr/bin/env python3
"""Narrow per-role SEH ABI check; shared Rust owns CFG discovery/comparison."""
import argparse
import json
from pathlib import Path
import re
import struct
from protocol import HERE,ROOT,sha

class Image:
    def __init__(self,path):
        self.path=path;self.data=path.read_bytes();self.pe=struct.unpack_from('<I',self.data,60)[0];self.optional=self.pe+24
        assert self.data[:2]==b'MZ'and self.data[self.pe:self.pe+4]==b'PE\0\0'
        self.base=struct.unpack_from('<I',self.data,self.optional+28)[0]
        self.sections=[struct.unpack_from('<IIII',self.data,self.optional+224+40*i+8)for i in range(struct.unpack_from('<H',self.data,self.pe+6)[0])]
    def offset(self,address,size=1):
        rva=address-self.base
        rows=[raw+rva-start for _,start,length,raw in self.sections if start<=rva and rva+size<=start+length]
        assert len(rows)==1;return rows[0]
    def read(self,address,size):
        at=self.offset(address,size);return self.data[at:at+size]
    def string(self,address):
        at=self.offset(address);return self.data[at:self.data.index(b'\0',at)].decode('ascii')
    def imported(self,slot):
        descriptor,size=struct.unpack_from('<II',self.data,self.optional+96+8)
        found=[]
        for offset in range(0,size,20):
            row=struct.unpack('<5I',self.read(self.base+descriptor+offset,20))
            if not any(row):break
            lookup,_,_,name,iat=row;module=self.string(self.base+name)
            for index in range(4096):
                value=struct.unpack('<I',self.read(self.base+(lookup or iat)+4*index,4))[0]
                if not value:break
                if self.base+iat+4*index==slot:
                    assert not value&0x80000000
                    found.append((module,self.string(self.base+value+2)))
        assert len(found)==1;return found[0]

def role(image,start,end):
    body=image.read(start,end-start)
    assert body[:6]==bytes.fromhex('558bec6aff68')and body[10]==0x68
    assert body[15:29]==bytes.fromhex('64a1000000005064892500000000')
    scope,exception=struct.unpack_from('<I',body,6)[0],struct.unpack_from('<I',body,11)[0]
    level,filter,handler=struct.unpack('<3I',image.read(scope,12))
    assert level==0xffffffff and start<filter<handler<end
    constant_filter=image.read(filter,handler-filter)
    assert constant_filter in [bytes.fromhex('6a0158c3'),bytes.fromhex('b801000000c3')],'local filter must return EAX1 with stack-neutral RET0'
    tails=[bytes.fromhex(prefix)for prefix in ['8b4df064890d000000005f5e5bc9c20800','8b4df064890d000000005f5e5b8be55dc20800']]
    sites=[(start+i,len(tail))for tail in tails for i in range(len(body)-len(tail)+1)if body[i:i+len(tail)]==tail]
    assert len(sites)==1,'parent must restore registration and return with RET8'
    parent_tail,tail_bytes=sites[0];parent_end=parent_tail+tail_bytes;assert handler<parent_tail<parent_end<=end
    assert image.read(handler,3)==bytes.fromhex('8b65e8'),'handler must restore saved parent ESP'
    assert bytes.fromhex('8965e8')in image.read(start,filter-start),'parent must save ESP in its registration frame'
    continuation=image.read(handler,parent_tail-handler)
    assert bytes.fromhex('834dfcff')in continuation or bytes.fromhex('c745fcffffffff')in continuation,'handler must join through its parent TryLevel cleanup'
    assert image.read(exception,2)==b'\xff\x25'
    slot=struct.unpack('<I',image.read(exception+2,4))[0]
    module,name=image.imported(slot)
    assert module.lower()=='msvcrt.dll'and name=='_except_handler3'
    return dict(start=start,end=parent_end,scope=scope,roots=[filter,handler],scope_fields=[level,filter,handler],parent_ret=dict(address=parent_end-3,cleanup=8),filter=dict(start=filter,end=handler,bytes=constant_filter.hex(),return_eax=1,cleanup=0,abi='compiler SEH filter; inherited parent registration, not ordinary cdecl'),handler=dict(start=handler,parent_esp_offset=-24,parent_try_level_offset=-4,role='internal parent-frame continuation'),exception_import=dict(thunk=exception,slot=slot,module=module,name=name),pe_sha256=sha(image.path))

def verify(original_path=None,rebuilt_path=None,map_path=None):
    original_path=original_path or ROOT/'toolchains/5.10/bin/RCDLL.DLL';rebuilt_path=rebuilt_path or HERE/'build/RCDLL.DLL';map_path=map_path or HERE/'build/rcdll.map'
    assert sha(original_path)=='bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0'
    old=role(Image(original_path),0x71c2bee2,0x71c2bfc4)
    mapping=map_path.read_text();rows=[(int(address,16),symbol)for symbol,address in re.findall(r'^\s*\w+:\w+\s+(\S+)\s+([0-9a-fA-F]+)\s+f\s',mapping,re.M)]
    start=[address for address,symbol in rows if symbol=='_rc_copy_macro_text@8'];assert len(start)==1;start=start[0]
    end=min(address for address,symbol in rows if address>start)
    new=role(Image(rebuilt_path),start,end)
    assert old['parent_ret']['cleanup']==new['parent_ret']['cleanup']==8 and old['filter']['cleanup']==new['filter']['cleanup']==0
    return dict(original=old,rebuilt=new,map_sha256=sha(map_path),scope='Whole enclosing C owner includes local filter/handler. Shared RET union check retained; these per-role residuals prevent swapped cleanup roles. Native execution is restricted to proved readable-memory copies; no native SEH claim.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path);a=p.parse_args();report=verify()
    if a.output:a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
