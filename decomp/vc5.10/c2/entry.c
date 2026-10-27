/* Ordinary Linux host only; original main/CRT identities and fixture objects
 * remain unchanged. The private native link renames App main and its callers. */
#define RECON_SHIM_IMPL
#include <string.h>
#include <windows.h>
#include <recon.h>

extern void mainCRTStartup(void);
extern const unsigned char recon_asset_exe_messages_start[];
extern const unsigned char recon_asset_exe_messages_end[];

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *process;
    ReconModuleSpec module;
    HMODULE instance;
    const char *name, *alternate;

    process = recon_native_entry_initialize(argc, argv, environment);
    if (!process) return 125;
    name = strrchr(process->program_path, '\\');
    alternate = strrchr(process->program_path, '/');
    if (!name || (alternate && alternate > name)) name = alternate;
    module.name = name ? name + 1 : process->program_path;
    module.path = process->program_path;
    module.sha256 = "e75aecaf4073b0817ffb638cae5fff636b2e2d1a090daabf8f68dbc954515fae";
    module.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    instance = recon_register_module(&module);
    if (!instance) return 125;
    recon_load_module_resources_span(instance, recon_asset_exe_messages_start,
        recon_asset_exe_messages_end - recon_asset_exe_messages_start);
    /* The shared helper owns the one argument/environment binding. Existing
     * C2 startup retrieves it via __getmainargs, sets binary fmode and x87 PC53,
     * runs its initializer lists, and exits through normal target cleanup. */
    mainCRTStartup();
    return 125;
}
