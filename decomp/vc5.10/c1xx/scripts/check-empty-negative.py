#!/usr/bin/env python3
"""Reject missing entries or false INT3 evidence across an empty callback."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import shutil
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args()
    d=load('empty_decoder','check-callback.py');h=load('empty_helpers','check-projection-negative.py');d.check(args.original,args.native)
    trial=json.loads((args.original/'trial.json').read_text());name=trial['trial'];row=next(row for row in trial['driver'] if row['name']==name)
    if row['end']-row['start']!=1 or row.get('code_chunks'):raise ValueError('not an identified one-byte empty callback')
    rva=row['start']-trial['target']['base'];native_name=row.get('native_symbol',name.lstrip('_'))
    cases=('missing-original-empty-entry','missing-native-empty-entry','false-frontier-PC');started=time.perf_counter()
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='empty-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o=Path(tmp)/'original';n=Path(tmp)/'native';shutil.copytree(args.original,o);shutil.copytree(args.native,n)
            if case=='missing-original-empty-entry':
                f=o/'events.txt';f.write_text(re.sub(r'^trap seq=\d+ rva=%08x.*\n'%rva,'',f.read_text(),flags=re.M));h.rehash(o,f.name)
            elif case=='missing-native-empty-entry':
                f=n/'trace.txt';f.write_text(f.read_text().replace(native_name+'\n',''));h.rehash(n,f.name)
            else:h.edit(n/'state.json',lambda j:j.update(pc=j['pc']+1));h.rehash(n,'state.json')
            h.rejected(case,lambda:d.check(o,n))
    print(json.dumps({'result':'PASS','trial':name,'count':3,'seconds':time.perf_counter()-started,'rejected':cases},indent=2))


if __name__=='__main__':main()
