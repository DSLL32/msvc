#!/usr/bin/env python3
"""Encode LINK's encountered string-table entry for the shared native loader."""
import hashlib
import importlib.util
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[4]
owner = "04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579"
image = (root / "toolchains/5.10/bin/LINK.EXE").read_bytes()
if hashlib.sha256(image).hexdigest() != owner:
    raise SystemExit("original LINK identity mismatch")
# Original RT_STRING blocks 563 and 564, language 1033. Each block holds
# sixteen length-prefixed UTF16 strings; select only the encountered IDs.
original_entries=[]
for message,first,last in [(9001,0x6c718,0x6c860),(9021,0x6c860,0x6cad8)]:
    cursor=first
    selected=None
    for index in range(16):
        length=int.from_bytes(image[cursor:cursor+2],"little")
        cursor+=2
        if cursor+length*2>last:
            raise SystemExit("original STRINGTABLE leaf bounds mismatch")
        text=image[cursor:cursor+length*2].decode("utf-16le")
        if index==message%16:selected=text
        cursor+=length*2
    if cursor!=last or selected is None:
        raise SystemExit("original STRINGTABLE leaf extent mismatch")
    original_entries.append((str(message),selected))
source = (Path(__file__).resolve().parents[1] / "src/messages.rc").read_text()
entries = re.findall(r'^\s*(\d+)\s+"([^"\n]*)"\s*$', source, re.M)
if entries != original_entries or "LANGUAGE 9, 1" not in source:
    raise SystemExit("encountered STRINGTABLE identity/text mismatch")
spec = importlib.util.spec_from_file_location("resources", root / "lib/recon/resources.py")
resources = importlib.util.module_from_spec(spec)
spec.loader.exec_module(resources)
resources.write_resources(sys.argv[1], owner,
                          [{"kind": "string", "id": int(message), "language": 1033, "text": text}
                           for message,text in original_entries])
