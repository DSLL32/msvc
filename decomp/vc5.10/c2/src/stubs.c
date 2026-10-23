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


char **c2_copy_arguments(void)
{
    __asm { int 3 }
    return NULL;
}
int __fastcall c2_match_option(C2Option *table, char *input, char *(__cdecl *next)(void), int mode)
{
    __asm { int 3 }
    return 0;
}

void __fastcall c2_validate_alignment(int alignment)
{
    __asm { int 3 }
}

char *__fastcall c2_get_diagnostic_string(int message)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall c2_system_fatal(int message, char *prefix, char *filename)
{
    __asm { int 3 }
}
