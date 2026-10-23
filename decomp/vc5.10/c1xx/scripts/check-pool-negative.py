#!/usr/bin/env python3
"""Reject wrong receiver/stack ABI, object projection and expanded BSS reset."""
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
    d=load('pool_decoder','check-callback.py');h=load('pool_helpers','check-projection-negative.py');history=load('pool_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/pool-wrapper-trials.json').read_text());started=time.perf_counter();passed=[]
    cases={None:('wrong-original-default','wrong-original-this','missing-original-this','wrong-original-stack','wrong-native-this','wrong-native-stack','wrong-layout','storage-alias','false-native-PC','hidden-member-credit','member-unmasked'),
           'restore-nonzero':('missing-original-nonzero','missing-native-nonzero','original-BSS-not-reset','native-BSS-not-reset','wrong-pointer-projection'),
           'restore-zero':('minimum-pages-as-pointer',)}
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        for case in cases[row['seed']]:
            with tempfile.TemporaryDirectory(prefix='pool-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o=Path(tmp)/'original';n=Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n);proof=Path(tmp)/'audit.json';shutil.copy2(audit,proof)
                if case in ('wrong-original-default','missing-original-nonzero','original-BSS-not-reset'):
                    suffix='000' if case=='wrong-original-default' else '006' if case=='missing-original-nonzero' else 'phase';name='pool-01-'+suffix+'.bin';f=o/name;raw=bytearray(f.read_bytes());struct.pack_into('<I',raw,0,0 if case=='missing-original-nonzero' else 1);f.write_bytes(raw);h.rehash(o,name)
                elif case in ('wrong-original-this','missing-original-this','wrong-original-stack'):
                    f=o/'events.txt';text=f.read_text();m=re.search(r'^trap seq=\d+ rva=000389d5 .*$',text,re.M);line=m[0]
                    if case=='wrong-original-this':line=line.replace('ecx=104c0ac8','ecx=104c0acc')
                    elif case=='missing-original-this':line=re.sub(r'ecx=\w+ ','',line)
                    else:line=re.sub(r'(stack=\w+,)00000001,',r'\g<1>00000002,',line)
                    f.write_text(text[:m.start()]+line+text[m.end():]);h.rehash(o,f.name)
                elif case=='member-unmasked':
                    f=o/'masked-text.bin';raw=bytearray(f.read_bytes());raw[0x389d5-0x1000]=0x8b;f.write_bytes(raw);h.rehash(o,f.name)
                elif case=='hidden-member-credit':h.edit(proof,lambda j:j['report']['owned_code_chunks']['original']['functions'].update({'CompilerPool::Initialize':[[0x104389d5,0x104389eb]]}))
                else:
                    name='pre.json' if case in ('missing-native-nonzero','minimum-pages-as-pointer','wrong-pointer-projection') else 'phase.json' if case=='native-BSS-not-reset' else 'state.json'
                    def change(j):
                        f=j['frontend']
                        if case=='wrong-native-this':j['args']['this']+=4
                        elif case=='wrong-native-stack':j['args']['minimum_pages']=2
                        elif case=='wrong-layout':f['pool_native_layout']['size']=20
                        elif case=='storage-alias':f['storage']['pool_01.remaining'][0]=f['storage']['pool_01.cursor'][0]
                        elif case=='false-native-PC':j['pc']+=1
                        elif case=='missing-native-nonzero':f['pools'][0]['cursor']=0
                        elif case=='native-BSS-not-reset':f['pools'][0]['remaining']=23
                        elif case=='minimum-pages-as-pointer':f['pools'][0]['minimum_pages']=f['saved_state']+37
                        elif case=='wrong-pointer-projection':f['pools'][0]['arena']+=4
                        else:raise RuntimeError(case)
                    h.edit(n/name,change);h.rehash(n,name)
                h.rejected(case,lambda:d.check(o,n,audit_path=proof));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
