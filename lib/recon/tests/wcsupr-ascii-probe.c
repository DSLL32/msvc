#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <recon.h>

static WCHAR *(__cdecl *convert)(WCHAR *)=recon_wcsupr;
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void check(const WCHAR *input,const WCHAR *expected,size_t count)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,0x12345678}};
    WCHAR storage[160];unsigned char before[108],after[108];size_t row,n;
    assert(count+8<160);
    for(row=0;row<sizeof(errors)/sizeof(errors[0]);++row) {
        memset(storage,0xd7,sizeof(storage));memcpy(storage+4,input,count*2);
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);assert(convert(storage+4)==storage+4);fp(after);
        assert(!memcmp(before,after,108));
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] &&
               recon_GetLastError()==errors[row][2]);
        assert(!memcmp(storage+4,expected,count*2));
        for(n=0;n<4;++n)assert(storage[n]==0xd7d7);
        for(n=count+4;n<160;++n)assert(storage[n]==0xd7d7);
    }
}
int main(int argc,char **argv)
{
    static const char lower[]="abcdefghijklmnopqrstuvwxyz",upper[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const WCHAR language[]={'L','A','N','G','U','A','G','E',0};
    static const WCHAR mixed[]={'l','a','n','g','u','a','g','e',0};
    WCHAR input[130],expected[130],*end;unsigned n,c;
    unsigned char before[108],after[108],mb[257];unsigned short table[257],*owner=recon_pctype;
    unsigned long handles[6];char *page;
    if(argc>1) {
        if(!strcmp(argv[1],"null")){convert(NULL);return 42;}
        if(!strcmp(argv[1],"nonascii")){WCHAR s[]={0x80,0};convert(s);return 42;}
        if(!strcmp(argv[1],"late-nonascii")){WCHAR s[]={'a','z',0x100,0};convert(s);return 42;}
        if(!strcmp(argv[1],"handle"))recon_lc_handle[2]=1033;
        if(!strcmp(argv[1],"codepage"))recon_lc_codepage=1252;
        if(!strcmp(argv[1],"mb-codepage"))recon_mb_codepage=1252;
        if(!strcmp(argv[1],"mb-width"))recon_mb_cur_max=2;
        if(!strcmp(argv[1],"partial")) {
            WCHAR original[]={'a','z',0x100,0};int status;pid_t child;
            WCHAR *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
            assert(shared!=MAP_FAILED);memcpy(shared,original,sizeof(original));
            child=fork();assert(child>=0);
            if(!child){convert(shared);_exit(42);}
            assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
            assert(!memcmp(shared,original,sizeof(original)));munmap(shared,4096);return 0;
        }
        if(strcmp(argv[1],"live")){WCHAR s[]={'a',0};convert(s);return 42;}
        __asm__ __volatile__("fld1\n\tfldpi":::"st");
    }
    memcpy(mb,recon_mbctype,sizeof(mb));memcpy(table,owner-1,sizeof(table));
    memcpy(handles,recon_lc_handle,sizeof(handles));
    check(language,language,9);check(mixed,language,9);
    for(c=1;c<128;++c) {
        input[0]=(WCHAR)c;input[1]='a';input[2]='Z';input[3]=0;input[4]=0x100;input[5]='q';
        memcpy(expected,input,12);expected[1]='A';
        for(n=0;n<26;++n)if(c==(unsigned char)lower[n])expected[0]=upper[n];
        check(input,expected,6);
    }
    for(c=1;c<128;++c) {
        input[c-1]=expected[c-1]=(WCHAR)c;
        for(n=0;n<26;++n)if(c==(unsigned char)lower[n])expected[c-1]=upper[n];
    }
    input[127]=expected[127]=0;check(input,expected,128);
    input[0]=expected[0]=0;input[1]=expected[1]=0x100;check(input,expected,2);
    page=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(page!=MAP_FAILED);memset(page,0xd7,4096);assert(!mprotect(page+4096,4096,PROT_NONE));
    end=(WCHAR *)(page+4096-sizeof(mixed));memcpy(end,mixed,sizeof(mixed));
    errno=2;recon_doserrno=2;recon_SetLastError(0);
    fp(before);assert(convert(end)==end);fp(after);
    assert(!memcmp(end,language,sizeof(language)) && !memcmp(before,after,108));
    assert(errno==2 && recon_doserrno==2 && !recon_GetLastError());
    for(n=0;n<8;++n)assert(((unsigned char*)end)[-1-(int)n]==0xd7);
    end=(WCHAR *)(page+4094);*end=0;assert(convert(end)==end && !*end);
    munmap(page,8192);
    assert(recon_pctype==owner && !memcmp(table,owner-1,sizeof(table)));
    assert(!memcmp(mb,recon_mbctype,sizeof(mb)) && !memcmp(handles,recon_lc_handle,sizeof(handles)));
    assert(!recon_lc_codepage && !recon_mb_codepage && recon_mb_cur_max==1);
    if(argc>1)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
