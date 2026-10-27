#!/usr/bin/env python3
"""Encode only encountered, source-verified ANSI message-table entries."""
import importlib.util
import struct
from pathlib import Path
HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[2]
entries=[]
(HERE/'.build').mkdir(exist_ok=True)
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
assert set(messages)=={312,313,314,1003,1107,2101,2104,2170,2175,20101,2109,2135,2151,2171,4067,4213,4506,4509,10001,20001}
for identity,text in sorted(messages.items()):
    entries.append(dict(kind='message',id=identity,language=1033,text=text))
spec=importlib.util.spec_from_file_location('resources',ROOT/'lib/recon/resources.py')
resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
resources.write_resources(HERE/'.build/help-resources.bin','bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0',entries)

# Standard MESSAGE_RESOURCE_DATA: encountered one-ID blocks, ANSI aligned entries.
records=[];blocks=[];offset=4+12*len(entries)
for entry in entries:
    payload=entry['text']+b'\0'
    size=(4+len(payload)+3)&~3
    record=struct.pack('<HH',size,0)+payload+b'\0'*(size-4-len(payload))
    blocks.append(struct.pack('<III',entry['id'],entry['id'],offset))
    records.append(record);offset+=len(record)
(HERE/'.build/help-messages.bin').write_bytes(struct.pack('<I',len(entries))+b''.join(blocks)+b''.join(records))
