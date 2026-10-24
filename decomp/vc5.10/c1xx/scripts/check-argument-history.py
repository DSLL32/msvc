#!/usr/bin/env python3
"""Revalidate frozen restore-only iterator evidence with current static audit."""
import importlib.util,json,time
from pathlib import Path

def load(name,filename):
    sp=importlib.util.spec_from_file_location(name,Path(__file__).with_name(filename));m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m);return m

if __name__=='__main__':
    started=time.perf_counter();packet=json.loads(Path('cases/minimal/argument-trial.json').read_text());record=packet['proof'];o,n=Path(record['original']),Path(record['native'])
    decoder=load('argument_frozen','check-callback.py');history=load('argument_history','check-history.py');audit=history.configure(decoder,o,n,saved_record=record)
    print(json.dumps(dict(result=decoder.check(o,n,audit_path=audit),current_audit=str(audit),seconds=time.perf_counter()-started),indent=2))
