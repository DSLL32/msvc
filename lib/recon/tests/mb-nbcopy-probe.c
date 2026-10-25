#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <recon.h>

static unsigned char *(__cdecl *copy_bytes)(unsigned char *,const unsigned char *,size_t)=recon_mbsnbcpy;
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void check(const unsigned char *source,size_t count)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,0x12345678}};
    unsigned char output[160],expected[160],before[108],after[108];unsigned row;
    assert(count<=136);
    memset(expected,0xd7,sizeof(expected));
    strncpy((char *)expected+8,(const char *)source,count);
    for(row=0;row<3;++row) {
        memset(output,0xd7,sizeof(output));
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);assert(copy_bytes(output+8,source,count)==output+8);fp(after);
        assert(!memcmp(output,expected,sizeof(output)) && !memcmp(before,after,108));
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] &&
               recon_GetLastError()==errors[row][2]);
    }
}
int main(int argc,char **argv)
{
    unsigned char text[137],saved[137],out[160],before[108],after[108],mb[257];
    unsigned short table[257],*owner=recon_pctype;unsigned long handles[6];
    unsigned n,count;char *page;int live=argc>1 && !strcmp(argv[1],"live");
    memset(text,'a',sizeof(text));text[136]=0;
    if(argc>1 && !live) {
        if(!strcmp(argv[1],"null-out")){copy_bytes(NULL,text,1);return 42;}
        if(!strcmp(argv[1],"null-source")){copy_bytes(out,NULL,1);return 42;}
        if(!strcmp(argv[1],"out-wrap")){copy_bytes((unsigned char *)(UINTPTR_MAX-2),text,4);return 42;}
        if(!strcmp(argv[1],"source-wrap")){copy_bytes(out,(unsigned char *)UINTPTR_MAX,1);return 42;}
        if(!strcmp(argv[1],"overlap-forward")){copy_bytes(text+1,text,4);return 42;}
        if(!strcmp(argv[1],"overlap-backward")){copy_bytes(text,text+1,4);return 42;}
        if(!strcmp(argv[1],"same")){copy_bytes(text,text,1);return 42;}
        if(!strcmp(argv[1],"nonascii")){text[2]=128;copy_bytes(out,text,4);return 42;}
        if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
        if(!strcmp(argv[1],"codepage"))recon_lc_codepage=1252;
        if(!strcmp(argv[1],"mb-codepage"))recon_mb_codepage=1252;
        if(!strcmp(argv[1],"mb-width"))recon_mb_cur_max=2;
        if(!strcmp(argv[1],"gate"))recon_mb_codepage_gate=1;
        if(!strcmp(argv[1],"partial")) {
            int status;pid_t child;
            unsigned char *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
            assert(shared!=MAP_FAILED);memset(shared,0xd7,4096);text[15]=128;
            child=fork();assert(child>=0);
            if(!child){copy_bytes(shared,text,16);_exit(42);}
            assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
            for(n=0;n<4096;++n)assert(shared[n]==0xd7);
            munmap(shared,4096);return 0;
        }
        copy_bytes(out,text,1);return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    memcpy(mb,recon_mbctype,sizeof(mb));memcpy(table,owner-1,sizeof(table));
    memcpy(handles,recon_lc_handle,sizeof(handles));
    for(n=0;n<128;++n)text[n]=(unsigned char)(n+1);
    text[127]='z';text[128]=0;memcpy(saved,text,sizeof(text));
    for(count=0;count<=136;++count)check(text,count);
    assert(!memcmp(text,saved,sizeof(text)));
    for(n=0;n<=128;++n) {
        memset(text,'X',sizeof(text));text[n]=0;text[n+1]=255;
        check(text,136); /* No inspection after the first NUL. */
    }
    check((const unsigned char *)"precompiled.obj\n",15);
    assert(copy_bytes(NULL,NULL,0)==NULL);
    page=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(page!=MAP_FAILED && !mprotect(page+4096,4096,PROT_NONE));
    memcpy(page+4096-15,"precompiled.obj",15);check((unsigned char *)page+4096-15,15);
    page[4095]=0;check((unsigned char *)page+4095,136);
    assert(copy_bytes((unsigned char *)page+4096,(unsigned char *)page+4096,0)==(unsigned char *)page+4096);
    errno=111;recon_doserrno=222;recon_SetLastError(333);fp(before);
    assert(copy_bytes((unsigned char *)page+4096-15,(const unsigned char *)"precompiled.obj",15)==(unsigned char *)page+4096-15);
    fp(after);assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
    /* Adjacent source and destination spans are permitted. */
    memcpy(page,"abcd",4);assert(copy_bytes((unsigned char *)page+4,(unsigned char *)page,4)==(unsigned char *)page+4);
    assert(!memcmp(page+4,"abcd",4));munmap(page,8192);
    assert(owner==recon_pctype && !memcmp(table,owner-1,sizeof(table)));
    assert(!memcmp(mb,recon_mbctype,sizeof(mb)) && !memcmp(handles,recon_lc_handle,sizeof(handles)));
    assert(!recon_mb_codepage && !recon_lc_codepage && recon_mb_cur_max==1 && !recon_mb_codepage_gate);
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
