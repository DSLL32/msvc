/* Native-only host glue: host file reads do not consume target CRT streams.
 * The public DLL pass uses its own Invoke vector, distinct from HOST CRT argv. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <errno.h>
#include "frontend.h"
#include "startup.h"
#include "cases.h"
#include <recon.h>

/* Original debugger vectors preserve ordered bytes; private vectors are explicit
 * hash-bound readonly fixture dependencies, never copied into this repository. */
static int read_vector(const char *path, char ***result)
{
    FILE *file = fopen(path, "rb");
    unsigned address, count, pointer, length, i;
    char **values;
    if (!file || fread(&address, 4, 1, file) != 1 ||
        fread(&count, 4, 1, file) != 1 || !address || !count || count > 512) return -1;
    values = (char **)malloc((count + 1) * sizeof(char *));
    if (!values) return -1;
    for (i = 0; i < count; ++i) {
        if (fread(&pointer, 4, 1, file) != 1 || fread(&length, 4, 1, file) != 1 ||
            !pointer || !length || length > 32768) return -1;
        values[i] = (char *)malloc(length);
        if (!values[i] || fread(values[i], 1, length, file) != length ||
            values[i][length - 1] || memchr(values[i], 0, length - 1)) return -1;
    }
    values[count] = NULL;
    if (fgetc(file) != EOF || fclose(file)) return -1;
    *result = values;
    return count;
}
int c1xx_case_minimal_setup(void)
{
    const char *reference = "b3d9cd5dbedf70b9083e516bca9b28cc3cf7af12cd26974e9fd92b39bde699d8";
    recon_load_system_info_profile("system-info-profile.bin", reference);
    recon_crt_bind_empty_ctype_1252(
        "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210", reference);
    recon_controlfp(_PC_53, _MCW_PC);
    recon_apply_x87_empty_entry_profile(reference, 0x027f, 0x0020, 0xffff);
    return 1;
}
int c1xx_case_minimal(int argc, char **argv)
{
    ReconModuleSpec spec = { "C1XX.DLL", "R:\\toolchains\\5.10\\bin\\C1XX.DLL",
        "e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9", RECON_MODULE_NO_TLS };
    HINSTANCE module = recon_register_module(&spec);
    char **host_arguments, **host_environment;
    int host_argc, result;
    host_argc = read_vector("/root/.ai-tmp/msvc2/vc5.10/c1xx/minimal-option-append-native-1/original-crt-argv.bin", &host_arguments);
    if (host_argc != 3 || read_vector("/root/.ai-tmp/msvc2/vc5.10/c1xx/minimal-option-append-native-1/original-crt-environment.bin", &host_environment) < 0) return 126;
    /* Host file reads precede the captured loader context; they are not target API calls. */
    errno = 0;
    recon_doserrno = 0;
    recon_SetLastError(203);
    if (!module || !_DllMainCRTStartup(module, DLL_PROCESS_ATTACH, NULL)) return 126;
    recon_crt_bind_arguments(host_argc, host_arguments, host_environment);
    result = InvokeCompilerPass(argc, argv, 1);
    if (!_DllMainCRTStartup(module, DLL_PROCESS_DETACH, NULL)) return 126;
    return result;
}
