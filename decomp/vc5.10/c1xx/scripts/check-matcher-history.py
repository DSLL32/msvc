#!/usr/bin/env python3
"""Revalidate immutable matcher pairs with current strict audits, never execute."""
import argparse,importlib.util,json,time
from pathlib import Path


def load(name,file):
    sp=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m);return m

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--role',choices=('normal','restore','all'),default='all');a=p.parse_args();packet=json.loads(Path('cases/minimal/matcher-trial.json').read_text());results=[]
    for role in ('normal','restore')if a.role=='all'else(a.role,):
        start=time.perf_counter();r=packet['proofs'][role];o,n=Path(r['original']),Path(r['native']);d=load('matcher_'+role,'check-callback.py');h=load('matcher_history_'+role,'check-history.py');audit=h.configure(d,o,n,saved_record=r);result=d.check(o,n,audit_path=audit);audit_sha=d.s.sha(audit);results.append(dict(role=role,result=result,current_audit=str(audit.parent/'audits'/(audit_sha+'.json')),audit_sha256=audit_sha,seconds=time.perf_counter()-start))
    print(json.dumps(results,indent=2))
