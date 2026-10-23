#!/usr/bin/env python3
"""Explicit current validator refresh of immutable captured inputs; never runs Wine."""
import argparse
import json
from pathlib import Path
from audit import run
from state import BINARY, trial_configuration, sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    _,expected=trial_configuration((BINARY/'functions.json').read_bytes())
    assert (a.original/'configuration.json').read_bytes()==expected,'captured trial differs from current candidate'
    assert (a.original/'base_configuration.json').read_bytes()==(BINARY/'functions.json').read_bytes(),'base configuration changed'
    a.output.mkdir(parents=True,exist_ok=False)
    receipt=run(a.original/'configuration.json',a.output/'graph-audit.json')
    receipt['original_execution_receipt_sha256']=sha((a.original/'receipt.json').read_bytes())
    (a.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))

if __name__=='__main__':main()
