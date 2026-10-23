#!/usr/bin/env python3
"""Reject missing per-instance inputs, object aliases and BSS-reset drift."""
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
    d=load('pool_instance_decoder','check-callback.py');h=load('pool_instance_helpers','check-projection-negative.py');history=load('pool_instance_history','check-history.py')
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--packet',type=Path,default=d.s.PROJECT/'cases/minimal/pool02-wrapper-trials.json');args=parser.parse_args()
    packet=json.loads(args.packet.read_text());started=time.perf_counter();passed=[]
    cases={None:('original-wrong-receiver','original-wrong-argument','native-wrong-receiver','native-wrong-argument','missing-raw-hidden-input','false-native-entry-PC','storage-alias','previous-instance-changed','original-previous-instance-changed','wrong-result','original-wrong-caller','native-wrong-caller'),
           'restore-nonzero':('missing-native-nonzero','missing-original-nonzero','original-BSS-not-cleared','native-BSS-not-cleared'),
           'restore-zero':('new-object-scalar-as-pointer','new-object-pointer-misprojected',)}
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        model=json.loads((native/'state-model.json').read_text());number=model['pool_projection']['count'];idx=number-1
        selected=list(cases[row['seed']])
        if row['seed'] is None and model.get('pool_entry_order_observed'):selected += ['duplicate-native-call','missing-native-call','reordered-native-calls']
        for case in selected:
            with tempfile.TemporaryDirectory(prefix='pool-instance-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o=Path(tmp)/'original';n=Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case in ('duplicate-native-call','missing-native-call','reordered-native-calls'):
                    f=n/'pool-entry-order.txt';lines=f.read_text().splitlines(True)
                    if case=='duplicate-native-call':lines.append(lines[-1])
                    elif case=='missing-native-call':lines.pop()
                    else:lines[0],lines[1]=lines[1],lines[0]
                    f.write_text(''.join(lines));h.rehash(n,f.name)
                elif case.startswith('original-wrong-'):
                    f=o/'events.txt';text=f.read_text();m=list(re.finditer(r'^trap seq=\d+ rva=000389d5 .*$',text,re.M))[-1];line=m[0]
                    if case=='original-wrong-receiver':line=line.replace('ecx=%08x'%model['pool_projection']['original_bases'][idx],'ecx=%08x'%model['pool_projection']['original_bases'][0])
                    elif case=='original-wrong-caller':line=re.sub(r'stack=\w+,','stack=104389d4,',line)
                    else:line=re.sub(r'(stack=\w+,)00000001,',r'\g<1>00000002,',line)
                    f.write_text(text[:m.start()]+line+text[m.end():]);h.rehash(o,f.name)
                elif case in ('missing-original-nonzero','original-BSS-not-cleared','original-previous-instance-changed'):
                    suffix='006' if case=='missing-original-nonzero' else 'phase' if case=='original-BSS-not-cleared' else '%03d'%int(re.findall(r'^trap seq=(\d+) rva=000389d5 ',(o/'events.txt').read_text(),re.M)[-1])
                    name='pool-%02d-'%(1 if case=='original-previous-instance-changed' else number)+suffix+'.bin';f=o/name;raw=bytearray(f.read_bytes());struct.pack_into('<I',raw,0,0 if case=='missing-original-nonzero' else 1);f.write_bytes(raw);h.rehash(o,name)
                else:
                    name='pre.json' if case in ('missing-native-nonzero','new-object-scalar-as-pointer','new-object-pointer-misprojected') else 'phase.json' if case=='native-BSS-not-cleared' else 'state.json' if case in ('storage-alias','wrong-result') else 'pool-entry-%02d.json'%number
                    def change(j):
                        f=j['frontend']
                        if case=='native-wrong-receiver':j['args']['this']=f['storage']['pool_01.cursor'][0]
                        elif case=='native-wrong-caller':j['raw_entry_abi']['return_pc']=0
                        elif case=='native-wrong-argument':j['args']['minimum_pages']=2
                        elif case=='missing-raw-hidden-input':j['raw_entry_abi'].pop('ecx')
                        elif case=='false-native-entry-PC':j['pc']+=1
                        elif case=='storage-alias':f['storage']['pool_%02d.cursor'%number][0]=f['storage']['pool_01.cursor'][0]
                        elif case=='previous-instance-changed':f['pools'][0]['minimum_pages']=0
                        elif case=='wrong-result':f['pools'][idx]['minimum_pages']=2
                        elif case=='missing-native-nonzero':f['pools'][idx]['cursor']=0
                        elif case=='native-BSS-not-cleared':f['pools'][idx]['remaining']=23
                        elif case=='new-object-scalar-as-pointer':f['pools'][idx]['minimum_pages']=f['saved_state']+37
                        elif case=='new-object-pointer-misprojected':f['pools'][idx]['arena']+=4
                        else:raise RuntimeError(case)
                    h.edit(n/name,change);h.rehash(n,name)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
