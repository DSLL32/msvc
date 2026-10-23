#!/usr/bin/env python3
"""Bounded persistent-field reset/preserve and ABI rejection controls."""
import argparse
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
    d=load('pdb_decoder','check-callback.py');h=load('pdb_helpers','check-projection-negative.py');history=load('pdb_history','check-history.py')
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--packet',type=Path,default=d.s.PROJECT/'cases/minimal/pdb01-trials.json');args=parser.parse_args()
    packet=json.loads(args.packet.read_text());number=int(packet['trial'][-2:]);object_name='input_pdb_context' if number==1 else 'output_pdb_context';stem='pdb' if number==1 else 'pdb02';started=time.perf_counter();passed=[]
    cases={None:('wrong-original-default','wrong-entry-PC','wrong-native-layout','storage-alias','false-original-return','wrong-cleanup'),
           'restore-nonzero':('native-persistent-state-cleared-by-BSS','original-persistent-state-cleared-by-BSS','reset-not-cleared','changed-original-gap','missing-original-frontier'),
           'restore-zero':('preserve-cleared','wrong-pointer-identity','scalar-as-pointer','wrong-branch-condition','missing-seed-event')}
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        for case in cases[row['seed']]:
            with tempfile.TemporaryDirectory(prefix='pdb-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o=Path(tmp)/'original';n=Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case in ('wrong-original-default','original-persistent-state-cleared-by-BSS','changed-original-gap'):
                    last=int(re.findall(r'^trap seq=(\d+) ',(o/'events.txt').read_text(),re.M)[-1]);suffix='000' if case=='wrong-original-default' else 'phase' if case=='original-persistent-state-cleared-by-BSS' else '%03d'%last
                    name='pdb-context-%02d-'%number+suffix+'.bin';f=o/name;raw=bytearray(f.read_bytes());struct.pack_into('<I',raw,16 if case=='changed-original-gap' else 0,1 if case!='original-persistent-state-cleared-by-BSS' else 0);f.write_bytes(raw);h.rehash(o,name)
                elif case in ('false-original-return','missing-original-frontier','missing-seed-event'):
                    f=o/'events.txt';text=f.read_text()
                    if case=='false-original-return':
                        matches=list(re.finditer(r'^trap seq=\d+ rva=000386ae .*?$',text,re.M));m=matches[-1];text=text[:m.start()]+m[0].replace('eax=00000000','eax=00000001')+text[m.end():]
                    elif case=='missing-original-frontier':text=re.sub(r'^frontier rva=\w+\n','',text,flags=re.M)
                    else:text=re.sub(r'^pdb-reuse-seed .*?\n','',text,flags=re.M)
                    f.write_text(text);h.rehash(o,f.name)
                else:
                    name='phase.json' if case=='native-persistent-state-cleared-by-BSS' else stem+'-entry.json' if case in ('wrong-entry-PC','wrong-branch-condition') else stem+'-registration.json' if case=='wrong-cleanup' else 'state.json'
                    def change(j):
                        f=j['frontend']
                        if case in ('native-persistent-state-cleared-by-BSS','preserve-cleared'):f[object_name]['database']=0
                        elif case=='reset-not-cleared':f[object_name]['database']=f['saved_state']
                        elif case=='wrong-pointer-identity':f[object_name]['types']+=4
                        elif case=='scalar-as-pointer':f[object_name]['read_mode']=f['saved_state']+37
                        elif case=='wrong-entry-PC':j['pc']+=1
                        elif case=='wrong-native-layout':f['pdb_context_layout']['size']=28
                        elif case=='storage-alias':f['storage'][object_name+'.names'][0]=f['storage'][object_name+'.database'][0]
                        elif case=='wrong-branch-condition':f['restore_mode']=0
                        elif case=='wrong-cleanup':j['args']['cleanup']+=4
                        else:raise RuntimeError(case)
                    h.edit(n/name,change);h.rehash(n,name)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
