#include "c2.h"
#include <recon.h>

/* Unknown child functions remain named trapping frontiers. */
int c2_math_error(struct _exception *exception)
{
    __asm { int 3 }
    return 0;
}

void __fastcall c2_read_environment(char *flags)
{
    __asm { int 3 }
}

void c2_initialize_options(void)
{
    __asm { int 3 }
}

void c2_print_banner(void)
{
    __asm { int 3 }
}

FILE *__fastcall c2_open_intermediate(char *base, char *suffix, char *mode, char *description)
{
    __asm { int 3 }
    return NULL;
}

void c2_read_header(void)
{
    __asm { int 3 }
}

void c2_reset_symbol_table(void)
{
    __asm { int 3 }
}

void *c2_create_type_table(void)
{
    __asm { int 3 }
    return NULL;
}

void c2_initialize_constants(void)
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

void c2_cleanup(void)
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

void c2_fatal(int message, ...)
{
    __asm { int 3 }
}

C2Initializer c2_register_onexit(C2Initializer callback)
{
    __asm { int 3 }
    return NULL;
}
