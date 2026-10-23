#!/usr/bin/env python3
"""Small content receipt; never treats later worktree drift as a Wine rerun rule."""
import argparse
import hashlib
import json
from pathlib import Path
HERE=Path(__file__).resolve().parents[1];ROOT=HERE.parents[2]
def write(path,kind):
    files=([HERE/'scripts/debug-original.c'] if kind=='debugger' else sorted((HERE/'src').glob('*'))+[HERE/'scripts/native-host.c'])
    files += [ROOT/'lib/recon/include/recon.h']
    if kind=='native':files+=sorted((ROOT/'lib/recon/src').glob('*.c'))+sorted((ROOT/'lib/recon/include').glob('*.h'))+sorted((ROOT/'lib/recon/native').rglob('*.h'))
    contents={}
    for file in files:
        if file.suffix not in ['.c','.h','.def','.mc','.rc']:continue
        data=file.read_bytes();contents[str(file.relative_to(ROOT))]=dict(sha256=hashlib.sha256(data).hexdigest(),text=data.decode())
    path.write_text(json.dumps(dict(kind=kind,contents=contents),indent=2)+'\n')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('kind',choices=['native','original','debugger']);p.add_argument('path',type=Path);a=p.parse_args();write(a.path,a.kind)
