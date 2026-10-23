#!/usr/bin/env python3
"""Derive the focused first-main helper from frozen, reviewed observer source.
Source preparation only; no original/helper execution or acceptance.
"""
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 source=ROOT/'scripts/observe-help.c';data=source.read_bytes()
 assert hashlib.sha256(data).hexdigest()=='0e6dba710f98f99821afccbc673bbc9d6f20f3ca1249f32a331303dd9845f8df'
 text=data.decode()
 def replace(old,new):
  nonlocal text
  assert text.count(old)==1,old[:90]
  text=text.replace(old,new)
 replace('int main(int argc, char **argv)\n{','#include "main-prefix.h"\n\nint main(int argc, char **argv)\n{')
 replace('    BYTE byte;\n    HANDLE input, output, error;','    BYTE byte;\n    int initializer_result;\n    HANDLE input, output, error;')
 replace('purpose=unmasked-target-diagnostic-only','purpose=CRT-bootstrap-then-fully-masked-first-main-prefix')
 replace('    memset(&security,0,sizeof(security)); security.nLength=sizeof(security); security.bInheritHandle=TRUE;','    if(!prefix_source_file(argv[1])) return 120;\n    memset(&security,0,sizeof(security)); security.nLength=sizeof(security); security.bInheritHandle=TRUE;')
 replace('            for(i=0;i<thunk_count;++i) { thunks[i].address=', '            if(!prefix_arm_initializers()) return fail_owned(150,"bootstrap-initializers-preimage");\n            for(i=0;i<thunk_count;++i) { thunks[i].address=')
 replace('                log_context("PE-entry",&c); entry_seen=1; c.Eip=address;', '                if(!prefix_table(0) || !prefix_pins()) return fail_owned(150,"bootstrap-PE-entry-pins");\n                log_context("PE-entry",&c); entry_seen=1; c.Eip=address;')
 replace('                log_context("main",&c); main_seen=1; c.Eip=address;', '                if(!prefix_at_main(&c)) return fail_owned(150,"bootstrap-main-or-whole-mask");\n                log_context("main",&c); main_seen=1; c.Eip=address;')
 replace('            } else if(code==EXCEPTION_BREAKPOINT && pending_return(address)) {', '''            } else if(code==EXCEPTION_BREAKPOINT && prefix_masked) {
                if(address!=0x416d3c || !prefix_spawn(&c)) return fail_owned(150,"unexpected-masked-app-frontier");
                return fail_owned(151,"expected-first-frontier-Spawn-before-body");
            } else if(code==EXCEPTION_BREAKPOINT && (initializer_result=prefix_initializer(address,&c))!=0) {
                if(initializer_result!=1) return fail_owned(150,"bootstrap-initializer-owner");
            } else if(code==EXCEPTION_BREAKPOINT && pending_return(address)) {''')
 a.output.mkdir(exist_ok=False)
 (a.output/'observe-main.c').write_text(text)
 (a.output/'main-prefix.h').write_bytes((ROOT/'scripts/main-prefix.h').read_bytes())
 for name in ['observer-state.c','observer-state.h']:(a.output/name).write_bytes((ROOT/'scripts'/name).read_bytes())
 record={'purpose':'Focused first main→Spawn frontier only; source/compiler preparation, no runtime authorization','base_source_path':str(source),'base_source_sha256':hashlib.sha256(data).hexdigest(),'sources':{str(f.name):hashlib.sha256(f.read_bytes()).hexdigest() for f in a.output.iterdir()}}
 (a.output/'source-receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2))
if __name__=='__main__':main()
