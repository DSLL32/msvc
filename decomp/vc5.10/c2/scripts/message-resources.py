#!/usr/bin/env python3
"""Validate C2's encountered STRINGTABLE source and encode its native resources."""
import hashlib
import importlib.util
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[2]
OWNER = "e75aecaf4073b0817ffb638cae5fff636b2e2d1a090daabf8f68dbc954515fae"
image = (ROOT / "toolchains/5.10/bin/C2.EXE").read_bytes()
if hashlib.sha256(image).hexdigest() != OWNER:
    raise SystemExit("C2 resource identity mismatch")
# Actual RT_STRING leaves in the shipped PE, language1033. Each contains16 strings.
LEAVES = {19: (647144, 78), 20: (647224, 122), 63: (647352, 1162)}
original = {}
for block, (first, length) in LEAVES.items():
    cursor, end = first, first + length
    for index in range(16):
        if cursor + 2 > end:
            raise SystemExit("C2 STRINGTABLE length outside leaf")
        size = struct.unpack_from("<H", image, cursor)[0]
        cursor += 2
        if cursor + 2 * size > end:
            raise SystemExit("C2 STRINGTABLE text outside leaf")
        original[(block - 1) * 16 + index] = image[cursor:cursor + 2 * size].decode("utf-16le")
        cursor += 2 * size
    if cursor != end:
        raise SystemExit("C2 STRINGTABLE leaf endpoint mismatch")
source = (HERE / "src/messages.rc").read_text()
if "LANGUAGE 9, 1" not in source:
    raise SystemExit("C2 resource language mismatch")
entries = []
for number, literal in re.findall(r'^\s*(\d+)\s+("[^\n]*")\s*$', source, re.M):
    identity, text = int(number), json.loads(literal)
    if original.get(identity) != text:
        raise SystemExit("C2 resource source differs: " + number)
    entries.append(dict(kind="string", id=identity, language=1033, text=text))
if len(entries) != 7 or {entry["id"] for entry in entries} != {301, 302, 303, 310, 311, 312, 1007}:
    raise SystemExit("C2 encountered resource inventory mismatch")
spec = importlib.util.spec_from_file_location("resources", ROOT / "lib/recon/resources.py")
resources = importlib.util.module_from_spec(spec)
spec.loader.exec_module(resources)
resources.write_resources(sys.argv[1], OWNER, entries)
