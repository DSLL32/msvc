#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void check(const WCHAR *left,const WCHAR *right,int expected)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,0x12345678}};
    unsigned char before[108],after[108];unsigned row;
    for(row=0;row<3;++row) {
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);assert(recon_wcsicmp(left,right)==expected);fp(after);
        assert(!memcmp(before,after,108));
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
    }
}
int main(int argc,char **argv)
{
    WCHAR left[12]={'M','O','V','E','A','B','L','E',0},right[12]={'A','L','T',0};
    WCHAR first[2],second[2],saved_left[12],saved_right[12];unsigned value,folded;
    unsigned char mb[257];unsigned short table[257],*owner=recon_pctype;unsigned long handles[6];
    char *page;int live=argc>1 && !strcmp(argv[1],"live");
    if(argc>1 && !live) {
        if(!strcmp(argv[1],"null-left")){recon_wcsicmp(NULL,right);return 42;}
        if(!strcmp(argv[1],"null-right")){recon_wcsicmp(left,NULL);return 42;}
        if(!strcmp(argv[1],"nonascii-left"))left[0]=128;
        if(!strcmp(argv[1],"nonascii-right"))right[0]=0x100;
        if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
        if(!strcmp(argv[1],"codepage"))recon_lc_codepage=1252;
        if(!strcmp(argv[1],"mb-codepage"))recon_mb_codepage=1252;
        if(!strcmp(argv[1],"mb-width"))recon_mb_cur_max=2;
        recon_wcsicmp(left,right);return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    memcpy(mb,recon_mbctype,sizeof(mb));memcpy(table,owner-1,sizeof(table));memcpy(handles,recon_lc_handle,sizeof(handles));
    memcpy(saved_left,left,sizeof(left));memcpy(saved_right,right,sizeof(right));
    check(left,right,12);check(right,left,-12);check(left,left,0);
    assert(!memcmp(left,saved_left,sizeof(left)) && !memcmp(right,saved_right,sizeof(right)));
    first[1]=second[1]=0;
    for(value=0;value<128;++value) {
        folded=value>='A' && value<='Z'?value+32:value;
        first[0]=(WCHAR)value;second[0]=(WCHAR)folded;check(first,second,0);
        second[0]=0;check(first,second,(int)folded);check(second,first,-(int)folded);
    }
    {WCHAR a[]={'a','b',0,0x100},b[]={'A','B','c',0};check(a,b,-99);check(b,a,99);}
    /* Original stops at the first unequal pair/NUL, without reading tails. */
    page=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(page!=MAP_FAILED && !mprotect(page+4096,4096,PROT_NONE));
    *(WCHAR *)(page+4094)='M';second[0]='A';check((WCHAR *)(page+4094),second,12);
    *(WCHAR *)(page+4094)=0;second[0]=0;check((WCHAR *)(page+4094),second,0);
    assert(!munmap(page,8192));
    assert(owner==recon_pctype && !memcmp(table,owner-1,sizeof(table)));
    assert(!memcmp(mb,recon_mbctype,sizeof(mb)) && !memcmp(handles,recon_lc_handle,sizeof(handles)));
    assert(!recon_mb_codepage && !recon_lc_codepage && recon_mb_cur_max==1);
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
