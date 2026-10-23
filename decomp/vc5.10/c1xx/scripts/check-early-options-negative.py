#!/usr/bin/env python3
"""Reject wrong argv/state, caller ownership and scan-before-parser writes."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


def main():
    d=load('early_decoder','check-callback.py');h=load('early_negative','check-projection-negative.py');history=load('early_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/early-options-trial.json').read_text());original,native=Path(packet['original']),Path(packet['native']);started=time.perf_counter();passed=[]
    audit=history.configure(d,original,native,saved_record=packet);d.check(original,native,audit_path=audit)
    for case in ['original-argv','original-parser-argument','original-parser-state','original-parser-return','original-BSS','original-initialized','original-saved','native-entry-argv','native-entry-formal','native-entry-caller','native-parser-argument','native-parser-state','native-parser-formal','native-parser-caller','native-parser-PC','native-neighbor','native-final-argument']:
        with tempfile.TemporaryDirectory(prefix='early-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case.startswith('original-'):
                if case in ('original-BSS','original-initialized','original-saved'):
                    final='%03d'%int(re.findall(r'^trap seq=(\d+) ',(o/'events.txt').read_text(),re.M)[-1]);name='early-options-final-bss.bin' if case=='original-BSS' else 'frontend-data-'+final+'.bin' if case=='original-initialized' else 'frontend-snapshot-'+final+'.bin';f=o/name;blob=bytearray(f.read_bytes());blob[-1]^=1;f.write_bytes(blob)
                else:
                    f=o/'events.txt';rva='00038d07' if case=='original-argv' else '00038d56';field={'original-argv':1,'original-parser-argument':1,'original-parser-state':2,'original-parser-return':0}[case]
                    def change(match):
                        words=match[2].split(',');words[field]='%08x'%(int(words[field],16)+1);return match[1]+','.join(words)
                    text,count=re.subn(r'(^trap seq=\d+ rva='+rva+r'[^\n]*stack=)([0-9a-f,]+)',change,f.read_text(),count=1,flags=re.M)
                    if count!=1:raise ValueError('missing mutation '+case)
                    f.write_text(text)
                h.rehash(o,f.name)
            else:
                name='early-options-entry.json' if case.startswith('native-entry-') else 'state.json' if case=='native-final-argument' else 'early-option-entry.json'
                def change(j):
                    if case=='native-entry-argv':j['args']['argv']+=4
                    elif case in ('native-entry-formal','native-parser-formal'):j['formal_inventory'].pop()
                    elif case in ('native-entry-caller','native-parser-caller'):j['raw_entry_abi']['return_pc']+=1
                    elif case in ('native-parser-argument','native-final-argument'):j['args']['argument']+=4
                    elif case=='native-parser-state':j['args']['state']=1
                    elif case=='native-parser-PC':j['pc']+=1
                    elif case=='native-neighbor':j['frontend']['source_signature']^=1
                    else:raise ValueError(case)
                h.edit(n/name,change);h.rehash(n,name)
            h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
