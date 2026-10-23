from pathlib import Path
import struct,sys,json,hashlib

def build(sections,symbols):
 cursor=20+40*len(sections);raws=[];rels=[]
 for name,data,flags,rr in sections:raws.append(cursor);cursor+=len(data)
 for name,data,flags,rr in sections:rels.append(cursor if rr else 0);cursor+=len(rr)*10
 symoff=cursor;strings=bytearray(4);syms=bytearray()
 for name,value,section,kind,storage in symbols:
  encoded=name.encode();field=encoded.ljust(8,b'\0') if len(encoded)<=8 else struct.pack('<II',0,len(strings))
  if len(encoded)>8:strings+=encoded+b'\0'
  syms+=struct.pack('<8sIhHBB',field,value,section,kind,storage,0)
 struct.pack_into('<I',strings,0,len(strings))
 header=struct.pack('<HHIIIHH',0x268,len(sections),0,symoff,len(symbols),0,0)
 for i,(name,data,flags,rr) in enumerate(sections):header+=struct.pack('<8sIIIIIIHHI',name.encode(),0,0,len(data),raws[i],rels[i],0,len(rr),0,flags)
 return header+b''.join(s[1] for s in sections)+b''.join(struct.pack('<IIH',*r) for s in sections for r in s[3])+syms+strings

if __name__=='__main__':
 directory=Path(sys.argv[1]);directory.mkdir(parents=True,exist_ok=True)
 sections=[('.text',bytes.fromhex('4ead00004e75'),0x60000020,[(2,1,8)]),('.text2',bytes.fromhex('4e75'),0x60000020,[]),('.text3',bytes.fromhex('600000004e75'),0x60000020,[(2,2,21)]),('.data',bytes(10),0xc0000040,[(0,1,3),(4,1,1),(6,1,20)])]
 symbols=[('_main',0,1,0x20,2),('_helper',0,2,0x20,3),('_leaf',0,3,0x20,3)]
 data=build(sections,symbols);(directory/'mac-special.obj').write_bytes(data);(directory/'layout.json').write_text(json.dumps(dict(sections=[dict(name=n,data=d.hex(),flags=hex(f),relocations=r) for n,d,f,r in sections],symbols=symbols,bytes=len(data),sha256=hashlib.sha256(data).hexdigest()),indent=2)+'\n')
