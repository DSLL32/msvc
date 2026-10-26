/* Shared typed original probe/native one-shot entry hook; no controller or
   private state edits. Marker initialize_limits RVA400000 is after the real
   main has registered its handler/cleanup and parsed arguments. */
#include "../../src/c2.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
    ((BOOL (WINAPI *)(DWORD))RECON_ORIGINAL_ADDRESS(probe,0x3c560))(CTRL_C_EVENT);
    recon_original_finish(probe,0); /* original handler must never return */
}
#else
extern void initialize_limits(void);
extern BOOL WINAPI compiler_control(DWORD event);
extern int main(int,char **,char **);
static void at_control(void)
{
    compiler_control(CTRL_C_EVENT);
    recon_native_entry_finish(0);
}
static void real_input(void *unused)
{
    char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\C23232.EXE",0};
    (void)unused;main(1,argv,0);
}
int console_control_case(void)
{
    recon_native_at_entry((const void*)initialize_limits,(const void*)at_control,real_input,0);
    return 0;
}
#endif
