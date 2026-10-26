/* Canonical native host. The reconstructed three-argument main is unchanged. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <recon.h>

extern int recon_entry_app_main(int, char **, char **);
extern const unsigned char recon_asset_exe_errors_start[];
extern const unsigned char recon_asset_exe_errors_end[];

static int register_catalog(const char *program)
{
    const char *slash = strrchr(program, '\\');
    size_t prefix, bytes = (uintptr_t)recon_asset_exe_errors_end -
        (uintptr_t)recon_asset_exe_errors_start;
    char *installed;
    int ok;
    if (!recon_register_embedded_file("c1.err", recon_asset_exe_errors_start, bytes))
        return 0;
    if (!slash) slash = strrchr(program, '/');
    if (!slash) return 0;
    prefix = slash + 1 - program;
    installed = malloc(prefix + sizeof("C1.ERR"));
    if (!installed) return 0;
    memcpy(installed, program, prefix);
    memcpy(installed + prefix, "C1.ERR", sizeof("C1.ERR"));
    ok = recon_register_embedded_file(installed, recon_asset_exe_errors_start, bytes);
    /* When cwd is the installation directory, the first alias already owns
       this normalized path and the same immutable catalog. */
    if (!ok && recon_GetLastError() == ERROR_ALREADY_EXISTS) ok = 1;
    free(installed);
    return ok;
}

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *entry = recon_native_entry_initialize(argc, argv, environment);
    ReconModuleSpec module = {
        "C1XX3232.EXE", NULL,
        "333d015dd85773df2f0f9ab541ca65c40da9a2ad9bc8df9e03815f7deb20b2f7",
        RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS
    };
    int result;
    if (!entry) return 125;
    module.path = entry->program_path;
    if (!recon_register_module(&module) || !register_catalog(entry->program_path)) {
        fputs("C1XX: native startup registration failed\n", stderr);
        return 125;
    }
    /* Existing support objects establish PC53 and binary default file mode;
       bootstrap owns mutable arguments/environment exactly once. */
    result = recon_entry_app_main(entry->argc, entry->argv, entry->environment);
    recon_record_terminal_status((uint32_t)result);
    return result;
}
