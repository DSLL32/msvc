#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
static int (__cdecl *compare)(const unsigned char *,const unsigned char *,size_t)=_mbsnbcmp;
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
struct Pair { const char *left,*right;size_t count;int expected; };
static void check(const unsigned char *left,const unsigned char *right,size_t count,int expected,unsigned row)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,333}};
    unsigned char before[108],after[108],mb[257];unsigned short ctype[257];
    memcpy(mb,recon_mbctype,sizeof(mb));memcpy(ctype,recon_pctype-1,sizeof(ctype));
    errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
    fp(before);assert(compare(left,right,count)==expected);fp(after);
    assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
    assert(!memcmp(before,after,108) && !memcmp(mb,recon_mbctype,sizeof(mb)) && !memcmp(ctype,recon_pctype-1,sizeof(ctype)));
}
int main(int argc,char **argv)
{
    static const struct Pair pairs[]={
        {"__imp__PostQuitMessage@4","__imp_??_",9,1},{"__imp_??_","__imp__PostQuitMessage@4",9,-1},
        {"__imp_??_","__imp_??_",9,0},{"a","A",1,1},{"A","a",1,-1},
        {"abc","abd",2,0},{"abc","abd",3,-1},{"ab","abc",3,-1},{"abc","ab",3,1},
        {"","",999,0},{"\0\200","\0\377",9,0},{"\1\200"," \377",9,-1},
        {"\177","\176",1,1},{"\t"," ",1,-1}
    };
    unsigned row,n;int live=argc>1 && !strcmp(argv[1],"live");unsigned char *memory,*left,*right;
    if(argc>1 && !live) {
        if(!strcmp(argv[1],"null")){compare(NULL,(const unsigned char *)"a",1);return 42;}
        if(!strcmp(argv[1],"high")){compare((const unsigned char *)"\200",(const unsigned char *)"a",1);return 42;}
        if(!strcmp(argv[1],"gate"))recon_mb_codepage_gate=1;
        else if(!strcmp(argv[1],"mbcp"))recon_mb_codepage=1252;
        else if(!strcmp(argv[1],"crtpage"))recon_lc_codepage=1252;
        else if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
        else if(!strcmp(argv[1],"mbmax"))recon_mb_cur_max=2;
        else assert(0);
        compare(NULL,NULL,0);return 42;
    }
    memory=mmap(NULL,4*4096,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(memory!=MAP_FAILED);
    assert(!mprotect(memory+4096,4096,PROT_READ|PROT_WRITE) && !mprotect(memory+3*4096,4096,PROT_READ|PROT_WRITE));
    left=memory+8192-9;right=memory+16384-9;
    memcpy(left,"__imp__Po",9);memcpy(right,"__imp_??_",9);
    assert(!mprotect(memory+4096,4096,PROT_READ) && !mprotect(memory+3*4096,4096,PROT_READ));
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(row=0;row<3;++row) {
        for(n=0;n<sizeof(pairs)/sizeof(pairs[0]);++n)
            check((const unsigned char *)pairs[n].left,(const unsigned char *)pairs[n].right,pairs[n].count,pairs[n].expected,row);
        check(NULL,NULL,0,0,row);check(memory,memory,0,0,row);
        check(left,right,9,1,row);check(left,left,9,0,row);
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    assert(!memcmp(left,"__imp__Po",9) && !memcmp(right,"__imp_??_",9) && !munmap(memory,4*4096));return 0;
}
