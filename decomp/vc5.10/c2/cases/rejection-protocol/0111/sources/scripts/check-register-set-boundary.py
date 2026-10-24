#!/usr/bin/env python3
"""Actual free-head/header C and native guard over nonzero auxiliary backing."""
import argparse,json,subprocess,tempfile,time
from pathlib import Path
from state import BINARY,ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();text=(BINARY/'src/register-sets.c').read_text();passed=[]
    with tempfile.TemporaryDirectory(prefix='register-set-boundary-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        folder=Path(temp);h=folder/'observers.c';h.write_text('''#define RECON_SHIM_IMPL
#include "register-sets.h"
#include <recon.h>
static C2RegisterSet storage[3];
static unsigned allocated,failed;
void recon_unsupported(const char *reason) { _exit(125); }
void *__fastcall c2_pool_allocate(int pool,unsigned bytes) {
    unsigned i;
    if(pool!=1 || bytes!=12 || allocated>=3) { failed=1;_exit(5); }
    for(i=0;i<12;++i)((unsigned char*)&storage[allocated])[i]=(unsigned char)(17+3*i+allocated*7);
    return &storage[allocated++];
}
int main(int argc,char **argv,char **env) {
    C2RegisterSet *value,*canary=(C2RegisterSet*)0x13579bdfU;
    unsigned i,j;unsigned char old[12];
    free_register_sets[0]=canary;free_register_sets[1]=NULL;
    if(argc>1) { new_register_set(atoi(argv[1]));return 31; }
    value=new_register_set(1);
    if(failed || allocated!=1 || value!=&storage[0] || free_register_sets[1] || free_register_sets[0]!=canary)return 1;
    for(i=0;i<12;++i) {
        unsigned char expected=(unsigned char)(17+3*i);
        if(i<4 || (i>=8 && i<11))expected=0;
        if(i==4)expected=1;
        if(((unsigned char*)value)[i]!=expected)return 2;
    }
    for(i=0;i<2;++i)for(j=0;j<12;++j)((unsigned char*)&storage[i])[j]=(unsigned char)(0x51+7*i+j);
    storage[0].links.next_free=&storage[1];storage[1].links.next_free=NULL;
    free_register_sets[1]=&storage[0];
    for(i=0;i<2;++i) {
        memcpy(old,&storage[i],12);value=new_register_set(1);
        if(value!=&storage[i] || allocated!=1 || free_register_sets[1]!=(i?NULL:&storage[1]) || free_register_sets[0]!=canary)return 3;
        for(j=0;j<12;++j) {
            unsigned char expected=old[j];
            if(j<4 || (j>=8 && j<11))expected=0;
            if(j==4)expected=1;
            if(((unsigned char*)value)[j]!=expected)return 4;
        }
    }
    return 0;
}
''')
        prefix=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-Wno-ignored-attributes','-O0','-g','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src')]
        def run(name,source):
            path=folder/(name+'.c');path.write_text(source);exe=folder/name;t=time.monotonic();r=subprocess.run(prefix+[str(path),str(h),'-o',str(exe)],capture_output=True);assert r.returncode==0,r.stderr.decode();build=time.monotonic()-t;t=time.monotonic();r=subprocess.run([str(exe)],capture_output=True);return exe,r.returncode,build,time.monotonic()-t
        exe,result,build,execution=run('actual',text);assert result==0,result;passed.append({'control':'actual allocation and two-cell reuse/nonzero padding/highstate/slot0','build_seconds':build,'execution_seconds':execution})
        for index in [0,2,12,-1]:
            r=subprocess.run([str(exe),str(index)],capture_output=True);assert r.returncode==125,(index,r.returncode);passed.append({'control':'guard unmodeled original-valid-or-unknown pool '+str(index),'status':125})
        cases=[('wrong allocation size','sizeof(C2RegisterSet));','sizeof(C2RegisterSet)-4);'),('wrong allocation pool','c2_pool_allocate(pool,','c2_pool_allocate(0,'),('missing pop',' = set->links.next_free;',' = set;'),('wrong pool byte','set->pool = (unsigned char)pool;','set->pool = 0;'),('wrong first','set->links.first = NULL;','set->links.first = (C2RegisterSetNode*)set;'),('clobbered highstate','set->state &= 0xff000000U;','set->state = 0;'),('lowstate not cleared','set->state &= 0xff000000U;','set->state &= 0xffffff00U;'),('padding clobbered','set->pool = (unsigned char)pool;','set->pool = (unsigned char)pool; set->pending05[0]=0;'),('wrong return','return set;','return NULL;'),('wrong table alias','set = free_register_sets[C2_REGISTER_SET_POOL(pool)];','set = free_register_sets[0];')]
        for name,before,after in cases:
            assert text.count(before)==1,(name,text.count(before));source=text.replace(before,after,1);assert source!=text;_,result,build,execution=run('mutant'+str(len(passed)),source);assert result!=0,name;passed.append({'control':name,'build_seconds':build,'execution_seconds':execution})
    a.output.write_text(json.dumps({'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'source_sha256':sha(text.encode()),'header_sha256':sha((BINARY/'src/register-sets.h').read_bytes()),'scope':'Actual source compiled unchanged/native guarded data projection. Nonzero backing/reuse and highbyte/padding/slot0 tests auxiliary only, not original path or unknownslot data/layout credit. Pools0/2/12/-1 rejected as unmodeled before access; pool12 valid original, not an input validity claim.'},indent=2)+'\n')
if __name__=='__main__':main()
