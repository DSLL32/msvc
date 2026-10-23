#!/usr/bin/env python3
"""Prepare only the narrow Spawn normal-_setjmp observation; no runtime."""
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=Path('/root/.ai-tmp/msvc2/watcom10.0a/wlink/main-observer-source-01')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 old=(BASE/'observe-main.c').read_bytes();header=(ROOT/'scripts/main-prefix.h').read_bytes()
 assert hashlib.sha256(old).hexdigest()=='a876631dec0dba01ff17d2f60d034570a638549d8f9e5d16aeef0cf417fe6cfd'
 assert hashlib.sha256(header).hexdigest()=='0419866d93a707139a3a1d0d52c42fdfa7ef17eb800c5a5776fd4f2ec96187cd'
 text=old.decode().replace('#include "main-prefix.h"','#include "main-prefix.h"\n#include "spawn-prefix.h"')
 before='''                if(address!=0x416d3c || !prefix_spawn(&c)) return fail_owned(150,"unexpected-masked-app-frontier");
                return fail_owned(151,"expected-first-frontier-Spawn-before-body");'''
 after='''                initializer_result=spawn_event(address,&c);
                if(initializer_result==2) return fail_owned(151,"expected-callback-frontier-before-body");
                if(initializer_result!=1) return fail_owned(150,"unexpected-Spawn-setjmp-frontier");'''
 assert text.count(before)==1;text=text.replace(before,after).replace('purpose=CRT-bootstrap-then-fully-masked-first-main-prefix','purpose=CRT-bootstrap-then-masked-Spawn-normal-setjmp-prefix')
 h=header.decode();anchor='    memcpy(prefix_expected+0x142e0,prefix_source+0x142e0,232);';assert h.count(anchor)==1
 h=h.replace(anchor,anchor+'''\n    memcpy(prefix_expected+0x6d3c,prefix_source+0x6d3c,51);
    memcpy(prefix_expected+0x23b14,prefix_source+0x23b14,57);
    prefix_expected[0x6d3c]=0xcc; /* actual Spawn entry */
    prefix_expected[0x23b14]=0xcc; /* actual CRT _setjmp entry */
    prefix_expected[0x6d57]=0xcc; /* actual caller continuation */''')
 h=h.replace('restored-main=004242e0..004243c8 restored-CRT=0','restored-main=232 restored-Spawn=51 restored-CRT-setjmp=57')
 h=h.replace('context("frontier-Spawn"','context("app-entry-Spawn"').replace('matched-first-frontier name=Spawn','observed-app-entry name=Spawn')
 a.output.mkdir(exist_ok=False)
 (a.output/'observe-spawn.c').write_text(text);(a.output/'main-prefix.h').write_text(h)
 for name in ['spawn-prefix.h','observer-state.c','observer-state.h']:(a.output/name).write_bytes((ROOT/'scripts'/name).read_bytes())
 record={'purpose':'One exact normal setjmp52 buffer call/return in Spawn; no longjmp/unwind/runtime authorized by this preparation','base_main_source_sha256':hashlib.sha256(old).hexdigest(),'base_main_header_sha256':hashlib.sha256(header).hexdigest(),'sources':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in a.output.iterdir()}}
 (a.output/'source-receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2))
if __name__=='__main__':main()
