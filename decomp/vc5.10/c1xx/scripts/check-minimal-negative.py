#!/usr/bin/env python3
"""Reject incorrect real-DLL input, state, trap and candidate proof."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile

spec=importlib.util.spec_from_file_location('minimal',Path(__file__).with_name('check-minimal.py'))
d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)


def edit(path,change):
    value=json.loads(path.read_text());change(value);path.write_text(json.dumps(value))


def rehash(directory,name):
    path=directory/name
    edit(directory/'receipt.json',lambda j:j['artifacts'].update({name:{'sha256':d.s.sha(path),'size':path.stat().st_size}}))


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
    args=parser.parse_args();d.check(args.original,args.native)
    cases=('raw-original-argv','raw-argc','wrong-BOOL','premature-mode-store','false-ELF-PC',
           'missing-frontier','missing-cold-owner','wrong-entry-context','changed-ELF','stale-native','changed-source-content')
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='minimal-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temp:
            root=Path(temp);o=root/'original';n=root/'native';shutil.copytree(args.original,o);shutil.copytree(args.native,n)
            audit=root/'audit.json';shutil.copy2(d.s.PROJECT/'build/audit-proof.json',audit)
            if case=='raw-original-argv':
                path=o/'invoke-entry-argv.bin';raw=path.read_bytes().replace(b'minimal.cpp\0',b'minimal.cXp\0');path.write_bytes(raw);rehash(o,path.name)
            elif case=='raw-argc':
                path=o/'events.txt';path.write_text(path.read_text().replace(',0000001a,',',0000001b,',1));rehash(o,path.name)
            elif case in ('wrong-BOOL','premature-mode-store','false-ELF-PC'):
                def change(value):
                    if case=='wrong-BOOL':value['invocation']['mode']=0
                    elif case=='premature-mode-store':value['target_state']['compiler_mode']=1
                    else:value['pc']+=1
                edit(n/'state.json',change);rehash(n,'state.json')
            elif case=='missing-frontier':
                path=o/'events.txt';path.write_text(path.read_text().replace('frontier rva=00038550\n',''));rehash(o,path.name)
            elif case=='missing-cold-owner':
                edit(audit,lambda j:j['report']['owned_code_chunks']['original']['functions'].update({'_InvokeCompilerPass@12':[[0x104384d4,0x10438550]]}))
            elif case=='wrong-entry-context':
                path=o/'invoke-entry-context.bin';raw=bytearray(path.read_bytes());struct.pack_into('<I',raw,184,0x10438551);path.write_bytes(raw);rehash(o,path.name)
            elif case=='changed-ELF':
                path=n/'native-executable.elf';path.write_bytes(path.read_bytes()+b'\0');rehash(n,path.name)
            elif case=='stale-native':edit(n/'receipt.json',lambda j:j.update(native_sha256='0'*64))
            else:
                path=next((n/'source-content').iterdir());path.write_bytes(path.read_bytes()+b'\n')
            try:d.check(o,n,audit_path=audit)
            except (ValueError,KeyError,TypeError,struct.error):continue
            raise RuntimeError('accepted altered minimal proof '+case)
    print(json.dumps({'count':len(cases),'rejected':list(cases)}))


if __name__=='__main__':main()
