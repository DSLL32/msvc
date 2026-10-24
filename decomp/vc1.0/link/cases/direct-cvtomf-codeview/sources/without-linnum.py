"""Derive the versioned no-lines input by removing only optional LINNUM."""
from pathlib import Path
import hashlib,json,struct
case=Path(__file__).resolve().parent.parent
source=case/'inputs/legacy.obj';raw=source.read_bytes();records=[];removed=[];at=0
while at<len(raw):
    size=struct.unpack_from('<H',raw,at+1)[0]+3;record=raw[at:at+size]
    assert len(record)==size and sum(record)%256==0
    if record[0] in (0x94,0x95):removed.append(dict(offset=at,bytes=size,hex=record.hex()))
    else:records.append(record)
    at+=size
assert at==len(raw) and len(removed)==1
result=b''.join(records);destination=case/'variants/no-lines/inputs';destination.mkdir(parents=True,exist_ok=True)
(destination/'legacy.obj').write_bytes(result)
layout=dict(variant='no-lines-v1',derivation='Remove ONLY optional LINNUM; all other complete OMF records remain byte-exact',source_sha256=hashlib.sha256(raw).hexdigest(),source_layout_sha256=hashlib.sha256((case/'inputs/layout.json').read_bytes()).hexdigest(),bytes=len(result),sha256=hashlib.sha256(result).hexdigest(),OMF_records=len(records),removed_records=removed,unchanged=dict(types=17,symbols=16,code=34,data=100,ordinary_fixups=8,debug_fixups=14))
(destination/'layout.json').write_text(json.dumps(layout,indent=2)+'\n')
print(len(result),len(records),layout['sha256'])
