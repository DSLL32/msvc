#!/usr/bin/env python3
"""Validate C1's encountered STRINGTABLE source and encode its native provider."""
import hashlib, importlib.util, json, re, struct, sys
from pathlib import Path
HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[2]
OWNER="7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d"
image=(ROOT/"toolchains/5.10/bin/C1.DLL").read_bytes()
if hashlib.sha256(image).hexdigest()!=OWNER:raise SystemExit("C1 resource identity mismatch")
# Authenticated RT_STRING block/file spans, language1033. Each leaf contains16 UTF16 strings.
LEAVES={19: (378928, 46), 20: (378976, 264), 21: (379240, 186), 22: (379428, 590), 23: (380020, 372), 24: (380392, 350), 25: (380744, 424), 26: (381168, 596), 57: (381764, 868), 132: (443280, 934), 258: (404176, 1388)}
original={}
for block,(first,length) in LEAVES.items():
    cursor=first;end=first+length
    for index in range(16):
        if cursor+2>end:raise SystemExit("STRINGTABLE length outside leaf")
        size=struct.unpack_from("<H",image,cursor)[0];cursor+=2
        if cursor+2*size>end:raise SystemExit("STRINGTABLE text outside leaf")
        original[(block-1)*16+index]=image[cursor:cursor+2*size].decode("utf-16le");cursor+=2*size
    if cursor!=end:raise SystemExit("STRINGTABLE leaf endpoint mismatch")
header=(HERE/"src/compiler-message-ids.h").read_text()
ids={name:int(value)for name,value in re.findall(r"^#define (C1_TEXT_\w+)\s+(\d+)\s*$",header,re.M)}
source=(HERE/"src/compiler-messages.rc").read_text()
if "LANGUAGE 9, 1"not in source:raise SystemExit("C1 resource language mismatch")
entries=[]
for name,literal in re.findall(r'^\s*(C1_TEXT_\w+)\s+("[^\n]*")\s*$',source,re.M):
    number=ids[name];text=json.loads(literal)
    if original.get(number)!=text:raise SystemExit("C1 resource source differs: "+name)
    entries.append(dict(kind="string",id=number,language=1033,text=text))
if len(entries)!=88 or len({x["id"]for x in entries})!=88 or set(ids.values())!={x["id"]for x in entries}:
    raise SystemExit("C1 encountered resource inventory mismatch")
spec=importlib.util.spec_from_file_location("resources",ROOT/"lib/recon/resources.py")
resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
resources.write_resources(sys.argv[1],OWNER,entries)
