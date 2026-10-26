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
# Original RT_STRING block 563 / language 1033, index 9: ID 9001.
# The preceding UTF16 length and exact text are inside that hash-bound leaf.
raw = image[0x6c72a:0x6c774]
if int.from_bytes(raw[:2], "little") != 36:
    raise SystemExit("original resource length mismatch")
text = raw[2:].decode("utf-16le")
source = (Path(__file__).resolve().parents[1] / "src/messages.rc").read_text()
entries = re.findall(r'^\s*(\d+)\s+"([^"\n]*)"\s*$', source, re.M)
if entries != [("9001", text)] or "LANGUAGE 9, 1" not in source:
    raise SystemExit("encountered STRINGTABLE identity/text mismatch")
spec = importlib.util.spec_from_file_location("resources", root / "lib/recon/resources.py")
resources = importlib.util.module_from_spec(spec)
spec.loader.exec_module(resources)
resources.write_resources(sys.argv[1], owner,
                          [{"kind": "string", "id": 9001, "language": 1033, "text": text}])
