#!/usr/bin/env python3
"""Actual factory across every kind byte, free-list reuse and capacity boundaries."""
import argparse,json,subprocess,tempfile,time
from pathlib import Path
from state import BINARY,ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();whole=(BINARY/'src/function-records.c').read_text();text=whole[whole.index('C2FunctionRegister *__fastcall new_function_register(unsigned char kind)'):];passed=[]
    with tempfile.TemporaryDirectory(prefix='register-node-boundary-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        folder=Path(temp);h=folder/'observer.c';h.write_text('''#include "register-sets.h"
#include <recon.h>
C2RegisterBlock *plain_register_block,*tracked_register_block,*secondary_register_block,*extra_register_block,*default_register_block;
C2FunctionRegister *free_plain_register_nodes,*free_tracked_register_nodes;
C2RegisterSet *tracked_register_set,*secondary_register_set;
static C2RegisterBlock blocks[6];
static C2FunctionRegister recycled[2];
static C2RegisterSet sets[2];
static unsigned grown,added,add_number;static C2RegisterSet *add_set;
C2RegisterBlock *new_register_block(void) { ++grown;return &blocks[5]; }
void __fastcall add_register_to_set(C2RegisterSet *set,unsigned number) { ++added;add_number=number;add_set=set; }
static int family(unsigned kind) {
    if(kind==3 || kind==6)return 0;
    if(kind==4 || kind==5)return 1;
    if(kind==7 || kind==8)return 2;
    if(kind==15)return 3;
    return 4;
}
int main(int argc,char **argv,char **environment) {
    unsigned scenario,kind,i,j,fam,reuse,expect_grow; C2FunctionRegister *reg,*wanted;
    for(scenario=0;scenario<4;++scenario)for(kind=0;kind<256;++kind) {
        for(i=0;i<sizeof(blocks);++i)((unsigned char*)blocks)[i]=(unsigned char)(0x27+i*3);
        for(i=0;i<6;++i) {
            blocks[i].next=NULL;blocks[i].cursor=blocks[i].records;blocks[i].end=blocks[i].records+32;
            for(j=0;j<32;++j)blocks[i].records[j].number=1000+32*i+j;
        }
        for(i=0;i<sizeof(recycled);++i)((unsigned char*)recycled)[i]=(unsigned char)(0x63+i*7);
        recycled[0].number=0xabcde123U;recycled[0].next_free=&recycled[1];
        plain_register_block=&blocks[0];tracked_register_block=&blocks[1];secondary_register_block=&blocks[2];extra_register_block=&blocks[3];default_register_block=&blocks[4];
        free_plain_register_nodes=free_tracked_register_nodes=NULL;
        tracked_register_set=&sets[0];secondary_register_set=&sets[1];grown=added=0;add_number=0;add_set=NULL;
        if(scenario==1)for(i=0;i<5;++i)blocks[i].cursor=blocks[i].end;
        if(scenario==2)extra_register_block=NULL;
        if(scenario==3)free_plain_register_nodes=free_tracked_register_nodes=&recycled[0];
        fam=family(kind);reuse=scenario==3 && fam<2;expect_grow=scenario==1 || (scenario==2 && fam==3);
        wanted=reuse?&recycled[0]:expect_grow?&blocks[5].records[0]:&blocks[fam].records[0];
        reg=new_function_register((unsigned char)kind);
        if(reg!=wanted || grown!=(reuse?0:expect_grow) || added!=(fam==1 || fam==2))return 1;
        if(fam==1 || fam==2)if(add_set!=&sets[fam-1] || add_number!=wanted->number)return 2;
        if(reuse) {
            if(wanted->number!=0xabcde123U)return 3;
            for(i=0;i<80;++i) {
                unsigned char value=i>=28 && i<32?(unsigned char)(0xabcde123U>>((i-28)*8)):0;
                if(((unsigned char*)wanted)[i]!=value)return 4;
            }
        }
        if(free_plain_register_nodes!=(scenario==3?(fam==0?&recycled[1]:&recycled[0]):NULL) || free_tracked_register_nodes!=(scenario==3?(fam==1?&recycled[1]:&recycled[0]):NULL))return 5;
        if(!reuse) {
            C2RegisterBlock *owner=expect_grow?&blocks[5]:&blocks[fam];
            if(owner->cursor!=owner->records+1 || wanted->number!=1000+32*(expect_grow?5:fam))return 6;
            /* Allocation path preserves every existing row byte, not just number. */
            for(i=0;i<80;++i) {
                unsigned char value=(unsigned char)(0x27+((unsigned char*)wanted-(unsigned char*)blocks+i)*3);
                if(i>=28 && i<32)value=(unsigned char)(wanted->number>>((i-28)*8));
                if(((unsigned char*)wanted)[i]!=value)return 7;
            }
            if((fam==0?plain_register_block:fam==1?tracked_register_block:fam==2?secondary_register_block:fam==3?extra_register_block:default_register_block)!=owner)return 8;
        }
    }
    return 0;
}
''')
        prefix=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-Wno-ignored-attributes','-O0','-g','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src')]
        def run(name,source):
            path=folder/(name+'.c');path.write_text('#include "function-records.h"\n#include <recon.h>\n'+source);exe=folder/name;t=time.monotonic();r=subprocess.run(prefix+[str(path),str(h),'-o',str(exe)],capture_output=True);assert r.returncode==0,r.stderr.decode();build=time.monotonic()-t;t=time.monotonic();r=subprocess.run([str(exe)],capture_output=True);return r.returncode,build,time.monotonic()-t
        result,build,execution=run('actual',text);assert result==0,result;passed.append({'control':'actual untouched factory all256kinds x4scenarios/reuse/highnumber/capacity/extraNULL','build_seconds':build,'execution_seconds':execution})
        cases=[('wrong plain free head','free_plain_register_nodes = reg->next_free;','free_plain_register_nodes = reg;'),('wrong tracked free head','free_tracked_register_nodes = reg->next_free;','free_tracked_register_nodes = reg;'),('wrong memset extent','memset(reg, 0, sizeof(C2FunctionRegister));','memset(reg, 0, sizeof(C2FunctionRegister)-4);'),('number not preserved','reg->number = number;','reg->number = 0;'),('wrong tracked set','add_register_to_set(tracked_register_set, reg->number);','add_register_to_set(secondary_register_set, reg->number);'),('wrong secondary set','add_register_to_set(secondary_register_set, reg->number);','add_register_to_set(tracked_register_set, reg->number);'),('wrong number input','add_register_to_set(tracked_register_set, reg->number);','add_register_to_set(tracked_register_set, reg->number+1);'),('wrong cursor stride','reg = block->cursor++;','reg = (block->cursor += 2)-2;'),('clobbered allocation row','reg = block->cursor++;','reg = block->cursor++; reg->kind=0;'),('capacity bound','block->cursor >= block->end','block->cursor > block->end'),('wrong return','return reg;','return NULL;')]
        for name,before,after in cases:
            assert before in text;source=text.replace(before,after);assert source!=text;result,build,execution=run('mutant'+str(len(passed)),source);assert result!=0,name;passed.append({'control':name,'build_seconds':build,'execution_seconds':execution})
    a.output.write_text(json.dumps({'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'whole_source_sha256':sha(whole.encode()),'factory_body_sha256':sha(text.encode()),'header_sha256':sha((BINARY/'src/function-records.h').read_bytes()),'scope':'Actual complete factory body extracted verbatim for auxiliary observer compilation. new_register_block/add_register_to_set are explicit observers, not dependency implementations/native fixture paths/new target vertices. Allkind/cold/reuse source behavior tested; original natural execution only kind4 before pending set insertion.'},indent=2)+'\n')
if __name__=='__main__':main()
