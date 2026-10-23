#!/usr/bin/env python3
"""Explicit current validator refresh of immutable captured inputs; never runs Wine."""
import argparse
import json
from pathlib import Path
from audit import run
from state import BINARY, trial_configuration, sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);p.add_argument('--pending-metadata-correction',type=Path);a=p.parse_args()
    _,expected=trial_configuration((BINARY/'functions.json').read_bytes())
    correction=None
    if a.pending_metadata_correction:
        assert a.native, 'static pending correction needs the complete historical pair'
        from state import load
        from pending_metadata import certificate
        _,original,inventory=load()
        correction=certificate(a.original,a.native,(BINARY/'functions.json').read_bytes(),a.pending_metadata_correction.resolve(),original,inventory)
    else:
        assert not a.native, '--native is only for an explicit pending correction'
        assert (a.original/'configuration.json').read_bytes()==expected,'captured trial differs from current candidate'
        assert (a.original/'base_configuration.json').read_bytes()==(BINARY/'functions.json').read_bytes(),'base configuration changed'
    a.output.mkdir(parents=True,exist_ok=False)
    configuration=a.original/'configuration.json'
    if correction:
        configuration=a.output/'configuration.json';configuration.write_bytes(expected)
        (a.output/'pending-correction.json').write_bytes(a.pending_metadata_correction.read_bytes())
    receipt=run(configuration,a.output/'graph-audit.json')
    if correction: receipt['pending_metadata_correction']=correction
    receipt['original_execution_receipt_sha256']=sha((a.original/'receipt.json').read_bytes())
    (a.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))

if __name__=='__main__':main()
