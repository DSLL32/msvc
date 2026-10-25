/* Typed host adapter; file loading uses host libc, not target shim FILE state. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "c1.h"
#include <recon-tool.h>
#include <recon.h>
static int invoke(int argc, char **argv)
{
 ReconModuleSpec spec = { "C1.DLL", "R:\\toolchains\\5.10\\bin\\C1.DLL",
 "7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d", RECON_MODULE_NO_TLS };
 HINSTANCE module = recon_register_module(&spec);
 C1Initializer *allocated;
 int (WINAPI *pass)(int, char **, int) = InvokeCompilerPass;
 void (WINAPI *abort_pass)(int) = AbortCompilerPass;
 recon_controlfp(0x10000, 0x30000);
 recon_SetLastError(14007);
 if (!module || !_DllMainCRTStartup(module,DLL_PROCESS_ATTACH,NULL)) return 3;
 allocated = c1_onexit_begin;
 if (c1_crt_attached != 1 || !allocated || c1_onexit_end != allocated || *allocated ||
     c1_raw_dllmain || c1_crt_adjust_fdiv || c1_dll_instance != module ||
     !recon_module_thread_calls_disabled(module) || recon_GetLastError() != 14007) return 4;
 printf("attach=1\r\n");
 if (!pass || !abort_pass || (void *)pass == (void *)abort_pass) return 5;
 printf("exports=1\r\n");
 if (!_DllMainCRTStartup(module,DLL_PROCESS_DETACH,NULL) || c1_crt_attached ||
     c1_onexit_begin || c1_onexit_end != allocated || c1_dll_instance != module ||
     recon_GetLastError() != 14007) return 6;
 printf("detach=1\r\n");
 return 0;
}
static const uint32_t order[] = { 0x1218e };
RECON_EXPORT const ReconCase case_descriptor = {
 RECON_ABI_VERSION, sizeof(ReconCase), "dll-lifecycle-and-exports",
 "Original attach, resolve exact two exports without invocation, detach.",
 order, RECON_COUNT(order), NULL, 0, RECON_IO, RECON_EXIT, 0,
 NULL, invoke, NULL, NULL, 0
};
