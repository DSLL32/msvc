/* Ordinary native startup; excluded from the App graph and Win32 build. */
#define RECON_SHIM_IMPL
#include <recon.h>

extern int recon_entry_app_main(int, char **, char **);

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *entry;
    int result;

    entry = recon_native_entry_initialize(argc, argv, environment);
    if (!entry) return 125;
    /* The original VC1 CRT selects 53-bit x87 precision before App main. */
    (void)recon_controlfp(0x10000, 0x30000);
    result = recon_entry_app_main(entry->argc, entry->argv, entry->environment);
    recon_record_terminal_status((uint32_t)result);
    return result;
}
