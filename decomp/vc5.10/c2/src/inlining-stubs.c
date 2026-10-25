#include "inlining.h"
#include <recon.h>

/* Genuine pending targets; declarations are based on their actual callers
 * and complete ABI analysis, with no implementation or extent credit. */

void __fastcall insert_input_record(C2Record *prior, C2Record *record)
{
    __asm { int 3 }
}

void __fastcall unlink_and_release_input_record(C2Record *record)
{
    __asm { int 3 }
}



char * __fastcall set_diagnostic_function(C2FunctionSymbol *symbol, int decorated)
{
    __asm { int 3 }
    return 0;
}

int __fastcall mark_inline_arguments(C2Function *function, C2Expression *entry, C2Expression *call)
{
    __asm { int 3 }
    return 0;
}

void __fastcall merge_inline_symbols(C2ScopeHash *caller, C2ScopeHash *callee)
{
    __asm { int 3 }
}

C2InlineBinding * __fastcall make_inline_bindings(C2Function *function, C2Expression *entry, C2Expression *call)
{
    __asm { int 3 }
    return 0;
}

int __fastcall rewrite_inline_returns(C2Expression *entry, C2InlineBinding *bindings, C2Expression **result)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall splice_inline_arguments(C2Expression *call, C2InlineBinding *bindings, C2Expression *before, C2Expression *last)
{
    __asm { int 3 }
    return 0;
}

void __fastcall rewrite_inline_result(C2Function *function, C2FunctionRegister *result, C2Expression *call)
{
    __asm { int 3 }
}

void __fastcall materialize_inline_arguments(C2Function *function)
{
    __asm { int 3 }
}

int __fastcall can_forward_inline_result(C2Expression *call, C2Expression *value)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall forward_inline_result(C2Expression *call, C2Expression *value)
{
    __asm { int 3 }
    return 0;
}

void __fastcall insert_inline_result(C2Expression *entry, C2Expression *result)
{
    __asm { int 3 }
}

int __fastcall is_direct_inline_return(C2Expression *record, C2Expression *before, C2Expression *call)
{
    __asm { int 3 }
    return 0;
}

C2InputSymbol * __fastcall new_inline_result_symbol(unsigned short kind, unsigned bytes, short indirect, int local, unsigned char pool, unsigned attributes)
{
    __asm { int 3 }
    return 0;
}

C2FunctionRegister * __fastcall find_inline_result_register(C2Expression *call, C2Expression **before, C2Expression *entry)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall raise_input_control_level(C2Expression *record, signed char amount)
{
    __asm { int 3 }
    return 0;
}

int __fastcall propagate_inline_constants(C2Expression **first, int count)
{
    __asm { int 3 }
    return 0;
}

void __fastcall fold_inline_constants(C2Expression *entry)
{
    __asm { int 3 }
}

void __fastcall prepare_inline_call(C2Expression *call)
{
    __asm { int 3 }
}

void __fastcall discard_inline_function(C2Function *function)
{
    __asm { int 3 }
}

C2Expression * __fastcall save_inline_argument_context(C2Expression *before, C2Expression *call)
{
    __asm { int 3 }
    return 0;
}

void __fastcall print_inline_function(unsigned serial)
{
    __asm { int 3 }
}

int __fastcall reject_inline_cost(C2Function *function, C2Expression *call, C2Expression *entry)
{
    __asm { int 3 }
    return 0;
}

void __fastcall scan_inline_assembly(C2Expression *records)
{
    __asm { int 3 }
}
C2Expression *__fastcall resolve_inline_argument(C2Expression *assignment,
    C2Expression *argument, C2Expression **context)
{
    __asm { int 3 }
    return 0;
}
