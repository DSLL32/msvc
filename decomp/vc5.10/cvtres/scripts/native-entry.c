/* Native instrumentation only, never part of the Win32 reconstruction graph. */
#include <stdlib.h>
#include <recon.h>

static void __attribute__((constructor,no_instrument_function))
original_crt_entry_barrier(void)
{
    if (getenv("CVTRES_INITIAL_TRAP")) { __asm { int 3 } }
    {
        ReconModuleSpec module;
        HMODULE handle;
        const char *provider = getenv("CVTRES_RESOURCE_PROVIDER");
        module.name = "CVTRES.EXE";
        module.path = getenv("RECON_PROGRAM_PATH");
        module.sha256 = "16a45886f9257d990a9478c32ee6c1cbf4ac1fbcf889e5755960539233962465";
        module.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
        handle = recon_register_module(&module);
        if (!handle || !provider) exit(125);
        recon_load_module_resources(handle, provider);
    }
    /* Exact FP8.OBJ startup contract, verified at original 010032e0. */
    recon_controlfp(0x10000, 0x30000);
}
