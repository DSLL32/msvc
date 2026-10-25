#include "code-input.h"
#include <recon.h>

/* Unreconstructed reader children stop at their own actual identities. */

C2Expression * __fastcall new_integer_expression(unsigned short type, unsigned __int64 value)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_binary_expression(unsigned opcode, unsigned short type, C2Expression *right, C2Expression *left, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall copy_input_expression(C2Expression *expression)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall attach_input_expression(C2Expression *owner, C2Expression **slot, C2Expression *expression)
{
    __asm { int 3 }
    return 0;
}

void __fastcall unlink_input_record(C2Record *record)
{
    __asm { int 3 }
}

C2Expression * __fastcall copy_input_operand(C2Expression *owner, C2Expression *value)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_temporary_operand(unsigned short type)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_assignment_expression(unsigned opcode, unsigned short type, C2Expression *operand, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return 0;
}


C2FunctionRegister * __fastcall new_address_register(C2FunctionRegister *source, unsigned short type, unsigned attributes, C2FunctionRegister *base)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall convert_input_expression(C2Expression **expression, unsigned short type, C2Expression **prior)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_call_expression(unsigned opcode, unsigned char convention, C2Expression *target, C2Expression *arguments, C2Expression *result, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall symbol_input_expression(C2InputSymbol *symbol)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_aggregate_expression(unsigned opcode, unsigned short type, C2Expression *target, void *detail, C2Expression *source, C2Expression *destination, unsigned attributes)
{
    __asm { int 3 }
    return 0;
}

int __fastcall set_input_expression_type(C2Expression *expression, unsigned short type)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_function_exit(C2Function *function)
{
    __asm { int 3 }
    return 0;
}






C2Expression * __fastcall convert_input_operand(C2InputStack *stack, unsigned short type, unsigned bytes, int flags)
{
    __asm { int 3 }
    return 0;
}

int __fastcall needs_input_return_storage(unsigned convention, unsigned short type, unsigned bytes)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall pop_input_expression(C2InputStack *stack)
{
    __asm { int 3 }
    return 0;
}


C2Expression * __fastcall new_register_expression(unsigned opcode, C2FunctionRegister *reg)
{
    __asm { int 3 }
    return 0;
}

void __fastcall mark_first_input_expression(C2Expression *expression)
{
    __asm { int 3 }
}





void __fastcall finish_input_expression(C2InputStack *stack)
{
    __asm { int 3 }
}

void __fastcall input_operand_pair(C2InputStack **stack, C2Expression **left, C2Expression **right, int condition)
{
    __asm { int 3 }
}

void __fastcall input_binary_operator(C2InputStack **stack, unsigned opcode)
{
    __asm { int 3 }
}

C2Expression * __fastcall new_input_expression(unsigned opcode, unsigned short type)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_transfer_expression(unsigned opcode, unsigned short type, C2Expression *left, C2Expression *right, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return 0;
}

void __fastcall swap_input_stacks(C2InputStack **stack)
{
    __asm { int 3 }
}

void __fastcall input_shift_operator(C2InputStack **stack, C2ILToken *token)
{
    __asm { int 3 }
}

unsigned char __fastcall input_comparison(C2InputStack **stack)
{
    __asm { int 3 }
    return 0;
}

void __fastcall adjust_input_operand(C2Expression *operand)
{
    __asm { int 3 }
}

void __fastcall input_call(C2InputStack **stack, C2ILToken *token, int depth, C2Function *function, int arguments)
{
    __asm { int 3 }
}

void __fastcall input_pragma(C2InputStack *stack, C2ILToken *token)
{
    __asm { int 3 }
}

void __fastcall set_input_operand_type(C2InputStack *stack, unsigned short type)
{
    __asm { int 3 }
}

C2Expression * __fastcall save_input_expression(C2InputStack *stack)
{
    __asm { int 3 }
    return 0;
}

void __fastcall input_assignment_operator(C2InputStack **stack, unsigned opcode, C2ILToken *token)
{
    __asm { int 3 }
}

C2Expression * __fastcall new_input_temporary(unsigned short type)
{
    __asm { int 3 }
    return 0;
}

void __fastcall bind_input_temporary(C2FunctionRegister *reg, C2Expression *expression)
{
    __asm { int 3 }
}

void __fastcall input_argument_list(C2InputStack **stack)
{
    __asm { int 3 }
}

C2Expression * __fastcall finish_input_clauses(C2InputClause *clauses, C2Expression *expression)
{
    __asm { int 3 }
    return 0;
}

C2InputSymbol * __fastcall new_input_local(C2ScopeHash *scope, unsigned short attributes, unsigned alignment, short kind, int pool, unsigned char source_kind, unsigned bytes)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall convert_input_pointer(C2InputStack *stack, unsigned short type)
{
    __asm { int 3 }
    return 0;
}

void __fastcall set_input_warning_state(unsigned short *state)
{
    __asm { int 3 }
}

C2Expression * __fastcall new_block_copy_expression(unsigned opcode, unsigned short type, C2Expression *left, C2Expression *right, C2Expression *bytes, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return 0;
}

unsigned __fastcall input_type_alignment(unsigned type)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_scalar_expression(unsigned opcode, unsigned value)
{
    __asm { int 3 }
    return 0;
}

void __fastcall append_deferred_input(C2InputStack *stack)
{
    __asm { int 3 }
}

void __fastcall finish_input_temporaries(C2InputStack *stack)
{
    __asm { int 3 }
}

C2InputSymbol * __fastcall new_input_storage(C2Function *function, unsigned kind, unsigned alignment, unsigned bytes)
{
    __asm { int 3 }
    return 0;
}

void __fastcall input_unary_operator(C2InputStack **stack, unsigned opcode)
{
    __asm { int 3 }
}

void __fastcall input_condition(C2InputStack **stack, C2Expression *previous, C2ILToken *token)
{
    __asm { int 3 }
}

C2Expression * __fastcall copy_input_expression_list(C2Expression *expressions)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_real_expression(unsigned short type, C2Real *value)
{
    __asm { int 3 }
    return 0;
}

void __fastcall input_exception(C2InputStack **stack, int depth, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall input_sequence_point(C2InputStack *stack, C2Expression *expression)
{
    __asm { int 3 }
}

C2Expression * __fastcall parse_input_assembly(unsigned char *text, C2ScopeHash *scope)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_input_assembly_record(unsigned opcode, unsigned short type)
{
    __asm { int 3 }
    return 0;
}

void __fastcall input_aggregate_return(C2InputStack **stack, C2ILToken *token)
{
    __asm { int 3 }
}

void __fastcall finish_input_declaration(C2InputStack *stack)
{
    __asm { int 3 }
}


void __fastcall read_input_type(C2ILToken *token)
{
    __asm { int 3 }
}


unsigned __int64 __cdecl read_input_longlong(void)
{
    __asm { int 3 }
    return 0;
}

unsigned __int64 __cdecl read_input_qword(void)
{
    __asm { int 3 }
    return 0;
}

C2Real __cdecl read_input_real(void)
{
    C2Real value;
    __asm { int 3 }
    value = c2_real_zero;
    return value;
}

int __cdecl input_real_kind(C2Real *value)
{
    __asm { int 3 }
    return 0;
}

int __cdecl input_real_to_float(C2Real *input, float *output)
{
    __asm { int 3 }
    return 0;
}

int __cdecl input_real_to_double(C2Real *input, double *output)
{
    __asm { int 3 }
    return 0;
}

void __cdecl input_real_from_float(float *input, C2Real *output)
{
    __asm { int 3 }
}

void __cdecl input_real_from_double(double *input, C2Real *output)
{
    __asm { int 3 }
}

short __fastcall read_counted_input_text(char *text, int capacity)
{
    __asm { int 3 }
    return 0;
}



void __fastcall discard_input_tree(C2Expression *tree)
{
    __asm { int 3 }
}

C2Expression *__fastcall last_input_tree(C2Expression *tree)
{
    __asm { int 3 }
    return 0;
}

int __fastcall input_has_side_effects(C2Expression *tree)
{
    __asm { int 3 }
    return 0;
}

C2Expression *__fastcall prune_cached_input(C2Expression *tree)
{
    __asm { int 3 }
    return 0;
}

int __fastcall rewrite_input_reference(C2Expression *tree, C2Expression *result)
{
    __asm { int 3 }
    return 0;
}

void __fastcall remove_input_result(C2Expression *tree, C2Expression *result)
{
    __asm { int 3 }
}


void __fastcall release_small_node_list(C2SmallNode *nodes)
{
    __asm { int 3 }
}

void __fastcall unlink_input_use(C2InputUse *use, C2Expression *owner)
{
    __asm { int 3 }
}

void __fastcall release_input_exception_context(C2Expression *tree)
{
    __asm { int 3 }
}

void __fastcall release_input_assembly_context(C2Expression *tree)
{
    __asm { int 3 }
}
