/* Native host configuration only; outside CL's reconstructed function graph. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <recon.h>

static void __attribute__((constructor)) configure_module(void)
{
    const char *path = getenv("RECON_PROGRAM_PATH");
    const char *provider = getenv("RECON_RESOURCE_PROVIDER");
    ReconModuleSpec spec;
    HMODULE module;
    if (!path) path = "R:\\toolchains\\5.10\\bin\\CL.EXE";
    if (!provider) provider = "build/resources.bin";
    spec.name = "CL.EXE";
    spec.path = path;
    spec.sha256 = "aa15ed605515693292c539827ab1d8e08ce2ed5b4e1d9ac2c7f033903e6844dd";
    spec.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    module = recon_register_module(&spec);
    if (!module) _exit(125);
    recon_load_module_resources(module, provider);
}

/* GNU ld adapts the Linux CRT caller, leaving the target's main symbol intact.
 * This is the same verified entry setup supplied by shared entry.c, whose
 * optional host main is disabled for this target. */
extern int __real_main(int, char **, char **);
extern char **cl_dependency_bind_entry(int, char **, char **);
int __wrap_main(int argc, char **argv, char **envp)
{
    int result;
    DWORD entry_error;
    const char *profile = getenv("CL_ENTRY_PROFILE");
    /* Captured canonical CRT/loader boundary, outside the application graph.
     * New cases must provide their own evidenced entry profile. */
    if (profile && !strcmp(profile,"help-canonical-msvcrt-v1") &&
        argc == 2 && !strcmp(argv[1],"/?"))
        entry_error = 2;
    else if (profile && !strcmp(profile,"minimal-c-canonical-msvcrt-v1") &&
        argc == 4 && !strcmp(argv[1],"/nologo") &&
        !strcmp(argv[2],"/c") && !strcmp(argv[3],"test.c"))
        entry_error = 0;
    else if (profile && !strcmp(profile,"response-options-canonical-msvcrt-v1") &&
        argc == 3 && !strcmp(argv[1],"/nologo") &&
        !strcmp(argv[2],"@options.rsp"))
        entry_error = 0;
    else
        recon_unsupported("unknown CL native entry profile");
    envp = cl_dependency_bind_entry(argc, argv, envp);
    errno = 0;
    recon_doserrno = 0;
    recon_SetLastError(entry_error);
    recon_controlfp(0x10000, 0x30000);
    result = __real_main(argc, argv, envp);
    recon_record_terminal_status((uint32_t)result);
    return result;
}
