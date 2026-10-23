#!/usr/bin/env python3
"""Reject every prototype-byte drift and reset/entry/neighbor/provenance gaps."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file))
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('prototype_decoder','check-callback.py');h=load('prototype_negative','check-projection-negative.py');history=load('prototype_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/prototype-trials.json').read_text());started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        if row.get('prototype_entry_seed'):cases=['original-entry-seed','native-entry-seed','original-neighbor','native-neighbor','missing-auxiliary-declaration']
        elif row['seed'] is None:cases=['native-result-byte-%d'%i for i in range(20)]+['original-default','native-width','native-alias','false-callback-entry-PC']
        elif row['seed']=='restore-nonzero':cases=['original-prestate','native-prestate','original-BSS-clear','native-BSS-clear']
        else:cases=['original-result','wrong-natural-padding']
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='prototype-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case.startswith('original-'):
                    if case=='original-neighbor':fname='prototype-bss-result.bin';index=0
                    else:
                        rows=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]+)',(o/'events.txt').read_text(),re.M)
                        suffix='000' if case=='original-default' else '006' if case=='original-prestate' else 'phase' if case=='original-BSS-clear' else '%03d'%int(next(seq for seq,rva in rows if rva=='00038ab7')) if case=='original-entry-seed' else '%03d'%int(rows[-1][0])
                        fname='empty-prototype-'+suffix+'.bin';index=19
                    f=o/fname;blob=bytearray(f.read_bytes());blob[index]^=1;f.write_bytes(blob);h.rehash(o,fname)
                else:
                    fname='prototype-entry.json' if case in ('native-entry-seed','false-callback-entry-PC') else 'prototype-result.json' if case=='native-neighbor' else 'pre.json' if case=='native-prestate' else 'phase.json' if case=='native-BSS-clear' else 'trial.json' if case=='missing-auxiliary-declaration' else 'state-model.json' if case=='wrong-natural-padding' else 'state.json'
                    def change(j):
                        if case=='missing-auxiliary-declaration':j.pop('auxiliary_prototype_data_seed');return
                        if case=='wrong-natural-padding':j['native_padding']['bytes']+=1;return
                        f=j['frontend']
                        if case.startswith('native-result-byte-'):f['empty_metadata_prototype'][int(case.rsplit('-',1)[1])]=1
                        elif case in ('native-entry-seed','native-prestate','native-BSS-clear'):f['empty_metadata_prototype'][19]^=1
                        elif case=='native-neighbor':f['source_signature']=0
                        elif case=='native-width':f['storage']['empty_metadata_prototype'][1]=24
                        elif case=='native-alias':f['storage']['empty_metadata_prototype'][0]=f['storage']['source_signature'][0]
                        else:j['pc']+=1
                    h.edit(n/fname,change);h.rehash(n,fname)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
