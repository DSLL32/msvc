/* Ordinary native host; excluded from the reconstructed graph and Win32 build. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <string.h>
#include <recon.h>

extern int recon_entry_app_main(int, char **, char **);
extern const unsigned char recon_asset_exe_cl_errors_start[];
extern const unsigned char recon_asset_exe_cl_errors_end[];
extern const unsigned char recon_asset_exe_cl_help_start[];
extern const unsigned char recon_asset_exe_cl_help_end[];
extern const unsigned char recon_asset_exe_c1_errors_start[];
extern const unsigned char recon_asset_exe_c1_errors_end[];
extern const unsigned char recon_asset_exe_c2_errors_start[];
extern const unsigned char recon_asset_exe_c2_errors_end[];

static int register_catalog(const char *program, const char *name,
                            const unsigned char *begin, const unsigned char *end)
{
    const char *slash = strrchr(program, '\\'), *other = strrchr(program, '/');
    char path[MAX_PATH];
    size_t directory;
    if (other && (!slash || other > slash)) slash = other;
    if (!slash) return 0;
    directory = slash + 1 - program;
    if (directory + strlen(name) >= sizeof(path)) return 0;
    memcpy(path, program, directory);
    strcpy(path + directory, name);
    return recon_register_embedded_file(path, begin, end - begin);
}

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *entry;
    ReconModuleSpec module;
    const char *basename, *other;
    entry = recon_native_entry_initialize(argc, argv, environment);
    if (!entry) return 125;
    basename = strrchr(entry->program_path, '\\');
    other = strrchr(entry->program_path, '/');
    if (other && (!basename || other > basename)) basename = other;
    if (!basename) return 125;
    module.name = basename + 1;
    module.path = entry->program_path;
    module.sha256 = "c8a1d25ceab0994b0a0a565fcbcaeb5824cab1ff60b8df3437873689e32d3de4";
    module.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    if (!recon_register_module(&module) ||
        !register_catalog(entry->program_path, "CL.ERR",
                          recon_asset_exe_cl_errors_start, recon_asset_exe_cl_errors_end) ||
        !register_catalog(entry->program_path, "CL32.MSG",
                          recon_asset_exe_cl_help_start, recon_asset_exe_cl_help_end) ||
        /* CL probes pass catalogs before building their explicit -ef paths.
         * Each real child separately registers the same shipped bytes. */
        !register_catalog(entry->program_path, "C1.ERR",
                          recon_asset_exe_c1_errors_start, recon_asset_exe_c1_errors_end) ||
        !register_catalog(entry->program_path, "C23.ERR",
                          recon_asset_exe_c2_errors_start, recon_asset_exe_c2_errors_end)) {
        fputs("CL native startup: cannot register module/catalogs\n", stderr);
        return 125;
    }
    /* Shared bootstrap owns arguments and environment; CL has no separately
     * reconstructed startup to invoke. Its normal exit retains CRT cleanup. */
    return recon_entry_app_main(entry->argc, entry->argv, entry->environment);
}
