#!/usr/bin/env python3
"""Snapshot compiler inputs and reject source drift during a build."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from state import BINARY, ROOT, sha


def inputs(mode):
    if mode in ('helper','capture'):
        source='debug-run.c'if mode=='helper'else'capture-inputs.c'
        paths=[BINARY/'scripts'/source,BINARY/'Makefile']+list((ROOT/'lib/recon/include').glob('*.h'))
        return {str(p.relative_to(ROOT)):sha(p.read_bytes())for p in sorted(set(paths))if p.is_file()}
    paths=list((BINARY/'src').glob('*'))+[BINARY/'Makefile']
    if mode=='native':
        paths.append(BINARY/'scripts/native-host.c')
        for directory in ('include','native','src'):
            paths.extend((ROOT/'lib/recon'/directory).rglob('*.h'if directory!='src'else '*.c'))
    else:paths.extend((ROOT/'lib/recon/include').glob('*.h'))
    return {str(p.relative_to(ROOT)):sha(p.read_bytes())for p in sorted(set(paths))if p.is_file()}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('stage',choices=['begin','finish']);p.add_argument('mode',choices=['native','original','helper','capture']);p.add_argument('--binary',type=Path);a=p.parse_args()
    start=BINARY/'build'/a.mode/'input-start.json'
    if a.stage=='begin':
        records=inputs(a.mode);digest=sha(json.dumps(records,sort_keys=True).encode())
        destination=BINARY/'build/provenance'/a.mode/digest
        destination.mkdir(parents=True,exist_ok=True)
        for name,expected in records.items():
            path=destination/name
            if not path.exists():path.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,path)
            assert sha(path.read_bytes())==expected,'snapshot input changed while copying'
        record={'mode':a.mode,'inputs':records,'snapshot':str(destination),'input_set_sha256':digest}
        start.parent.mkdir(parents=True,exist_ok=True);start.write_text(json.dumps(record,indent=2)+'\n')
    else:
        record=json.loads(start.read_text())
        assert inputs(a.mode)==record['inputs'],'compiler inputs drifted during build; rebuild from stable inputs'
        record['binary_sha256']=sha(a.binary.read_bytes())
        (BINARY/'build'/a.mode/'input-provenance.json').write_text(json.dumps(record,indent=2)+'\n')

if __name__=='__main__':main()
