#include <assert.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static void vectors(void)
{
    unsigned c,seed;char one[2],wide[2048];size_t result;
    unsigned short cw,sw,new_cw,new_sw;unsigned char mb[257];unsigned short table[257];
    unsigned long handles[6];ReconCrtCtypeSelection selection;char *page;
    memcpy(mb,recon_mbctype,sizeof(mb));memcpy(table,recon_pctype-1,sizeof(table));
    memcpy(handles,recon_lc_handle,sizeof(handles));selection=*recon_crt_ctype_selection_state();
    assert(_mbstrlen("")==0 && _mbstrlen("program.c(19) : warning C4124: ")==31);
    for(seed=0;seed<2;++seed)for(c=1;c<128;++c) {
        one[0]=(char)c;one[1]=0;errno=seed?EDOM:0;recon_doserrno=73;recon_SetLastError(91);
        __asm__ __volatile__("fld1; fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
        result=_mbstrlen(one);
        __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(new_cw),"=m"(new_sw));
        assert(result==1 && one[0]==(char)c && !one[1]);
        assert(errno==(seed?EDOM:0) && recon_doserrno==73 && recon_GetLastError()==91);
        assert(cw==new_cw && sw==new_sw);__asm__ __volatile__("fstp %%st(0)":::"st");
    }
    memset(wide,'a',sizeof(wide));wide[2047]=0;assert(_mbstrlen(wide)==2047);
    for(c=0;c<2047;++c)assert(wide[c]=='a');assert(!wide[2047]);
    page=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(page!=MAP_FAILED);
    memcpy(page,"read-only",10);page[4095]=0;assert(!mprotect(page,4096,PROT_READ));
    assert(!mprotect(page+4096,4096,PROT_NONE));
    assert(_mbstrlen(page)==9 && _mbstrlen(page+4095)==0);assert(!munmap(page,8192));
    assert(!memcmp(mb,recon_mbctype,sizeof(mb)) && !memcmp(table,recon_pctype-1,sizeof(table)));
    assert(!memcmp(handles,recon_lc_handle,sizeof(handles)) && !memcmp(&selection,recon_crt_ctype_selection_state(),sizeof(selection)));
}
#endif
int main(int argc,char **argv)
{
#ifndef RECON_CRT_PROFILE_MSVCRT_510
#ifdef _mbstrlen
#error Other original-name profiles stay unmapped.
#endif
    (void)argc;(void)argv;recon_mbstrlen("ASCII");return 42;
#else
    const char *mode=argc>1?argv[1]:"";char text[2048];
    if(!*mode)vectors();
    /* Reuse the existing isolated1252 selection contract/native fixture API;
     * no new original API observation or locale/provider state is invented. */
    recon_crt_bind_empty_ctype_1252("e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210",
      "d2dbc20e792aaba5b168a4c9c4c233ee306b407770d5be7527b5fdd204b857d5");
    errno=0;recon_doserrno=0;recon_SetLastError(0);assert(recon_setlocale(2,""));
    assert(!recon_setmbcp(-4));
    if(!*mode) {vectors();return 0;}
    strcpy(text,"ASCII");
    if(!strcmp(mode,"null")) {_mbstrlen(NULL);return 42;}
    if(!strcmp(mode,"wrap")) {_mbstrlen((const char *)0xfffffffeUL);return 42;}
    if(!strcmp(mode,"high"))text[1]=(char)128;
    if(!strcmp(mode,"long"))memset(text,'A',sizeof(text));
    if(!strcmp(mode,"mbcp"))recon_mb_codepage=932;
    if(!strcmp(mode,"gate"))recon_mb_codepage_gate=1;
    if(!strcmp(mode,"max"))recon_mb_cur_max=2;
    if(!strcmp(mode,"lccp"))recon_lc_codepage=932;
    if(!strcmp(mode,"handle"))recon_lc_handle[2]=2057;
    if(!strcmp(mode,"other-handle"))recon_lc_handle[0]=1033;
    if(!strcmp(mode,"mbtable"))recon_mbctype['A'+1]^=1;
    if(!strcmp(mode,"ctype"))recon_pctype['A']^=1;
    _mbstrlen(text);return 42;
#endif
}
