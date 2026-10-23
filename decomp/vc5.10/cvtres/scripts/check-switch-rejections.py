#!/usr/bin/env python3
"""Explicit rebuilt switch-content controls, using copies of linked PE bytes."""
import argparse,json,re,struct
from pathlib import Path
from binary import PE
from rebuilt_data import read_pixel_switch
p=argparse.ArgumentParser();p.add_argument('--audit',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
pe=PE(a.audit/'rebuilt.exe');obj=(a.audit/'pixels.obj').read_bytes();maptext=(a.audit/'rebuilt.map').read_text();_,proof=read_pixel_switch(pe,obj,maptext);start=proof['start']
tests=[('classmap-content',start+0x12d,b'\x01'),('case-target-content',start+0x115,struct.pack('<I',start+0xb2)),('selector-bound',start+0x1d,b'\x18'),('table-load-identity',start+0x32,struct.pack('<I',start+0x119))]
results=[]
for name,pc,value in tests:
 changed=PE(a.audit/'rebuilt.exe');raw=bytearray(changed.data);offset=changed.offset(pc);raw[offset:offset+len(value)]=value;changed.data=bytes(raw)
 try:read_pixel_switch(changed,obj,maptext)
 except AssertionError as error:results.append(dict(name=name,rejected=True,reason=str(error)))
 else:raise AssertionError(name+' accepted')
a.output.write_text(json.dumps(dict(scope='Copied exact linked bytes; no rebuilt/original mutation or new execution',cases=results),indent=2)+'\n');print(json.dumps(results,indent=2))
