#!/usr/bin/env python3
"""Reject queue-pointer reset, storage, entry, neighbor and provenance drift."""
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
    d=load('pending-group_decoder','check-callback.py');h=load('pending-group_negative','check-projection-negative.py');history=load('pending-group_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/pending-group-trials.json').read_text());started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        if row.get('group_entry_seed'):cases=['original-entry-seed','native-entry-seed','original-neighbor','native-neighbor','missing-auxiliary-declaration']
        elif row['seed'] is None:cases=['native-result-head','native-result-active']+['original-default','native-width','native-alias','false-callback-entry-PC']
        elif row['seed']=='restore-nonzero':cases=['original-prestate','native-prestate','original-BSS-clear','native-BSS-clear']
        else:cases=['original-result','wrong-natural-padding']
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='pending-group-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case.startswith('original-'):
                    if case=='original-neighbor':fname='group-bss-result.bin';index=0xc6b44-0xb9000
                    else:
                        rows=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]+)',(o/'events.txt').read_text(),re.M)
                        suffix='000' if case=='original-default' else '006' if case=='original-prestate' else 'phase' if case=='original-BSS-clear' else '%03d'%int(next(seq for seq,rva in rows if rva=='00038ae2')) if case=='original-entry-seed' else '%03d'%int(rows[-1][0])
                        fname='pending-groups-'+suffix+'.bin';index=8
                    f=o/fname;blob=bytearray(f.read_bytes());blob[index]^=1;f.write_bytes(blob);h.rehash(o,fname)
                else:
                    fname='group-entry.json' if case in ('native-entry-seed','false-callback-entry-PC') else 'group-result.json' if case=='native-neighbor' else 'pre.json' if case=='native-prestate' else 'phase.json' if case=='native-BSS-clear' else 'trial.json' if case=='missing-auxiliary-declaration' else 'state-model.json' if case=='wrong-natural-padding' else 'state.json'
                    def change(j):
                        if case=='missing-auxiliary-declaration':j.pop('auxiliary_group_data_seed');return
                        if case=='wrong-natural-padding':j['native_padding']['bytes']+=1;return
                        f=j['frontend']
                        if case in ('native-result-head','native-result-active'):f['pending_group_head' if case.endswith('head') else 'active_group']=1
                        elif case in ('native-entry-seed','native-prestate','native-BSS-clear'):f['active_group']^=4
                        elif case=='native-neighbor':f['source_signature']=0
                        elif case=='native-width':f['storage']['active_group'][1]=8
                        elif case=='native-alias':f['storage']['active_group'][0]=f['storage']['pending_group_head'][0]
                        else:j['pc']+=1
                    h.edit(n/fname,change);h.rehash(n,fname)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
