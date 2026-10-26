#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <recon.h>
static void byte_vectors(unsigned limit)
{
    unsigned c,seed;char source;WCHAR output[2];unsigned short cw,sw,cw_after,sw_after;int result;
    for(seed=0;seed<2;++seed)for(c=0;c<limit;++c) {
        source=(char)c;output[0]=0xaabb;output[1]=0xccdd;
        errno=seed?EDOM:0;recon_doserrno=73;recon_SetLastError(91);
        __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
        result=recon_mbtowc(output,&source,1);
        __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(cw_after),"=m"(sw_after));
        assert(result==(c?1:0) && output[0]==c && output[1]==0xccdd);
        assert(errno==(seed?EDOM:0) && recon_doserrno==73 && recon_GetLastError()==91);
        assert(cw==cw_after && sw==sw_after);
        assert(recon_mbtowc(NULL,&source,1)==(c?1:0));
    }
    output[0]=0xaabb;output[1]=0xccdd;
    assert(!recon_mbtowc(output,NULL,1) && output[0]==0xaabb && output[1]==0xccdd);
    assert(!recon_mbtowc(output,"x",0) && output[0]==0xaabb && output[1]==0xccdd);
}
int main(int argc,char **argv)
{
    char source='w';WCHAR *output;size_t count=1;pid_t child;int status;unsigned n;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    unsigned char mb[257];unsigned short table[257];unsigned long handles[6];
    ReconCrtCtypeSelection selection;
#endif
    const char *mode=argc>1?argv[1]:"";
    byte_vectors(256); /* The prior C-locale byte domain remains intact. */
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    /* Reuse the already-retained isolated-selection receipt; no new original
     * API observation or locale provider is introduced by these native tests. */
    recon_crt_bind_empty_ctype_1252("e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210",
      "d2dbc20e792aaba5b168a4c9c4c233ee306b407770d5be7527b5fdd204b857d5");
    errno=0;recon_doserrno=0;recon_SetLastError(0);
    assert(recon_setlocale(2,""));
    if(strcmp(mode,"before-mb"))assert(!recon_setmbcp(-4));
    if(!*mode) {
        memcpy(mb,recon_mbctype,sizeof(mb));memcpy(table,recon_pctype-1,sizeof(table));
        memcpy(handles,recon_lc_handle,sizeof(handles));selection=*recon_crt_ctype_selection_state();
        byte_vectors(128);
        assert(!memcmp(mb,recon_mbctype,sizeof(mb)) && !memcmp(table,recon_pctype-1,sizeof(table)));
        assert(!memcmp(handles,recon_lc_handle,sizeof(handles)) && !memcmp(&selection,recon_crt_ctype_selection_state(),sizeof(selection)));
        puts("mbtowc isolated1252 ASCII/WCHAR16/tails/errors/state and retained CP0 vectors pass");return 0;
    }
    if(!strcmp(mode,"high"))source=(char)0x80;
    if(!strcmp(mode,"count"))count=2;
    if(!strcmp(mode,"mbcp"))recon_mb_codepage=932;
    if(!strcmp(mode,"gate"))recon_mb_codepage_gate=1;
    if(!strcmp(mode,"max"))recon_mb_cur_max=2;
    if(!strcmp(mode,"lccp"))recon_lc_codepage=932;
    if(!strcmp(mode,"handle"))recon_lc_handle[2]=2057;
    if(!strcmp(mode,"other-handle"))recon_lc_handle[0]=1033;
    if(!strcmp(mode,"mbtable"))recon_mbctype['A'+1]^=1;
    if(!strcmp(mode,"ctype"))recon_pctype['A']^=1;
    if(!strcmp(mode,"bound"))((ReconCrtCtypeSelection *)recon_crt_ctype_selection_state())->bound=0;
    if(!strcmp(mode,"calls"))((ReconCrtCtypeSelection *)recon_crt_ctype_selection_state())->calls=2;
#else
    recon_lc_handle[2]=1033;
#endif
    output=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(output!=MAP_FAILED);
    for(n=0;n<4;++n)output[n]=0xaabb;
    child=fork();assert(child>=0);
    if(!child) {
        WCHAR *destination=output;
        if(!strcmp(mode,"align"))destination=(WCHAR *)((char *)output+1);
        if(!strcmp(mode,"overlap")) {((char *)output)[0]='w';source='w';recon_mbtowc(output,(char *)output,1);}
        else recon_mbtowc(destination,&source,count);
        _exit(42);
    }
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
    for(n=0;n<4;++n)assert(output[n]==(!strcmp(mode,"overlap") && !n?0xaa77:0xaabb));
    assert(!munmap(output,4096));return 0;
}
