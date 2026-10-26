#include "optimizer.h"
#include "optimizer-translate.h"
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
C2FunctionRegister *optimizer_address_registers; /* 490bc4 */
unsigned optimizer_state_c;                  /* 490bac */
unsigned optimizer_operand_limit;       /* 490bec */
unsigned optimizer_expression_limit;          /* 48fb58 */
unsigned optimizer_register_count;            /* 490ba8 */
unsigned optimizer_final_index_limit;        /* 491010 */
int optimizer_records_finished;              /* 48fb3c */
unsigned optimizer_reserved_index_base;       /* 490bb4 */
unsigned optimizer_reserved_index_cursor;     /* 490b9c */
unsigned optimizer_reserved_register_base;    /* 490bc0 */
unsigned optimizer_reserved_register_cursor;  /* 490b94 */

void __fastcall optimize_function_flow(C2Function *function)
{
    C2FlowGraph *flow;
    C2BasicBlock *block;
    C2OptimizerContext *symbol_context;
    unsigned alias_count;
    C2OptimizerNode *root, *symbol_root, **symbol_table;
    C2RegisterSet *used, *defined, *copy;
    C2RegisterSet *records;
    unsigned count, symbols, operand_limit, expression_limit;

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
        subtract_optimizer_symbol_sets(copy, used);
        symbol_table = c2_pool_allocate(4, symbols * 4);
        root = map_optimizer_records(block, symbols, used, symbol_table, copy);
        solve_optimizer_alias_sets(block, 0);
        solve_allocation_liveness(flow->first);
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
    optimizer_address_registers = NULL;
    optimizer_state_c = 0;
    optimizer_call_states = NULL;
    optimizer_operand_limit = operand_limit;
    expression_limit = translate_optimizer_input(block, count, c2_alias_optimization,
                                     input_no_global_optimization);
    optimizer_expression_limit = expression_limit;
    if (c2_alias_optimization) operand_limit = fold_optimizer_records(operand_limit);
    if (c2_zero_alignment) prepare_optimizer_values(operand_limit);
    analyze_optimizer_values(alias_count, operand_limit, c2_alias_optimization);
    if (c2_alias_optimization) rewrite_optimizer_records(operand_limit);
    initialize_optimizer_registers();
    number_optimizer_registers();
    build_optimizer_register_sets(expression_limit, count, alias_count, optimizer_register_count,
                                   c2_alias_optimization);
    if (function->flags & 0x40)
        finish_optimizer_symbol_map(symbol_context, symbol_root, optimizer_register_count);
    records = finish_optimizer_records(block->records, optimizer_index_limit,
                                        optimizer_original_limit, c2_alias_optimization);
    build_optimizer_conflicts(records, optimizer_final_index_limit, optimizer_index_limit);
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
                                subtract_optimizer_symbol_sets(uses, variant->optimizer->aliases);
                                merge_optimizer_symbol_sets(kills, variant->optimizer->aliases);
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
    solve_optimizer_alias_sets(first, 0);
    number = 1;
    for (register_block = first_function_register_block; register_block;
         register_block = register_block->next)
        for (reg = register_block->records; reg < register_block->cursor; ++reg)
            if (reg->kind != 16 && (reg->view_flags & 8))
                table[number++].reg = reg;
    return table;
}

void __fastcall subtract_optimizer_symbol_sets(C2RegisterSet *destination,
    C2RegisterSet *source)
{
    C2RegisterSetNode *input, *node;
    unsigned group;
    if (!destination->links.first) return;
    for (input = source->links.first; input; input = input->next) {
        group = input->group & ~31U;
        node = destination->links.first;
        while (node && node->group < group) node = node->next;
        if (node && node->group == group) {
            node->bits &= ~input->bits;
            if (!node->bits) remove_register_set_node(destination, node);
        }
    }
}

void __fastcall merge_optimizer_symbol_sets(C2RegisterSet *destination,
    C2RegisterSet *source)
{
    C2RegisterSetNode *input, *node, **place;
    unsigned group;
    for (input = source->links.first; input; input = input->next) {
        group = input->group & ~31U;
        place = &destination->links.first;
        node = *place;
        while (node && node->group < group) {
            place = &node->next;
            node = *place;
        }
        if (!node || node->group != group) {
            node = new_register_set_node(destination);
            node->group = group;
            node->next = *place;
            *place = node;
        }
        node->bits |= input->bits;
    }
}

void __fastcall solve_optimizer_alias_sets(C2BasicBlock *first, int constrained)
{
    C2BasicBlock *block;
    C2FlowEdge *edge;
    C2RegisterSet *temporary;
    int changed;
    for (block = first; block; block = block->previous) {
        block->alias_in = new_register_set(6);
        block->alias_out = copy_optimizer_symbol_set(block->alias_uses);
    }
    temporary = new_register_set(6);
    do {
        changed = 0;
        for (block = first->previous; block; block = block->previous) {
            clear_register_set(temporary);
            for (edge = block->incoming; edge; edge = edge->next_incoming)
                merge_optimizer_symbol_sets(temporary, edge->source->alias_out);
            if (constrained)
                intersect_optimizer_symbol_sets(temporary, block->alias_constraints);
            if (!optimizer_symbol_sets_equal(temporary, block->alias_in)) {
                copy_optimizer_symbol_set_into(block->alias_in, temporary);
                update_optimizer_alias_set(block->alias_out, temporary,
                                            block->alias_kills, block->alias_uses);
                changed = 1;
            }
        }
    } while (changed);
    release_optimizer_symbol_set(temporary);
}

C2RegisterSet *__fastcall copy_optimizer_symbol_set(C2RegisterSet *source)
{
    C2RegisterSet *result = new_register_set(source->pool);
    C2RegisterSetNode *input = source->links.first;
    C2RegisterSetNode *spare = result->links.first, *node = spare, *next;
    while (input) {
        if (spare) {
            node = spare;
            spare = spare->next;
        } else
            node = insert_register_set_node(result, node);
        node->group = input->group;
        node->bits = input->bits;
        input = input->next;
    }
    while (spare) {
        next = spare->next;
        remove_register_set_node(result, spare);
        spare = next;
    }
    return result;
}

C2RegisterSetNode *__fastcall insert_register_set_node(C2RegisterSet *set,
    C2RegisterSetNode *after)
{
    C2RegisterSetNode *node = new_register_set_node(set);
    if (!after) {
        node->next = set->links.first;
        set->links.first = node;
    } else {
        node->next = after->next;
        after->next = node;
    }
    return node;
}

int __fastcall optimizer_symbol_sets_equal(C2RegisterSet *first,
    C2RegisterSet *second)
{
    C2RegisterSetNode *left = first->links.first, *right = second->links.first;
    while (left) {
        if (!right || left->group != right->group || left->bits != right->bits)
            return 0;
        left = left->next;
        right = right->next;
    }
    return right == NULL;
}

void __fastcall copy_optimizer_symbol_set_into(C2RegisterSet *destination,
    C2RegisterSet *source)
{
    C2RegisterSetNode *input = source->links.first;
    C2RegisterSetNode *spare = destination->links.first, *node = spare, *next;
    while (input) {
        if (spare) {
            node = spare;
            spare = spare->next;
        } else
            node = insert_register_set_node(destination, node);
        node->group = input->group;
        node->bits = input->bits;
        input = input->next;
    }
    while (spare) {
        next = spare->next;
        remove_register_set_node(destination, spare);
        spare = next;
    }
}

void __fastcall update_optimizer_alias_set(C2RegisterSet *destination,
    C2RegisterSet *input, C2RegisterSet *kills, C2RegisterSet *uses)
{
    copy_optimizer_symbol_difference(destination, input, kills);
    merge_optimizer_symbol_sets(destination, uses);
}

void __fastcall copy_optimizer_symbol_difference(C2RegisterSet *destination,
    C2RegisterSet *input_set, C2RegisterSet *kills)
{
    C2RegisterSetNode *input = input_set->links.first;
    C2RegisterSetNode *spare = destination->links.first, *node = spare, *next;
    while (input) {
        if (spare) {
            node = spare;
            spare = spare->next;
        } else
            node = insert_register_set_node(destination, node);
        node->group = input->group;
        node->bits = input->bits;
        input = input->next;
    }
    while (spare) {
        next = spare->next;
        remove_register_set_node(destination, spare);
        spare = next;
    }
    subtract_optimizer_symbol_sets(destination, kills);
}

void __fastcall release_optimizer_symbol_set(C2RegisterSet *set)
{
    C2RegisterSetNode *node, *last;
    unsigned pool;
    if (!set) return;
    node = set->links.first;
    if (node) {
        pool = C2_REGISTER_SET_POOL(set->pool);
        if (free_register_set_nodes[pool])
            last_free_register_set_nodes[pool]->next = node;
        else
            free_register_set_nodes[pool] = node;
        last = node;
        while (last->next) last = last->next;
        last_free_register_set_nodes[pool] = last;
        set->links.first = NULL;
    }
    release_optimizer_set_header(set);
}

void __fastcall intersect_optimizer_symbol_sets(C2RegisterSet *destination,
    C2RegisterSet *source)
{
    C2RegisterSetNode *node, *next, *match = source->links.first;
    if (!match) {
        clear_register_set(destination);
        return;
    }
    for (node = destination->links.first; node; node = next) {
        while (match && match->group < node->group) match = match->next;
        next = node->next;
        if (!match || node->group != match->group)
            remove_register_set_node(destination, node);
        else {
            node->bits &= match->bits;
            if (!node->bits) remove_register_set_node(destination, node);
            match = match->next;
        }
    }
}

void __fastcall analyze_optimizer_values(unsigned alias_count, unsigned operand_limit,
    int aliases)
{
    unsigned pass = 0, index, number;
    C2OptimizerAlias *alias;
    C2OptimizerUse *use, *previous;
    C2FunctionRegister *owner, *canonical;
    int changed;
    do {
        changed = 0;
        for (index = alias_count; index < operand_limit; ++index) {
            alias = &optimizer_aliases[index];
            if (!alias->effects) continue;
            owner = alias->reg->optimizer->owner;
            number = next_optimizer_symbol(alias->effects);
            while (number != ~0U) {
                if ((int)number < (int)alias_count && !aliases) {
                    if (!pass) {
                        previous = alias->uses;
                        alias->uses = add_optimizer_use(previous, optimizer_memory_register, 4);
                        if (alias->uses != previous) changed = 1;
                    }
                } else {
                    for (use = optimizer_aliases[number].uses; use; use = use->next) {
                        previous = alias->uses;
                        alias->uses = add_optimizer_use(previous, use->reg, 4);
                        if (alias->uses != previous) {
                            changed = 1;
                            if (use->reg->kind == 14 && owner) {
                                canonical = use->reg->canonical;
                                if (owner != canonical)
                                    canonical->dependent_uses = add_optimizer_use(canonical->dependent_uses, owner, 4);
                            }
                        }
                    }
                }
                number = next_optimizer_symbol(NULL);
            }
        }
        ++pass;
    } while (changed);
    for (index = alias_count; index < operand_limit; ++index) {
        alias = &optimizer_aliases[index];
        if (alias->effects) {
            release_optimizer_symbol_set(alias->effects);
            alias->effects = NULL;
        }
        if (alias->pending10)
            for (use = alias->uses; use; use = use->next)
                if (use->reg->view_flags & 2) use->reg->view_flags |= 4;
    }
}

C2OptimizerUse *__fastcall add_optimizer_use(C2OptimizerUse *head,
    C2FunctionRegister *reg, int pool)
{
    C2OptimizerUse *use;
    if (reg->kind == 10 || reg->kind == 11) return head;
    for (use = head; use; use = use->next)
        if (use->reg == reg) return head;
    use = c2_pool_allocate(pool, sizeof(C2OptimizerUse));
    use->reg = reg;
    use->next = head;
    return use;
}

void number_optimizer_registers(void)
{
    optimizer_reserved_index_base = optimizer_reserved_index_cursor = optimizer_index_limit;
    optimizer_reserved_register_base = optimizer_reserved_register_cursor = optimizer_register_count;
    optimizer_index_limit += 16;
    optimizer_register_count += 16;
}

/* Shared by the expression-rewriting pass and its reached tree walkers. */
extern unsigned diagnostic_position;
int expression_simplification_mode;
int expression_warning_issued;
void __fastcall simplify_statement_expressions(C2Record *record);

void __fastcall simplify_function_expressions(C2Function *function, int mode)
{
    C2Record *record = function->flow->last->records->next;
    C2Record *end = function->flow->first->last_record, *next;
    C2Expression *result;
    expression_simplification_mode = mode;
    for (; record != end; record = next) {
        next = record->next;
        if (record->line) {
            record_line_delta = record->line;
            diagnostic_position = function->source_line_base + record->line;
        }
        expression_warning_issued = 0;
        switch (record->kind) {
        case 11: case 12: case 13: case 15: case 17:
            result = record->results;
            if (!result || !((1U << (result->kind & 31)) & 6) ||
                result->value.reg->value) break;
            /* Definitions owned by a temporary are reached through their uses. */
        case 14: case 16: case 18: case 19:
            simplify_statement_expressions(record);
            break;
        }
    }
}

C2Record *expression_statement_context;     /* 48fb30 */
C2Expression *expression_following_record;  /* 48fb34 */
C2Expression *__fastcall simplify_expression_tree(C2Expression *expression);
C2Expression *__fastcall finalize_expression_tree(C2Expression *expression);

void __fastcall simplify_statement_expressions(C2Record *record)
{
    C2Expression *root = (C2Expression *)record, *argument = record->value.arguments;
    if (!argument) return;
    if (record->opcode == 0x140) {
        if (!((1U << (argument->kind & 31)) & 6)) return;
        root = argument->value.reg->value;
        if (!root) return;
        expression_statement_context = record;
    } else if (record->opcode == 0x159) {
        root = argument->next->value.reg->value;
        expression_statement_context = record;
    } else {
        expression_statement_context = NULL;
    }
    expression_following_record = root->next;
    root = simplify_expression_tree(root);
    finalize_expression_tree(root);
}

int __fastcall mark_expression_execution(C2Expression *expression, C2Expression *result);
void __fastcall simplify_expression_operands(C2Expression *expression);
void __fastcall prune_detached_operands(C2Expression *expression);
C2Expression *__fastcall materialize_expression_result(C2Expression *expression);
C2Expression *__fastcall fold_expression_value(C2Expression *expression, C2Expression *context);

C2Expression *__fastcall simplify_expression_tree(C2Expression *expression)
{
    C2Expression *operand, *next, *first, *second, *value, *replacement;
    switch (expression->kind) {
    case 12: case 13: case 14: case 16: case 17: case 18: case 19:
        expression->control_flags = 1;
        break;
    default:
        mark_expression_execution(expression, NULL);
        break;
    }
    if (expression->control_flags == 1) {
        unlink_input_record((C2Record *)expression);
        for (operand = expression->left; operand; operand = operand->next)
            if (((1U << (operand->kind & 31)) & 6) && operand->value.reg->value)
                simplify_expression_tree(operand->value.reg->value);
        prune_detached_operands(expression);
        return expression;
    }
    simplify_expression_operands(expression);
    for (operand = expression->tail.right; operand; operand = next) {
        next = operand->next;
        if (!((1U << (operand->kind & 31)) & 0x60)) continue;
        first = operand->aggregate_value;
        if (first && (!((1U << (first->kind & 31)) & 6) || !first->value.reg->value))
            continue;
        second = operand->aggregate_second;
        if (second && (!((1U << (second->kind & 31)) & 6) || !second->value.reg->value))
            continue;
        if (first == second) continue;
        value = simplify_expression_tree(first->value.reg->value);
        if (value->opcode != 0x131) continue;
        replacement = fold_expression_value(operand, NULL);
        if (replacement != operand) replace_input_result(expression, operand, replacement);
    }
    return materialize_expression_result(expression);
}

int force_expression_materialization;       /* 48fb28; initialized zero, no target writes */
int __fastcall register_views_differ(C2Expression *expression);

int __fastcall mark_expression_execution(C2Expression *expression, C2Expression *result)
{
    C2Expression *operand = expression->tail.right;
    if (input_has_side_effects(expression) ||
        (operand->kind == 1 && operand->value.reg->kind == 3 &&
         operand->value.reg->value && register_views_differ(operand)) ||
        expression->kind == 17) {
        expression->control_flags = 1;
        return 1;
    }
    expression->control_flags = 2;
    if (force_expression_materialization || (expression->type & 0xf000) == 0x5000 ||
        operand->next) return 1;
    if (((1U << (operand->kind & 31)) & 6) && operand->value.reg->value &&
        operand->value.reg->canonical != operand->value.reg &&
        (!result || (result->type & 0xfff) > (operand->type & 0xfff))) return 1;
    switch (expression->opcode) {
    case 0x131: case 0x138: case 0x14c: case 0x15f:
        return 1;
    default:
        return 0;
    }
}

void __fastcall simplify_expression_operands(C2Expression *expression)
{
    C2Expression *operand, *definition, *first, *second;
    unsigned kinds;
    for (operand = expression->left; operand; operand = operand->next) {
        if (operand->location.expression.properties & 0x20) continue;
        kinds = 1U << (operand->kind & 31);
        if (kinds & 6) {
            definition = operand->value.reg->value;
            if (definition) {
                if (mark_expression_execution(definition, operand))
                    simplify_expression_tree(definition);
                else
                    simplify_expression_operands(definition);
                continue;
            }
        }
        if (!(kinds & 0x60)) continue;
        first = operand->aggregate_value;
        if (first && (!((1U << (first->kind & 31)) & 6) || !first->value.reg->value))
            continue;
        second = operand->aggregate_second;
        if (second && (!((1U << (second->kind & 31)) & 6) || !second->value.reg->value))
            continue;
        if (first != second) simplify_expression_tree(first->value.reg->value);
    }
    unlink_input_record((C2Record *)expression);
    prune_detached_operands(expression);
}

int __fastcall register_views_differ(C2Expression *expression)
{
    C2FunctionRegister *value = expression->value.reg;
    C2FunctionRegister *original = expression->original_register;
    return original->canonical != value->canonical ||
        original->displacement != value->displacement;
}

void __fastcall prune_detached_operands(C2Expression *expression)
{
    C2Expression *operand, *previous, *next;
    unsigned kinds;
    for (operand = expression->tail.right; operand; operand = operand->next)
        if ((1U << (operand->kind & 31)) & 0x60)
            operand->location.expression.comparison |= 0x20;
    previous = NULL;
    for (operand = expression->left; operand; operand = next) {
        next = operand->next;
        kinds = 1U << (operand->kind & 31);
        if (kinds & 0x60) operand->location.expression.comparison |= 0x20;
        if ((kinds & 0x7fe) && (operand->location.expression.properties & 0x20)) {
            previous->next = next;
            operand->next = NULL;
        } else {
            previous = operand;
        }
    }
}

int expression_rewrite_changed;             /* 48fb2c */
C2Expression *__fastcall prepare_expression_rewrite(C2Expression *expression);
C2Expression *__fastcall rewrite_expression_tree(C2Expression *expression, C2Expression *context, int mode);
C2Expression *__fastcall fold_expression_tree(C2Expression *expression, C2Expression *context);
C2Expression *__fastcall simplify_expression_conditions(C2Expression *expression, C2Expression *context);
C2Expression *__fastcall canonicalize_expression_tree(C2Expression *expression, C2Expression *context);

C2Expression *__fastcall materialize_expression_result(C2Expression *expression)
{
    C2Expression *saved = copy_input_expression(expression->tail.right);
    C2Expression *temporary = new_temporary_operand(expression->tail.right->type);
    C2Expression *result;
    C2FunctionRegister *reg;
    int iterations;
    replace_input_result(expression, expression->tail.right, temporary);
    reg = expression->tail.right->value.reg;
    if (!reg->canonical->variant_next) reg->value = expression;
    else bind_input_temporary(reg, expression);
    result = prepare_expression_rewrite(expression);
    iterations = 0;
    do {
        if (++iterations > 100) c2_fatal(1, "E:\\utc\\src\\\\P2\\optimize.c", 0x19e);
        expression_rewrite_changed = 0;
        result = rewrite_expression_tree(result, NULL, 0);
        result = fold_expression_tree(result, NULL);
    } while (expression_rewrite_changed && !((1U << (result->kind & 31)) & 0x7fe));
    iterations = 0;
    while (!((1U << (result->kind & 31)) & 0x7fe)) {
        if (++iterations > 100) c2_fatal(1, "E:\\utc\\src\\\\P2\\optimize.c", 0x1b1);
        expression_rewrite_changed = 0;
        result = rewrite_expression_tree(result, NULL, 1);
        if (!expression_rewrite_changed) break;
    }
    iterations = 0;
    while (!((1U << (result->kind & 31)) & 0x7fe)) {
        if (++iterations > 100) c2_fatal(1, "E:\\utc\\src\\\\P2\\optimize.c", 0x1c3);
        expression_rewrite_changed = 0;
        result = simplify_expression_conditions(result, NULL);
        if (!expression_rewrite_changed) break;
    }
    if (!((1U << (result->kind & 31)) & 0x7fe))
        result = canonicalize_expression_tree(result, NULL);
    if (!((1U << (result->kind & 31)) & 0x7fe)) {
        temporary = result->tail.right;
        release_function_register(temporary->value.reg->canonical);
        replace_input_result(result, temporary, saved);
    } else {
        result = new_binary_expression(0x131, saved->type, saved, result, NULL, NULL);
        prune_detached_operands(result);
        result->control_flags = 2;
    }
    if (((1U << (saved->kind & 31)) & 6) && saved->value.reg->value) {
        reg = saved->value.reg;
        if (!reg->canonical->variant_next) reg->value = result;
        else bind_input_temporary(reg, result);
    }
    return result;
}

int __fastcall expression_can_flatten(C2Expression *expression);
void __fastcall flatten_expression_inputs(C2Expression *owner, C2Expression *child);
C2Expression *__fastcall negate_rewritten_operand(C2Expression *expression, C2Expression *context);

C2Expression *__fastcall prepare_expression_rewrite(C2Expression *expression)
{
    C2Expression *operand = expression->left, *next, *value, *definition, *conversion;
    unsigned short type;
    int negate_next = 0;
    expression->left = NULL;
    for (; operand; operand = next) {
        next = operand->next;
        operand->next = NULL;
        operand->location.expression.properties &= ~0x10;
        value = operand;
        if ((1U << (operand->kind & 31)) & 6) {
            definition = operand->value.reg->value;
            if (definition && definition->control_flags == 2 &&
                !mark_expression_execution(definition, operand))
                value = prepare_expression_rewrite(definition);
        }
        if (value != operand) {
            type = expression->opcode == 0x135 ? operand->type : expression->type;
            if (type != value->type && value->opcode != 0x151) {
                conversion = new_assignment_expression(0x135, operand->type,
                    new_temporary_operand(operand->type), NULL, NULL);
                attach_input_expression(conversion, conversion->left, value);
                conversion->control_flags = 2;
                value = conversion;
            }
            if (value != operand) release_small_node(operand);
        }
        if (expression->opcode == 0x141) {
            if (negate_next) {
                value = negate_rewritten_operand(value, NULL);
                negate_next = 0;
            }
        } else if (expression->opcode == 0x142 && (expression->type & 0xf000) != 0x4000) {
            expression->opcode = 0x141;
            negate_next = 1;
        }
        if (expression->opcode == value->opcode && expression_can_flatten(value)) {
            flatten_expression_inputs(expression, value);
            release_small_node(value);
        } else {
            copy_input_operand(expression, value);
        }
    }
    return expression;
}

C2Expression *__fastcall simplify_expression_operation(C2Expression *expression, C2Expression *context);
C2Expression *__fastcall fold_expression_constants(C2Expression *expression, C2Expression *context);
C2Expression *__fastcall rewrite_expression_operands(C2Expression *expression, C2Expression *context);
void __fastcall finish_expression_rewrite(C2Expression *expression);
C2Expression *__fastcall propagate_expression_conversion(C2Expression *expression, C2Expression *operand,
                                                        C2Expression *context);

C2Expression *__fastcall rewrite_expression_tree(C2Expression *expression, C2Expression *context, int mode)
{
    C2Expression *operand, *next, *result;
restart:
    for (operand = expression->left; operand; operand = next) {
        next = operand->next;
        if ((1U << (operand->kind & 31)) & 0x7fe) continue;
        operand = rewrite_expression_tree(operand, expression, mode);
        if (!mode || operand->opcode != 0x135) continue;
        result = propagate_expression_conversion(expression, operand, context);
        if (!result) continue;
        if (result == expression) goto restart;
        rewrite_expression_tree(expression, result, mode);
        return result;
    }
    result = simplify_expression_operation(expression, context);
    result = fold_expression_constants(result, context);
    if (mode || result->opcode == 0x151) {
        result = rewrite_expression_operands(result, context);
        finish_expression_rewrite(result);
    }
    return result;
}

void __fastcall remove_optimizer_symbol(C2RegisterSet *set, unsigned number)
{
    C2RegisterSetNode *node;
    unsigned group;

    group = number & ~31U;
    for (node = set->links.first; node && node->group < group; node = node->next) ;
    if (node && node->group == group) {
        node->bits &= ~(1U << (number & 31));
        if (!node->bits) remove_register_set_node(set, node);
    }
}
