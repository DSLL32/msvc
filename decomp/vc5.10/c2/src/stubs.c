#include "code-records.h"
#include "coff-output.h"
#include "function-records.h"
#include "register-sets.h"
#include "function-reader.h"
#include "symbol-storage.h"
#include "flow.h"
#include "expression-order.h"
#include <recon.h>

/* Unknown child functions remain named trapping frontiers. */
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















unsigned input_extended_flags(void) { __asm { int 3 } return 0; }
void __fastcall clone_alias_symbol(C2Symbol *symbol) { __asm { int 3 } }
void __fastcall read_qualified_symbol(C2Symbol *symbol, int pool) { __asm { int 3 } }
void compiler_warning(int level, int code, ...) { __asm { int 3 } }
void __fastcall read_module_directive(int kind) { __asm { int 3 } }
int __fastcall register_source_file(char *name) { __asm { int 3 } return 0; }
char *__fastcall lookup_source_file(char *name, unsigned *bytes) { __asm { int 3 } return NULL; }
void __fastcall uppercase_symbol_name(char *name) { __asm { int 3 } }

int __fastcall is_constant_symbol(C2Symbol *symbol)
{
    return (((C2InputSymbol *)symbol)->attributes & 0x70) == 0x40;
}

C2Section *__fastcall select_symbol_section(C2FunctionSymbol *function, C2Symbol *symbol) { __asm { int 3 } return NULL; }

void __fastcall read_initializer_values(C2InitRecord *record) { __asm { int 3 } }


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






int __fastcall skip_global_optimization(C2Function *function)
{
    __asm { int 3 }
    return 0;
}



void __fastcall function_pass_456857(C2Function *function, int mode)
{
    __asm { int 3 }
}

void __fastcall function_pass_4570e0(C2Function *function, int mode)
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




void __fastcall function_pass_4828f8(C2Function *function)
{
    __asm { int 3 }
}

















void __fastcall function_pass_464b8a(C2Function *function)
{
    __asm { int 3 }
}

















void __fastcall function_pass_483b6f(C2Function *function)
{
    __asm { int 3 }
}




void __fastcall splice_symbol_record_range(C2Expression *first,
    C2Expression *last, C2Expression *anchor)
{
    __asm { int 3 }
}





void __fastcall clear_function_symbol_references(C2Function *function, C2InputSymbol *symbol)
{
    __asm { int 3 }
}

void __fastcall remove_pending_symbol_references(C2InputSymbol *symbol)
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













void __fastcall output_callback_442b9f(unsigned line, unsigned offset)
{
    __asm { int 3 }
}

void __fastcall output_callback_442d21(char *file)
{
    __asm { int 3 }
}

void output_callback_442d33()
{
    __asm { int 3 }
}

void __fastcall output_callback_44308f(C2Symbol *symbol)
{
    __asm { int 3 }
}

void output_callback_4432a1()
{
    __asm { int 3 }
}




void output_callback_454ba8(void)
{
    __asm { int 3 }
}

void __fastcall output_callback_4849f2(C2Symbol *symbol, char *file, int unwind, unsigned line)
{
    __asm { int 3 }
}

unsigned __fastcall output_callback_4849f5(C2InitRecord *value, C2Symbol *symbol, C2Section *section)
{
    __asm { int 3 }
    return 0;
}

void __fastcall output_callback_4849fa(C2Symbol *symbol)
{
    __asm { int 3 }
}

void __fastcall output_callback_4850f5(unsigned offset, C2Symbol *symbol)
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



C2Symbol *__fastcall resolve_alias_symbol(C2Symbol *symbol) { __asm { int 3 } return NULL; }
int __fastcall symbol_requires_references(C2Symbol *symbol) { __asm { int 3 } return 0; }
int __fastcall initializer_is_static(C2InitRecord *record) { __asm { int 3 } return 0; }






void __fastcall assembly_callback_4894ca(C2Symbol *symbol)
{
    __asm { int 3 }
}

void __fastcall assembly_callback_489596(C2Symbol *symbol)
{
    __asm { int 3 }
}

void __fastcall assembly_callback_489693(C2Symbol *symbol)
{
    __asm { int 3 }
}

void __fastcall assembly_callback_48a251(C2Symbol *symbol)
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



int __fastcall record_closes_scope(C2Record *record)
{
    __asm { int 3 }
    return 0;
}



void __fastcall rebuild_branch_target_edges(C2Function *function,
    C2BasicBlock *source, C2Expression *targets)
{
    __asm { int 3 }
}



int __fastcall convert_tail_return(C2Function *function, C2BasicBlock *block, C2Expression *record)
{
    __asm { int 3 }
    return 0;
}







void __fastcall remove_function_block(C2Function *function, C2BasicBlock *block)
{
    __asm { int 3 }
}


void __fastcall repair_block_scope(C2BasicBlock *block, C2BlockScope *scope, C2Function *function)
{
    __asm { int 3 }
}











void __fastcall mark_flow_region(C2FlowRegion *region)
{
    __asm { int 3 }
}








void __fastcall remove_switch_record(C2Expression *record)
{
    __asm { int 3 }
}

void __fastcall prepare_branch_removal(C2Expression *record)
{
    __asm { int 3 }
}

C2BasicBlock *__fastcall rewrite_flow_branch(C2BasicBlock *block, C2Function *function)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall merge_flow_labels(C2Expression *first, C2Expression *last,
    C2Expression *boundary, C2Function *function)
{
    __asm { int 3 }
}










C2Expression *__fastcall propagate_conversion_to_operand(C2Expression *expression,
    C2Expression *operand, unsigned short type, C2Expression *context)
{
    __asm { int 3 }
    return NULL;
}

C2Expression *__fastcall distribute_expression_conversion(C2Expression *expression,
    C2Expression *operand, unsigned short type, C2Expression *context)
{
    __asm { int 3 }
    return NULL;
}
















int __fastcall expression_values_are_integral(C2Expression *operands)
{
    __asm { int 3 }
    return 0;
}

int __fastcall expression_is_power_safe(C2Expression *expression)
{
    __asm { int 3 }
    return 0;
}

int __fastcall expression_is_nonnegative(C2Expression *expression)
{
    __asm { int 3 }
    return 0;
}







int c2_real_divide(C2Real *left, C2Real *right, C2Real *result)
{
    __asm { int 3 }
    return 0;
}

int __fastcall evaluate_comparison_rewrite(unsigned short type, C2Expression *result, C2Expression *left, C2Expression *right, unsigned condition, int mode)
{
    __asm { int 3 }
    return 0;
}

void __fastcall remove_statement_expression(C2Record *statement, C2Expression *expression)
{
    __asm { int 3 }
}

C2Expression * __fastcall simplify_nested_comparison(C2Expression *expression, C2Expression *context)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall simplify_boolean_comparison(C2Expression *expression, C2Expression *context)
{
    __asm { int 3 }
    return 0;
}

unsigned __int64 __fastcall expression_sign_mask(C2Expression *expression, unsigned bits)
{
    __asm { int 3 }
    return 0;
}





















int __fastcall expression_type_is_significant(C2Expression *expression)
{
    __asm { int 3 }
    return 0;
}

void __fastcall reorder_constant_factors(C2Expression *expression, unsigned opcode, unsigned marker)
{
    __asm { int 3 }
}

int __fastcall can_merge_factored_constants(C2Expression *owner, C2Expression *first, C2Expression *second, unsigned opcode)
{
    __asm { int 3 }
    return 0;
}

int __fastcall expression_can_be_duplicated(C2Expression *expression)
{
    __asm { int 3 }
    return 0;
}

C2Expression *__fastcall copy_reusable_expression(C2Expression *expression)
{
    __asm { int 3 }
    return NULL;
}


















void __fastcall retain_restored_aggregate(C2Expression *owner, C2Expression *operand)
{
    __asm { int 3 }
}





int c2_real_add(C2Real *left, C2Real *right, C2Real *result)
{
    __asm { int 3 }
    return 0;
}



int c2_real_multiply(C2Real *left, C2Real *right, C2Real *result)
{
    __asm { int 3 }
    return 0;
}




int __fastcall previous_numbered_block(C2BasicBlock *block)
{ __asm { int 3 } return 0; }












void __fastcall rewrite_block_label_use(C2BasicBlock *block, C2Function *function,
    C2Expression *assignment, C2Expression *branch, C2Expression *comparison,
    C2Expression *conditional)
{ __asm { int 3 } }










void __cdecl write_output_word(unsigned short value) { __asm { int 3 } }
void __cdecl write_output_long(unsigned value) { __asm { int 3 } }
