#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static void fp(unsigned char b[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])b)); }
static void errors(void)
{ errno=2;recon_doserrno=2;recon_SetLastError(0); }
static void check(const unsigned char *input,const unsigned char *expected,size_t size)
{
    unsigned char storage[1024],before[108],after[108],mb[257];
    unsigned short table[257],*owner=recon_pctype;size_t n;
    ReconCrtCtypeSelection state=*recon_crt_ctype_selection_state();
    assert(size+16<sizeof(storage));memset(storage,0xd7,sizeof(storage));
    memcpy(storage+8,input,size);memcpy(table,owner-1,sizeof(table));memcpy(mb,recon_mbctype,257);
    errors();fp(before);assert(recon_mbslwr(storage+8)==storage+8);fp(after);
    assert(!memcmp(before,after,108) && errno==2 && recon_doserrno==2 && !recon_GetLastError());
    assert(!memcmp(storage+8,expected,size));
    for(n=0;n<8;++n) assert(storage[n]==0xd7);
    for(n=8+size;n<sizeof(storage);++n) assert(storage[n]==0xd7);
    assert(recon_pctype==owner && !memcmp(table,owner-1,sizeof(table)) && !memcmp(mb,recon_mbctype,257));
    assert(!memcmp(&state,recon_crt_ctype_selection_state(),sizeof(state)));
    assert(!recon_mb_codepage && !recon_mb_codepage_gate && !recon_lc_codepage && recon_mb_cur_max==1);
    for(n=0;n<6;++n) assert(!recon_lc_handle[n]);
}
static void read_span(const char *path,unsigned char *bytes,size_t size)
{
    FILE *f=fopen(path,"rb");assert(f);assert(fread(bytes,1,size,f)==size && fgetc(f)==EOF);fclose(f);
}
int main(int argc,char **argv)
{
    unsigned char input[160],expected[160],actual[32],output[32];
    const char *mode;unsigned n,c;unsigned short foreign[257];
    ReconCrtCtypeSelection *state=(ReconCrtCtypeSelection *)recon_crt_ctype_selection_state();
    assert(argc==4);mode=argv[3];errors();
    if (!strcmp(mode,"null")) {recon_mbslwr(NULL);return 42;}
    if (!strcmp(mode,"mbcp")) recon_mb_codepage=1252;
    if (!strcmp(mode,"mbgate")) recon_mb_codepage_gate=1;
    if (!strcmp(mode,"lccp")) recon_lc_codepage=1252;
    if (!strcmp(mode,"mbmax")) recon_mb_cur_max=2;
    if (!strcmp(mode,"handle")) recon_lc_handle[2]=1033;
    if (!strcmp(mode,"other-handle")) recon_lc_handle[5]=1033;
    if (!strcmp(mode,"mbtable")) recon_mbctype[250]^=1;
    if (!strcmp(mode,"ctype")) recon_pctype['A']^=1;
    if (!strcmp(mode,"eof")) recon_pctype[-1]^=1;
    if (!strcmp(mode,"pointer")) {memcpy(foreign,recon_pctype-1,sizeof(foreign));recon_pctype=foreign+1;}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if (!strcmp(mode,"bound")) state->bound=1;
    if (!strcmp(mode,"calls")) state->calls=1;
    if (!strcmp(mode,"mbcalls")) state->mb_calls=1;
#endif
    if (!strcmp(mode,"errno")) errno=0;
    if (!strcmp(mode,"doserrno")) recon_doserrno=0;
    if (!strcmp(mode,"last")) recon_SetLastError(1234);
    if (!strcmp(mode,"nonascii")) {unsigned char s[]={'A','B',128,0};recon_mbslwr(s);return 42;}
    if (!strcmp(mode,"partial")) {
        unsigned char *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_SHARED,-1,0);
        pid_t child;int status;assert(shared!=MAP_FAILED);memcpy(shared,"ABC\200",5);
        child=fork();assert(child>=0);
        if(!child) {recon_mbslwr(shared);_exit(42);}
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        assert(!memcmp(shared,"ABC\200",5));munmap(shared,4096);puts("prevalidation PASS");return 0;
    }
    if(strcmp(mode,"normal") && strcmp(mode,"live")) {unsigned char s[]="ABC";recon_mbslwr(s);return 42;}
    read_span(argv[1],actual,32);read_span(argv[2],output,32);
    if(!strcmp(mode,"live")) __asm__ __volatile__("fld1\n\tfldpi":::"st");
    check(actual,output,32);
    for(c=1;c<128;++c) {
        input[0]=(unsigned char)c;input[1]='A';input[2]='Z';input[3]=0;input[4]=0x80;input[5]='Q';
        memcpy(expected,input,6);if(c>='A' && c<='Z') expected[0]=(unsigned char)(c+32);
        expected[1]='a';expected[2]='z';check(input,expected,6);
    }
    for(n=0;n<127;++n) {input[n]=(unsigned char)(n+1);expected[n]=(n+1>='A' && n+1<='Z')?(unsigned char)(n+33):(unsigned char)(n+1);}
    input[127]=expected[127]=0;check(input,expected,128);
    input[0]=expected[0]=0;input[1]=expected[1]=0x80;check(input,expected,2);
    if(!strcmp(mode,"live")) __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    puts("actual32B plus129 ASCII spans/alias/tails/errors/full108FP PASS");return 0;
}
