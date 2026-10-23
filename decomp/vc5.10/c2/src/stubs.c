#include "c2.h"
#include <recon.h>

/* Unknown child functions remain named trapping frontiers. */
int c2_math_error(struct _exception *exception)
{
    __asm { int 3 }
    return 0;
}

void c2_print_banner(void)
{
    __asm { int 3 }
}

int __fastcall c2_open_pdb(char *name, char *object_name)
{
    __asm { int 3 }
    return 0;
}
void c2_real_from_int(int *input, C2Real *output)
{
    __asm { int 3 }
}

void c2_initialize_opcodes(void)
{
    __asm { int 3 }
}

void c2_initialize_machine(void)
{
    __asm { int 3 }
}

void c2_initialize_trees(void)
{
    __asm { int 3 }
}

void c2_initialize_codegen(void)
{
    __asm { int 3 }
}

void c2_initialize_registers(void)
{
    __asm { int 3 }
}

void c2_initialize_optimizer(void)
{
    __asm { int 3 }
}

void c2_compile_unit(void)
{
    __asm { int 3 }
}

void c2_finalize_output(void)
{
    __asm { int 3 }
}

void c2_flush_debug(void)
{
    __asm { int 3 }
}



BOOL WINAPI c2_console_handler(DWORD control)
{
    __asm { int 3 }
    return FALSE;
}

void __fastcall c2_report_internal(char *source, int line)
{
    __asm { int 3 }
}






void __fastcall c2_system_fatal(int message, char *prefix, char *filename)
{
    __asm { int 3 }
}









unsigned __fastcall resolve_diagnostic_location(unsigned location, char **file)
{
    __asm { int 3 }
    return 0;
}
