#!/usr/bin/env python3
"""Reject signature default/reset/signedness/storage and frontier divergence."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('signature_decoder','check-callback.py');h=load('signature_helpers','check-projection-negative.py');history=load('signature_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/signature-trials.json').read_text());row=next(r for r in packet['trials'] if r['seed']=='restore-nonzero')
    original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
    cases=('wrong-original-default','missing-nonzero-prestate','BSS-not-cleared','not-initialized','signed-coercion','wrong-width','storage-alias','false-entry-PC','missing-original-frontier','wrong-original-result');started=time.perf_counter()
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='signature-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o=Path(tmp)/'original';n=Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case in ('wrong-original-default','wrong-original-result'):
                rows=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8})',(o/'events.txt').read_text(),re.M);suffix='000' if case=='wrong-original-default' else '%03d'%int(rows[-1][0]);name='source-signature-'+suffix+'.bin';(o/name).write_bytes(struct.pack('<I',1 if case=='wrong-original-default' else 0));h.rehash(o,name)
            elif case=='missing-original-frontier':
                f=o/'events.txt';f.write_text(re.sub(r'^frontier rva=\w+\n','',f.read_text(),flags=re.M));h.rehash(o,f.name)
            else:
                name='pre.json' if case=='missing-nonzero-prestate' else 'phase.json' if case=='BSS-not-cleared' else 'signature-entry.json' if case=='false-entry-PC' else 'state.json'
                def change(j):
                    f=j['frontend']
                    if case in ('missing-nonzero-prestate','not-initialized'):f['source_signature']=0
                    elif case=='BSS-not-cleared':f['source_signature']=37
                    elif case=='signed-coercion':f['source_signature']=-1
                    elif case=='wrong-width':f['storage']['source_signature'][1]=8
                    elif case=='storage-alias':f['storage']['source_signature'][0]=f['storage']['sentinel_token.kind'][0]
                    else:j['pc']+=1
                h.edit(n/name,change);h.rehash(n,name)
            h.rejected(case,lambda:d.check(o,n,audit_path=audit))
    print(json.dumps({'result':'PASS','count':len(cases),'seconds':time.perf_counter()-started,'rejected':cases},indent=2))


if __name__=='__main__':main()
