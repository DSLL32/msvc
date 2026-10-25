#include "code-records.h"
#include "coff-output.h"
#include "function-records.h"
#include "register-sets.h"
#include "function-reader.h"
#include "symbol-storage.h"
#include <recon.h>

/* Unknown child functions remain named trapping frontiers. */
unsigned reset_function_line_input(void)
{
    __asm { int 3 }
    return 0;
}
C2Record *new_statement_boundary(void)
{
    __asm { int 3 }
    return NULL;
}
void __fastcall link_code_nodes(C2Record *first, C2Record *last)
{
    __asm { int 3 }
}
void __fastcall create_function_flow(C2Function *function, C2Record *first,
                                     C2Record *last, int pool)
{
    __asm { int 3 }
}
void __fastcall read_function_code(C2Function *function, C2Record **pending,
                                   C2Record *first, int limit, int outermost)
{
    __asm { int 3 }
}
void __fastcall validate_function_code(C2Record *first)
{
    __asm { int 3 }
}
void finish_function_input(void)
{
    __asm { int 3 }
}
void __fastcall finish_function_debug(C2Function *function)
{
    __asm { int 3 }
}
void __fastcall finish_optimized_function_input(C2Function *function)
{
    __asm { int 3 }
}
FILE *__fastcall read_pdb_symbol_il(unsigned serial, unsigned index)
{
    __asm { int 3 }
    return NULL;
}
FILE *__fastcall read_pdb_expression_symbols(unsigned serial, unsigned index)
{
    __asm { int 3 }
    return NULL;
}
FILE *__fastcall read_pdb_expression_il(unsigned serial, unsigned index)
{
    __asm { int 3 }
    return NULL;
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

C2Symbol *__fastcall lookup_symbol(C2ScopeHash *scope, unsigned serial)
{
    __asm { int 3 }
    return NULL;
}


void __fastcall queue_symbol_debug(C2Symbol *symbol)
{
    __asm { int 3 }
}


unsigned __fastcall align_stack_storage(unsigned position, unsigned bytes, int alignment)
{
    __asm { int 3 }
    return 0;
}
void __fastcall reserve_section_symbol(C2Symbol *symbol)
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

unsigned input_extended_flags(void) { __asm { int 3 } return 0; }
void __fastcall clone_alias_symbol(C2Symbol *symbol) { __asm { int 3 } }
void __fastcall read_qualified_symbol(C2Symbol *symbol, int pool) { __asm { int 3 } }
void compiler_warning(int level, int code, ...) { __asm { int 3 } }
void __fastcall read_module_directive(int kind) { __asm { int 3 } }
int __fastcall register_source_file(char *name) { __asm { int 3 } return 0; }
char *__fastcall lookup_source_file(char *name, unsigned *bytes) { __asm { int 3 } return NULL; }
void __fastcall uppercase_symbol_name(char *name) { __asm { int 3 } }

int __fastcall is_constant_symbol(C2Symbol *symbol) { __asm { int 3 } return 0; }
int __fastcall is_known_symbol(unsigned serial) { __asm { int 3 } return 0; }
C2Section *__fastcall select_symbol_section(C2FunctionSymbol *function, C2Symbol *symbol) { __asm { int 3 } return NULL; }

C2InitRecord *__fastcall new_initializer(int kind) { __asm { int 3 } return NULL; }
void __fastcall read_initializer_values(C2InitRecord *record) { __asm { int 3 } }
C2InitRecord *__fastcall queue_initializer(C2InitRecord *record) { __asm { int 3 } return NULL; }
void __fastcall emit_initializer(C2InitRecord *record) { __asm { int 3 } }
void __fastcall free_initializer(C2InitRecord *record) { __asm { int 3 } }


int check_extended_backend(void)
{
    __asm { int 3 }
    return 0;
}

unsigned short __fastcall listing_source_index(unsigned serial, unsigned source)
{
    __asm { int 3 }
    return 0;
}

int prepare_deferred_function(void)
{
    __asm { int 3 }
    return 0;
}

void finish_deferred_function(void)
{
    __asm { int 3 }
}



void __fastcall activate_scope(C2ScopeHash *scope)
{
    __asm { int 3 }
}

void __fastcall canonicalize_function(C2Function *function)
{
    __asm { int 3 }
}

int __fastcall skip_global_optimization(C2Function *function)
{
    __asm { int 3 }
    return 0;
}

void __fastcall function_pass_409dac(C2Function *function, int mode)
{
    __asm { int 3 }
}

void __fastcall function_pass_40d9be(C2Function *function, int mode)
{
    __asm { int 3 }
}

void __fastcall function_pass_456857(C2Function *function, int mode)
{
    __asm { int 3 }
}

void __fastcall function_pass_4570e0(C2Function *function, int mode)
{
    __asm { int 3 }
}

void finish_function(void)
{
    __asm { int 3 }
}

void __fastcall function_pass_463ff1(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_456b13(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_45729f(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_458c9f(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_46481c(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_483f46(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_4088c7(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_427d89(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_4828f8(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_421bdf(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_421e34(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_409c5c(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42127d(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_4180e8(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_413245(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_4132fa(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_413369(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_412292(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_412412(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_464b8a(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_412457(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_4120d4(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_412d5c(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_4144ef(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_414862(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_40d254(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_414a64(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_41759e(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_416e6a(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42a8aa(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42b3e2(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42b750(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42e689(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_483b6f(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42e6a6(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_402cb6(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_402b6e(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42d635(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42d65a(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42f060(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_402c2c(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42f3e4(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_42f446(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_4330f7(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_433546(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_4338ab(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_465486(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_47d1d3(C2Function *function)
{
    __asm { int 3 }
}

void __fastcall function_pass_433906(C2Function *function)
{
    __asm { int 3 }
}




void output_callback_41f3f2()
{
    __asm { int 3 }
}

void __fastcall output_callback_41fa04(C2Symbol *symbol)
{
    __asm { int 3 }
}

void output_callback_434139()
{
    __asm { int 3 }
}

void output_callback_4346cf()
{
    __asm { int 3 }
}

void output_callback_435633()
{
    __asm { int 3 }
}

void output_callback_4357b8()
{
    __asm { int 3 }
}

void output_callback_435af6()
{
    __asm { int 3 }
}

void output_callback_43696a()
{
    __asm { int 3 }
}

void output_callback_442b9f()
{
    __asm { int 3 }
}

void output_callback_442d21()
{
    __asm { int 3 }
}

void output_callback_442d33()
{
    __asm { int 3 }
}

void output_callback_44308f()
{
    __asm { int 3 }
}

void output_callback_4432a1()
{
    __asm { int 3 }
}

void output_callback_44a79d()
{
    __asm { int 3 }
}


void output_callback_4541f3()
{
    __asm { int 3 }
}

void output_callback_454ba8()
{
    __asm { int 3 }
}

void output_callback_4849f2()
{
    __asm { int 3 }
}

void output_callback_4849f5()
{
    __asm { int 3 }
}

void __fastcall output_callback_4849fa(C2Symbol *symbol)
{
    __asm { int 3 }
}

void output_callback_4850f5()
{
    __asm { int 3 }
}

void initialize_codeview_output(void)
{
    __asm { int 3 }
}

unsigned query_incremental_stream_size(void)
{
    __asm { int 3 }
    return 0;
}
unsigned __fastcall read_incremental_stream(void *buffer, unsigned bytes)
{
    __asm { int 3 }
    return 0;
}
unsigned char read_incremental_byte(void)
{
    __asm { int 3 }
    return 0;
}
int validate_incremental_options(void)
{
    __asm { int 3 }
    return 0;
}
void prepare_output_486e67(void) { __asm { int 3 } }
void prepare_output_486a23(void) { __asm { int 3 } }
void prepare_output_48710f(void) { __asm { int 3 } }
void __fastcall writer_callback_48511c(int error, char *source, int line) { __asm { int 3 } }
void writer_callback_45477a() { __asm { int 3 } }

int read_incremental_output(void) { __asm { int 3 } return 0; }
void restore_incremental_output(void) { __asm { int 3 } }
unsigned __fastcall add_incremental_symbol(C2CoffRecord *record) { __asm { int 3 } return 0; }
unsigned __fastcall reserve_incremental_symbols(unsigned count, unsigned short section, int reuse) { __asm { int 3 } return 0; }
void grow_coff_symbol_buffer(void) { __asm { int 3 } }

void __fastcall emit_incremental_string(char *value) { __asm { int 3 } }

void __fastcall write_output_fill(unsigned char value, unsigned count)
{
    __asm { int 3 }
}

unsigned short __fastcall add_associative_coff_section(char *name, unsigned flags,
    unsigned char selection, unsigned short association, int flag8, int reuse)
{
    __asm { int 3 }
    return 0;
}
unsigned char __fastcall coff_selection_code(unsigned char selection)
{
    __asm { int 3 }
    return 0;
}
void __fastcall mark_debug_section(unsigned section)
{
    __asm { int 3 }
}

unsigned char get_assembly_comment_character(void)
{
    __asm { int 3 }
    return 0;
}

void __fastcall write_assembly_header(char *source)
{
    __asm { int 3 }
}


int __fastcall has_local_symbol(unsigned serial)
{
    __asm { int 3 }
    return 0;
}

C2Symbol *__fastcall resolve_alias_symbol(C2Symbol *symbol) { __asm { int 3 } return NULL; }
void __fastcall record_symbol_references(C2Symbol *symbol) { __asm { int 3 } }
int __fastcall symbol_requires_references(C2Symbol *symbol) { __asm { int 3 } return 0; }
int __fastcall initializer_is_static(C2InitRecord *record) { __asm { int 3 } return 0; }
C2DebugLink *new_symbol_link(void) { __asm { int 3 } return NULL; }






void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor)
{
    __asm { int 3 }
}

void assembly_callback_4894ca()
{
    __asm { int 3 }
}

void assembly_callback_489596()
{
    __asm { int 3 }
}

void assembly_callback_489693()
{
    __asm { int 3 }
}

void assembly_callback_48a251()
{
    __asm { int 3 }
}

C2InitRecord *__fastcall lookup_initializer(unsigned serial)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall write_assembly_initializer(C2InitRecord *record)
{
    __asm { int 3 }
}




void reset_function_listing(void)
{
    __asm { int 3 }
}
