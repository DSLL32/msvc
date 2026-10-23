/* Linux-only test harness, outside the reconstructed function graph. */
#include "cvpack.h"
#include <recon.h>
#undef main

int main(int argc, char **argv, char **environment)
{
    ReconModuleSpec module;
    HMODULE target;
    /* Host startup supplies only the canonical process-name input. Target CRT
     * code is independently identified and counted outside application code. */
    argv[0] = getenv("VC510_CVPACK_ARGV0");
    if (!argv[0]) return 125;
    module.name = "CVPACK.EXE";
    module.path = getenv("RECON_PROGRAM_PATH");
    module.sha256 = "62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6";
    module.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    target = recon_register_module(&module);
    if (!target) return 125;
    if (!getenv("VC510_CVPACK_RESOURCES")) return 125;
    recon_load_module_resources(target, getenv("VC510_CVPACK_RESOURCES"));
    return cvpack_main(argc, argv, environment);
}
