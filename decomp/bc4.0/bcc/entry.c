/* Native host, excluded from the application graph and period build. */
#define RECON_SHIM_IMPL
#include <string.h>
#include <recon.h>

extern void initialize_exception_handler(void);
extern int recon_entry_app_main(int, char **, char **);

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *entry;
    ReconModuleSpec module;
    const char *name, *slash;

    entry = recon_native_entry_initialize(argc, argv, environment);
    if (!entry) return 125;
    name = strrchr(entry->program_path, '\\');
    slash = strrchr(entry->program_path, '/');
    if (slash && (!name || slash > name)) name = slash;
    if (!name) return 125;
    module.name = name + 1;
    module.path = entry->program_path;
    module.sha256 = "15342e1a2579244d839e0ed7a93f747ac3ea8721b779d105497fcae020dcadc5";
    module.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    if (!recon_register_module(&module)) return 125;

    initialize_exception_handler();
    recon_borland_400_initialize_gp_handler();
    return recon_entry_app_main(entry->argc, entry->argv, entry->environment);
}
