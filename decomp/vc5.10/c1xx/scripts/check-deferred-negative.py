#!/usr/bin/env python3
"""Reject wrong pointer identity, reset, constructor and callback evidence."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('deferred_decoder','check-callback.py');h=load('deferred_helpers','check-projection-negative.py');history=load('deferred_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/deferred-trials.json').read_text());row=next(r for r in packet['trials'] if r['seed']=='restore-nonzero')
    original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
    cases=('wrong-original-default','missing-nonzero-prestate','BSS-not-cleared','constructor-not-cleared','wrong-owned-input','false-entry-PC','wrong-cleanup-input','wrong-cleanup-count','wrong-field-width','undeclared-mock-role')
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='deferred-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
            root=Path(temporary);o=root/'original';n=root/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case=='wrong-original-default':
                path=o/'deferred-work-000.bin';path.write_bytes(struct.pack('<I',1));h.rehash(o,path.name)
            elif case=='undeclared-mock-role':
                for directory in (o,n):
                    h.edit(directory/'trial.json',lambda j:j['auxiliary_deferred_data_seed'].update(code_or_allocator_mock=True));h.rehash(directory,'trial.json')
            else:
                name='pre.json' if case=='missing-nonzero-prestate' else 'phase.json' if case=='BSS-not-cleared' else 'deferred-entry.json' if case in ('wrong-owned-input','false-entry-PC') else 'deferred-registration.json' if case=='wrong-cleanup-input' else 'state.json'
                def change(j):
                    if case=='missing-nonzero-prestate':j['frontend']['deferred_work']=0
                    elif case in ('BSS-not-cleared','constructor-not-cleared'):j['frontend']['deferred_work']=j['frontend']['saved_state']
                    elif case=='wrong-owned-input':j['frontend']['deferred_work']+=4
                    elif case=='false-entry-PC':j['pc']+=1
                    elif case=='wrong-cleanup-input':j['args']['cleanup']=j['frontend']['callback_slots'][1]
                    elif case=='wrong-cleanup-count':j['frontend']['cleanup_count']=8
                    else:j['frontend']['storage']['deferred_work'][1]=8
                h.edit(n/name,change);h.rehash(n,name)
            h.rejected(case,lambda:d.check(o,n,audit_path=audit))
    print(json.dumps({'result':'PASS','count':len(cases),'rejected':cases},indent=2))


if __name__=='__main__':main()
