#!/usr/bin/env python3
"""Append one exact RC-produced record for each bounded duplicate case."""
import argparse,hashlib,json,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--input',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();data=a.input.read_bytes();sha=lambda b:hashlib.sha256(b).hexdigest()
assert sha(data)=='fffd75070f25f2ad6952aef7bf4fecf38b03089ba608c50baa8a335dbf5b8606'
def identifier(offset):
 value=struct.unpack_from('<H',data,offset)[0]
 if value==65535:return struct.unpack_from('<H',data,offset+2)[0],offset+4
 start=offset
 while struct.unpack_from('<H',data,offset)[0]:offset+=2
 return data[start:offset].decode('utf-16le'),offset+2
records=[];offset=0
while offset<len(data):
 size,header_size=struct.unpack_from('<II',data,offset);kind,cursor=identifier(offset+8);name,cursor=identifier(cursor);cursor=(cursor+3)&~3
 assert cursor+16==offset+header_size
 language=struct.unpack_from('<H',data,cursor+6)[0];end=(offset+header_size+size+3)&~3
 assert end<=len(data);records.append(dict(start=offset,end=end,type=kind,name=name,language=language,data_size=size,header_size=header_size));offset=end
assert len(records)==6
selected={'duplicate-ordinal':next(r for r in records if r['type']==2 and r['name']==104),'duplicate-named':next(r for r in records if isinstance(r['type'],str) and r['type'].strip(chr(34))=='CUSTOMTYPE'),'duplicate-unknown':next(r for r in records if r['type']==256)}
a.output.mkdir(exist_ok=True,parents=True);result=[]
for case,record in selected.items():
 output=data+data[record['start']:record['end']];path=a.output/(case+'.res');path.write_bytes(output);result.append(dict(case=case,input_sha256=sha(data),copied_record=record,copied_record_sha256=sha(data[record['start']:record['end']]),output_sha256=sha(output),bytes=len(output),operation='Exact complete RC record appended, retaining its original header, payload and alignment.'))
(a.output/'duplicate-inputs.json').write_text(json.dumps(dict(schema=1,records=records,cases=result),indent=2)+'\n');print(json.dumps(result,indent=2))
