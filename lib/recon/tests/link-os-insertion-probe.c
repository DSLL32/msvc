#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static const char provider[]="e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210";
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"middle",*text="ALPHA=one\0MIDDLE=two\0ZED=last\0";
    const char *expected="ALPHA=one\0LINK_REPRO_NAME=\\deflib.rsp\0MIDDLE=two\0ZED=last\0";
    size_t bytes=30,n,expected_bytes=57;WCHAR block[128],saved[128];
    char *args[]={"LINK.EXE",NULL},*env[]={"CRT_ONLY=kept",NULL};
    unsigned char before[108],after[108];BOOL (WINAPI *typed)(LPCSTR,LPCSTR)=recon_SetEnvironmentVariableA;
    if(!strcmp(mode,"front")){text="ZED=last\0";bytes=10;expected="LINK_REPRO_NAME=\\deflib.rsp\0ZED=last\0";expected_bytes=37;}
    if(!strcmp(mode,"append")){text="ALPHA=one\0";bytes=11;expected="ALPHA=one\0LINK_REPRO_NAME=\\deflib.rsp\0";expected_bytes=38;}
    /* Derive MULTISZ extents from the actual terminal double NUL. */
    for(bytes=0;text[bytes] || text[bytes+1];++bytes){}bytes+=2;
    for(expected_bytes=0;expected[expected_bytes] || expected[expected_bytes+1];++expected_bytes){}expected_bytes+=2;
    for(n=0;n<bytes;++n)block[n]=(unsigned char)text[n];
    if(!strcmp(mode,"existing-os")){
        text="link_repro_name=existing\0";for(bytes=0;text[bytes] || text[bytes+1];++bytes){}bytes+=2;
        for(n=0;n<bytes;++n)block[n]=(unsigned char)text[n];
    }
    if(!strcmp(mode,"existing-crt"))env[0]="link_repro_name=existing";
    assert(!setenv("LINK_REPRO_NAME","host-control",1));
    if(!strcmp(mode,"unbound"))return typed("LINK_REPRO_NAME","\\deflib.rsp");
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    typed("LINK_REPRO_NAME","\\deflib.rsp");return 42;
#endif
    recon_bind_process_os_environment(block,bytes,provider);recon_crt_bind_arguments(1,args,env);recon_crt_initenv=env;
    memcpy(saved,block,bytes*sizeof(*block));
    if(!strcmp(mode,"crt-vector"))recon_crt_environment=args;
    if(!strcmp(mode,"crt-update"))recon_crt_environment_updates=1;
    if(!strcmp(mode,"locale"))recon_mb_codepage=1252;
    if(!strcmp(mode,"table"))recon_mbctype[1]^=1;
    __asm__ __volatile__("fld1\n\tfldpi":::"st");errno=111;recon_doserrno=222;recon_SetLastError(333);fp(before);
    assert(typed(!strcmp(mode,"null-name")?NULL:!strcmp(mode,"other-name")?"OTHER":"LINK_REPRO_NAME",
          !strcmp(mode,"null-value")?NULL:!strcmp(mode,"other-value")?"other":"\\deflib.rsp"));fp(after);
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333 && !memcmp(before,after,108));
    assert(recon_crt_os_environment!=block && recon_crt_os_environment_units==expected_bytes);
    for(n=0;n<expected_bytes;++n)assert(recon_crt_os_environment[n]==(unsigned char)expected[n]);
    assert(!memcmp(block,saved,bytes*sizeof(*block)) && recon_crt_environment==env && recon_crt_initenv==env);
    assert(recon_getenv("CRT_ONLY")==env[0]+9 && !recon_getenv("LINK_REPRO_NAME"));
    assert(!strcmp(getenv("LINK_REPRO_NAME"),"host-control") && recon_crt_environment_updates==0);
    assert(recon_crt_environment_vectors[0].vector==env && recon_crt_environment_vectors[0].count==1 && recon_crt_environment_vectors[0].borrowed);
    if(!strcmp(mode,"repeat"))typed("LINK_REPRO_NAME","\\deflib.rsp");
    if(!strcmp(mode,"corrupt"))((WCHAR *)recon_crt_os_environment)[0]='X';
    recon_crt_validate_spawn_cp0_profile();
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");return 0;
}
