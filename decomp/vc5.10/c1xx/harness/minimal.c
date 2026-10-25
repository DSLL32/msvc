/* Native-only typed DLL host matching the original forwarding host. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "frontend.h"
#include "startup.h"
#include "cases.h"
#include <recon-fixture.h>
#include <recon.h>

/* Bind only explicit case inputs consumed by the current prefix. Other
 * Windows environment names remain outside this adapter's qualified scope. */
static char *declared_environment[] = { NULL };
static int bind_declared_environment(void)
{
    /* The current prefix's sole environment query is MSC_CMD_FLAGS. The case
     * explicitly removes it. This is not full Windows namespace equivalence. */
    return getenv("MSC_CMD_FLAGS") == NULL;
}
int c1xx_case_minimal_setup(void)
{
    const char *reference = "b3d9cd5dbedf70b9083e516bca9b28cc3cf7af12cd26974e9fd92b39bde699d8";
    char profile[4096];
    const char *assets = recon_case_assets();
    if (!assets || strlen(assets) + sizeof("/system-info-profile.bin") > sizeof(profile))
        return 0;
    strcpy(profile, assets);
    strcat(profile, "/system-info-profile.bin");
    recon_load_system_info_profile(profile, reference);
    recon_crt_bind_empty_ctype_1252(
        "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210", reference);
    recon_controlfp(_PC_53, _MCW_PC);
    return 1;
}
int c1xx_case_minimal(int argc, char **argv)
{
    ReconModuleSpec spec = { "C1XX.DLL", "R:\\toolchains\\5.10\\bin\\C1XX.DLL",
        "e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9", RECON_MODULE_NO_TLS };
    HINSTANCE module = recon_register_module(&spec);
    int result;
    if (!bind_declared_environment()) return 126;
    recon_crt_bind_arguments(argc, argv, declared_environment);
    if (!module || !_DllMainCRTStartup(module, DLL_PROCESS_ATTACH, NULL)) return 126;
    result = InvokeCompilerPass(argc, argv, 1);
    if (!_DllMainCRTStartup(module, DLL_PROCESS_DETACH, NULL)) return 126;
    return result;
}
