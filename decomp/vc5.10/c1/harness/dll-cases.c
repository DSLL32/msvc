/* Native-only DLL adapters. Host file IO must not use target shim FILE state. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "startup.h"
#include "dll-cases.h"
#include <recon.h>
int c1_lifecycle_invoke(int argc, char **argv)
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

int c1_minimal_setup(void)
{
 const char *reference = "cc2f15a9e873f45b33fba0c0d3861cdfb1238d87de021fe230e4cfea7909af63";
 recon_load_system_info_profile("system-info-profile.bin", reference);
 recon_crt_bind_empty_ctype_1252("e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210", reference);
 recon_controlfp(0x10000, 0x30000);
 return 1;
}
int c1_minimal_invoke(int argc, char **argv)
{
 ReconModuleSpec spec = { "C1.DLL", "R:\\toolchains\\5.10\\bin\\C1.DLL",
 "7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d", RECON_MODULE_NO_TLS };
 HINSTANCE module = recon_register_module(&spec);
 /* This case consumes only getenv("MSC_CMD_FLAGS"), observed absent in the
  * new typed-host invocation. Other environment queries need their own input. */
 static char *environment[] = { NULL };
 int result;
 if (!module || !_DllMainCRTStartup(module, DLL_PROCESS_ATTACH, NULL)) return 126;
 recon_crt_bind_arguments(argc, argv, environment);
 result = InvokeCompilerPass(argc, argv, TRUE);
 if (!_DllMainCRTStartup(module, DLL_PROCESS_DETACH, NULL)) return 126;
 return result;
}
