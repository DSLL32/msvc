#!/usr/bin/env python3
"""Reject mutated canonical free evidence using only the captured provider file."""
import copy
import json
from pathlib import Path
import tempfile
import time
from import_evidence import verify_free_provider
from state import BINARY, sha


def main():
    start=time.monotonic();path=BINARY/'cases/analysis/canonical-free-abi.json'
    content=path.read_bytes();verify_free_provider(path,sha(content));value=json.loads(content)
    controls=[
        ('wrong public export name',lambda p:p['public_export'].__setitem__('name','malloc')),
        ('wrong public export ordinal',lambda p:p['public_export'].__setitem__('ordinal',605)),
        ('wrong public export entry',lambda p:p['public_export'].__setitem__('address',0x7800138b)),
        ('wrong canonical provider',lambda p:p.__setitem__('provider_sha256','0'*64)),
        ('wrong captured provider identity',lambda p:p.__setitem__('actual_hfile_sha256','0'*64)),
        ('wrong complete body bytes',lambda p:p['body'].__setitem__('sha256','0'*64)),
        ('shortened complete body',lambda p:p['body'].__setitem__('end',0x780013f1)),
        ('omitted normal return',lambda p:p['normal_returns'].pop()),
        ('wrong restore bytes',lambda p:p['normal_returns'][0].__setitem__('bytes','9090c3')),
        ('wrong declaration inputs',lambda p:p['declaration'].__setitem__('sha256','0'*64)),
    ]
    rejected=[]
    with tempfile.TemporaryDirectory(prefix='provider-abi-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as directory:
        for name,change in controls:
            proof=copy.deepcopy(value);change(proof);p=Path(directory)/'proof.json';p.write_text(json.dumps(proof))
            try:verify_free_provider(p,sha(p.read_bytes()))
            except (AssertionError,KeyError,ValueError):rejected.append(name)
            else:raise AssertionError('mutated provider accepted: '+name)
    print(json.dumps({'rejected':rejected,'fresh_controls':len(rejected),'seconds':time.monotonic()-start,'accepted':False},indent=2))


if __name__=='__main__':main()
