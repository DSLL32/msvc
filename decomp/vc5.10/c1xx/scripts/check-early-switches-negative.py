#!/usr/bin/env python3
"""Reject parser ABI/settings drift and the first external text-data transport."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


def main():
    d=load('switch_decoder','check-callback.py');h=load('switch_negative','check-projection-negative.py');history=load('switch_history','check-history.py');switches=load('switch_state','early_switches_state.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/early-switches-trials.json').read_text());started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        cases=['native-argument','native-state','native-formal','native-return','native-RET0','native-register','native-result-setting','original-result','original-RET0','original-argument','native-neighbor','native-saved','original-snapshot-tail'] if row['seed'] is None else ['native-live-'+name for name in switches.FIELDS]+['native-saved-'+name for name in switches.FIELDS] if row['seed']=='restore-nonzero' else ['native-phase-'+name for name in switches.FIELDS]
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='switch-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case.startswith('original-'):
                    if case=='original-snapshot-tail':
                        final='%03d'%int(re.findall(r'^trap seq=(\d+) ',(o/'events.txt').read_text(),re.M)[-1]);f=o/('frontend-snapshot-'+final+'.bin');blob=bytearray(f.read_bytes());blob[-1]^=1;f.write_bytes(blob)
                    else:
                        f=o/'events.txt';pattern={'original-result':r'^(early-option-return .*eax=)([0-9a-f]+)','original-RET0':r'^(early-option-return .*esp=)([0-9a-f]+)','original-argument':r'^(early-option-entry .*argument=)([0-9a-f]+)'}[case]
                        text,count=re.subn(pattern,lambda m:m[1]+'%08x'%(int(m[2],16)+1),f.read_text(),count=1,flags=re.M)
                        if count!=1:raise ValueError('missing mutation '+case)
                        f.write_text(text)
                    h.rehash(o,f.name)
                else:
                    fname='pre.json' if case.startswith(('native-live-','native-saved-')) else 'phase.json' if case.startswith('native-phase-') else 'early-switch-entries.json' if case in ('native-argument','native-state','native-formal') else 'early-switch-returns.json' if case in ('native-return','native-RET0','native-register','native-result-setting') else 'state.json'
                    def change(j):
                        if case=='native-argument':j[0]['args'][0]+=1
                        elif case=='native-state':j[0]['args'][1]=1
                        elif case=='native-formal':j[0]['formal_inventory'].pop()
                        elif case=='native-return':j[0]['eax']=1
                        elif case=='native-RET0':j[0]['esp']+=8
                        elif case=='native-register':j[0]['esi']+=1
                        elif case=='native-result-setting':j[-1]['values'][0]=63
                        elif case=='native-neighbor':j['frontend']['source_signature']^=1
                        elif case=='native-saved':j['frontend']['saved_option_zb']=64
                        elif case.startswith('native-live-'):j['frontend'][case[len('native-live-'):]]+=1
                        elif case.startswith('native-saved-'):j['frontend']['saved_'+case[len('native-saved-'):]]+=1
                        elif case.startswith('native-phase-'):j['frontend'][case[len('native-phase-'):]]+=1
                        else:raise ValueError(case)
                    h.edit(n/fname,change);h.rehash(n,fname)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append({'seed':row['seed'],'rejected':case})
    row=packet['trials'][0];original,native=Path(row['original']),Path(row['native']);trial=json.loads((native/'trial.json').read_text());metadata=json.loads((native/'graph-metadata.json').read_text());snapshot=(original/'relocated-text.bin').read_bytes();masked=(original/'masked-text.bin').read_bytes()
    for case in ('wrong-shared-start','data-as-code-envelope','wrong-data-owner','data-as-entry','padding-in-range','wrong-metadata-range','wrong-target','changed-table','changed-readback','restored-padding'):
        cfg=copy.deepcopy(trial);meta=copy.deepcopy(metadata);raw=bytearray(snapshot);actual=bytearray(masked);record=next(r for r in cfg['driver'] if r['name']=='_process_early_option');data=next(r for r in cfg['ranges'] if r.get('kind')=='text_data');at=0x38de0-0x1000
        if case=='wrong-shared-start':record['shared'][0][0]-=6
        elif case=='data-as-code-envelope':record['end']=0x10438df0
        elif case=='wrong-data-owner':data['owner']='_main'
        elif case=='data-as-entry':data['entry']=1
        elif case=='padding-in-range':data['start_rva']-=6
        elif case=='wrong-metadata-range':meta['functions']['_process_early_option']['original']['text_data'][0][0]-=6
        elif case=='wrong-target':meta['functions']['_process_early_option']['original']['switches'][str(0x10438d69)][0]=0x10438d86
        elif case=='changed-table':struct.pack_into('<I',raw,at,0x10438d86);actual[at:at+16]=raw[at:at+16]
        elif case=='changed-readback':actual[at]^=1
        elif case=='restored-padding':actual[at-1]=0
        h.rejected(case,lambda:switches.check_table(d.s.require,cfg,meta,raw,actual,d.s.ROOT/'toolchains/5.10/bin/C1XX.DLL'));passed.append({'kind':'direct exact table/ownership guard','rejected':case})
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
