/* The dynamic loader has already consumed allocator tunables before native
 * constructors. Hide this host-only input from direct-main target namespaces. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__) && defined(RECON_NATIVE_STARTUP_CONSUME_GLIBC_TUNABLES)
#define RECON_SHIM_IMPL
#include <recon.h>
static void __attribute__((constructor(102))) consume_host_allocator_control(void)
{
    unsigned char fp[108];int saved_errno=errno;
    unsigned long saved_dos=recon_doserrno;DWORD saved_error=recon_GetLastError();
    __asm__ __volatile__("fnsave %0":"=m"(fp));
    if(unsetenv("GLIBC_TUNABLES"))recon_unsupported("consume native host allocator control");
    __asm__ __volatile__("frstor %0"::"m"(fp));
    errno=saved_errno;recon_doserrno=saved_dos;recon_SetLastError(saved_error);
}
#endif

/* Proven original CRT precision initialization for native direct-main hosts. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__) && (defined(RECON_NATIVE_STARTUP_CONTROLFP_53) || defined(RECON_NATIVE_STARTUP_WATCOM_100A_FP))
#define RECON_SHIM_IMPL
#include <recon.h>
static void __attribute__((constructor(103))) initialize_direct_main_precision(void)
{
#if defined(RECON_NATIVE_STARTUP_WATCOM_100A_FP)
    /* Watcom's proved startup loads127f:53-bit precision and affine infinity.
     * Change only these control bits; native status/tags/payload stay native. */
    (void)recon_control87(_PC_53|_IC_AFFINE,_MCW_PC|_MCW_IC);
#else
    (void)recon_controlfp(0x10000,0x30000);
#endif
}
#endif

/* Optional native host entry for programs whose PE CRT startup is separate. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__) && defined(RECON_NATIVE_ENTRY)
#define RECON_SHIM_IMPL
#include <unistd.h>
#include <recon.h>
extern char **environ;
#if defined(RECON_NATIVE_ENTRY_CONTROLFP_53)
#define RECON_ENTRY_INITIALIZE() ((void)recon_controlfp(0x10000,0x30000))
#else
#define RECON_ENTRY_INITIALIZE() ((void)0)
#endif
#if defined(RECON_NATIVE_ENTRY_TWO_ARGS)
extern int RECON_NATIVE_ENTRY(int,char **);
int main(int argc,char **argv) { int result; RECON_ENTRY_INITIALIZE(); result=RECON_NATIVE_ENTRY(argc,argv); recon_record_terminal_status((uint32_t)result); return result; }
#else
extern int RECON_NATIVE_ENTRY(int,char **,char **);
int main(int argc,char **argv) { int result; RECON_ENTRY_INITIALIZE(); result=RECON_NATIVE_ENTRY(argc,argv,environ); recon_record_terminal_status((uint32_t)result); return result; }
#endif
#endif
