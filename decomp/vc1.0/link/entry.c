/* Ordinary native process host, outside LINK's reconstructed function graph.
 * The common initializer owns argv/env/raw command line for process lifetime;
 * LINK's real two-argument dispatcher owns its normal initialization/cleanup.
 * No captured responses or fixture state are installed here. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <recon.h>
extern int recon_entry_app_main(int,char **);
extern int link_native_runtime_setup(const char *);
extern const unsigned char recon_asset_exe_messages_start[];
extern const unsigned char recon_asset_exe_messages_end[];

int main(int argc,char **argv,char **environment)
{
    ReconNativeEntry *process;
    char catalog[MAX_PATH];
    char *basename;
    int result,saved_errno;
    unsigned long saved_dos;
    DWORD saved_last;

    process=recon_native_entry_initialize(argc,argv,environment);
    if(!process)recon_unsupported("LINK native process initialization");
    saved_errno=errno;saved_dos=recon_doserrno;saved_last=recon_GetLastError();
    if(!link_native_runtime_setup(process->program_path))
        recon_unsupported("LINK native primary-module setup");
    if(strlen(process->program_path)>=sizeof(catalog))
        recon_unsupported("LINK native catalog path exceeds MAX_PATH");
    strcpy(catalog,process->program_path);
    basename=strrchr(catalog,'\\');
    if(!basename||basename-catalog+1+sizeof("LINK.ERR")>sizeof(catalog))
        recon_unsupported("LINK native catalog sibling path");
    strcpy(basename+1,"LINK.ERR");
    if(!recon_register_embedded_file(catalog,recon_asset_exe_messages_start,
        (size_t)(recon_asset_exe_messages_end-recon_asset_exe_messages_start)))
        recon_unsupported("LINK embedded message catalog registration");
    /* The original LIBC startup selects53-bit precision before App main. */
    (void)recon_controlfp(0x10000,0x30000);
    errno=saved_errno;recon_doserrno=saved_dos;recon_SetLastError(saved_last);
    result=recon_entry_app_main(process->argc,process->argv);
    recon_record_terminal_status((uint32_t)result);
    return result;
}
