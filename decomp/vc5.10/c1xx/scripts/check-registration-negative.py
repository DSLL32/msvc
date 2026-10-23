#!/usr/bin/env python3
"""Reject cleanup registration input, count, slot and frontier divergences."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


def main():
    d=load('registration_decoder','check-callback.py')
    h=load('registration_helpers','check-projection-negative.py')
    history=load('registration_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/registration-trials.json').read_text())
    row=packet['trials'][0];original,native=Path(row['original']),Path(row['native'])
    audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
    cases=('wrong-count','wrong-slot','unused-slot-written','wrong-input','preexisting-count','constructor-changed','false-frontier-PC','missing-original-frontier')
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='registration-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temp:
            root=Path(temp);o=root/'original';n=root/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case=='missing-original-frontier':
                path=o/'events.txt';path.write_text(__import__('re').sub(r'^frontier rva=\w+\n','',path.read_text(),flags=__import__('re').M));h.rehash(o,path.name)
            else:
                name='registration-entry.json' if case in ('wrong-input','preexisting-count') else 'state.json'
                def change(j):
                    if case=='wrong-count':j['frontend']['cleanup_count']=0
                    elif case=='wrong-slot':j['frontend']['cleanup_callbacks'][0]=j['frontend']['callback_slots'][1]
                    elif case=='unused-slot-written':j['frontend']['cleanup_callbacks'][1]=j['frontend']['cleanup_callbacks'][0]
                    elif case=='wrong-input':j['args']['cleanup']=j['frontend']['callback_slots'][1]
                    elif case=='preexisting-count':j['frontend']['cleanup_count']=1
                    elif case=='constructor-changed':j['frontend']['output_buffers'][0]['descriptor']=0
                    else:j['pc']+=1
                h.edit(n/name,change);h.rehash(n,name)
            h.rejected(case,lambda:d.check(o,n,audit_path=audit))
    print(json.dumps({'result':'PASS','count':len(cases),'rejected':cases},indent=2))


if __name__=='__main__':main()
