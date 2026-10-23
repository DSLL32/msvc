#!/usr/bin/env python3
"""Retain immutable compiled-source content with the resulting artifact identity."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
p=argparse.ArgumentParser();p.add_argument('--kind',required=True);p.add_argument('--binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
files=[BINARY/'Makefile',BINARY/'scripts/native-entry.c',BINARY/'scripts/capture-host.c']
for directory in [BINARY/'src',ROOT/'lib/recon/include',ROOT/'lib/recon/native',ROOT/'lib/recon/src']:
 files.extend(f for f in directory.rglob('*') if f.is_file() and f.suffix in ('.c','.h','.rc'))
records={}
for file in sorted(set(files)):
 data=file.read_bytes();records[str(file.relative_to(ROOT))]=dict(sha256=hashlib.sha256(data).hexdigest(),content_base64=base64.b64encode(data).decode())
x=dict(schema=1,kind=a.kind,binary_sha256=hashlib.sha256(a.binary.read_bytes()).hexdigest(),files=records)
a.output.write_text(json.dumps(x,indent=2)+'\n')
