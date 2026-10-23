#!/usr/bin/env python3
"""Generate coherent source bitmap assets; never run by native fixtures."""
import argparse,hashlib,json,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
manifest=[]
for bits in (4,8):
 width,height=8,4;stride=((width*bits+31)//32)*4;colors=1<<bits
 palette=[]
 for i in range(colors):
  color=(0,0x808080,0xaaaaaa,0xffffff,0x555555)[i] if i<5 else ((i*17%256)<<16)|((i*53%256)<<8)|(i*97%256)
  palette.append(struct.pack('<I',color))
 indices=([0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]*2 if bits==4 else [0,1,2,3,4,8,16,31,63,64,95,127,128,159,191,223,255,254,200,150,100,50,25,12,6,5,4,3,2,1,0,255])
 pixels=bytearray()
 for y in range(height):
  row=indices[y*width:(y+1)*width]
  packed=bytes(row) if bits==8 else bytes((row[n]<<4)|row[n+1] for n in range(0,width,2))
  pixels+=packed+b'\0'*(stride-len(packed))
 dib=struct.pack('<IiiHHIIiiII',40,width,height,1,bits,0,len(pixels),0,0,0,0)+b''.join(palette)+pixels
 data=struct.pack('<2sIHHI',b'BM',14+len(dib),0,0,14+40+colors*4)+dib
 name='palette%d.bmp'%bits;(a.output/name).write_bytes(data)
 manifest.append(dict(name=name,bits=bits,width=width,height=height,stride=stride,palette_entries=colors,pixel_indices=indices,sha256=hashlib.sha256(data).hexdigest(),bytes=len(data)))
(a.output/'payload.bin').write_bytes(b'CVTRES final resource validation\0\x01\x02\xfe\xff')
(a.output/'palettes.rc').write_text('LANGUAGE 9, 1\n104 BITMAP "palette4.bmp"\n105 BITMAP "palette8.bmp"\n"NamedData" "CUSTOMTYPE" "payload.bin"\n107 256 "payload.bin"\n"NamedKnown" RCDATA "payload.bin"\n')
(a.output/'assets.json').write_text(json.dumps(dict(schema=1,bitmaps=manifest),indent=2)+'\n')
print(json.dumps(dict(output=str(a.output),bitmaps=manifest),indent=2))
