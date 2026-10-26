/* Canonical native host only. The App keeps its original three-argument main;
   the shared partial-link rule gives this private copy a distinct ELF name. */
#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <recon.h>

extern int recon_entry_app_main(int, char **, char **);
extern const unsigned char recon_asset_exe_c23_start[];
extern const unsigned char recon_asset_exe_c23_end[];

/* These are two explicit shipped-catalog locations, not a basename redirect.
   A different -ef path still opens its actual ordinary filesystem contents. */
static void register_catalogs(const ReconNativeEntry *process)
{
    char directory[PATH_MAX], installed[PATH_MAX], relative[PATH_MAX];
    char *separator, *alternate;
    size_t bytes = (uintptr_t)recon_asset_exe_c23_end -
                   (uintptr_t)recon_asset_exe_c23_start;
    if (strlen(process->program_path) >= sizeof(directory))
        recon_unsupported("C2 native module path length");
    strcpy(directory, process->program_path);
    separator = strrchr(directory, '\\');
    alternate = strrchr(directory, '/');
    if (!separator || (alternate && alternate > separator)) separator = alternate;
    if (!separator) recon_unsupported("C2 native module directory");
    separator[1] = 0;
    if (strlen(directory) + sizeof("C23.ERR") > sizeof(directory))
        recon_unsupported("C2 native catalog path length");
    strcat(directory, "C23.ERR");
    if (!recon_fullpath(installed, directory, sizeof(installed)) ||
        !recon_fullpath(relative, "c23.err", sizeof(relative)))
        recon_unsupported("C2 native catalog path");
    if (!recon_register_embedded_file(relative, recon_asset_exe_c23_start, bytes))
        recon_unsupported("C2 default diagnostic catalog registration");
    if (strcasecmp(relative, installed) &&
        !recon_register_embedded_file(installed, recon_asset_exe_c23_start, bytes))
        recon_unsupported("C2 installed diagnostic catalog registration");
}

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *process = recon_native_entry_initialize(argc, argv, environment);
    ReconModuleSpec module = {
        NULL, process->program_path,
        "0a64eab35cfb4bfb8af452653d21707947ada33952a82a5e712c275ab7712d26",
        RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS
    };
    const char *name = strrchr(process->program_path, '\\');
    const char *alternate = strrchr(process->program_path, '/');
    int status;
    if (!name || (alternate && alternate > name)) name = alternate;
    module.name = name ? name + 1 : process->program_path;
    if (!recon_register_module(&module))
        recon_unsupported("C2 primary module registration");
    /* The proved direct-App startup boundary supplies BINMODE and x87 PC53.
       App main itself changes exception masks and registers its cleanup. */
    *recon_p_fmode() = 0x8000;
    (void)recon_controlfp(0x10000, 0x30000);
    register_catalogs(process);
    status = recon_entry_app_main(process->argc, process->argv, process->environment);
    recon_record_terminal_status((uint32_t)status);
    return status;
}
