#!/usr/bin/env python3
"""Read-only frozen append observations with fresh strict audits; never executes/promotes."""
import argparse
import importlib.util
import json
from pathlib import Path
import time


def module(name, file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--role',choices=('normal','restore','all'),default='normal')
    p.add_argument('--packet',type=Path,default=Path('cases/minimal/append-trial.json'))
    args=p.parse_args();packet=json.loads(args.packet.read_text());results=[]
    for role in ('normal','restore') if args.role=='all' else (args.role,):
        started=time.perf_counter();record=packet['proofs'][role]
        o,n=Path(record['original']),Path(record['native'])
        decoder=module('append_history_'+role,'check-callback.py')
        history=module('append_audit_'+role,'check-history.py')
        audit=history.configure(decoder,o,n,saved_record=record)
        result=decoder.check(o,n,audit_path=audit)
        results.append(dict(role=role,result=result,current_audit_sha256=decoder.s.sha(audit),seconds=time.perf_counter()-started))
    print(json.dumps(results,indent=2))


if __name__=='__main__':main()
