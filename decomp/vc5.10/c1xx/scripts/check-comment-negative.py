#!/usr/bin/env python3
"""Reject corrupted observed comment text, lengths, entry ABI and neighbors."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('comment_decoder','check-callback.py');h=load('comment_negative','check-projection-negative.py');history=load('comment_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/comment-constructor-trials.json').read_text());started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        if row.get('comment_entry_seed'):cases=['original-entry-seed','native-entry-seed','original-unknown-tail','native-neighbor','missing-auxiliary-declaration']
        elif row['seed'] is None:cases=['native-result-first','native-result-last','native-result-offset','native-result-length','native-result-COM','original-default','native-width','native-alias','false-entry-PC','false-entry-return','false-cleanup-argument']
        elif row['seed']=='restore-nonzero':cases=['original-prestate','native-prestate','original-BSS-clear','native-BSS-clear']
        else:cases=['original-result','wrong-natural-padding']
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='comment-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case.startswith('original-'):
                    if case=='original-unknown-tail':fname='comment-bss-result.bin';index=0xc6ba8+74-0xb9000
                    else:
                        rows=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]+)',(o/'events.txt').read_text(),re.M)
                        suffix='000' if case=='original-default' else '006' if case=='original-prestate' else 'phase' if case=='original-BSS-clear' else '%03d'%int(next(seq for seq,rva in rows if rva=='00038af9')) if case=='original-entry-seed' else '%03d'%int(rows[-1][0])
                        fname='generated-comment-'+suffix+'.bin';index=77
                    f=o/fname;blob=bytearray(f.read_bytes());blob[index]^=1;f.write_bytes(blob);h.rehash(o,fname)
                else:
                    fname='comment-entry.json' if case in ('native-entry-seed','false-entry-PC','false-entry-return') else 'comment-result.json' if case in ('native-neighbor','false-cleanup-argument') else 'pre.json' if case=='native-prestate' else 'phase.json' if case=='native-BSS-clear' else 'trial.json' if case=='missing-auxiliary-declaration' else 'state-model.json' if case=='wrong-natural-padding' else 'state.json'
                    def change(j):
                        if case=='missing-auxiliary-declaration':j.pop('auxiliary_comment_data_seed');return
                        if case=='wrong-natural-padding':j['native_padding']['bytes']+=1;return
                        f=j['frontend']
                        if case in ('native-result-first','native-result-last','native-entry-seed','native-prestate','native-BSS-clear'):f['generated_comment'][73 if case=='native-result-last' else 0]^=1
                        elif case=='native-result-offset':f['comment_value_offset']+=1
                        elif case=='native-result-length':f['generated_comment_length']+=1
                        elif case=='native-result-COM':f['com_initialized']=1
                        elif case=='native-neighbor':f['source_signature']=0
                        elif case=='native-width':f['storage']['generated_comment'][1]=75
                        elif case=='native-alias':f['storage']['generated_comment_length'][0]=f['storage']['comment_value_offset'][0]
                        elif case=='false-entry-PC':j['pc']+=1
                        elif case=='false-entry-return':j['raw_entry_abi']['return_pc']=0
                        else:j['raw_entry_abi']['stack_arguments'][0]+=4
                    h.edit(n/fname,change);h.rehash(n,fname)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
