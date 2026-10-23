#!/usr/bin/env python3
"""Reject wrong constructor boundaries and split-wrapper ownership evidence."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('comment_decoder','check-callback.py');h=load('comment_negative','check-projection-negative.py');history=load('comment_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/comment-wrapper-trials.json').read_text());row=packet['trials'][0];original,native=Path(row['original']),Path(row['native']);started=time.perf_counter();audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit);passed=[]
    for case in ('raw-frontier','native-unknown-argument','native-PC','native-wrong-callee','early-cleanup-registration','missing-cold-chunk'):
        with tempfile.TemporaryDirectory(prefix='comment-wrapper-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n);a=Path(tmp)/'audit.json';shutil.copy2(audit,a)
            if case=='raw-frontier':
                f=o/'events.txt';f.write_text(f.read_text().replace('frontier rva=00038af9','frontier rva=00038afa'));h.rehash(o,f.name)
            elif case=='missing-cold-chunk':
                h.edit(a,lambda j:j['report']['owned_code_chunks']['original']['functions'].update({'_initialize_frontend_39':[[0x10438aef,0x10438af9]]}))
            else:
                def change(j):
                    if case=='native-unknown-argument':j['args']['unknown']=1
                    elif case=='native-PC':j['pc']+=1
                    elif case=='native-wrong-callee':j['symbol']='initialize_frontend_40'
                    else:j['frontend']['cleanup_count']+=1
                h.edit(n/'state.json',change);h.rehash(n,'state.json')
            h.rejected(case,lambda:d.check(o,n,audit_path=a));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
