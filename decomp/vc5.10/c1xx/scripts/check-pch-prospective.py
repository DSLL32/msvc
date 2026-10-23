#!/usr/bin/env python3
"""Read only a frozen raw-only PCH tail observation; never audit or promote it."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path


def main():
    project=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--packet',type=Path,default=project/'cases/minimal/pch-prospective-trials.json')
    args=parser.parse_args();packet=json.loads(args.packet.read_text())
    spec=importlib.util.spec_from_file_location('pch_prospective',Path(__file__).with_name('check-callback.py'))
    d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
    results=[]
    for row in packet['trials']:
        o,n=Path(row['original']),Path(row['native'])
        for path,key in [(o,'original'),(n,'native')]:
            d.s.require(hashlib.sha256((path/'receipt.json').read_bytes()).hexdigest()==row[key+'_receipt_sha256'],'not the immutable prospective proof')
            d.s.require(json.loads((path/'receipt.json').read_text())['scope']=='prospective raw-only tail chain; no graph acceptance or function credit','prospective receipt scope changed')
        d.s.HISTORICAL_SNAPSHOT=n
        result=d.check(o,n,require_audit=False)
        d.s.require(json.loads((n/'trial.json').read_text()).get('prospective_only'),'not a raw-only pending-member proof')
        results.append(result)
    print(json.dumps({'kind':'read-only raw observation; not acceptance','promotion':False,'results':results},indent=2))


if __name__=='__main__':main()
