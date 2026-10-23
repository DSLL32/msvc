/* Linux transport only; not part of the reconstructed target function graph. */
#include <stdio.h>
#include "c1.h"
#include <recon.h>

HMODULE c1_host_module;
const ReconModuleSpec *c1_host_identity;

int main(void)
{
    ReconModuleSpec spec;
    spec.name = "C1.DLL";
    spec.path = "R:\\toolchains\\5.10\\bin\\C1.DLL";
    spec.sha256 = "7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d";
    spec.flags = RECON_MODULE_NO_TLS;
    c1_host_module = recon_register_module(&spec);
    if (!c1_host_module) return 2;
    c1_host_identity = recon_module_identity(c1_host_module);
    if (!c1_host_identity) return 2;
    /* Explicit initial state captured at the original guarded DLL entry. */
    SetLastError(14007);
    if (!c1_crt_entry(c1_host_module, DLL_PROCESS_ATTACH, NULL)) return 3;
    puts("attach=1");
    /* The separate artifact audit verifies names, ordinals and both ABIs. */
    {
        int (WINAPI *invoke)(int, char **, int) = InvokeCompilerPass;
        void (WINAPI *abort_pass)(int) = AbortCompilerPass;
        if (!invoke || !abort_pass) return 4;
    }
    puts("exports=1");
    if (!c1_crt_entry(c1_host_module, DLL_PROCESS_DETACH, NULL)) return 5;
    puts("detach=1");
    return 0;
}
