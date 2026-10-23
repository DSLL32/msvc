#!/usr/bin/env python3
"""Explicit original CL-to-C1XX observation; never imported by fixtures."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

PROJECT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('wine_transport',Path(__file__).with_name('wine-tool.py'))
t=importlib.util.module_from_spec(spec);spec.loader.exec_module(t)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()


def vector(path):
    raw=path.read_bytes();address,count=struct.unpack_from('<II',raw);cursor=8;strings=[]
    for _ in range(count):
        pointer,length=struct.unpack_from('<II',raw,cursor);cursor+=8
        value=raw[cursor:cursor+length];cursor+=length
        if not pointer or not value.endswith(b'\0') or b'\0' in value[:-1]:raise ValueError('invalid raw vector')
        strings.append(value[:-1].decode('latin1'))
    if cursor!=len(raw):raise ValueError('raw vector length differs')
    return address,strings


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--name',required=True)
    args=parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9_-]+',args.name):raise ValueError('invalid fresh capture name')
    out=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx'/args.name;out.mkdir(exist_ok=False)
    for source,name in ((PROJECT/'build/invoke-contract-probe.exe','invoke-contract-probe.exe'),
                        (PROJECT/'scripts/invoke-contract-probe.c','invoke-contract-probe.c'),
                        (PROJECT/'cases/sources/minimal.cpp','minimal.cpp'),
                        (t.TOOLCHAIN/'bin/MSVCRT.DLL','MSVCRT.DLL')):shutil.copy2(source,out/name)
    command=['wine',t.windows(out/'invoke-contract-probe.exe'),t.windows(t.TOOLCHAIN/'bin/CL.EXE'),
             '/nologo /c /TP /Od minimal.cpp /Fominimal.obj','events.txt']
    environment=t.environment();started=time.perf_counter()
    with (out/'stdout.bin').open('wb') as stdout,(out/'stderr.bin').open('wb') as stderr:
        run=subprocess.run(command,cwd=out,env=environment,stdout=stdout,stderr=stderr,timeout=60)
    elapsed=time.perf_counter()-started
    receipt={'kind':'explicit original CL-to-C1XX invocation; no function acceptance','command':command,
        'exit':run.returncode,'seconds':elapsed,'transport_environment':{key:environment.get(key) for key in
        ('WINEPREFIX','WINEARCH','WINEDLLOVERRIDES','WINEPATH','INCLUDE','LIB')},
        'artifacts':{str(path.relative_to(out)):{'sha256':sha(path),'size':path.stat().st_size}
                     for path in sorted(out.rglob('*')) if path.is_file()}}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    if run.returncode:raise ValueError('original CL failed; preserve capture '+str(out))
    for name,source in (('loaded-c1xx.bin',t.TOOLCHAIN/'bin/C1XX.DLL'),('loaded-msvcrt.bin',t.TOOLCHAIN/'bin/MSVCRT.DLL')):
        if sha(out/name)!=sha(source):raise ValueError('actual loaded provider differs')
    text=(out/'events.txt').read_text();match=re.findall(r'^invoke argc=(\d+) flag=(\d+)$',text,re.M)
    if len(match)!=1 or re.findall(r'^invoke-return status=(\d+)$',text,re.M)!=['0']:raise ValueError('unknown invocation/return')
    argc,mode=map(int,match[0]);address,argv=vector(out/'invoke-argv.bin');_,env=vector(out/'invoke-environment.bin')
    if argc!=len(argv):raise ValueError('raw argc/vector mismatch')
    selected={}
    for line in env:
        key,separator,value=line.partition('=')
        if key.upper() in ('TMP','TEMP','INCLUDE','MSC_CMD_FLAGS','CL','_CL_') and separator:selected[key.upper()]=value
    manifest={'schema':1,'case':'minimal','capture':str(out),'receipt_sha256':sha(out/'receipt.json'),
              'original_argv':argv,'argc':argc,'mode':mode,'return':0,'consumed_environment_seed':selected,
              'source_sha256':sha(out/'minimal.cpp'),'target_sha256':sha(out/'loaded-c1xx.bin'),
              'crt_sha256':sha(out/'loaded-msvcrt.bin'),'ir':{path.name:{'size':path.stat().st_size,'sha256':sha(path)}
                                                            for path in sorted((out/'captured-ir').iterdir())},
              'raw_vectors':['invoke-argv.bin','invoke-environment.bin'],'raw_context':'invoke-context.bin'}
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps({'output':str(out),'argc':argc,'mode':mode,'exit':run.returncode,'seconds':elapsed}))


if __name__=='__main__':main()
