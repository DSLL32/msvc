#include "optimizer.h"
#include <recon.h>

extern int c2_alias_optimization, input_no_global_optimization;
extern int c2_zero_alignment, c2_basic_mode, c2_exception_mode;

/* Encountered optimizer state. These are independent original DWORDs; the
 * record/index limits are distinct from frontend register numbering. */
int optimizer_input_changed;                 /* 48fb5c */
int optimizer_context_mode;                  /* 490bd8 */
unsigned optimizer_index_limit;              /* 48e2e8 */
unsigned optimizer_initial_limit;            /* 490b8c */
unsigned optimizer_original_limit;           /* 490b88 */
C2OptimizerAlias *optimizer_aliases;       /* 490bb0 */
unsigned optimizer_state_a;                  /* 490bc4 */
unsigned optimizer_state_b;                  /* 490ba4 */
unsigned optimizer_state_c;                  /* 490bac */
unsigned optimizer_operand_limit;       /* 490bec */
C2OptimizerNode *optimizer_root;              /* 48fb58 */
unsigned optimizer_register_count;            /* 490ba8 */
void *optimizer_register_map;                /* 491010: address passed only */
int optimizer_records_finished;              /* 48fb3c */

void __fastcall optimize_function_flow(C2Function *function)
{
    C2FlowGraph *flow;
    C2BasicBlock *block;
    C2OptimizerContext *symbol_context;
    unsigned alias_count;
    C2OptimizerNode *root, *symbol_root, **symbol_table;
    C2RegisterSet *used, *defined, *copy;
    C2Expression *records;
    unsigned count, symbols, operand_limit;

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
    alias_count = assign_optimizer_registers(c2_alias_optimization, optimizer_context_mode);
    if (optimizer_context_mode) prepare_alias_input(function);
    optimizer_original_limit = optimizer_index_limit;
    block = function->flow->last;
    operand_limit = number_optimizer_operands(block->records, alias_count);
    optimizer_aliases = build_optimizer_aliases(block, alias_count, operand_limit);
    optimizer_state_a = optimizer_state_b = optimizer_state_c = 0;
    optimizer_operand_limit = operand_limit;
    root = translate_optimizer_input(block, count, c2_alias_optimization,
                                     input_no_global_optimization);
    optimizer_root = root;
    if (c2_alias_optimization) operand_limit = fold_optimizer_records(operand_limit);
    if (c2_zero_alignment) prepare_optimizer_values(operand_limit);
    analyze_optimizer_values(alias_count, operand_limit, c2_alias_optimization);
    if (c2_alias_optimization) rewrite_optimizer_records(operand_limit);
    initialize_optimizer_registers();
    number_optimizer_registers();
    build_optimizer_register_sets(root, count, alias_count, optimizer_register_count,
                                   c2_alias_optimization);
    if (function->flags & 0x40)
        finish_optimizer_symbol_map(symbol_context, symbol_root, optimizer_register_count);
    records = finish_optimizer_records(block->records, optimizer_index_limit,
                                        optimizer_original_limit, c2_alias_optimization);
    emit_optimizer_records(records, optimizer_register_map, optimizer_index_limit);
    reset_input_pool(4);
    reset_input_pool(6);
    optimizer_records_finished = 1;
}

unsigned __fastcall count_optimizer_records(C2Record *first)
{
    C2Expression *record, *operand;
    C2FunctionRegister *reg, *variant;
    C2OptimizerScope *scope;
    C2InputHandler *handler;
    C2OptimizerCapture *capture;
    int expressions = 0;
    unsigned scopes = 1;
    for (record = (C2Expression *)first; record; record = record->next) {
        if (record->flags & 1) {
            ++expressions;
            switch (record->opcode) {
            case 0x13f:
                for (operand = record->tail.right; operand; operand = operand->next)
                    if (operand->kind == 2 && !operand->value.reg->value)
                        for (variant = operand->value.reg->canonical; variant;
                             variant = variant->variant_next)
                            variant->view_flags |= 2;
                /* The call's left operand uses the same test as an assignment. */
            case 0x13e:
                operand = record->left;
                if (operand->kind == 2) {
                    reg = operand->value.reg;
                    if (!reg->value && (reg->kind != 3 || !(reg->flags & 0x80)))
                        for (variant = reg->canonical; variant;
                             variant = variant->variant_next)
                            variant->view_flags |= 2;
                }
                break;
            case 0x151:
                break;
            case 0x160:
                scan_optimizer_clauses(record->detail.clauses);
                break;
            case 0x161:
                if (!c2_basic_mode) {
                    scope = record->detail.data;
                    mark_optimizer_register_range(record, (C2Expression *)
                        (scope->kind == 2 ? scope->alternate : scope->symbol)->initializer,
                        1, 0);
                }
                break;
            case 0x16d:
                mark_optimizer_register_range(get_optimizer_handler_body(record), NULL, 1, 1);
                break;
            case 0x171:
                if (!c2_exception_mode) {
                    handler = record->detail.data;
                    capture = handler->context->detail.data;
                    mark_optimizer_register_range(record,
                        (C2Expression *)capture->symbol->initializer, 1, 0);
                }
                break;
            default:
                for (operand = record->left; operand; operand = operand->next)
                    if (operand->kind == 3)
                        for (variant = operand->value.reg->canonical; variant;
                             variant = variant->variant_next)
                            variant->view_flags |= 2;
                break;
            }
        } else if (record->kind == 21 && record->opcode == 0x17e && record->value.pointer)
            ++scopes;
    }
    if (expressions > 10000) {
        scopes = 1;
        c2_alias_optimization = 0;
        optimizer_context_mode = 1;
    }
    return scopes;
}

C2FunctionRegister *optimizer_memory_register;       /* 48fb54 */
C2FunctionRegister *optimizer_parameter_register;    /* 48fb38 */
C2RegisterSet *optimizer_live_registers;             /* 48fa58 */
unsigned optimizer_initial_register_count;          /* 490ba0 */

unsigned __fastcall assign_optimizer_registers(int aliases, int mode)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg;
    C2InputSymbol *symbol;
    C2OptimizerRegisterInfo *info;
    unsigned groups = 1, unnamed_index = 0, unnamed_register = 0;
    unsigned char flags;

    optimizer_memory_register = c2_pool_allocate(1, sizeof(C2FunctionRegister));
    optimizer_memory_register->kind = 12;
    optimizer_parameter_register = c2_pool_allocate(1, sizeof(C2FunctionRegister));
    optimizer_parameter_register->kind = 12;
    optimizer_live_registers = new_register_set(1);
    optimizer_register_count = 1;
    for (block = first_function_register_block; block; block = block->next) {
        for (reg = block->records; reg < block->cursor; ++reg) {
            if (reg->kind == 16) continue;
            flags = reg->view_flags;
            reg->optimizer_index = 0;
            if (!(flags & 2)) {
                if (reg->kind == 4 || reg->kind == 5 ||
                    (reg->kind == 3 && !reg->value)) {
                    if ((reg->kind != 3 || reg->value) && reg->canonical == reg) {
                        symbol = (C2InputSymbol *)reg->symbol;
                        symbol->detail.optimizer_number = (unsigned short)optimizer_index_limit++;
                        reg->optimizer_index = optimizer_register_count++;
                    }
                    if (!mode) {
                        reg->view_flags = flags | 8;
                        info = c2_pool_allocate(4, sizeof(*info));
                        reg->optimizer = info;
                        info->aliases = new_register_set(6);
                        add_register_to_set(info->aliases, groups);
                        info->number = groups++;
                    } else
                        reg->optimizer = NULL;
                } else {
                    reg->optimizer = NULL;
                    if (reg->canonical != reg || (reg->kind != 7 && reg->kind != 8))
                        continue;
                    if (!(flags & 4)) {
                        optimizer_memory_register->optimizer_uses = add_optimizer_use(
                            optimizer_memory_register->optimizer_uses, reg, 1);
                        add_register_to_set(optimizer_live_registers, reg->number);
                    }
                    if (aliases) {
                        info = c2_pool_allocate(4, sizeof(*info));
                        reg->optimizer = info;
                        info->secondary = c2_pool_allocate(4, sizeof(*info));
                    }
                    symbol = (C2InputSymbol *)reg->symbol;
                    if (symbol->symbol.name) {
                        symbol->detail.optimizer_number = (unsigned short)optimizer_index_limit++;
                        reg->optimizer_index = optimizer_register_count++;
                    } else {
                        if (!unnamed_index) {
                            unnamed_index = optimizer_index_limit++;
                            unnamed_register = optimizer_register_count++;
                        }
                        symbol->detail.optimizer_number = (unsigned short)unnamed_index;
                        reg->optimizer_index = unnamed_register;
                    }
                }
            } else {
                reg->optimizer = NULL;
                if (reg->canonical != reg || reg->kind == 10 || reg->kind == 11)
                    continue;
                add_register_to_set(optimizer_live_registers, reg->number);
                symbol = (C2InputSymbol *)reg->symbol;
                if (symbol->symbol.name || reg->kind == 4 || reg->kind == 5) {
                    symbol->detail.optimizer_number = (unsigned short)optimizer_index_limit++;
                    reg->optimizer_index = optimizer_register_count++;
                } else {
                    if (!unnamed_index) {
                        unnamed_index = optimizer_index_limit++;
                        unnamed_register = optimizer_register_count++;
                    }
                    symbol->detail.optimizer_number = (unsigned short)unnamed_index;
                    reg->optimizer_index = unnamed_register;
                }
                if (aliases) {
                    info = c2_pool_allocate(4, sizeof(*info));
                    reg->optimizer = info;
                    info->secondary = c2_pool_allocate(4, sizeof(*info));
                }
                if (reg->kind == 5)
                    optimizer_parameter_register->optimizer_uses = add_optimizer_use(
                        optimizer_parameter_register->optimizer_uses, reg, 1);
                else if (reg->kind == 7 || reg->kind == 8)
                    optimizer_memory_register->optimizer_uses = add_optimizer_use(
                        optimizer_memory_register->optimizer_uses, reg, 1);
            }
        }
    }
    optimizer_initial_register_count = optimizer_register_count;
    return groups;
}

unsigned __fastcall number_optimizer_operands(C2Record *first, unsigned alias_count)
{
    C2Expression *record, *operand;
    C2OptimizerRegisterInfo *info;
    for (record = (C2Expression *)first; record; record = record->next)
        if (record->flags & 1)
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind == 2 && (operand->value.reg->view_flags & 8)) {
                    info = operand->value.reg->optimizer;
                    operand->optimizer_alias_number = alias_count;
                    add_register_to_set(info->aliases, alias_count);
                    ++alias_count;
                } else
                    operand->optimizer_alias_number = 0;
            }
    return alias_count;
}

C2OptimizerAlias *__fastcall build_optimizer_aliases(C2BasicBlock *first,
    unsigned alias_count, unsigned operand_limit)
{
    C2OptimizerAlias *table = c2_pool_allocate(4, operand_limit * sizeof(*table));
    C2BasicBlock *block;
    C2Expression *record, *operand;
    C2FunctionRegister *reg, *variant;
    C2RegisterBlock *register_block;
    C2RegisterSet *uses, *kills;
    unsigned number, i;
    for (block = first; block; block = block->previous) {
        uses = new_register_set(6);
        block->alias_uses = uses;
        kills = new_register_set(6);
        block->alias_kills = kills;
        if (block == first)
            for (i = 1; i < alias_count; ++i)
                add_register_to_set(uses, i);
        record = (C2Expression *)block->records;
        for (;;) {
            if (record->flags & 1)
                for (operand = record->tail.right; operand; operand = operand->next) {
                    number = operand->optimizer_alias_number;
                    if (number) {
                        reg = operand->value.reg;
                        for (variant = reg->canonical; variant;
                             variant = variant->variant_next)
                            if (reg->displacement <= variant->displacement &&
                                (long)((unsigned)variant->displacement + variant->value_bytes) <=
                                (long)((unsigned)reg->displacement + reg->value_bytes)) {
                                merge_optimizer_symbol_sets(uses, variant->optimizer->aliases);
                                subtract_optimizer_symbol_sets(kills, variant->optimizer->aliases);
                            }
                        add_register_to_set(uses, number);
                        table[number].record = record;
                        table[number].reg = reg;
                    }
                }
            if (record == (C2Expression *)block->last_record) break;
            record = record->next;
        }
    }
    set_optimizer_record_mode(first, 0);
    number = 1;
    for (register_block = first_function_register_block; register_block;
         register_block = register_block->next)
        for (reg = register_block->records; reg < register_block->cursor; ++reg)
            if (reg->kind != 16 && (reg->view_flags & 8))
                table[number++].reg = reg;
    return table;
}
