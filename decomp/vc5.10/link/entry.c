/* Ordinary native process host, outside the reconstructed App/Win32 graph.
 * Shared startup owns writable arguments, command line and both environment
 * namespaces. The observed direct-App CRT boundary owns no second binding. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include "src/initializers.h"
#include <recon.h>

extern int recon_entry_app_main(int, char **, char **);
extern const unsigned char recon_asset_exe_messages_start[];
extern const unsigned char recon_asset_exe_messages_end[];

static void register_resources(const char *program)
{
    ReconModuleSpec module;
    HMODULE handle;
    const char *name = strrchr(program, '\\');

    if (!name || !name[1])
        recon_unsupported("LINK native executable identity");
    module.name = name + 1;
    module.path = program;
    module.sha256 = "04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579";
    module.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    handle = recon_register_module(&module);
    if (!handle)
        recon_unsupported("LINK native primary module registration");
    recon_load_module_resources_span(handle, recon_asset_exe_messages_start,
        recon_asset_exe_messages_end - recon_asset_exe_messages_start);
}

int main(int argc, char **argv, char **environment)
{
    static ReconInitializer c_initializers[] = { NULL };
    static ReconInitializer application_initializers[] = {
        NULL, initialize_buffer12, initialize_buffer8,
        initialize_buffer4, initialize_timing
    };
    ReconNativeEntry *process;
    int status, saved_errno;
    unsigned long saved_dos;
    DWORD saved_error;

    process = recon_native_entry_initialize(argc, argv, environment);
    if (!process)
        return 125;
    saved_errno = errno;
    saved_dos = recon_doserrno;
    saved_error = recon_GetLastError();
    register_resources(process->program_path);
    recon_fmode = 0;
    recon_commode = 0;
    (void)recon_controlfp(_PC_53, _MCW_PC);
    errno = saved_errno;
    recon_doserrno = saved_dos;
    recon_SetLastError(saved_error);
    recon_initterm(c_initializers, c_initializers + 1);
    recon_initterm(application_initializers, application_initializers + 5);
    status = recon_entry_app_main(process->argc, process->argv,
                                 process->environment);
    recon_record_terminal_status((uint32_t)status);
    return status;
}
