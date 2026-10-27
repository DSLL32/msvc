#include "code-input.h"
#include "function-reader.h"
#include <recon.h>

/* Unreconstructed reader children stop at their own actual identities. */





























void __fastcall bind_input_temporary(C2FunctionRegister *reg, C2Expression *expression)
{
    __asm { int 3 }
}



C2Expression * __fastcall finish_input_clauses(C2InputClause *clauses, C2Expression *expression)
{
    __asm { int 3 }
    return 0;
}

/* Original499aec is a DWORD serial override, initialized0. Recovered
 * readers use0 to request a fresh serial; its later writers remain pending. */
unsigned input_symbol_serial_override;
C2InputSymbol * __fastcall new_input_local(C2ScopeHash *scope, unsigned short attributes, unsigned alignment, short kind, int pool, unsigned char source_kind, unsigned bytes)
{
    C2InputSymbol *symbol;
    C2PendingInput *link;
    unsigned serial;

    symbol = (C2InputSymbol *)c2_allocate_node(1, pool);
    symbol->symbol.value = alignment;
    symbol->symbol.name = NULL;
    symbol->symbol.source_kind = 0x26;
    symbol->attributes = (symbol->attributes & 0xffff7800U) |
        ((((source_kind & 15U) << 3) | ((unsigned)kind & 7U)) << 4) |
        (attributes & 15U);
    symbol->symbol.flags = bytes;
    serial = input_symbol_serial_override;
    if (!serial) serial = next_symbol_serial();
    symbol->symbol.serial = serial;
    put_symbol(scope, &symbol->symbol);
    if (kind != 3) {
        update_symbol_storage(&symbol->symbol);
        if (!(symbol->symbol.flags & 0x480)) reserve_symbol_storage(&symbol->symbol);
    }
    if (scope != &global_scope && kind != 3) {
        link = (C2PendingInput *)c2_pool_allocate(1, 8);
        link->symbol = symbol;
        link->next = pending_input_records;
        pending_input_records = link;
    }
    return symbol;
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

extern int input_address_mode;
void __fastcall input_unary_operator(C2InputStack **stack, unsigned opcode)
{
    C2InputStack *context = *stack;
    C2Expression *operand = pop_input_expression(context);
    if (!input_address_mode)
        convert_input_expression(&operand,
            common_input_value_type(operand->type, 0x1004), &context->last);
    context->last = new_assignment_expression(opcode, operand->type, operand,
        link_code_nodes, (C2Record *)context->last);
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
    C2Expression *expression = (C2Expression *)allocate_small_node(8);
    expression->type = type;
    expression->value.real = value;
    expression->opcode = 0x11e;
    return expression;
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

/* The hidden12-byte structure-result pointer is callee-popped (RET4). */
C2Real __stdcall read_input_real(void)
{
    C2Real value;
    read_il_block(sizeof(value), (char *)&value);
    return value;
}

int __cdecl input_real_kind(C2Real *value)
{
    return (short)value->kind;
}

int __cdecl input_real_to_float(C2Real *input, float *output)
{
    __asm { int 3 }
    return 0;
}

/* VC5 has no 80-bit scalar C type. The genuine range-check callee receives
 * limit on ST(0), value on ST(1), and returns EAX status plus ST(0) value.
 * Only that ABI bridge requires assembly; it introduces no wrapper vertex. */
int constrain_input_real(void);
int __cdecl input_real_to_double(C2Real *input, double *output)
{
    int status;
    __asm {
        mov esi, input
        fld tbyte ptr [esi]
        fld tbyte ptr [double_conversion_limit]
        call constrain_input_real
        mov status, eax
        mov edi, output
        fstp qword ptr [edi]
        fwait
    }
    return status;
}

void __cdecl input_real_from_float(float *input, C2Real *output)
{
    __asm { int 3 }
}

void __cdecl input_real_from_double(double *input, C2Real *output)
{
    /* VC5 has no 80-bit scalar type. The existing shared store takes the
     * ten-byte value on ST(0) and its destination in ESI. */
    __asm {
        mov edi, input
        fld qword ptr [edi]
    }
    output->kind = 8;
    __asm {
        mov esi, output
        call store_extended
    }
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






void __fastcall release_input_exception_context(C2Expression *tree)
{
    __asm { int 3 }
}

void __fastcall release_input_assembly_context(C2Expression *tree)
{
    __asm { int 3 }
}

C2Expression *__fastcall convert_deferred_input_operand(C2InputStack *stack,
                                                       unsigned short type, int flags)
{
    __asm { int 3 }
    return 0;
}

C2Expression *__fastcall materialize_input_operand(C2InputStack *stack, unsigned short type)
{
    __asm { int 3 }
    return 0;
}

int __fastcall is_input_result(C2Expression *owner, C2Expression *value)
{
    __asm { int 3 }
    return 0;
}


void __fastcall replace_register_uses(C2FunctionRegister *source, C2FunctionRegister *replacement)
{
    __asm { int 3 }
}




void c2_real_from_unsigned(unsigned *input, C2Real *output)
{
    __asm { int 3 }
}

int c2_real_to_int(C2Real *input, int *output)
{
    __asm { int 3 }
    return 0;
}

int c2_real_to_unsigned(C2Real *input, unsigned *output)
{
    __asm { int 3 }
    return 0;
}

void __fastcall copy_input_assembly_context(C2Expression *source, C2Expression *copy)
{
    __asm { int 3 }
}

void __fastcall input_deferred_store(C2InputStack **stack, C2ILToken *token)
{
    __asm { int 3 }
}

int __fastcall input_basic_has_side_effects(C2Expression *tree)
{
    __asm { int 3 }
    return 0;
}

int __fastcall input_instruction_has_side_effects(C2Expression *tree)
{
    __asm { int 3 }
    return 0;
}



/* Genuine callees encountered while recovering call input. Each remains a
 * distinct entry trap until this workload reaches it. */
C2Expression *__fastcall new_input_intrinsic_expression(unsigned opcode, unsigned short type,
    C2Expression *first, C2Expression *second, C2Expression *third,
    C2Expression *fourth, unsigned short intrinsic, C2InputLink link, C2Record *prior)
{ __asm { int 3 } return NULL; }

C2InputStack *__fastcall prepare_intrinsic_argument_stacks(C2InputStack *arguments)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall lower_input_intrinsic_call(C2Function *function, C2Expression *call)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall materialize_input_call_result(C2Function *function,
    C2Expression *call, unsigned bytes)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall finish_input_call_prefix(C2Expression *prefix, C2Expression *call)
{ __asm { int 3 } return NULL; }






