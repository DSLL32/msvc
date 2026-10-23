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
int __fastcall function_retains_symbols(C2FunctionSymbol *symbol, int conditional)
{
    __asm { int 3 }
    return 0;
}

C2Symbol *__fastcall lookup_symbol(C2ScopeHash *scope, unsigned serial)
{
    __asm { int 3 }
    return NULL;
}

C2Symbol *__fastcall put_symbol(C2ScopeHash *scope, C2Symbol *symbol)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall queue_symbol_debug(C2Symbol *symbol)
{
    __asm { int 3 }
}

void __fastcall update_symbol_storage(C2Symbol *symbol)
{
    __asm { int 3 }
}

unsigned __fastcall reserve_symbol_storage(C2Symbol *symbol)
{
    __asm { int 3 }
    return 0;
}

void __fastcall record_filename(C2Symbol *symbol)
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

long input_compact_signed(void) { __asm { int 3 } return 0; }
short input_compact_short(void) { __asm { int 3 } return 0; }
unsigned input_extended_flags(void) { __asm { int 3 } return 0; }
void __fastcall input_string(char *buffer, unsigned capacity) { __asm { int 3 } }
void __fastcall copy_symbol_type(C2Symbol *symbol) { __asm { int 3 } }
char *__fastcall intern_symbol_name(C2Symbol *symbol, char *name, int pool) { __asm { int 3 } return NULL; }
char *__fastcall arena_strdup(char *name, int pool) { __asm { int 3 } return NULL; }
C2Symbol *__fastcall lookup_global_symbol(unsigned serial) { __asm { int 3 } return NULL; }
void __fastcall clone_alias_symbol(C2Symbol *symbol) { __asm { int 3 } }
void __fastcall read_qualified_symbol(C2Symbol *symbol, int pool) { __asm { int 3 } }
void compiler_warning(int level, int code, ...) { __asm { int 3 } }
void __fastcall read_module_directive(int kind) { __asm { int 3 } }
int __fastcall register_source_file(char *name) { __asm { int 3 } return 0; }
char *__fastcall lookup_source_file(char *name, unsigned *bytes) { __asm { int 3 } return NULL; }
