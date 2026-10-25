#include "optimizer.h"
#include <recon.h>

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

C2OptimizerContext * __fastcall new_optimizer_context(int aliases, int mode)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall prepare_alias_input(C2Function *function)
{
    __asm { int 3 }
}

C2Expression * __fastcall build_optimizer_input(C2Record *first, C2OptimizerContext *context)
{
    __asm { int 3 }
    return NULL;
}

C2OptimizerNode * __fastcall collect_optimizer_blocks(C2BasicBlock *block, C2OptimizerContext *context, C2Expression *records)
{
    __asm { int 3 }
    return NULL;
}

C2OptimizerNode * __fastcall translate_optimizer_input(C2BasicBlock *block, unsigned count, int aliases, int no_global)
{
    __asm { int 3 }
    return NULL;
}

C2Expression * __fastcall fold_optimizer_records(C2Expression *records)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall prepare_optimizer_values(C2Expression *records)
{
    __asm { int 3 }
}

void __fastcall analyze_optimizer_values(C2OptimizerContext *context, C2Expression *records, int aliases)
{
    __asm { int 3 }
}

void __fastcall rewrite_optimizer_records(C2Expression *records)
{
    __asm { int 3 }
}

void initialize_optimizer_registers(void)
{
    __asm { int 3 }
}

void number_optimizer_registers(void)
{
    __asm { int 3 }
}

void __fastcall build_optimizer_register_sets(C2OptimizerNode *root, unsigned count, C2OptimizerContext *context, void *map, int aliases)
{
    __asm { int 3 }
}

C2Expression * __fastcall finish_optimizer_records(C2Record *first, unsigned limit, unsigned original_limit, int aliases)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall emit_optimizer_records(C2Expression *records, void *register_map, unsigned limit)
{
    __asm { int 3 }
}

unsigned __fastcall collect_optimizer_symbols(C2RegisterSet *used, C2RegisterSet *defined)
{
    __asm { int 3 }
    return 0;
}

C2RegisterSet * __fastcall copy_optimizer_symbol_set(C2RegisterSet *source)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall merge_optimizer_symbol_sets(C2RegisterSet *destination, C2RegisterSet *source)
{
    __asm { int 3 }
}

C2OptimizerNode * __fastcall map_optimizer_records(C2BasicBlock *block, unsigned count, C2RegisterSet *symbols, C2OptimizerNode **table, C2RegisterSet *copy)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall set_optimizer_record_mode(C2BasicBlock *block, int mode)
{
    __asm { int 3 }
}

void __fastcall reset_block_analysis(C2BasicBlock *block)
{
    __asm { int 3 }
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

void __fastcall release_optimizer_symbol_set(C2RegisterSet *set)
{
    __asm { int 3 }
}

void __fastcall finish_optimizer_symbol_map(C2OptimizerContext *context, C2OptimizerNode *root, void *map)
{
    __asm { int 3 }
}

