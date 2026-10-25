/* Native host configuration only; outside CL's reconstructed function graph. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <recon.h>

static void configure_module(void)
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
extern int main(int, char **, char **);
extern void cl_configure_dependency(void);
extern char **cl_dependency_bind_entry(int, char **, char **);
extern char **link_bind_entry(int, char **, char **);
extern char **mre_bind_entry(int, char **, char **);
int cl_case_main(int argc, char **argv, char **envp)
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
    else if (profile && !strcmp(profile,"usage-canonical-msvcrt-v1") && argc == 1)
        entry_error = 0;
    else if (profile && !strcmp(profile,"unknown-option-canonical-msvcrt-v1") &&
        argc == 3 && !strcmp(argv[1],"/nologo") &&
        !strcmp(argv[2],"/unknown-reconstruction-option"))
        entry_error = 0;
    else if (profile && !strcmp(profile,"explicit-inputs-canonical-msvcrt-v1") &&
        argc == 11 && !strcmp(argv[1],"/nologo") &&
        !strcmp(argv[2],"/Tcpretend.c") && !strcmp(argv[3],"/Tppretend.cpp") &&
        !strcmp(argv[4],"/Topretend.obj") && !strcmp(argv[5],"/ba1") &&
        !strcmp(argv[6],"/bc12") && !strcmp(argv[7],"/bx1.pre") &&
        !strcmp(argv[8],"/?") && !strcmp(argv[9],"/link") && !strcmp(argv[10],"/DEBUG"))
        entry_error = 2;
    else if (profile && !strcmp(profile,"preprocessor-pass-options-canonical-msvcrt-v1") &&
        argc == 5 && !strcmp(argv[1],"/nologo") && !strcmp(argv[2],"/E") &&
        !strcmp(argv[3],"/Bp1.pre") && !strcmp(argv[4],"/?"))
        entry_error = 2;
    else if (profile && !strcmp(profile,"macro-language-conflict-canonical-msvcrt-v1") &&
        argc == 7 && !strcmp(argv[1],"/nologo") && !strcmp(argv[2],"/D_MT") &&
        !strcmp(argv[3],"/MD") && !strcmp(argv[4],"/TC") &&
        !strcmp(argv[5],"/TP") && !strcmp(argv[6],"/?"))
        entry_error = 2;
    else if (profile && !strcmp(profile,"optimization-defaults-canonical-msvcrt-v1") &&
        argc == 5 && !strcmp(argv[1],"/nologo") && !strcmp(argv[2],"/Og") &&
        !strcmp(argv[3],"/O1") && !strcmp(argv[4],"/?"))
        entry_error = 2;
    else if (profile && !strcmp(profile,"link-object-canonical-msvcrt-v1") &&
        argc == 8 && !strcmp(argv[1],"/nologo") && !strcmp(argv[2],"/Fetest.exe") &&
        !strcmp(argv[3],"test.obj") && !strcmp(argv[4],"/link") &&
        !strcmp(argv[5],"/ENTRY:main") && !strcmp(argv[6],"/SUBSYSTEM:CONSOLE") &&
        !strcmp(argv[7],"/NODEFAULTLIB"))
        entry_error = 0;
    else if (profile && !strcmp(profile,"link-naming-canonical-msvcrt-v1") &&
        argc == 11 && !strcmp(argv[1],"/nologo") && !strcmp(argv[2],"/Bz") &&
        !strcmp(argv[3],"/LD") && !strcmp(argv[4],"/Fmcustom.map") &&
        !strcmp(argv[5],"/Feout/path.dll") && !strcmp(argv[6],"test.obj") &&
        !strcmp(argv[7],"/link") && !strcmp(argv[8],"/ENTRY:main") &&
        !strcmp(argv[9],"/SUBSYSTEM:CONSOLE") && !strcmp(argv[10],"/NODEFAULTLIB"))
        entry_error = 0;
    else if (profile && !strcmp(profile,"grouped-dry-run-canonical-msvcrt-v1") &&
        argc == 7 && !strcmp(argv[1],"/nologo") && !strcmp(argv[2],"/Bz") &&
        !strcmp(argv[3],"/ZM") && !strcmp(argv[4],"/c") &&
        !strcmp(argv[5],"test.c") && !strcmp(argv[6],"test.c"))
        entry_error = 0;
    else if (profile && !strcmp(profile,"link-timing-canonical-msvcrt-v1") &&
        argc == 9 && !strcmp(argv[1],"/nologo") && !strcmp(argv[2],"/Bt") &&
        !strcmp(argv[3],"/Fetest.exe") && !strcmp(argv[4],"test.obj") &&
        !strcmp(argv[5],"/link") && !strcmp(argv[6],"/ENTRY:main") &&
        !strcmp(argv[7],"/SUBSYSTEM:CONSOLE") && !strcmp(argv[8],"/NODEFAULTLIB"))
        entry_error = 0;
    else if (profile && !strcmp(profile,"mre-object-canonical-msvcrt-v1") &&
        argc==8 && !strcmp(argv[1],"/nologo") && !strcmp(argv[2],"/Gm") &&
        !strcmp(argv[3],"/Zi") && !strcmp(argv[4],"/c") && !strcmp(argv[5],"/DVALUE=7") &&
        !strcmp(argv[6],"/Fdcase.pdb") && !strcmp(argv[7],"test.obj"))
        entry_error=0;
    else
        recon_unsupported("unknown CL native entry profile");
    configure_module();
    cl_configure_dependency();
    envp = cl_dependency_bind_entry(argc, argv, envp);
    envp = link_bind_entry(argc, argv, envp);
    envp = mre_bind_entry(argc, argv, envp);
    errno = 0;
    recon_doserrno = 0;
    recon_SetLastError(entry_error);
    recon_controlfp(0x10000, 0x30000);
    result = main(argc, argv, envp);
    recon_record_terminal_status((uint32_t)result);
    return result;
}
