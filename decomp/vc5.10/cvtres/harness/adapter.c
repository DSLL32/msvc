/* Native host setup only. Original application functions remain in src/. */
#include <stdlib.h>
#include <recon-tool.h>
#include "adapter.h"
#include <recon.h>

extern int main(int, char **, char **);

RECON_EXPORT int cvtres_case_setup(void)
{
    ReconModuleSpec spec;
    HMODULE module;
    spec.name = "CVTRES.EXE";
    spec.path = getenv("RECON_PROGRAM_PATH");
    spec.sha256 = binary_descriptor.original_sha256;
    spec.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    module = recon_register_module(&spec);
    if (!module) return 0;
    recon_load_module_resources(module, "cvtres.resources");
    return 1;
}

int cvtres_case_invoke(int argc, char **argv)
{
    return main(argc, argv, (char **)0);
}
