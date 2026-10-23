#!/usr/bin/env python3
"""Reject altered genuine allocator/first-copy evidence; never execute originals."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile

spec=importlib.util.spec_from_file_location('projection_negatives',Path(__file__).with_name('check-projection-negative.py'))
helpers=importlib.util.module_from_spec(spec);spec.loader.exec_module(helpers)
d=helpers.d


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
    args=parser.parse_args();d.check(args.original,args.native)
    cases=('allocation-size','allocation-prestate','missing-copy-state','copy-before-reset','snapshot-identity',
           'snapshot-CRT-alias','missing-allocator-cold-owner','wrong-CRT-malloc','original-copy-diff','false-INT3-PC')
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='allocator-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temp:
            root=Path(temp);o=root/'original';n=root/'native';shutil.copytree(args.original,o);shutil.copytree(args.native,n)
            audit=root/'audit.json';shutil.copy2(d.s.PROJECT/'build/audit-proof.json',audit)
            if case=='missing-allocator-cold-owner':
                helpers.edit(audit,lambda j:j['report']['owned_code_chunks']['original']['functions'].update({'_checked_malloc':[[0x104233ba,0x104233d7]]}))
            elif case=='wrong-CRT-malloc':
                path=o/'events.txt';path.write_text(path.read_text().replace('provider iat=000c90a8 resolved=78001498','provider iat=000c90a8 resolved=78001499'));helpers.rehash(o,path.name)
            elif case=='original-copy-diff':
                path=o/'frontend-snapshot-phase.bin';raw=bytearray(path.read_bytes());raw[0]^=1;path.write_bytes(raw);helpers.rehash(o,path.name)
            else:
                name='allocation-entry.json' if case in ('allocation-size','allocation-prestate') else 'phase.json' if case in ('missing-copy-state','copy-before-reset','snapshot-identity') else 'state.json'
                def change(j):
                    if case=='allocation-size':j['args']['bytes']=8516
                    elif case=='allocation-prestate':j['frontend']['saved_state']=1
                    elif case=='missing-copy-state':j['frontend']['saved_first']=0
                    elif case=='copy-before-reset':j['frontend']['first_invocation']=0
                    elif case=='snapshot-identity':j['frontend']['saved_state']+=4
                    elif case=='snapshot-CRT-alias':j['frontend']['saved_state']=j['target_state']['onexit_begin']
                    else:j['pc']+=1
                helpers.edit(n/name,change);helpers.rehash(n,name)
            helpers.rejected(case,lambda:d.check(o,n,audit_path=audit))
    print(json.dumps({'result':'PASS','count':len(cases),'rejected':cases},indent=2))


if __name__=='__main__':main()
