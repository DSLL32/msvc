/* Native process host only; never compiled into the Win32 reconstruction. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <recon.h>

extern int recon_entry_app_main(int, char **, char **);
extern const unsigned char recon_asset_exe_c1_errors_start[];
extern const unsigned char recon_asset_exe_c1_errors_end[];

static int register_catalog(const ReconNativeEntry *process)
{
    char installed[4096], local[4096], *leaf;
    size_t bytes = recon_asset_exe_c1_errors_end - recon_asset_exe_c1_errors_start;
    if (strlen(process->program_path) >= sizeof(installed)) return 0;
    strcpy(installed, process->program_path);
    leaf = strrchr(installed, '\\');
    if (!leaf || (size_t)(leaf - installed) + sizeof("\\C1.ERR") > sizeof(installed))
        return 0;
    strcpy(leaf + 1, "C1.ERR");
    /* The original default is relative; CL supplies the installed -ef path.
     * Other explicitly selected catalogs retain ordinary filesystem lookup. */
    if (strlen(process->current_directory) + sizeof("\\c1.err") > sizeof(local))
        return 0;
    strcpy(local, process->current_directory);
    if (local[strlen(local) - 1] != '\\') strcat(local, "\\");
    strcat(local, "c1.err");
    if (!recon_register_embedded_file("c1.err", recon_asset_exe_c1_errors_start, bytes))
        return 0;
    if (strcasecmp(local, installed) &&
        !recon_register_embedded_file(installed, recon_asset_exe_c1_errors_start, bytes))
        return 0;
    return 1;
}

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *process = recon_native_entry_initialize(argc, argv, environment);
    ReconModuleSpec image;
    int result;
    if (!process) return 125;
    image.name = strrchr(process->program_path, '\\');
    if (!image.name) return 125;
    ++image.name;
    image.path = process->program_path;
    image.sha256 = "7061440a14554b4c81d4908ddb34cd674265594d6b04932d3f70cecf7a731063";
    image.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    if (!recon_register_module(&image) || !register_catalog(process)) {
        fputs("C13232.EXE: native startup failed\n", stderr);
        return 125;
    }
    /* Existing support constructors preserve verified 53-bit precision and
     * binary default file mode; the original application owns its setup. */
    result = recon_entry_app_main(process->argc, process->argv, process->environment);
    recon_exit(result);
    return result;
}
