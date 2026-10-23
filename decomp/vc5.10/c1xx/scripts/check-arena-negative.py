#!/usr/bin/env python3
"""Affected arena projection/frontier controls; frozen inputs, no execution."""
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
    d=load('arena_negative_decoder','check-callback.py');h=load('arena_negative_helpers','check-projection-negative.py');history=load('arena_negative_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/environment-trials.json').read_text());row=packet['trials'][1];original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
    cases=['native-seeded-pointer','native-seeded-scalar','native-descriptor-identity','native-reset','native-final','native-descriptor-default','native-layout','native-caller','native-stub-PC','native-entry-formal','original-seed','original-reset','original-descriptor','original-BSS-neighbor','original-return']
    started=time.perf_counter();passed=[]
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='arena-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case.startswith('native-'):
                name='pre.json' if case.startswith('native-seeded-') or case=='native-descriptor-identity' else 'phase.json' if case=='native-reset' else 'arena-entry.json' if case=='native-entry-formal' else 'state.json'
                def change(j):
                    if case=='native-seeded-pointer':j['frontend']['arenas'][0]+=4
                    elif case=='native-seeded-scalar':j['frontend']['arenas'][3]+=1
                    elif case=='native-descriptor-identity':j['frontend']['arenas'][4]+=20
                    elif case=='native-reset':j['frontend']['arenas'][7]=29
                    elif case=='native-final':j['frontend']['arenas'][64]=1
                    elif case=='native-descriptor-default':j['frontend']['arena_descriptors'][44]=8
                    elif case=='native-layout':j['frontend']['arena_native_layout']['size']+=4
                    elif case=='native-caller':j['caller_return_pc']+=1
                    elif case=='native-stub-PC':j['pc']+=1
                    elif case=='native-entry-formal':j['args']['invented']=1
                    else:raise ValueError(case)
                h.edit(n/name,change);h.rehash(n,name)
            else:
                if case=='original-return':
                    f=o/'events.txt'
                    def change(m):
                        v=m[2].split(',');v[0]='10438dfa';return m[1]+','.join(v)
                    text,count=re.subn(r'(^trap seq=\d+ rva=00038e33[^\n]*stack=)([0-9a-f,]+)',change,f.read_text(),count=1,flags=re.M)
                    if count!=1:raise ValueError('missing original helper entry')
                    f.write_text(text)
                else:
                    f=o/({'original-seed':'arenas-006.bin','original-reset':'arenas-phase.bin','original-descriptor':'arena-descriptors-006.bin','original-BSS-neighbor':'arena-bss-frontier.bin'}[case]);blob=bytearray(f.read_bytes());blob[-1]^=1;f.write_bytes(blob)
                h.rehash(o,f.name)
            h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
