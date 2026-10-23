#!/usr/bin/env python3
"""Reject altered actual main argc/argv and post-initializer mode evidence."""
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
    d=load('main_frontier','check-callback.py');h=load('main_negative','check-projection-negative.py');history=load('main_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/empty42-trials.json').read_text());row=packet['trials'][0];original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit);started=time.perf_counter();passed=[]
    for case in ('native-argc','native-argv','native-mode','original-argc','original-argv','original-mode'):
        with tempfile.TemporaryDirectory(prefix='main-frontier-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case.startswith('native-'):
                def change(j):
                    if case=='native-mode':j['target_state']['compiler_mode']=0
                    else:j['args'][case[7:]]+=1
                h.edit(n/'state.json',change);h.rehash(n,'state.json')
            elif case=='original-mode':
                rows=re.findall(r'^trap seq=(\d+) rva=00038bfd', (o/'events.txt').read_text(),re.M);f=o/('state-%03d.bin'%int(rows[0]));blob=bytearray(f.read_bytes());struct.pack_into('<I',blob,8,0);f.write_bytes(blob);h.rehash(o,f.name)
            else:
                f=o/'events.txt';text=f.read_text()
                def change(match):
                    words=match[2].split(',');index=1 if case=='original-argc' else 2;words[index]='%08x'%(int(words[index],16)+1);return match[1]+','.join(words)
                text=re.sub(r'(^trap seq=\d+ rva=00038bfd[^\n]*stack=)([0-9a-f,]+)',change,text,flags=re.M);f.write_text(text);h.rehash(o,f.name)
            h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
