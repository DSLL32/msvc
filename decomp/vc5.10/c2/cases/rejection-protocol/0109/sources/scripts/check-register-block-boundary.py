#!/usr/bin/env python3
"""Actual constructor over nonzero backing and forced directory growth (auxiliary only)."""
import argparse,json,subprocess,tempfile,time
from pathlib import Path
from state import BINARY,ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();text=(BINARY/'src/function-records.c').read_text();passed=[]
    with tempfile.TemporaryDirectory(prefix='register-block-boundary-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        folder=Path(temp);h=folder/'observers.c';h.write_text('''#include "function-records.h"
#include "code-records.h"
#include <recon.h>
C2DebugLink *pending_debug_symbols;
static union { unsigned align; unsigned char bytes[2572]; } storage[4];
static union { unsigned align; unsigned char bytes[128]; } directories[2];
static C2RegisterBlock *first_directory[1];
static unsigned allocated,grown,failed;
void *__fastcall c2_pool_allocate(int pool,unsigned bytes) {
    unsigned i,index=allocated;
    if(pool!=1)failed=1;
    if(bytes==2572 && allocated<4) {
        for(i=0;i<2572;++i)storage[index].bytes[i]=(unsigned char)(17+11*index+3*i);
        ++allocated;return storage[index].bytes;
    }
    if(grown<2 && bytes==(grown?16:8)) {
        for(i=0;i<128;++i)directories[grown].bytes[i]=0xa5;
        return directories[grown++].bytes;
    }
    failed=1;return storage[0].bytes;
}
C2RegisterSet *__fastcall new_register_set(int pool) { failed=1;return NULL; }
C2FunctionRegister *__fastcall new_function_register(unsigned char kind) { failed=1;return NULL; }
void __fastcall clear_register_set(C2RegisterSet *set) { failed=1; }
int main(int argc,char **argv,char **environment) {
    unsigned trial,i,j,k,seed,capacity,wanted_grows;
    C2RegisterBlock *block,*expected,*next; C2RegisterBlock **directory;
    for(trial=0;trial<2;++trial) {
        seed=trial?0xfffffff0U:7U; allocated=grown=failed=0;
        function_register_blocks=next_function_register_block=first_directory;
        end_function_register_blocks=first_directory+1;
        first_function_register_block=last_function_register_block=NULL;
        next_function_register_number=seed;
        for(i=0;i<4;++i) {
            block=new_register_block();expected=(C2RegisterBlock*)storage[i].bytes;
            wanted_grows=i==0?0:i==1?1:2;capacity=i==0?1:i==1?2:4;
            directory=wanted_grows?(C2RegisterBlock**)directories[wanted_grows-1].bytes:first_directory;
            if(failed || allocated!=i+1 || grown!=wanted_grows || block!=expected || function_register_blocks!=directory || next_function_register_block!=directory+i+1 || end_function_register_blocks!=directory+capacity || next_function_register_number!=seed+32*(i+1))return 1;
            if(first_function_register_block!=(C2RegisterBlock*)storage[0].bytes || last_function_register_block!=block)return 2;
            for(j=0;j<=i;++j) {
                expected=(C2RegisterBlock*)storage[j].bytes;next=j<i?(C2RegisterBlock*)storage[j+1].bytes:NULL;
                if(directory[j]!=expected || expected->next!=next || expected->cursor!=expected->records || expected->end!=expected->records+32)return 3;
                for(k=12;k<2572;++k) {
                    unsigned row=(k-12)/80,byte=(k-12)%80;
                    unsigned char value=(unsigned char)(17+11*j+3*k);
                    if(byte>=28 && byte<32)value=(unsigned char)((seed+32*j+row)>>((byte-28)*8));
                    if(storage[j].bytes[k]!=value)return 4;
                }
            }
            if(wanted_grows)for(j=(i+1)*4;j<capacity*4;++j)if(directories[wanted_grows-1].bytes[j]!=0xa5)return 5;
        }
    }
    return 0;
}
''')
        prefix=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-Wno-ignored-attributes','-O0','-g','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src')]
        def run(name,source):
            path=folder/(name+'.c');path.write_text(source);exe=folder/name;t=time.monotonic();r=subprocess.run(prefix+[str(path),str(h),'-o',str(exe)],capture_output=True);assert r.returncode==0,r.stderr.decode();build=time.monotonic()-t;t=time.monotonic();r=subprocess.run([str(exe)],capture_output=True);return r.returncode,build,time.monotonic()-t
        result,build,execution=run('actual',text);assert result==0;passed.append({'control':'actual constructor/nonzero untouchedbytes/32numbers/4nodes/twogrowths/unsignedwrap','build_seconds':build,'execution_seconds':execution})
        cases=[('wrong allocation bytes','sizeof(C2RegisterBlock));','sizeof(C2RegisterBlock) - 4);'),('wrong row cursor','block->cursor = block->records;','block->cursor = block->records + 1;'),('wrong row end','block->end = block->records + 32;','block->end = block->records + 31;'),('wrong number increment','reg->number = next_function_register_number++;','reg->number = (next_function_register_number += 2);'),('unmodeled row field clobbered','reg->number = next_function_register_number++;','{ reg->number = next_function_register_number++; reg->kind = 0; }'),('short growth copy','               capacity * sizeof(C2RegisterBlock *));','               (capacity - 1) * sizeof(C2RegisterBlock *));'),('wrong grown cursor','next_function_register_block = function_register_blocks + capacity;','next_function_register_block = function_register_blocks + capacity - 1;'),('wrong grown limit','end_function_register_blocks = function_register_blocks + 2 * capacity;','end_function_register_blocks = function_register_blocks + capacity;'),('missing previous block link','last_function_register_block->next = block;','last_function_register_block->next = NULL;'),('wrong last block','last_function_register_block = block;','last_function_register_block = first_function_register_block;'),('missing directory write','*next_function_register_block++ = block;','++next_function_register_block;'),('self-cycle newblock','block->next = NULL;','block->next = block;')]
        for name,before,after in cases:
            assert text.count(before)==1;source=text.replace(before,after,1);assert source!=text;result,build,execution=run('mutant'+str(len(passed)),source);assert result!=0,name;passed.append({'control':name,'build_seconds':build,'execution_seconds':execution})
    a.output.write_text(json.dumps({'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'source_sha256':sha(text.encode()),'scope':'Actual complete C constructor compiled unchanged; isolated allocation observer only. Two unsigned seeds incl wrap, nonzero unmodeled backing, fourblocks, forced capacity1->2->4 and exact existing-pointer copy. This auxiliary exercises source cold branch; original natural capacity1000 branch remains unvisited/unclaimed. No allocator/dependency/new target vertex credit.'},indent=2)+'\n')
if __name__=='__main__':main()
