#include <assert.h>
#include <setjmp.h>
#include <recon.h>
static unsigned char current_fp[108];
static void snapshot(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
static void __attribute__((noinline)) transfer(void *owner,int value)
{
    __asm__ __volatile__("fldpi":::"st");
    _control87(_RC_DOWN,_MCW_RC);
    *recon_errno()=113;recon_doserrno=317;SetLastError(419);
    snapshot(current_fp);recon_longjmp(owner,value);
}
int main(int argc,char **argv)
{
    struct { unsigned prefix;int owner[16];unsigned suffix; } state;
    unsigned char saved[sizeof(state)],after[108];int result;
    const char *mode=argc>1?argv[1]:"normal";unsigned control;
    memset(&state,0x5a,sizeof(state));memcpy(saved,&state,sizeof(state));
    if(!strcmp(mode,"null"))recon_longjmp(NULL,2);
    if(!strcmp(mode,"unknown"))recon_longjmp(state.owner,2);
    if(!strcmp(mode,"count"))recon_setjmp3_context(state.owner,64,1);
    if(!strcmp(mode,"size"))recon_setjmp3_context(state.owner,32,0);
    control=_control87(0,0);__asm__ __volatile__("fld1":::"st");
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    assert(sizeof(jmp_buf)==64);
    result=setjmp(state.owner);
#else
    result=recon_host_setjmp(*recon_setjmp3_context(state.owner,sizeof(state.owner),0));
#endif
    if(!result) {
        if(!strcmp(mode,"zero"))recon_longjmp(state.owner,0);
        if(!strcmp(mode,"one"))transfer(state.owner,1);
        if(!strcmp(mode,"seven"))recon_longjmp(state.owner,7);
        if(!strcmp(mode,"negative"))recon_longjmp(state.owner,-1);
        transfer(state.owner,2);
    }
    snapshot(after);assert(result==(!strcmp(mode,"one")?1:2) && !memcmp(saved,&state,sizeof(state)));
    /* Compare observable control/status/tag and the two valid stack values.
     * Incidental instruction/data pointers and empty registers are not an
     * original-equivalence requirement for this host-context implementation. */
    assert(!memcmp(current_fp,after,2) && !memcmp(current_fp+4,after+4,2) &&
           !memcmp(current_fp+8,after+8,2) && !memcmp(current_fp+28,after+28,20));
    assert(*recon_errno()==113 && recon_doserrno==317 && GetLastError()==419);
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    _control87(control,_MCW_RC);return 0;
}
