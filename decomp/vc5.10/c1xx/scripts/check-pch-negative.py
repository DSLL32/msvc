#!/usr/bin/env python3
"""Fail closed on PCH closure scope, hidden ABI, every field and neighbor drift."""
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
    d=load('pch_decoder','check-callback.py');h=load('pch_negative','check-projection-negative.py');history=load('pch_history','check-history.py');pch=load('pch_fields','pch_state.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/pch-joint-trials.json').read_text());started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        if row.get('pch_entry_seed'):cases=['nonzero-entry-field-'+str(i) for i in range(17)]+['original-return-EAX','original-return-EDI','original-return-ESP','native-return-EAX','native-return-EDI','native-return-ESP','original-neighbor','native-neighbor','hidden-constructor-seed']
        elif row['seed'] is None:cases=['wrong-native-this','missing-native-EDI','wrong-native-caller','wrong-original-this','missing-original-EDI','wrong-original-caller','wrong-closure-order','extra-closure-member']
        elif row['seed']=='restore-nonzero':cases=['original-prestate','native-prestate','original-BSS-clear','native-BSS-clear']
        else:cases=['wrong-field-layout','wrong-field-map','wrong-return-PC']
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='pch-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case in ('wrong-closure-order','extra-closure-member','hidden-constructor-seed'):
                    for folder in (o,n):
                        def change(j):
                            if case=='wrong-closure-order':j['ordered_closure'].reverse()
                            elif case=='extra-closure-member':j['ordered_closure'].append('_initialize_frontend_38');j['accepted'].append('_initialize_frontend_38')
                            else:j['test_pch_seed']=False;j.pop('auxiliary_pch_data_seed')
                        h.edit(folder/'trial.json',change);h.rehash(folder,'trial.json')
                elif case.startswith('original-return-') or case.startswith('wrong-original-') or case=='missing-original-EDI':
                    f=o/'events.txt';text=f.read_text()
                    pattern=r'^pch-return .*' if case.startswith('original-return-') else r'^trap seq=\d+ rva=0003bd41 .*'
                    m=re.search(pattern,text,re.M);line=m[0]
                    if case=='original-return-EAX':line=re.sub(r'eax=\w+','eax=00000000',line)
                    elif case=='original-return-EDI':line=re.sub(r'edi=\w+','edi=ffffffff',line)
                    elif case=='original-return-ESP':line=re.sub(r'esp=\w+','esp=00000000',line)
                    elif case=='wrong-original-this':line=line.replace('ecx=104c64b8','ecx=104c64bc')
                    elif case=='missing-original-EDI':line=re.sub(r' edi=\w+','',line)
                    else:line=re.sub(r'stack=\w+,','stack=10438ae2,',line)
                    f.write_text(text[:m.start()]+line+text[m.end():]);h.rehash(o,f.name)
                elif case in ('original-neighbor','original-prestate','original-BSS-clear'):
                    fname='pch-bss-return.bin' if case=='original-neighbor' else 'pch-context-006.bin' if case=='original-prestate' else 'pch-context-phase.bin';f=o/fname;blob=bytearray(f.read_bytes());blob[0]^=1;f.write_bytes(blob);h.rehash(o,fname)
                else:
                    fname='pre.json' if case=='native-prestate' else 'phase.json' if case=='native-BSS-clear' else 'pch-return.json' if case.startswith('native-return-') or case in ('native-neighbor','wrong-return-PC') else 'pch-entry.json' if case.startswith('nonzero-entry-field-') or case in ('wrong-native-this','missing-native-EDI','wrong-native-caller') else 'state.json'
                    def change(j):
                        if case.startswith('nonzero-entry-field-'):j['frontend']['pch_context'][pch.FIELDS[int(case.rsplit('-',1)[1])][0]]=0
                        elif case=='wrong-native-this':j['args']['this']+=4
                        elif case=='missing-native-EDI':j['raw_entry_abi'].pop('edi')
                        elif case=='wrong-native-caller':j['raw_entry_abi']['return_pc']=0
                        elif case=='native-return-EAX':j['eax']=0
                        elif case=='native-return-EDI':j['edi']^=1
                        elif case=='native-return-ESP':j['esp']+=4
                        elif case=='native-neighbor':j['frontend']['source_signature']=0
                        elif case=='native-prestate':j['frontend']['pch_context']['root_image']=0
                        elif case=='native-BSS-clear':j['frontend']['pch_context']['image_count']=17
                        elif case=='wrong-field-layout':j['frontend']['pch_native_layout']['fields']['file_offset'][0]+=4
                        elif case=='wrong-field-map':j['frontend']['storage']['pch_context.original_pages'][0]+=4
                        elif case=='wrong-return-PC':j['pc']+=1
                        else:raise RuntimeError(case)
                    h.edit(n/fname,change);h.rehash(n,fname)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    prospective=json.loads((d.s.PROJECT/'cases/minimal/pch-prospective-trials.json').read_text())['trials'][0]
    raw=json.loads((Path(prospective['native'])/'trial.json').read_text())
    h.rejected('raw-prospective-no-acceptance-audit',lambda:d.s.audit(raw));passed.append('raw-prospective-no-acceptance-audit')
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
