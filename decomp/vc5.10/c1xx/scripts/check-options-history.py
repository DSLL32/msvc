#!/usr/bin/env python3
"""Read-only current static revalidation of the frozen options proof pairs."""
import argparse,importlib.util,json
from pathlib import Path


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--role',choices=['normal','restore','all'],default='all');args=parser.parse_args()
    packet=json.loads(Path('cases/minimal/options-trial.json').read_text());out=[]
    for role in ('normal','restore')if args.role=='all'else(args.role,):
        record=packet['proofs'][role];d=load('options_historical_'+role,'check-callback.py');h=load('options_reader_'+role,'check-history.py');o,n=Path(record['original']),Path(record['native']);audit=h.configure(d,o,n,saved_record=record);out.append(dict(role=role,result=d.check(o,n,audit_path=audit),current_audit=str(audit)))
    print(json.dumps(out,indent=2))


if __name__=='__main__':main()
