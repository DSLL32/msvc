#define RECON_SHIM_IMPL
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include <recon.h>
int main(void)
{
    recon_host_jmp_buf environment; int value;
    assert(SIGABRT==6 && SIGINT==2 && SIGTERM==15);
    assert((uintptr_t)SIG_DFL==0 && (uintptr_t)SIG_IGN==1 && (uintptr_t)SIG_ERR==(uintptr_t)-1);
    value=recon_host_setjmp(environment);
    if (!value) longjmp(environment,7);
    assert(value==7); return 0;
}
