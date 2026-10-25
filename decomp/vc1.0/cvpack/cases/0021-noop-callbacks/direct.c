/* Internal return-behavior test only. No records, globals or parser continuation. */
#ifndef RECON_ORIGINAL_DIRECT
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#endif
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
extern void size_old_register_relative32(void);
extern void convert_old_symbol_register_relative32(void);
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#define size_old_register_relative32 (*(void (__cdecl *)(void))RECON_ORIGINAL_ADDRESS(probe,0x13260))
#define convert_old_symbol_register_relative32 (*(void (__cdecl *)(void))RECON_ORIGINAL_ADDRESS(probe,0x14a50))
#endif
static void callbacks(void)
{
    size_old_register_relative32();
    convert_old_symbol_register_relative32();
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,1);
#else
    recon_native_entry_finish(1);
#endif
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    probe=context;
    if (!context || context->abi_version!=RECON_ORIGINAL_PROBE_VERSION ||
        context->struct_size!=sizeof(*context) || context->marker_rva!=0x7000)
        recon_original_finish(context,0);
    callbacks();
}
#else
extern int main(int,char **,char **);
static int at_main(int argc,char **argv,char **env)
{
    (void)argc; (void)argv; (void)env;
    callbacks();
    return 0;
}
static void invoke(void *unused)
{
    char *args[]={"CVPACK.EXE",NULL};
    (void)unused;
    main(1,args,(char **)0);
}
int cvpack_direct(void)
{
    int result=recon_native_at_entry((const void *)main,(const void *)at_main,invoke,NULL);
    recon_record_terminal_status(result==1?0:1);
    syscall(SYS_exit_group,result==1?0:1);
    return 0;
}
#endif
