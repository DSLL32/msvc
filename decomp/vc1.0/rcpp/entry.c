/* Ordinary native host. The approved CRT bridge enters the RCPP application
 * directly; retained Win32 startup is not part of this executable. */
#define RECON_SHIM_IMPL
#include <stdint.h>
#include <recon.h>

extern int rc_main(int, char **, char **);

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *entry;
    int status;
    entry = recon_native_entry_initialize(argc, argv, environment);
    status = rc_main(entry->argc, entry->argv, entry->environment);
    recon_record_terminal_status((uint32_t)status);
    return status;
}
