#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
extern char **environ;
static unsigned char before[108],after[108];
static int after_errno;static unsigned long after_dos;static DWORD after_error;
static const char *initial;
static void __attribute__((constructor(101))) before_control(void)
{
    initial=getenv("GLIBC_TUNABLES");
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    __asm__ __volatile__("fld1\n\tfnsave %0\n\tfrstor %0":"=m"(before));
}
static void __attribute__((constructor(104))) after_control(void)
{
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(after));
    after_errno=errno;after_dos=recon_doserrno;after_error=recon_GetLastError();
    __asm__ __volatile__("fstp %%st(0)":::"st");
}
static int has_control(char **env)
{
    for(;env && *env;++env)if(!strncmp(*env,"GLIBC_TUNABLES=",15))return 1;
    return 0;
}
int main(int argc,char **argv,char **envp)
{
    (void)argc;(void)argv;
    assert(!memcmp(before,after,108));
    assert(after_errno==111 && after_dos==222 && after_error==333);
#ifdef RECON_NATIVE_STARTUP_CONSUME_GLIBC_TUNABLES
    assert(!getenv("GLIBC_TUNABLES") && !recon_getenv("GLIBC_TUNABLES"));
    assert(!has_control(environ) && !has_control(envp));
#else
    assert((getenv("GLIBC_TUNABLES")!=NULL)==(initial!=NULL));
    assert(has_control(environ)==(initial!=NULL) && has_control(envp)==(initial!=NULL));
#endif
    assert(getenv("RECON_TEST_OTHER") && !strcmp(getenv("RECON_TEST_OTHER"),"retained"));
    puts("host-control namespace and full FP/errors pass");return 0;
}
