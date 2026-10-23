#!/usr/bin/env python3
"""Bounded actual-entry/return and nonzero pool-assignment rejection controls."""
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
    d=load('pool_member_decoder','check-callback.py');h=load('pool_member_helpers','check-projection-negative.py');history=load('pool_member_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/pool-member-trials.json').read_text());started=time.perf_counter();passed=[]
    cases={None:('original-return-this','original-RET4','original-return-PC','missing-return-observation','native-return-this','native-RET4','native-return-PC','native-entry-PC','raw-native-receiver','raw-native-argument','original-return-fields','native-return-fields','neighbor-state'),
           'restore-nonzero':('missing-original-constructor-seed','missing-native-constructor-seed','native-final-cursor','original-final-minimum-pages'),
           'restore-zero':('missing-raw-seed-event',)}
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        for case in cases[row['seed']]:
            with tempfile.TemporaryDirectory(prefix='pool-member-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o=Path(tmp)/'original';n=Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case in ('original-return-this','original-RET4','original-return-PC','missing-return-observation','missing-raw-seed-event'):
                    f=o/'events.txt';text=f.read_text();m=re.search(r'^pool-return rva=000389d4 eax=(\w+) esp=(\w+)$',text,re.M)
                    if case=='original-return-this':text=text[:m.start()]+m[0].replace('eax=104c0ac8','eax=104c0acc')+text[m.end():]
                    elif case=='original-RET4':text=text[:m.start()]+m[0].replace('esp='+m[2],'esp=%08x'%(int(m[2],16)-4))+text[m.end():]
                    elif case=='original-return-PC':text=text.replace('pool-return rva=000389d4','pool-return rva=000389d3')
                    elif case=='missing-return-observation':text=text[:m.start()]+text[m.end():]
                    else:text=re.sub(r'^pool-constructor-seed .*?\n','',text,flags=re.M)
                    f.write_text(text);h.rehash(o,f.name)
                elif case in ('original-return-fields','missing-original-constructor-seed','original-final-minimum-pages'):
                    rows=re.findall(r'^trap seq=(\d+) rva=(\w+)',(o/'events.txt').read_text(),re.M)
                    suffix='pool-return' if case=='original-return-fields' else '%03d'%int(next(seq for seq,rva in rows if rva=='000389d5')) if case=='missing-original-constructor-seed' else '%03d'%int(rows[-1][0])
                    name='pool-01-'+suffix+'.bin';f=o/name;raw=bytearray(f.read_bytes());struct.pack_into('<I',raw,0 if case=='missing-original-constructor-seed' else 8,0);f.write_bytes(raw);h.rehash(o,name)
                else:
                    name='pool-entry.json' if case in ('native-entry-PC','raw-native-receiver','raw-native-argument','missing-native-constructor-seed') else 'state.json' if case=='native-final-cursor' else 'pool-return.json'
                    def change(j):
                        if case=='native-return-this':j['eax']+=4
                        elif case=='native-RET4':j['esp']-=4
                        elif case in ('native-return-PC','native-entry-PC'):j['pc']+=1
                        elif case=='raw-native-receiver':j['raw_entry_abi']['ecx']+=4
                        elif case=='raw-native-argument':j['raw_entry_abi']['stack_argument']=2
                        elif case=='missing-native-constructor-seed':j['frontend']['pools'][0]['remaining']=0
                        elif case=='native-final-cursor':j['frontend']['pools'][0]['cursor']=j['frontend']['saved_state']
                        elif case=='native-return-fields':j['frontend']['pools'][0]['minimum_pages']=0
                        elif case=='neighbor-state':j['frontend']['source_signature']=37
                        else:raise RuntimeError(case)
                    h.edit(n/name,change);h.rehash(n,name)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
