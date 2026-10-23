/* Proven original CRT precision initialization for native direct-main hosts. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__) && defined(RECON_NATIVE_STARTUP_CONTROLFP_53)
#define RECON_SHIM_IMPL
#include <recon.h>
static void __attribute__((constructor(103))) initialize_direct_main_precision(void)
{
    (void)recon_controlfp(0x10000,0x30000);
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
