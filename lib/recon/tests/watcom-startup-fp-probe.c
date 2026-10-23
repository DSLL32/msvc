#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static unsigned char before[108],after[108];
static int saved_errno;
static unsigned long saved_dos;
static DWORD saved_error;
static void snapshot(unsigned char *bytes)
{ __asm__ volatile("fnsave (%0); frstor (%0)"::"r"(bytes):"memory"); }
/* Immediately surround the shared constructor; no target instructions run. */
static void __attribute__((constructor(102))) observe_before(void)
{
    snapshot(before);
    saved_errno=errno; saved_dos=recon_doserrno; saved_error=recon_GetLastError();
}
static void __attribute__((constructor(104))) observe_after(void)
{
    snapshot(after);
    assert(errno==saved_errno && recon_doserrno==saved_dos && recon_GetLastError()==saved_error);
}
int main(void)
{
    unsigned short first,second; unsigned i;
    memcpy(&first,before,2); memcpy(&second,after,2);
    assert(first==0x037f);
#ifdef RECON_NATIVE_STARTUP_WATCOM_100A_FP
    assert(second==0x127f);
#else
    assert(second==0x037f);
#endif
    /* Exclude only the two proved control bits; every other environment byte
     * and all eight native register payloads must remain unchanged. */
    assert((first&~0x1300)==(second&~0x1300));
    for(i=2;i<108;++i)assert(before[i]==after[i]);
    assert(*(unsigned short *)(after+4)==0 && *(unsigned short *)(after+8)==0xffff);
    return 0;
}
