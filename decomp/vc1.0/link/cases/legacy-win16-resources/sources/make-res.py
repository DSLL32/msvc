"""Construct Win16 .res input records, never reconstruction implementation.

Outer records follow target read_resource_name/convert_resource_file evidence.
Dialog layout follows Microsoft's 16-bit classic dialog template description:
https://devblogs.microsoft.com/oldnewthing/20040618-00/?p=38803
All fields are little-endian; Win16 resource records are unaligned.
"""
from pathlib import Path
import struct,json
HERE=Path(__file__).resolve().parent
w=lambda n:struct.pack('<H',n)
d=lambda n:struct.pack('<I',n)
def z(s):return s.encode('cp1252')+b'\0'
def name(n):return b'\xff'+w(n) if isinstance(n,int) else z(n)
def resource(t,n,payload):return name(t)+name(n)+w(0x1030)+d(len(payload))+payload
bitmap=d(40)+d(8)+d(2)+w(1)+w(1)+d(0)+d(8)+d(0)+d(0)+d(2)+d(0)+bytes.fromhex('00000000ffffff00aa00000055000000')
menu=w(0)+w(0)+w(0x90)+z('&File')+w(0)+w(101)+z('&Open')+w(0x80)+w(102)+z('E&xit')
def control(x,y,cx,cy,ident,style,cls,text):return b''.join(map(w,[x,y,cx,cy,ident]))+d(style)+cls+name(text)+b'\0'
dialog=d(0x90c800c0)+b'\x02'+w(10)+w(10)+w(160)+w(75)+name(103)+b'\0'+z('Legacy dialog')+w(9)+z('Arial')
dialog+=control(8,8,140,12,201,0x50000000,b'\x82','Win16 resource conversion')
dialog+=control(55,45,50,14,1,0x50010001,z('BUTTON'),'OK')
strings=['Alpha','Beta','Legacy strings']+['']*12+['Last entry']
stringtable=b''.join(bytes([len(s.encode('cp1252'))])+s.encode('cp1252') for s in strings)
accelerators=b'\x09'+w(ord('N'))+w(101)+b'\x81'+w(0x70)+w(102)
items=[(2,101,bitmap),(4,103,menu),(5,104,dialog),(6,1,stringtable),(9,105,accelerators),(10,106,bytes(range(12))),('CUSTOM','NamedData',b'custom named resource\0')]
image=b''.join(resource(*item) for item in items)
(HERE.parent/'inputs/legacy.res').write_bytes(image)
(HERE.parent/'inputs/layout.json').write_text(json.dumps(dict(format='Win16 .res: type/name encoded ordinalFF+WORD or ANSIz, WORDmemoryflags, DWORDsize, unaligned payload',resources=[dict(type=t,name=n,bytes=len(p)) for t,n,p in items],provenance=['Target original read_resource_name0x42e10 and convert_resource_file0x432d0 outer record decoding','Microsoft Old New Thing 16-bit classic dialog templates,2004-06-18','Target original type-conversion dispatch table0x7ff98 and decoder field accesses; untouched-original success and repeat are required before acceptance']),indent=2)+'\n')
print(len(image),'Win16 resource bytes')
