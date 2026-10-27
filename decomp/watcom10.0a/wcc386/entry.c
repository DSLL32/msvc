/* Ordinary native process host; excluded from Win32 and fixture libraries. */
#define RECON_SHIM_IMPL
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include "startup.h"
#include <recon.h>

extern int recon_entry_app_main(void);

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *process;
    ReconModuleSpec module;
    char **crt_environment;
    static char *empty_argv[1];
    size_t count,i;
    const char *basename;

    /* Keep the actual process OS environment and raw command line. This
     * deferred-binding form leaves WCC's cstart adapter the sole CRT owner. */
    process=recon_native_entry_prepare(argc,argv,environment);
    if (!process) return 125;
    for (count=0;process->environment[count];++count) {}
    crt_environment=malloc((count+1)*sizeof(*crt_environment));
    if (!crt_environment) recon_unsupported("WCC startup environment pointers");
    /* cstart scans forward and PUSHes each pointer. Its saved ESP therefore
     * presents the CRT environment in reverse order; OS bytes stay natural. */
    for (i=0;i<count;++i) crt_environment[i]=process->environment[count-1-i];
    crt_environment[count]=NULL;
    /* Original argument-free main does not consume ___Argv (NULL there).
     * The shim requires this live empty vector; actual CLI args remain owned
     * by process, and getcmd consumes its exact Windows command line. */
    recon_crt_bind_arguments(0,empty_argv,crt_environment);

    basename=strrchr(process->program_path,'\\');
    if (!basename) recon_unsupported("WCC native executable path");
    module.name=basename+1;
    module.path=process->program_path;
    /* Logical original image identity; this is not the native ELF's hash. */
    module.sha256="c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b";
    module.flags=RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS;
    if (!recon_register_module(&module))
        recon_unsupported("WCC native primary module registration");
    /* This WCC has compiled-in messages and an empty InitMsg: no original
     * executable/resource container or fixture assets are registered. */
    recon_fmode=0x4000; /* Existing text default; explicit rb opens stay binary. */
    (void)recon_control87(_PC_53|_IC_AFFINE,_MCW_PC|_MCW_IC);
    return wcc_startup_run(process->program_path,crt_environment,
                           recon_entry_app_main);
}
