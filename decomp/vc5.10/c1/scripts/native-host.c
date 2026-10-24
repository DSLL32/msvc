/* Linux transport only; not part of the reconstructed target function graph. */
#include <stdio.h>
#include <float.h>
#include <string.h>
#include "c1.h"
#include "host-vectors.h"
#include <recon.h>

HMODULE c1_host_module;
const ReconModuleSpec *c1_host_identity;

/* Debug observation point outside the target graph. */
void c1_host_observe(DWORD reason, BOOL status)
{
    (void)reason;
    (void)status;
}

int c1_native_host(int argc, char **argv)
{
    ReconModuleSpec spec;
    C1Initializer *allocated;
    BOOL status;
    int pass_result;
    const char *system_profile, *system_reference, *x87_reference;
    char *pass_args[] = { "c1.dll", "-il", ".\\minimal", "-f", "test.c", "-W", "1", "-G5", "-Ot", "-Ob0", "-nologo", NULL };
    system_profile = recon_host_getenv("RECON_C1_SYSTEM_INFO_PROFILE");
    system_reference = recon_host_getenv("RECON_C1_SYSTEM_INFO_REFERENCE");
    if (system_profile) {
        if (!system_reference) return 126;
        recon_load_system_info_profile(system_profile, system_reference);
    } else if (system_reference) {
        return 126;
    }
    /* Captured original loader context: x87 precision53, control word027f. */
    _controlfp(_PC_53, _MCW_PC);
    spec.name = "C1.DLL";
    spec.path = "R:\\toolchains\\5.10\\bin\\C1.DLL";
    spec.sha256 = "7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d";
    spec.flags = RECON_MODULE_NO_TLS;
    c1_host_module = recon_register_module(&spec);
    if (!c1_host_module) return 2;
    c1_host_identity = recon_module_identity(c1_host_module);
    if (!c1_host_identity) return 2;
    x87_reference = recon_host_getenv("RECON_C1_X87_ENTRY_REFERENCE");
    if (x87_reference)
        recon_apply_x87_empty_entry_profile(x87_reference, 0x027f, 0x0020, 0xffff);
    /* Explicit initial state captured at the original guarded DLL entry. */
    SetLastError(14007);
    status = _DllMainCRTStartup(c1_host_module, DLL_PROCESS_ATTACH, NULL);
    c1_host_observe(DLL_PROCESS_ATTACH, status);
    if (!status || c1_crt_attached != 1 || !c1_onexit_begin ||
            c1_onexit_end != c1_onexit_begin || *c1_onexit_begin ||
            c1_raw_dllmain || c1_crt_adjust_fdiv ||
            c1_dll_instance != c1_host_module ||
            !recon_module_thread_calls_disabled(c1_host_module) ||
            GetLastError() != 14007) return 3;
    allocated = c1_onexit_begin;
    puts("attach=1");
    /* The separate artifact audit verifies names, ordinals and both ABIs. */
    {
        int (WINAPI *invoke)(int, char **, int) = InvokeCompilerPass;
        void (WINAPI *abort_pass)(int) = AbortCompilerPass;
        if (!invoke || !abort_pass) return 4;
    }
    puts("exports=1");
    if (argc == 2 && !strcmp(argv[1], "minimal")) {
        const char *crt_argv = recon_host_getenv("RECON_C1_CRT_ARGV");
        const char *crt_env = recon_host_getenv("RECON_C1_CRT_ENVIRONMENT");
        const char *locale_reference = recon_host_getenv("RECON_C1_ISOLATED_LOCALE_REFERENCE");
        if (crt_argv || crt_env) {
            char **host_argv, **host_env;
            int host_argc;
            if (!crt_argv || !crt_env) return 126;
            host_argc = read_host_vector(crt_argv, &host_argv);
            read_host_vector(crt_env, &host_env);
            recon_crt_bind_arguments(host_argc, host_argv, host_env);
        }
        if (locale_reference)
            recon_crt_bind_empty_ctype_1252("e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210", locale_reference);
        /* Captured direct-host case: the final slot is BOOL, not callbacks. */
        SetLastError(0);
        pass_result = InvokeCompilerPass(11, pass_args, TRUE);
        printf("invoke=%d\n", pass_result);
        if (pass_result) return pass_result;
        /* The completed lifecycle fixture uses its separate original state. */
        SetLastError(14007);
    }
    status = _DllMainCRTStartup(c1_host_module, DLL_PROCESS_DETACH, NULL);
    c1_host_observe(DLL_PROCESS_DETACH, status);
    if (!status || c1_crt_attached || c1_onexit_begin ||
            c1_onexit_end != allocated || c1_raw_dllmain ||
            c1_crt_adjust_fdiv || c1_dll_instance != c1_host_module ||
            !recon_module_thread_calls_disabled(c1_host_module) ||
            GetLastError() != 14007) return 5;
    puts("detach=1");
    return 0;
}
