#!/usr/bin/env python3
"""Semantic controls for the newly modeled context stack and BSS reset."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    callback=load('callback_negative','check-callback.py');helpers=load('context_helpers','check-projection-negative.py')
    history=load('context_history','check-history.py')
    packet=json.loads((callback.s.PROJECT/'cases/minimal/callback01-trials.json').read_text())
    row=next(row for row in packet['trials'] if row['seed']=='restore-nonzero');original,native=Path(row['original']),Path(row['native'])
    audit=history.configure(callback,original,native,saved_record=row);callback.check(original,native,audit_path=audit)
    cases=('missing-nonzero-prestate','uncleared-new-field','changed-original-prestate','final-context-nonzero','wrong-context-layout','wrong-callback-PC','missing-callback-frontier','wrong-snapshot-identity')
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='context-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temp:
            root=Path(temp);o=root/'original';n=root/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case=='changed-original-prestate':
                path=o/'context-stack-006.bin';raw=bytearray(path.read_bytes());struct.pack_into('<I',raw,0,0);path.write_bytes(raw);helpers.rehash(o,path.name)
            elif case=='missing-callback-frontier':
                path=o/'events.txt';path.write_text(path.read_text().replace('frontier rva=00038622\n',''));helpers.rehash(o,path.name)
            else:
                name='pre.json' if case=='missing-nonzero-prestate' else 'phase.json' if case in ('uncleared-new-field','wrong-snapshot-identity') else 'state.json'
                def change(j):
                    if case=='missing-nonzero-prestate':j['frontend']['context_stack']=[0,0,0]
                    elif case=='uncleared-new-field':j['frontend']['context_stack'][2]=1
                    elif case=='final-context-nonzero':j['frontend']['context_stack'][0]=1
                    elif case=='wrong-context-layout':j['frontend']['context_stack_fields'].reverse()
                    elif case=='wrong-callback-PC':j['pc']+=1
                    else:j['frontend']['saved_state']+=4
                helpers.edit(n/name,change);helpers.rehash(n,name)
            helpers.rejected(case,lambda:callback.check(o,n,audit_path=audit))
    print(json.dumps({'result':'PASS','rejected':cases,'count':len(cases)},indent=2))


if __name__=='__main__':main()
