#!/usr/bin/env python3
"""Reject changed typed output layout, mocked data roles and constructor results."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


def main():
    d=load('output_decoder','check-callback.py');helpers=load('output_helpers','check-projection-negative.py');history=load('output_history','check-history.py')
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--packet',type=Path,default=d.s.PROJECT/'cases/minimal/output01-trials.json');args=parser.parse_args()
    packet=json.loads(args.packet.read_text());index=int(packet['trial'][-2:])-1
    row=next(row for row in packet['trials'] if row['seed']=='restore-nonzero');original,native=Path(row['original']),Path(row['native'])
    audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
    cases=('wrong-native-padding','wrong-mode-width','BSS-clear-skipped','constructor-ok-wrong','descriptor-unsigned','wrong-unused-callback','FILE-outside-owned-role','false-constructor-PC','changed-registration-callback','changed-real-initializer-table')
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='output-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temp:
            root=Path(temp);o=root/'original';n=root/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            name='phase.json' if case=='BSS-clear-skipped' else 'constructor-entry.json' if case in ('wrong-unused-callback','FILE-outside-owned-role','false-constructor-PC') else 'state.json'
            if index and name=='constructor-entry.json':name='constructor%02d-entry.json'%(index+1)
            if index and case=='changed-registration-callback':name='registration%02d-entry.json'%(index+1)
            def change(j):
                fields=j['frontend'];buffer=fields['output_buffers'][index]
                if case=='wrong-native-padding':fields['output_native_layout']['size']=292
                elif case=='wrong-mode-width':fields['output_native_layout']['fields']['mode'][1]=4
                elif case=='BSS-clear-skipped':buffer['data']=fields['saved_state']
                elif case=='constructor-ok-wrong':buffer['ok']=0
                elif case=='descriptor-unsigned':buffer['descriptor']=0xffffffff
                elif case=='wrong-unused-callback':buffer['relocation']=fields['callback_slots'][2]
                elif case=='FILE-outside-owned-role':buffer['file']=fields['saved_state']+8524
                elif case=='false-constructor-PC':j['pc']+=1
                elif case=='changed-registration-callback':j['args']['cleanup']=fields['callback_slots'][1]
                else:fields['callback_slots'][1]=fields['callback_slots'][2]
            helpers.edit(n/name,change);helpers.rehash(n,name)
            helpers.rejected(case,lambda:d.check(o,n,audit_path=audit))
    print(json.dumps({'result':'PASS','count':len(cases),'rejected':cases},indent=2))


if __name__=='__main__':main()
