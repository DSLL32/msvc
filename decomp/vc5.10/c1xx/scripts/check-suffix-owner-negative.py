#!/usr/bin/env python3
"""Reject coherent later-phase owner substitution at the existing boundary tie."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import time

from suffix_state import check


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    original,native=args.original.resolve(),args.native.resolve();trial=json.loads((native/'trial.json').read_text());state=json.loads((native/'state.json').read_text());text=(original/'events.txt').read_text();started=time.perf_counter()
    spec=importlib.util.spec_from_file_location('suffix_owner_decoder',Path(__file__).with_name('check-callback.py'));d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
    positive=check(d.s,d.d,original,native,trial,text,state,{})
    changes=[]
    with tempfile.TemporaryDirectory(prefix='suffix-owner-control-',dir=native.parent)as temporary:
        new=Path(temporary)
        for item in native.iterdir():(new/item.name).symlink_to(item,target_is_directory=item.is_dir())
        for name,minimum in [('suffix-entries',0),('suffix-returns',0),('suffix-append-entries',0),('actions-entries',3),('actions-returns',3),('actions-match-returns',3),('actions-outer-entries',4),('actions-outer-returns',4),('actions-matches',4),('actions-signatures',4 if trial.get('test_seed')else 5),('actions-signature-returns',4 if trial.get('test_seed')else 5)]:
            p=new/(name+'.json');rows=json.loads(p.read_text())
            for row in rows[minimum:]:row['locale']['pctype']+=2
            p.unlink();p.write_text(json.dumps(rows));changes.append(dict(file=p.name,first_row=minimum,rows=len(rows)-minimum))
        try:check(d.s,d.d,original,new,trial,text,state,{})
        except ValueError as error:
            rejection=str(error)
            if 'same real first suffix checkpoint'not in rejection:raise ValueError('owner control failed before/after intended boundary tie: '+rejection)
        else:raise ValueError('coherent later-phase owner substitution accepted')
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    result=dict(kind='one fresh coherent all-later-phase owner control; earlier numeric proof untouched, no execution',original=str(original),native=str(native),original_receipt_sha256=sha(original/'receipt.json'),native_receipt_sha256=sha(native/'receipt.json'),checker_sha256=sha(Path(__file__).with_name('suffix_state.py')),control_sha256=sha(Path(__file__)),positive=positive,mutations=changes,rejection=rejection,count=1,seconds=time.perf_counter()-started)
    if args.output.exists():raise ValueError('refuse overwrite')
    args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=1,seconds=result['seconds'])))


if __name__=='__main__':main()
