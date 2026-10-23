#!/usr/bin/env python3
"""Encode only encountered, source-verified ANSI message-table entries."""
import importlib.util
import json
import struct
from pathlib import Path
from protocol import HERE,ROOT,sha
metadata=json.loads((HERE/'cases/help/message-entries.json').read_text())
source=ROOT/'toolchains/5.10/bin/RCDLL.DLL'
assert sha(source)==metadata['source_sha256']
data=source.read_bytes();entries=[]
lines=(HERE/'src/help.mc').read_text(encoding='ascii').splitlines(keepends=True)
messages={}
index=0
while index<len(lines):
    line=lines[index].strip();index+=1
    if not line.startswith('MessageId='):continue
    identity=int(line.split('=',1)[1]);assert lines[index].strip()=='Language=English';index+=1
    body=[]
    while index<len(lines) and lines[index].strip()!='.':body.append(lines[index]);index+=1
    assert index<len(lines) and identity not in messages;index+=1
    messages[identity]=''.join(body).replace('\n','\r\n').encode('ascii')
assert set(messages)=={10001,20001}
for row in metadata['entries']:
    assert row['id'] in [10001,20001] and row['resource_type']==11 and row['resource_name']==1
    assert row['language']=='0x409' and row['entry_flags']==0
    at=row['file_offset'];size,flags=struct.unpack_from('<HH',data,at)
    assert size==row['entry_bytes'] and flags==0
    payload=data[at+4:at+size]
    assert payload.hex()==row['payload_hex']
    text,tail=payload.split(b'\0',1)
    assert not any(tail) and messages[row['id']]==text
    entries.append(dict(kind='message',id=row['id'],language=1033,text=text))
assert len(entries)==2
spec=importlib.util.spec_from_file_location('resources',ROOT/'lib/recon/resources.py')
resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
resources.write_resources(HERE/'build/help-resources.bin',metadata['source_sha256'],entries)

# Standard MESSAGE_RESOURCE_DATA: two one-ID blocks, ANSI aligned entries.
records=[];blocks=[];offset=4+12*len(entries)
for entry in entries:
    payload=entry['text']+b'\0'
    size=(4+len(payload)+3)&~3
    record=struct.pack('<HH',size,0)+payload+b'\0'*(size-4-len(payload))
    blocks.append(struct.pack('<III',entry['id'],entry['id'],offset))
    records.append(record);offset+=len(record)
(HERE/'build/help-messages.bin').write_bytes(struct.pack('<I',len(entries))+b''.join(blocks)+b''.join(records))
