#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static const char provider[]="e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210";
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";char *args[]={"target.exe",NULL},*env[]={"CRT_ONLY=kept",NULL};
    static const char text[]="OS_ONLY=one\0Other=two\0";WCHAR block[sizeof(text)],saved[sizeof(text)];
    char temporary_provider[65];unsigned char before[108],after[108];size_t n,units=sizeof(text);
    for(n=0;n<units;++n)block[n]=(unsigned char)text[n];memcpy(saved,block,sizeof(block));
    strcpy(temporary_provider,provider);recon_crt_initenv=env;
    if(!strcmp(mode,"after-args"))recon_crt_bind_arguments(1,args,env);
    if(!strcmp(mode,"bad-provider"))temporary_provider[0]='0';
    if(!strcmp(mode,"early"))block[1]=block[2]=0;
    if(!strcmp(mode,"high"))block[0]=128;
    if(!strcmp(mode,"short"))units--;
    if(!strcmp(mode,"empty"))units=2;
    __asm__ __volatile__("fld1\n\tfldpi":::"st");errno=111;recon_doserrno=222;recon_SetLastError(333);fp(before);
    recon_bind_process_os_environment(!strcmp(mode,"null")?NULL:!strcmp(mode,"unaligned")?(WCHAR *)((char *)block+1):!strcmp(mode,"wrap")?(WCHAR *)0xfffffffeU:block,
         !strcmp(mode,"large")?32768:units,temporary_provider);
    assert(!recon_crt_arguments_bound && !recon_crt_os_environment && !recon_crt_environment);
    if(!strcmp(mode,"repeat"))recon_bind_process_os_environment(block,units,provider);
    memset(temporary_provider,'x',64); /* Deferred consume must not borrow this string. */
    recon_crt_bind_arguments(1,args,env);fp(after);
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333 && !memcmp(before,after,108));
    assert(recon_crt_os_environment==block && recon_crt_os_environment_units==units && !memcmp(block,saved,sizeof(block)));
    assert(recon_crt_environment==env && recon_crt_initenv==env && recon_getenv("CRT_ONLY")==env[0]+9);
    assert(!recon_getenv("OS_ONLY") && recon_crt_environment_updates==0);
    assert(recon_crt_environment_vectors[0].borrowed && recon_crt_environment_vectors[0].live && recon_crt_environment_vectors[0].vector==env);
    if(!strcmp(mode,"after-consume"))recon_bind_process_os_environment(block,units,provider);
    recon_crt_bind_arguments(1,args,env);assert(recon_crt_os_environment==block);
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");return 0;
}
