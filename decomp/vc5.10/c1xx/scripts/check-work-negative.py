#!/usr/bin/env python3
"""Reject work-pointer reset, actual work entry and reserve-argument drift."""
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
    d=load('work_decoder','check-callback.py');h=load('work_negative','check-projection-negative.py');history=load('work_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/work-trials.json').read_text());started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        cases=['native-reserve','native-commit','original-reserve','original-commit','original-return','work-entry-PC','main-to-work-phase','original-BSS-neighbor','native-neighbor','wrong-buffer-width','buffer-alias','wrong-buffer-layout'] if row['seed'] is None else ['native-pre-pointer-%d'%i for i in range(4)]+['original-pre-pointer','native-BSS-clear','original-BSS-clear'] if row['seed']=='restore-nonzero' else ['native-frontier-pointer','wrong-helper-formal']
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='work-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case in ('original-reserve','original-commit','original-return'):
                    f=o/'events.txt'
                    def change(match):
                        words=match[2].split(',');index={'original-return':0,'original-reserve':1,'original-commit':2}[case];words[index]='%08x'%(int(words[index],16)+1);return match[1]+','.join(words)
                    f.write_text(re.sub(r'(^trap seq=\d+ rva=00038cc9[^\n]*stack=)([0-9a-f,]+)',change,f.read_text(),flags=re.M));h.rehash(o,f.name)
                elif case.startswith('original-'):
                    fname='work-bss-first-reserve.bin' if case=='original-BSS-neighbor' else 'work-buffers-'+('006' if case=='original-pre-pointer' else 'phase')+'.bin';f=o/fname;blob=bytearray(f.read_bytes());blob[0]^=1;f.write_bytes(blob);h.rehash(o,fname)
                else:
                    fname='pre.json' if case.startswith('native-pre-pointer') else 'phase.json' if case=='native-BSS-clear' else 'work-entry.json' if case in ('work-entry-PC','main-to-work-phase') else 'state.json'
                    def change(j):
                        f=j['frontend']
                        if case=='native-reserve':j['args']['reserve_bytes']+=1
                        elif case=='native-commit':j['args']['commit_bytes']+=1
                        elif case=='work-entry-PC':j['pc']+=1
                        elif case=='main-to-work-phase':f['compiler_phase']=2
                        elif case=='native-neighbor':f['source_signature']=0
                        elif case=='wrong-buffer-width':f['storage']['work_buffer_01.data'][1]=8
                        elif case=='buffer-alias':f['storage']['work_buffer_02.data'][0]=f['storage']['work_buffer_01.data'][0]
                        elif case=='wrong-buffer-layout':f['work_buffer_layout']['fields']['limit']=[8,4]
                        elif case.startswith('native-pre-pointer'):
                            index=int(case.rsplit('-',1)[1]);f['work_buffers'][index//2][index%2]+=4
                        elif case in ('native-BSS-clear','native-frontier-pointer'):f['work_buffers'][1][1]=f['saved_state']+400
                        else:j['args']['unknown']=0
                    h.edit(n/fname,change);h.rehash(n,fname)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
