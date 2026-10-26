#include <assert.h>
#include <setjmp.h>
#include <recon.h>
extern int *__errno_location(void);
static unsigned char current_fp[108];
static void snapshot(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static void transfer(unsigned *owner)
{
    __asm__ __volatile__("fldpi":::"st");
    *__errno_location()=113;errno=211;recon_doserrno=317;SetLastError(419);
    snapshot(current_fp);longjmp(owner,1);
}
static int run(const char *mode)
{
    struct { unsigned prefix;jmp_buf owner;unsigned suffix; } state;
    unsigned char saved[sizeof(state)],after[108];int result;
    memset(&state,0x5a,sizeof(state));memcpy(saved,&state,sizeof(state));
    assert(sizeof(state.owner)==52);
    if(!strcmp(mode,"null"))longjmp(NULL,1);
    if(!strcmp(mode,"unknown"))longjmp(state.owner,1);
    if(!strcmp(mode,"foreign")){recon_jmp_context(state.owner);longjmp(state.owner,1);}
    if(!strcmp(mode,"opaque")){recon_setjmp3_context(state.owner,64,0);longjmp(state.owner,1);}
    __asm__ __volatile__("fld1":::"st");
    result=setjmp(state.owner);
    if(!result) {
        if(!strcmp(mode,"zero"))longjmp(state.owner,0);
        if(!strcmp(mode,"other"))longjmp(state.owner,2);
        if(!strcmp(mode,"generic"))recon_longjmp(state.owner,1);
        transfer(state.owner);
    }
    snapshot(after);assert(result==1 && !memcmp(current_fp,after,108));
    assert(!memcmp(saved,&state,sizeof(state)));
    assert(*__errno_location()==113 && errno==211 && recon_doserrno==317 && GetLastError()==419);
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
#endif
int main(int argc,char **argv)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    return run(argc>1?argv[1]:"normal");
#else
    (void)argc;(void)argv;recon_watcom_longjmp(NULL,1);return 42;
#endif
}
