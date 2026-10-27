#include "optimizer.h"
#include "optimizer-translate.h"
#include "optimizer-sets.h"
#include <recon.h>

void __fastcall subtract_optimizer_dense_sets(C2RegisterSet *destination, C2RegisterSet *source)
{
    __asm { int 3 }
}

C2FunctionRegister *__fastcall find_optimizer_register_variant(C2FunctionRegister *reg,
    long displacement)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall add_optimizer_register_range(C2FunctionRegister *reg, C2RegisterSet *set,
    long displacement, unsigned bytes)
{
    __asm { int 3 }
}

void __fastcall prepare_optimizer_register_aliases(C2FunctionRegister *reg,
    unsigned register_count)
{
    __asm { int 3 }
}

void __fastcall fill_optimizer_dense_uses(C2OptimizerUse *uses, C2RegisterSet *set,
    unsigned register_count)
{
    __asm { int 3 }
}




C2Expression *__fastcall translate_optimizer_operand(C2Expression *operand,
    C2RegisterSet *active, unsigned number, C2BasicBlock *block)
{
    __asm { int 3 }
    return NULL;
}


unsigned __fastcall resolve_optimizer_alias(C2Expression *operand,
    C2RegisterSet *active, unsigned number)
{
    __asm { int 3 }
    return 0;
}

int __fastcall compare_optimizer_alias(C2FunctionRegister *reg,
    C2Expression *operand, C2RegisterSet *active, void *context)
{
    __asm { int 3 }
    return 0;
}

void __fastcall collect_optimizer_call_uses(C2Expression *operand,
    C2OptimizerCallState *state, C2RegisterSet *active)
{
    __asm { int 3 }
}




void __fastcall collect_optimizer_scope_uses(C2Expression *first,
    C2Expression *last, int inputs, int outputs, C2OptimizerScopeState *state)
{
    __asm { int 3 }
}

C2OptimizerUse *__fastcall merge_optimizer_uses(C2OptimizerUse *destination,
    C2OptimizerUse *source, int pool)
{
    __asm { int 3 }
    return NULL;
}

C2OptimizerCaptureState *current_optimizer_capture(void)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall add_optimizer_exception_indices(C2Expression *record,
    C2OptimizerScope *scope, unsigned number, C2FunctionRegister *parameter)
{
    __asm { int 3 }
}

C2OptimizerScope *__fastcall lookup_optimizer_scope(C2OptimizerScope *index,
    unsigned serial)
{
    __asm { int 3 }
    return NULL;
}

C2Expression *__fastcall get_optimizer_handler_body(C2Expression *record)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall mark_optimizer_register_range(C2Expression *first,
    C2Expression *last, int mark, int nested)
{
    __asm { int 3 }
}

void __fastcall scan_optimizer_clauses(C2InputClause *clauses)
{
    __asm { int 3 }
}


void __fastcall prepare_alias_input(C2Function *function)
{
    __asm { int 3 }
}



unsigned __fastcall fold_optimizer_records(unsigned operand_limit)
{
    __asm { int 3 }
    return 0;
}

void __fastcall prepare_optimizer_values(unsigned operand_limit)
{
    __asm { int 3 }
}

void __fastcall rewrite_optimizer_records(unsigned operand_limit)
{
    __asm { int 3 }
}


unsigned __fastcall collect_optimizer_symbols(C2RegisterSet *used, C2RegisterSet *defined)
{
    __asm { int 3 }
    return 0;
}



C2OptimizerNode * __fastcall map_optimizer_records(C2BasicBlock *block, unsigned count, C2RegisterSet *symbols, C2OptimizerNode **table, C2RegisterSet *copy)
{
    __asm { int 3 }
    return NULL;
}



C2OptimizerContext * __fastcall build_optimizer_symbol_map(C2OptimizerNode *root, C2RegisterSet *symbols, unsigned count, C2OptimizerNode **table)
{
    __asm { int 3 }
    return NULL;
}

C2OptimizerNode * __fastcall apply_optimizer_symbol_map(C2BasicBlock *block, C2OptimizerContext *context, unsigned count, C2RegisterSet *used, C2RegisterSet *defined, C2OptimizerNode **table, C2RegisterSet *copy)
{
    __asm { int 3 }
    return NULL;
}


void __fastcall finish_optimizer_symbol_map(C2OptimizerContext *context, C2OptimizerNode *root, unsigned register_count)
{
    __asm { int 3 }
}













