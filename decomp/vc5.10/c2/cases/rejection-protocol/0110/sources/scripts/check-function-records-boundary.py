#!/usr/bin/env python3
"""Compile the actual initializer with isolated child observers, not child implementations."""
import argparse,json,subprocess,tempfile,time
from pathlib import Path
from state import BINARY,ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();text=(BINARY/'src/function-records.c').read_text();passed=[]
    with tempfile.TemporaryDirectory(prefix='function-records-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        folder=Path(temp);harness=folder/'observers.c';harness.write_text('''#include "function-records.h"
#include "code-records.h"
#include <recon.h>
C2DebugLink *pending_debug_symbols;
static C2RegisterBlock *directory[1000];
static unsigned block_ids[4],set_ids[2],phase,failed;
static C2FunctionRegister node;
static unsigned char saved[80];
static C2RegisterBlock **block_slots[4];
void *__fastcall c2_pool_allocate(int pool,unsigned bytes) {
    if(phase || pool!=1 || bytes!=4000)failed=1;phase=1;return directory;
}
C2RegisterBlock *new_register_block(void) {
    unsigned i,index=phase-1;
    if(phase<1 || phase>4 || function_register_blocks!=directory || next_function_register_block!=directory || end_function_register_blocks!=directory+1000 || next_function_register_number || first_function_register_block || last_function_register_block)failed=1;
    if(extra_register_block!=(C2RegisterBlock*)1 || free_plain_register_nodes!=(C2FunctionRegister*)1 || free_tracked_register_nodes!=(C2FunctionRegister*)1 || pending_debug_symbols!=(C2DebugLink*)1)failed=1;
    for(i=0;i<4;++i)if(*block_slots[i]!=(i<index?(C2RegisterBlock*)&block_ids[i]:(C2RegisterBlock*)1))failed=1;
    ++phase;return (C2RegisterBlock*)&block_ids[index<4?index:0];
}
C2RegisterSet *__fastcall new_register_set(int pool) {
    unsigned index=phase-5;
    if(phase<5 || phase>6 || pool!=1 || extra_register_block || free_plain_register_nodes || free_tracked_register_nodes || pending_debug_symbols)failed=1;
    if(secondary_register_set!=(phase==5?(C2RegisterSet*)1:(C2RegisterSet*)&set_ids[0]) || tracked_register_set!=(C2RegisterSet*)1)failed=1;
    ++phase;return (C2RegisterSet*)&set_ids[index<2?index:0];
}
C2FunctionRegister *__fastcall new_function_register(unsigned char kind) {
    if(phase!=7 || kind!=4 || secondary_register_set!=(C2RegisterSet*)&set_ids[0] || tracked_register_set!=(C2RegisterSet*)&set_ids[1])failed=1;
    ++phase;return &node;
}
void __fastcall clear_register_set(C2RegisterSet *set) {
    unsigned i;
    if(phase!=8 || set!=(C2RegisterSet*)&set_ids[1] || empty_function_register!=&node || node.kind!=16)failed=1;
    for(i=0;i<80;++i)if(i!=4 && ((unsigned char*)&node)[i]!=saved[i])failed=1;
    ++phase;
}
int main(int argc,char **argv,char **environment) {
    unsigned i;
    block_slots[0]=&tracked_register_block;block_slots[1]=&secondary_register_block;block_slots[2]=&plain_register_block;block_slots[3]=&default_register_block;
    for(i=0;i<4;++i)*block_slots[i]=(C2RegisterBlock*)1;
    function_register_blocks=next_function_register_block=end_function_register_blocks=(C2RegisterBlock**)1;
    next_function_register_number=123;first_function_register_block=last_function_register_block=extra_register_block=(C2RegisterBlock*)1;
    free_plain_register_nodes=free_tracked_register_nodes=empty_function_register=(C2FunctionRegister*)1;
    pending_debug_symbols=(C2DebugLink*)1;secondary_register_set=tracked_register_set=(C2RegisterSet*)1;
    for(i=0;i<80;++i)saved[i]=((unsigned char*)&node)[i]=(unsigned char)(17+11*i);
    for(i=0;i<1000;++i)directory[i]=(C2RegisterBlock*)(i+1);
    initialize_function_records();
    if(phase!=9 || failed)return 1;
    for(i=0;i<1000;++i)if(directory[i]!=(C2RegisterBlock*)(i+1))return 2;
    return 0;
}
''')
        prefix=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-Wno-ignored-attributes','-O0','-g','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src')]
        def run(name,source):
            path=folder/(name+'.c');path.write_text(source);exe=folder/name;t=time.monotonic();r=subprocess.run(prefix+[str(path),str(harness),'-o',str(exe)],capture_output=True);assert r.returncode==0,r.stderr.decode();build=time.monotonic()-t;t=time.monotonic();r=subprocess.run([str(exe)],capture_output=True);return r.returncode,build,time.monotonic()-t
        result,build,execution=run('actual',text);assert result==0;passed.append({'control':'actual whole initializer nonzero identities/ordered calls/exact one kind BYTE','build_seconds':build,'execution_seconds':execution})
        cases=[('directory bytes','1000 * sizeof(C2RegisterBlock *)','999 * sizeof(C2RegisterBlock *)'),('directory end','function_register_blocks + 1000','function_register_blocks + 999'),('directory cursor','next_function_register_block = function_register_blocks;','next_function_register_block = function_register_blocks + 1;'),('number reset','next_function_register_number = 0;','next_function_register_number = 1;'),('first-list reset','first_function_register_block = NULL;','first_function_register_block = (C2RegisterBlock*)1;'),('last-list reset','last_function_register_block = NULL;','last_function_register_block = (C2RegisterBlock*)1;'),('early deferred-list clear','    tracked_register_block = new_register_block();','    pending_debug_symbols = NULL;\n    tracked_register_block = new_register_block();'),('reuse wrong block identity','secondary_register_block = new_register_block();','tracked_register_block = new_register_block();'),('missing existing pending-list reset','    pending_debug_symbols = NULL;',''),('wrong set pool','secondary_register_set = new_register_set(1);','secondary_register_set = new_register_set(2);'),('wrong new-node kind','new_function_register(4)','new_function_register(3)'),('wrong marker kind','kind = 0x10;','kind = 0x11;'),('kind overwrites adjacent unknown byte','empty_function_register->kind = 0x10;','*(unsigned short *)&empty_function_register->kind = 0x10;'),('clears wrong set','clear_register_set(tracked_register_set);','clear_register_set(secondary_register_set);'),('missing final child','    clear_register_set(tracked_register_set);','')]
        for name,before,after in cases:
            assert text.count(before)==1;source=text.replace(before,after,1);assert source!=text;result,build,execution=run('mutant'+str(len(passed)),source);assert result!=0,name;passed.append({'control':name,'build_seconds':build,'execution_seconds':execution})
    a.output.write_text(json.dumps({'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'source_sha256':sha(text.encode()),'scope':'Actual initializer compiled unchanged, isolated observers supply distinct child-return address identities only. No child body/ABI or unreachable actual path credit. Full current natural first-child raw proof remains independent.'},indent=2)+'\n')
if __name__=='__main__':main()
