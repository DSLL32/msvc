#include "optimizer.h"
#include <recon.h>

extern int c2_alias_optimization, input_no_global_optimization;

/* Encountered optimizer state. These are independent original DWORDs; the
 * record/index limits are distinct from frontend register numbering. */
int optimizer_input_changed;                 /* 48fb5c */
int optimizer_context_mode;                  /* 490bd8 */
unsigned optimizer_index_limit;              /* 48e2e8 */
unsigned optimizer_initial_limit;            /* 490b8c */
unsigned optimizer_original_limit;           /* 490b88 */
C2OptimizerNode *optimizer_first_block;       /* 490bb0 */
unsigned optimizer_state_a;                  /* 490bc4 */
unsigned optimizer_state_b;                  /* 490ba4 */
unsigned optimizer_state_c;                  /* 490bac */
C2Expression *optimizer_input_records;       /* 490bec */
C2OptimizerNode *optimizer_root;              /* 48fb58 */
int optimizer_prepare_values_mode;           /* 490b74 */
void *optimizer_register_sets;               /* 490ba8: address passed only */
void *optimizer_register_map;                /* 491010: address passed only */
int optimizer_records_finished;              /* 48fb3c */

void __fastcall optimize_function_flow(C2Function *function)
{
    C2FlowGraph *flow;
    C2BasicBlock *block;
    C2OptimizerContext *context, *symbol_context;
    C2OptimizerNode *root, *symbol_root, **symbol_table;
    C2RegisterSet *used, *defined, *copy;
    C2Expression *records;
    unsigned count, symbols;

    reset_input_pool(4);
    reset_input_pool(6);
    optimizer_input_changed = 0;
    optimizer_context_mode = 0;
    flow = function->flow;
    block = flow->last;
    count = count_optimizer_records(block->records);
    optimizer_index_limit = count + 2;
    if (function->flags & 0x40) {
        used = new_register_set(6);
        defined = new_register_set(6);
        symbols = collect_optimizer_symbols(used, defined);
        copy = copy_optimizer_symbol_set(defined);
        merge_optimizer_symbol_sets(copy, used);
        symbol_table = c2_pool_allocate(4, symbols * 4);
        root = map_optimizer_records(block, symbols, used, symbol_table, copy);
        set_optimizer_record_mode(block, 0);
        reset_block_analysis(flow->first);
        symbol_context = build_optimizer_symbol_map(root, used, symbols, symbol_table);
        symbol_root = apply_optimizer_symbol_map(block, symbol_context, symbols,
                                                 used, defined, symbol_table, copy);
        release_optimizer_symbol_set(used);
        release_optimizer_symbol_set(defined);
        release_optimizer_symbol_set(copy);
    }
    optimizer_initial_limit = optimizer_index_limit;
    context = new_optimizer_context(c2_alias_optimization, optimizer_context_mode);
    if (optimizer_context_mode) prepare_alias_input(function);
    optimizer_original_limit = optimizer_index_limit;
    block = function->flow->last;
    records = build_optimizer_input(block->records, context);
    optimizer_first_block = collect_optimizer_blocks(block, context, records);
    optimizer_state_a = optimizer_state_b = optimizer_state_c = 0;
    optimizer_input_records = records;
    root = translate_optimizer_input(block, count, c2_alias_optimization,
                                     input_no_global_optimization);
    optimizer_root = root;
    if (c2_alias_optimization) records = fold_optimizer_records(records);
    if (optimizer_prepare_values_mode) prepare_optimizer_values(records);
    analyze_optimizer_values(context, records, c2_alias_optimization);
    if (c2_alias_optimization) rewrite_optimizer_records(records);
    initialize_optimizer_registers();
    number_optimizer_registers();
    build_optimizer_register_sets(root, count, context, optimizer_register_sets,
                                   c2_alias_optimization);
    if (function->flags & 0x40)
        finish_optimizer_symbol_map(symbol_context, symbol_root, optimizer_register_sets);
    records = finish_optimizer_records(block->records, optimizer_index_limit,
                                        optimizer_original_limit, c2_alias_optimization);
    emit_optimizer_records(records, optimizer_register_map, optimizer_index_limit);
    reset_input_pool(4);
    reset_input_pool(6);
    optimizer_records_finished = 1;
}
