#include "compiler-state.h"
#include <recon.h>

/* Original main: hot127a7..12830, cold30420..3043b. Unknown callees remain
 * separate named entries; the first frontier is initialize_work_buffers. */
int __cdecl main(int argc, char **argv)
{
    c1_data.compiler_phase = 1;
    initialize_work_buffers();
    register_compiler_cleanup(release_work_buffers_238da);
    c1_bss.command_argc = argc;
    c1_bss.command_argv = argv;
    scan_internal_switches(argv);
    initialize_state_12998();
    GetSystemInfo(&c1_data.system_info);
    initialize_compiler_arenas();
    parse_command_line(argc, argv);
    if (c1_bss.debug_break)
        DebugBreak();
    if (c1_data.c9_il)
        c1_data.compiler_flags |= 0x2000;
    else
        c1_data.compiler_flags &= ~0x2000UL;
    return compile_translation_unit();
}

void __cdecl release_work_buffers_238da(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_12998(void)
{
}

int __cdecl compile_translation_unit(void)
{
    __asm { int 3 }
    return 1;
}
