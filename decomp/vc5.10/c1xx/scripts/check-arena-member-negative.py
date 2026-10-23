#!/usr/bin/env python3
"""Affected thiscall-input witnesses at the genuine pending arena member."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import tempfile
import time


def load(name,file):
    s=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m


def main():
    d=load('arena_member_decoder','check-callback.py');h=load('arena_member_negative_helpers','check-projection-negative.py');history=load('arena_member_history','check-history.py')
    row=json.loads((d.s.PROJECT/'cases/minimal/default-arena-trial.json').read_text());original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
    cases=['original-this','original-descriptor','original-address','original-caller','native-this','native-descriptor','native-address','native-formals','native-raw-ECX','native-raw-stack','native-caller'];started=time.perf_counter();passed=[]
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='arena-member-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case.startswith('native-'):
                f=n/'arena-member-entry.json'
                def change(j):
                    if case=='native-this':j['args']['this']+=32
                    elif case=='native-descriptor':j['args']['descriptor']+=20
                    elif case=='native-address':j['args']['address']=4
                    elif case=='native-formals':j['formal_inventory'].pop()
                    elif case=='native-raw-ECX':j['raw_entry_abi']['ecx']+=32
                    elif case=='native-raw-stack':j['raw_entry_abi']['stack_arguments'].reverse()
                    elif case=='native-caller':j['raw_entry_abi']['return_pc']+=1
                h.edit(f,change);h.rehash(n,f.name)
            else:
                f=o/'events.txt'
                if case=='original-this':
                    text,count=re.subn(r'(^trap seq=\d+ rva=00038e70[^\n]*ecx=)[0-9a-f]+',r'\g<1>104c6520',f.read_text(),count=1,flags=re.M)
                else:
                    field={'original-caller':0,'original-descriptor':1,'original-address':2}[case]
                    def change(m):
                        words=m[2].split(',');words[field]='%08x'%(int(words[field],16)+4);return m[1]+','.join(words)
                    text,count=re.subn(r'(^trap seq=\d+ rva=00038e70[^\n]*stack=)([0-9a-f,]+)',change,f.read_text(),count=1,flags=re.M)
                if count!=1:raise ValueError('missing member ABI for '+case)
                f.write_text(text);h.rehash(o,f.name)
            h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
