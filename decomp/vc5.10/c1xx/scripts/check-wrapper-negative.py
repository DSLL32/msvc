#!/usr/bin/env python3
"""Verify a split initializer cannot absorb its nested constructor."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args()
    d=load('wrapper','check-callback.py');helpers=load('wrapper_helpers','check-projection-negative.py');d.check(args.original,args.native)
    trial=json.loads((args.original/'trial.json').read_text())
    second=trial['trial']=='_initialize_frontend_09'
    wrapper=trial['trial'];constructor='_initialize_output_buffer_02' if second else '_initialize_output_buffer_01'
    first,entry,continuation,last=(0x104386d4,0x104386db,0x10438715,0x10438723) if second else (0x1043865f,0x10438666,0x104386a0,0x104386ae)
    cases=('missing-continuation','absorbed-constructor','wrong-original-frontier','constructor-unmasked','false-native-PC','extra-constructor-credit')
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='wrapper-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
            root=Path(temporary);o=root/'original';n=root/'native';shutil.copytree(args.original,o);shutil.copytree(args.native,n)
            audit=root/'audit.json';shutil.copy2(d.s.PROJECT/'build/audit-proof.json',audit)
            if case in ('missing-continuation','absorbed-constructor','extra-constructor-credit'):
                def change(j):
                    owners=j['report']['owned_code_chunks']['original']['functions']
                    if case=='missing-continuation':owners[wrapper]=[[first,entry]]
                    elif case=='absorbed-constructor':owners[wrapper]=[[first,last]]
                    else:owners[constructor]=[[entry,continuation]]
                helpers.edit(audit,change)
            elif case=='wrong-original-frontier':
                path=o/'events.txt';path.write_text(path.read_text().replace('frontier rva=%08x'%(entry-0x10400000),'frontier rva=000386d4'));helpers.rehash(o,path.name)
            elif case=='constructor-unmasked':
                path=o/'masked-text.bin';raw=bytearray(path.read_bytes());raw[entry-0x10400000-0x1000]=0x33;path.write_bytes(raw);helpers.rehash(o,path.name)
            else:helpers.edit(n/'state.json',lambda j:j.update(pc=j['pc']+1));helpers.rehash(n,'state.json')
            helpers.rejected(case,lambda:d.check(o,n,audit_path=audit))
    print(json.dumps({'result':'PASS','count':len(cases),'rejected':cases},indent=2))


if __name__=='__main__':main()
