#include "global-optimizer.h"
#include "global-expressions.h"
#include "optimizer-translate.h"
#include "inlining.h"
#include <recon.h>

/* Original DWORD owners used by the global expression and loop passes. */
int global_optimization_active;              /* 48e2b0 */
int global_aliases_changed;                  /* 48fa3c */
C2FunctionRegister *global_pending_registers; /* 49100c; -1-terminated chain. */
C2RegisterSet *global_expression_registers;   /* 48fa78 */
int global_optimization_phase;               /* 49101c */
int global_region_limited;                   /* 48fa7c */
int global_block_state;                      /* 48fa68 */
int global_motion_enabled;                   /* 48fa70 */
int global_region_motion_enabled;            /* 48fa80 */
int global_region_analysis_enabled;          /* 48fa84 */
/* Initialized image DWORD, compared against a signed block-number span. */
int global_region_size_limit = 100;          /* 491038 */
C2RegisterSet *global_motion_registers;       /* 49103c */
C2RegisterSet *global_motion_inputs;          /* 49102c */
C2RegisterSet *global_motion_outputs;         /* 491030 */

typedef char C2GlobalDefinitionsOffset[
    offsetof(C2BasicBlock, global_definitions) == 84 ? 1 : -1];
typedef char C2RegionRegistersOffset[
    offsetof(C2FlowRegion, global_registers) == 52 ? 1 : -1];
typedef char C2GlobalDefinitionOffset[
    offsetof(C2FunctionRegister, global_expression) == 24 ? 1 : -1];

void __fastcall optimize_global_flow(C2Function *function)
{
    C2BasicBlock *block, *next, *header, *last;
    C2FlowRegion *region;
    C2GlobalExpression *definition;
    C2Expression *operand;
    C2RegisterSet *selected, *active;
    C2FlowEdge *edge;
    unsigned number;
    int valid;

    initialize_global_optimizer();
    reset_input_pool(13);
    reset_input_pool(6);
    reset_input_pool(3);
    global_optimization_active = 1;
    initialize_global_registers(function);
    global_aliases_changed = 0;
    global_pending_registers = (C2FunctionRegister *)-1;
    global_expression_registers = new_register_set(13);
    global_optimization_phase = 0;
    prepare_global_blocks(function);
    if (global_aliases_changed) rewrite_global_aliases(function, 0);
    initialize_global_flow_analysis(function);
    build_global_flow_analysis(function);
    rewrite_global_function(function, 7);
    finish_global_function_rewrite(function);
    finish_global_flow_analysis(function);
    if (function->flow_regions) {
        prepare_region_register_sets(function);
        analyze_region_register_sets(function);
        solve_region_register_sets(function);
        prepare_region_register_sets(function);
    }
    prepare_global_expressions(function);
    prepare_global_assignments(function);
    global_optimization_phase = 1;
    global_region_limited = 0;
    global_block_state = 0;
    global_motion_enabled = 1;
    global_region_motion_enabled = 1;
    global_region_analysis_enabled = 1;
    for (block = function->flow->last; block; block = next) {
        region = block->block_scope;
        next = block->previous;
        if (region && region->header == block) {
            header = region->header;
            last = region->last;
            validate_global_region(region);
            if (!(region->flags & 1)) {
                if (!global_region_limited ||
                    global_region_size_limit <
                        (int)(short)region->last->number84 -
                        (int)(short)region->header->number84 + 1) {
                    selected = new_register_set(4);
                    active = region->global_registers;
                    for (number = next_optimizer_symbol(region->header->next->global_definitions);
                         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
                        definition = lookup_function_register(number)->global_expression;
                        if (definition->opcode == 0x131 ||
                            operation_is_comparison(definition->opcode)) {
                            valid = 1;
                            for (operand = definition->operands; operand;
                                 operand = operand->next) {
                                switch (operand->kind) {
                                case 1: case 2:
                                    if (!optimizer_set_contains(active, operand->value.reg->number))
                                        valid = 0;
                                    break;
                                case 3: case 4: case 5: case 7: case 8:
                                    break;
                                default:
                                    valid = 0;
                                    break;
                                }
                            }
                            if (valid) add_register_to_set(selected, number);
                        }
                    }
                    if (selected->links.first)
                        for (edge = region->header->incoming; edge;
                             edge = edge->next_incoming)
                            merge_optimizer_symbol_sets(edge->source->global_definitions, selected);
                    release_optimizer_symbol_set(selected);
                } else solve_global_register_sets(header, last);
                if (global_region_motion_enabled) prepare_region_motion(header, last);
            }
        }
        optimize_global_block(block, function);
    }
    global_block_state = 0;
    finish_global_block_pass(function);
    clear_global_block_marks(function);
    clear_global_register_marks(function);
    if (global_aliases_changed) rewrite_global_aliases(function, 1);
    rewrite_global_function(function, 5);
    finish_global_function_rewrite(function);
    if (function->flow_regions) {
        prepare_global_function_cleanup(function);
        prepare_region_register_sets(function);
        analyze_region_register_sets(function);
        solve_region_register_sets(function);
        prepare_region_register_sets(function);
        finish_region_register_sets(function);
    }
    prepare_global_assignments(function);
    solve_global_register_sets(function->flow->last, function->flow->first);
    global_optimization_phase = 2;
    global_region_limited = 0;
    global_region_motion_enabled = 0;
    global_region_analysis_enabled = 1;
    global_motion_enabled = 0;
    global_motion_registers = new_temporary_register_set();
    global_motion_inputs = new_temporary_register_set();
    global_motion_outputs = new_temporary_register_set();
    initialize_region_motion_sets();
    for (region = function->flow_regions; region; region = region->next)
        optimize_region_motion(region, function);
    release_region_motion_sets();
    global_optimization_phase = 3;
    finish_region_motion(function);
    solve_global_register_sets(function->flow->last, function->flow->first);
    global_region_limited = 1;
    global_region_motion_enabled = 0;
    global_region_analysis_enabled = 0;
    global_motion_enabled = 1;
    for (block = function->flow->last; block; block = next) {
        region = block->block_scope;
        next = block->previous;
        if (region && region->header == block) {
            header = region->header;
            last = region->last;
            validate_global_region(region);
            if (!(region->flags & 1)) {
                if (!global_region_limited ||
                    global_region_size_limit <
                        (int)(short)region->last->number84 -
                        (int)(short)region->header->number84 + 1) {
                    selected = new_register_set(4);
                    active = region->global_registers;
                    for (number = next_optimizer_symbol(region->header->next->global_definitions);
                         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
                        definition = lookup_function_register(number)->global_expression;
                        if (definition->opcode == 0x131 ||
                            operation_is_comparison(definition->opcode)) {
                            valid = 1;
                            for (operand = definition->operands; operand;
                                 operand = operand->next) {
                                switch (operand->kind) {
                                case 1: case 2:
                                    if (!optimizer_set_contains(active, operand->value.reg->number))
                                        valid = 0;
                                    break;
                                case 3: case 4: case 5: case 7: case 8:
                                    break;
                                default:
                                    valid = 0;
                                    break;
                                }
                            }
                            if (valid) add_register_to_set(selected, number);
                        }
                    }
                    if (selected->links.first)
                        for (edge = region->header->incoming; edge;
                             edge = edge->next_incoming)
                            merge_optimizer_symbol_sets(edge->source->global_definitions, selected);
                    release_optimizer_symbol_set(selected);
                } else solve_global_register_sets(header, last);
                if (global_region_motion_enabled) prepare_region_motion(header, last);
            }
        }
        optimize_global_block(block, function);
        region = block->block_scope;
        if (region && region->last == block) finish_global_region(region);
    }
    if (global_aliases_changed) rewrite_global_aliases(function, 0);
    clear_global_block_marks(function);
    clear_global_register_marks(function);
    rewrite_global_function(function, 1);
    finish_global_function_rewrite(function);
    if (global_aliases_changed) {
        rewrite_global_aliases(function, 0);
        rewrite_global_function(function, 1);
    }
    prepare_global_function_cleanup(function);
    restore_global_function(function);
    finish_global_function(function);
    reset_input_pool(13);
    reset_input_pool(6);
    reset_input_pool(3);
    global_optimization_active = 0;
    release_global_optimizer();
}

/* Hash-chain payload is deliberately incomplete until its actual consumer.
 * This initializer establishes all101 pointer cells; no node layout is implied. */
C2GlobalExpression *global_expression_buckets[101]; /* 48f8a0 */
unsigned global_expression_count;                   /* 48fa9c */
unsigned global_expression_cursor;                  /* 48fa98 */
C2FunctionRegister *global_cached_expression_registers;            /* 48fa94 */
C2RegisterSet **global_index_registers;              /* 48fa6c */
C2RegisterSet **global_symbol_registers;             /* 48fa88 */
C2RegisterSet **global_index_direct_uses;            /* 48fb14 */
C2RegisterSet **global_index_variant_uses;           /* 48fa50 */
C2RegisterSet *global_pending_expressions;           /* 48fa74 */
extern unsigned optimizer_final_index_limit;
extern C2RegisterSet *optimizer_symbol_indices, *optimizer_value_indices;

/* Scratch sets are assigned on each nonempty region analysis. No pre-analysis
 * contents or defaults are consumed by this initializer. */
C2RegisterSet *region_register_domain;       /* 497414 */
C2RegisterSet *region_defined_registers;     /* 497418 */
C2RegisterSet *region_examined_registers;    /* 49741c */
C2RegisterSet *__fastcall new_region_register_domain(unsigned pool)
{
    C2RegisterSet *set = new_register_set(pool);
    merge_optimizer_symbol_sets(set, tracked_register_set);
    merge_optimizer_symbol_sets(set, secondary_register_set);
    return set;
}
void __fastcall clear_region_block_flags(C2Function *function)
{
    C2BasicBlock *block;
    for (block = function->flow->last; block; block = block->previous)
        block->flags &= ~1U;
}
void __fastcall analyze_single_region_registers(C2FlowRegion *region)
{
    C2FlowRegion *child;
    C2BasicBlock *block, *first, *end;
    C2Expression *record, *operand, *base, *use_operand;
    C2FunctionRegister *reg, *variant;
    C2OptimizerUse *use, *current, *saved;
    C2RegisterSet *set;
    unsigned number, block_flags;
    int invariant;

    for (child = region->children; child; child = child->next)
        analyze_single_region_registers(child);
    first = region->header;
    end = region->last->previous;
    for (block = first; block != end; block = block->previous) {
        if (block->flags & 1) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->tail.right; operand; operand = operand->next) {
                switch (operand->kind) {
                case 1: case 2:
                    reg = operand->value.reg;
                    if (reg->value) break;
                    for (variant = reg->canonical; variant; variant = variant->variant_next)
                        if (variant == reg ||
                            (variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                             reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes)))
                            add_register_to_set(region_defined_registers, variant->number);
                    break;
                case 6: case 10:
                    number = operand->kind == 6 ? operand->immediate : operand->value.optimizer_index;
                    if (test_and_add_global_register(region_examined_registers, number)) break;
                    use = get_global_index_uses(number);
                    saved = NULL;
                    while (use) {
                        current = use;
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            current = reg->optimizer_uses;
                            if (!current) {
                                use = use->next;
                                continue;
                            }
                            reg = current->reg;
                        }
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            if (variant == reg ||
                                (variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                                 reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes)))
                                add_register_to_set(region_defined_registers, variant->number);
                        use = current->next;
                        if (!use && saved) {
                            use = saved->next;
                            saved = NULL;
                        }
                    }
                    break;
                }
            }
        }
    }
    set = new_register_set(12);
    region->global_registers = set;
    copy_optimizer_symbol_set_into(set, region_register_domain);
    subtract_optimizer_symbol_sets(set, region_defined_registers);
    for (block = first; block != end; block = block->previous) {
        if (block->flags & 1) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->left; operand; operand = operand->next) {
                operand->location.expression.comparison &= 0x7f;
                invariant = 0;
                switch (operand->kind) {
                case 1: case 2:
                    reg = operand->value.reg;
                    if (!reg->value && !optimizer_set_contains(region_defined_registers, reg->number))
                        invariant = 1;
                    break;
                case 3: case 4: case 7: case 8:
                    invariant = 1;
                    break;
                case 6:
                    use = get_global_index_uses(operand->immediate);
                    saved = NULL;
                    while (use) {
                        current = use;
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            current = reg->optimizer_uses;
                            if (!current) {
                                use = use->next;
                                continue;
                            }
                            reg = current->reg;
                        }
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            if (optimizer_set_contains(region_defined_registers, variant->number))
                                goto next_operand;
                        use = current->next;
                        if (!use && saved) {
                            use = saved->next;
                            saved = NULL;
                        }
                    }
                    /* The indexed address still has its actual base register. */
                case 5:
                    if (operand->opcode != 0x122) invariant = 1;
                    else {
                        base = operand->aggregate_value;
                        if (!(((1U << (base->kind & 31)) & 6) && base->value.reg->value) &&
                            !optimizer_set_contains(region_defined_registers, base->value.reg->number))
                            invariant = 1;
                    }
                    break;
                }
                if (invariant) operand->location.expression.comparison |= 0x80;
next_operand: ;
            }
        }
    }
    for (block = first; block != end; block = block->previous) {
        block_flags = block->flags;
        if (block_flags & 1) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            operand = record->tail.right;
            if (!operand || !((1U << (operand->kind & 31)) & 6) ||
                !operand->value.reg->value || (operand->location.expression.comparison & 0x80)) continue;
            for (base = record->left; base; base = base->next)
                if (!(base->location.expression.comparison & 0x80)) break;
            if (base) continue;
            operand->location.expression.comparison |= 0x80;
            follow_optimizer_result(record, &use_operand);
            use_operand->location.expression.comparison |= 0x80;
        }
        block->flags = block_flags | 1;
    }
}
void __fastcall detach_input_operand(C2Expression *owner, C2Expression *operand);
void __fastcall solve_region_register_sets(C2Function *function)
{
    C2Expression *record, *next, *end, *result, *consumer, *use;
    C2Expression *first, *second, *increment, *selected, *scan, *output;
    if (!function->flow_regions) return;
    end = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != end; record = next) {
        next = record->next;
        if (record->opcode != 0x141) continue;
        result = record->tail.right;
        if (!((1U << (result->kind & 31)) & 6) || !result->value.reg->value) continue;
        consumer = follow_optimizer_result(record, &use);
        if (consumer->opcode != 0x141) continue;
        if (compare_input_operands(result, consumer->left)) swap_global_operands(consumer);
        if (compare_input_operands(result, consumer->left)) continue;
        first = record->left;
        second = first->next;
        increment = consumer->left->next;
        if (!(increment->location.expression.comparison & 0x80)) continue;
        if (first->location.expression.comparison & 0x80) {
            if ((second->location.expression.comparison & 0x80) || second->kind != 2) continue;
            selected = second;
        } else {
            if (!(second->location.expression.comparison & 0x80) || first->kind != 2) continue;
            selected = first;
        }
        detach_input_operand(record, selected);
        detach_input_operand(consumer, increment);
        copy_input_operand(record, increment);
        copy_input_operand(consumer, selected);
        record->tail.right->location.expression.comparison |= 0x80;
        consumer->left->location.expression.comparison |= 0x80;
        next = record->next;
        for (scan = next; scan != consumer; scan = scan->next) {
            output = scan->tail.right;
            if (output && (!((1U << (output->kind & 31)) & 6) ||
                           !output->value.reg->value)) break;
        }
        move_input_record_before((C2Record *)scan, (C2Record *)record);
    }
}

void __fastcall analyze_region_register_sets(C2Function *function)
{
    C2FlowRegion *region;
    if (!function->flow_regions) return;
    region_register_domain = new_region_register_domain(4);
    region_defined_registers = new_register_set(4);
    region_examined_registers = new_register_set(4);
    clear_region_block_flags(function);
    for (region = function->flow_regions; region; region = region->next) {
        analyze_single_region_registers(region);
        clear_register_set(region_defined_registers);
        clear_register_set(region_examined_registers);
    }
    reset_input_pool(4);
}

C2FlowEdge *__fastcall find_flow_edge(C2BasicBlock *source, C2BasicBlock *destination);

/* Assigned from the real region entry's definitions immediately before the
 * motion children consume it; no earlier set contents/default is inferred. */
C2RegisterSet *global_region_definitions; /* 49742c */
extern int global_available_additions_suppressed;
void __fastcall coalesce_region_assignments(C2FlowRegion *region);
void __fastcall prepare_region_motion_assignments(C2FlowRegion *region, int mode);
void __fastcall build_region_motion_sets(C2FlowRegion *region);
void __fastcall set_region_motion_anchor(C2FlowRegion *region);
void __fastcall move_global_region_block(C2BasicBlock *block, C2FlowRegion *region,
    C2Function *function);
void __fastcall finish_region_motion_candidates(C2FlowRegion *region, C2Function *function);

extern int global_rewrite_state;
void __fastcall prepare_region_motion_assignments(C2FlowRegion *region, int mode)
{
    if (mode & 2) global_rewrite_state = 1;
    clear_global_rewrite_marks(region->rewrite_entry, region->last);
    propagate_global_rewrites((mode & 1) ? region->rewrite_entry : region->header, region->last);
    global_rewrite_state = 0;
}

/* Both sets are assigned from the actual entry predecessor after the region
 * solver; no contents or initial object extent are inferred here. */
C2RegisterSet *region_entry_uses;      /* 497428, predecessor+48 */
C2RegisterSet *region_entry_alias_out; /* 497424, predecessor+60 */
void __fastcall solve_region_motion_availability(C2FlowRegion *region);
void __fastcall collect_region_motion_definitions(C2FlowRegion *region);

void __fastcall solve_region_motion_block(C2BasicBlock *block, int entry, int *changed);

extern C2RegisterSet *global_temporary_registers;
void __fastcall solve_region_motion_block(C2BasicBlock *block, int entry, int *changed)
{
    C2FlowEdge *edge;
    C2FlowRegion *region;
    C2RegisterSet *input;
    edge = block->incoming;
    input = block->rewrite_mode4[5];
    if (edge) {
        if (!entry) {
            copy_optimizer_symbol_set_into(input, edge->source->rewrite_mode4[6]);
            for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
                intersect_optimizer_symbol_sets(input, edge->source->rewrite_mode4[6]);
        } else {
            region = block->block_scope;
            if (edge->source->block_scope != region) edge = edge->next_incoming;
            /* The original assumes another incoming edge exists after skipping
             * the outside-region edge; it reads that edge without a NULL test. */
            copy_optimizer_symbol_set_into(input, edge->source->rewrite_mode4[6]);
            for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
                if (edge->source->block_scope == region)
                    intersect_optimizer_symbol_sets(input, edge->source->rewrite_mode4[6]);
        }
    }
    update_available_register_transfer(block->rewrite_mode4[6], global_temporary_registers,
        input, block->rewrite_mode4[1], changed);
}

/* Exact insertion cursor, assigned before the region block walker. */
C2Record *region_motion_anchor; /* 497420 */
void __fastcall set_region_motion_anchor(C2FlowRegion *region)
{
    region_motion_anchor = region->rewrite_entry->last_record;
}

extern C2Expression *global_next_record;
extern unsigned diagnostic_position;
int __fastcall block_reaches_region_exits(C2BasicBlock *block, C2FlowRegion *region);
int __fastcall region_register_is_live(C2FunctionRegister *reg, C2RegisterSet *uses,
    C2RegisterSet *aliases);

int __fastcall optimizer_dense_contains(C2RegisterSet *set, unsigned number);
int __fastcall region_register_is_live(C2FunctionRegister *reg, C2RegisterSet *uses,
    C2RegisterSet *aliases)
{
    C2FunctionRegister *canonical, *variant;
    unsigned uncovered, required;
    long first, last, unit, count;

    if (optimizer_set_contains(uses, reg->number)) return 1;
    canonical = reg->canonical;
    for (variant = canonical; variant; variant = variant->variant_next) {
        if (variant == reg) continue;
        if (variant->displacement < (long)(reg->value_bytes + (unsigned)reg->displacement) &&
            reg->displacement < (long)(variant->value_bytes + (unsigned)variant->displacement) &&
            optimizer_set_contains(uses, variant->number)) break;
    }
    if (!variant) return 0;
    if ((long)canonical->value_bytes > 0x100) return 1;
    uncovered = ~0U;
    for (variant = canonical; variant; variant = variant->variant_next)
        if (optimizer_set_contains(aliases, variant->number)) {
            first = variant->displacement / 8;
            last = (long)(variant->value_bytes + (unsigned)variant->displacement - 1) / 8;
            for (unit = first; unit <= last; ++unit)
                uncovered &= ~(1U << (unit & 31));
        }
    count = (long)(canonical->value_bytes + 7) / 8;
    /* Keep the original x86 shift-count wrap, including its zero mask at32. */
    required = (1U << (count & 31)) - 1;
    return (uncovered & required) != 0;
}

int __fastcall block_reaches_region_exits(C2BasicBlock *block, C2FlowRegion *region)
{
    C2FlowBlockLink *exit;
    for (exit = region->exits; exit; exit = exit->next)
        if (!optimizer_dense_contains(exit->block->dominators, (int)(short)block->number84))
            return 0;
    return 1;
}

void __fastcall move_global_region_block(C2BasicBlock *block, C2FlowRegion *region,
    C2Function *function)
{
    C2FlowEdge *edge;
    C2Expression *record, *end, *output, *next, *input, *moved, *definition;
    C2FunctionRegister *reg;
    C2RegisterSet *input_set;
    unsigned number;
    int movable, saw_operation;

    input_set = block->rewrite_mode4[5];
    if (block == region->header) {
        for (edge = block->incoming; edge; edge = edge->next_incoming)
            if (edge->source->block_scope == block->block_scope) {
                copy_optimizer_symbol_set_into(input_set, edge->source->rewrite_mode4[6]);
                break;
            }
        for (edge = block->incoming; edge; edge = edge->next_incoming)
            if (edge->source->block_scope == block->block_scope)
                intersect_optimizer_symbol_sets(input_set, edge->source->rewrite_mode4[6]);
    } else {
        edge = block->incoming;
        if (edge) {
            copy_optimizer_symbol_set_into(input_set, edge->source->rewrite_mode4[6]);
            for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
                intersect_optimizer_symbol_sets(input_set, edge->source->rewrite_mode4[6]);
        }
    }
    global_available_registers = block->rewrite_mode4[6];
    copy_optimizer_symbol_set_into(global_available_registers, input_set);
    movable = block_reaches_region_exits(block, region);
    end = (C2Expression *)block->last_record;
    saw_operation = 0;
    for (record = (C2Expression *)block->records->next; record != end; record = global_next_record) {
        global_next_record = record->next;
        if (record->location.line) {
            diagnostic_position = function->source_line_base + record->location.line;
            record_line_delta = record->location.line;
        }
        if (!(record->flags & 1)) continue;
        if (record->kind == 11) saw_operation = 1;
        if (!movable) goto update_record;
        output = record->tail.right;
        if (record->opcode == 0x131) {
            if (((1U << (output->kind & 31)) & 6) && output->value.reg->value)
                goto update_record;
            reg = record->value.reg;
            if (!((1U << (output->kind & 31)) & 6) ||
                region_register_is_live(output->value.reg, region_entry_uses,
                    region_entry_alias_out)) goto update_record;
        } else {
            if (!output || !((1U << (output->kind & 31)) & 6)) goto update_record;
            reg = output->value.reg;
            if (!reg->global_expression) goto update_record;
        }
        number = reg->number;
        if (!optimizer_set_contains(global_available_registers, number) ||
            input_has_side_effects(record)) goto update_record;
        if (record->opcode == 0x141 || record->opcode == 0x142) {
            next = record->next;
            if (next->opcode == 0x131 && !compare_input_operands(output, next->left)) {
                if (!compare_input_operands(next->tail.right, record->left) ||
                    !compare_input_operands(next->tail.right, record->left->next))
                    goto update_record;
            }
        }
        if (record->kind == 17 || record->kind == 15)
            input = reg->global_expression->operands;
        else input = record->left;
        for (; input; input = input->next)
            if (((1U << (input->kind & 31)) & 6) && input->value.reg->global_expression &&
                !optimizer_set_contains(global_region_definitions, input->value.reg->number))
                goto update_record;
        moved = record;
        if (record->precision_flags == 3) moved = copy_input_expression(record);
        else unlink_input_record((C2Record *)record);
        if (moved->kind == 17) {
            for (input = moved->left; input; input = input->next)
                if (((1U << (input->kind & 31)) & 6) && input->value.reg->value) {
                    definition = input->value.reg->value;
                    unlink_input_record((C2Record *)definition);
                    prepend_input_record(region_motion_anchor, (C2Record *)definition);
                }
        } else if (moved->kind == 15) {
            for (input = moved->left; input; input = input->next)
                if (((1U << (input->kind & 31)) & 6) && input->value.reg->value) {
                    definition = input->value.reg->value;
                    unlink_input_record((C2Record *)definition);
                    prepend_input_record(region_motion_anchor, (C2Record *)definition);
                }
        }
        prepend_input_record(region_motion_anchor, (C2Record *)moved);
        mark_global_register_available(reg);
        add_register_to_set(region->header->global_available_in, number);
        add_register_to_set(global_region_definitions, number);
        add_register_to_set(global_motion_outputs, number);
        if (moved->opcode == 0x131 && ((1U << (output->kind & 31)) & 6) &&
            output->value.reg->global_expression) {
            number = moved->tail.right->value.reg->number;
            add_register_to_set(region->header->global_available_in, number);
            add_register_to_set(global_region_definitions, number);
            add_register_to_set(global_motion_outputs, number);
        }
        continue;
update_record:
        update_global_available_registers(record);
    }
    if (saw_operation && !find_next_global_operation((C2Expression *)block->records))
        global_aliases_changed = 1;
}

void __fastcall collect_region_motion_definitions(C2FlowRegion *region)
{
    C2BasicBlock *block;
    clear_register_set(global_motion_registers);
    for (block = region->header; block != region->last->previous; block = block->previous)
        merge_optimizer_symbol_sets(global_motion_registers, block->rewrite_mode4[2]);
}

void __fastcall solve_region_motion_availability(C2FlowRegion *region)
{
    C2BasicBlock *first = region->header, *last = region->last, *block, *end;
    int changed;
    copy_optimizer_symbol_set_into(last->rewrite_mode4[6], last->global_definitions);
    do {
        changed = 0;
        copy_optimizer_symbol_set_into(first->rewrite_mode4[5], last->rewrite_mode4[6]);
        end = last->previous;
        for (block = first; block != end; block = block->previous)
            solve_region_motion_block(block, block == first, &changed);
    } while (changed);
}

void __fastcall build_region_motion_sets(C2FlowRegion *region)
{
    C2FlowBlockLink *exit;
    C2FlowEdge *edge;
    C2BasicBlock *destination;
    solve_region_motion_availability(region);
    region_entry_uses = region->header->next->rewrite_mode1[3];
    region_entry_alias_out = region->header->next->alias_out;
    clear_register_set(global_motion_inputs);
    for (exit = region->exits; exit; exit = exit->next)
        for (edge = exit->block->outgoing; edge; edge = edge->next_outgoing) {
            destination = edge->destination;
            if (destination->block_scope != region)
                merge_optimizer_symbol_sets(global_motion_inputs, destination->rewrite_mode1[2]);
        }
    collect_region_motion_definitions(region);
    clear_register_set(global_motion_outputs);
}

void __fastcall coalesce_region_assignments(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *output, *input, *next;
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (record->opcode != 0x131) continue;
            output = record->tail.right;
            if (!((1U << (output->kind & 31)) & 6) || output->value.reg->kind != 3) continue;
            input = record->left;
            if (!((1U << (input->kind & 31)) & 6) || input->value.reg->value) continue;
            next = record->next;
            if (next->opcode != 0x131 || compare_input_operands(output, next->left)) continue;
            if (input_has_side_effects(record->left) || input_has_side_effects(next->left)) continue;
            attach_input_expression(next, next->left, record->left);
            finish_global_assignment(next, 1);
        }
}

extern int global_substitution_check_enabled;
void __fastcall region_candidate_pass_3caf9(C2FlowRegion *region);
void __fastcall region_candidate_pass_3ccb2(C2FlowRegion *region);
void __fastcall region_candidate_pass_3d127(C2FlowRegion *region);
unsigned __fastcall region_candidate_pass_3d37b(C2FlowRegion *region);
void __fastcall region_candidate_pass_3ba96(C2FlowRegion *region);
int __fastcall region_candidate_pass_3d4a4(C2FlowRegion *region, C2Function *function);
unsigned __fastcall region_candidate_pass_3d93c(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3de8b(C2FlowRegion *region, C2Function *function, unsigned changes);
void __fastcall region_candidate_pass_3c245(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3c33f(C2FlowRegion *region);
void __fastcall region_candidate_pass_3c3a0(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3c4a5(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3c87c(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3bbb3(C2FlowRegion *region);
void __fastcall region_candidate_pass_3e5ed(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3e73e(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3e91d(C2FlowRegion *region);
void __fastcall region_candidate_pass_3e9fe(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3ea57(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3ec06(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3ec67(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3edd3(C2FlowRegion *region);

/* Candidate preparation clears its three real region DWORDs at the end.
 * Their later list/count meanings remain with their reached consumers. */
unsigned region_candidate_state; /* 497434 */
void __fastcall remove_input_operand(C2Expression *owner, C2Expression *operand);
C2Expression *__fastcall new_region_binary_expression(unsigned opcode,
    unsigned short type, C2Expression *left, C2Expression *right,
    C2InputLink link, C2Record *prior);
int __fastcall region_operand_is_invariant(C2Expression *operand);

int __fastcall region_operand_is_invariant(C2Expression *operand)
{
    C2FunctionRegister *reg;
    if ((1U << (operand->kind & 31)) & 0x180) return 1;
    if ((1U << (operand->kind & 31)) & 0x18) return 1;
    if (operand->kind != 2) return 0;
    reg = operand->value.reg;
    if (reg->rewrite_flags & 2) return 1;
    if (reg->value || optimizer_set_contains(global_motion_registers, reg->number)) return 0;
    if (reg->kind != 3) return 1;
    return optimizer_set_contains(global_region_definitions, reg->number) != 0;
}

void __fastcall region_candidate_pass_3caf9(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *next, *constant, *operation;
    reset_input_pool(3);
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = next) {
            next = record->next;
            record->flags &= ~0x40;
            if (record->precision_flags == 7 || record->precision_flags == 8)
                record->precision_flags = 0;
            if (record->opcode == 0x137 && (record->type & 0xf000) == 0x1000 &&
                !input_has_side_effects(record)) {
                constant = new_integer_expression(record->type, ~(unsigned __int64)0);
                operation = new_region_binary_expression(0x143, record->type, record->left,
                    constant, prepend_input_record, (C2Record *)record);
                attach_input_expression(record, record->left, operation->tail.right);
                record->opcode = 0x131;
                finish_global_assignment(record, 1);
            }
            if (record->opcode == 0x142 && (record->type & 0xf000) == 0x1000 &&
                region_operand_is_invariant(record->left) &&
                !region_operand_is_invariant(record->left->next) &&
                !input_has_side_effects(record)) {
                constant = new_integer_expression(record->type, ~(unsigned __int64)0);
                operation = new_region_binary_expression(0x143, record->type, record->left->next,
                    constant, prepend_input_record, (C2Record *)record);
                operation = new_region_binary_expression(0x141, record->type, operation->tail.right,
                    record->left, prepend_input_record, (C2Record *)record);
                attach_input_expression(record, record->left, operation->tail.right);
                remove_input_operand(record, record->left->next);
                record->opcode = 0x131;
                finish_global_assignment(record, 1);
            }
        }
    region->pending28_word = 0;
    region->pending2c_word = 0;
    region->allocation_next = NULL;
    region_candidate_state = 0;
}

extern C2RegisterSet *region_motion_sets[11];
int __fastcall global_operand_is_restricted(C2Expression *operand)
{
    if (!((1U << (operand->kind & 31)) & 6)) return 0;
    return optimizer_set_contains(region_motion_sets[9], operand->value.reg->number) != 0;
}

void __fastcall region_candidate_pass_3ccb2(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *operand;
    C2OptimizerUse *use, *saved, *current;
    C2FunctionRegister *reg, *canonical, *variant, *other;
    unsigned number;
    int wide;

    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind == 6 || operand->kind == 10) {
                    use = get_global_index_uses(operand->kind == 6 ? operand->immediate :
                        operand->value.optimizer_index);
                    saved = NULL;
                    while (use) {
                        current = use;
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            current = reg->optimizer_uses;
                            if (!current) { use = use->next; continue; }
                            reg = current->reg;
                        }
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            add_register_to_set(region_motion_sets[4], variant->number);
                        use = current->next;
                        if (!use && saved) { use = saved->next; saved = NULL; }
                    }
                } else if (((1U << (operand->kind & 31)) & 6) &&
                    operand->value.reg->canonical->symbol) {
                    add_register_to_set(region_motion_sets[8], operand->value.reg->number);
                    if (input_has_side_effects(operand))
                        add_register_to_set(region_motion_sets[4], operand->value.reg->number);
                }
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if ((1U << (operand->kind & 31)) & 6) {
                    reg = operand->value.reg;
                    if (!reg->value) {
                        number = reg->number;
                        if (input_has_side_effects(operand))
                            add_register_to_set(region_motion_sets[4], number);
                        if (block->block_scope != region)
                            add_register_to_set(region_motion_sets[4], number);
                    }
                } else if (operand->kind == 6 || operand->kind == 10) {
                    use = get_global_index_uses(operand->kind == 6 ? operand->immediate :
                        operand->value.optimizer_index);
                    saved = NULL;
                    while (use) {
                        current = use;
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            current = reg->optimizer_uses;
                            if (!current) { use = use->next; continue; }
                            reg = current->reg;
                        }
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            add_register_to_set(region_motion_sets[4], variant->number);
                        use = current->next;
                        if (!use && saved) { use = saved->next; saved = NULL; }
                    }
                }
            }
            if (record->opcode == 0x131 && (record->type & 0xf000) == 0x5000 &&
                record->tail.right->kind == 2) reg = record->tail.right->value.reg;
            else if (record->opcode == 0x13e && record->left->kind == 2)
                reg = record->left->value.reg;
            else reg = NULL;
            if (reg)
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg ||
                        (variant->displacement < (long)(reg->value_bytes + (unsigned)reg->displacement) &&
                         reg->displacement < (long)(variant->value_bytes + (unsigned)variant->displacement))) {
                        add_register_to_set(region_motion_sets[4], variant->number);
                        add_register_to_set(region_motion_sets[2], variant->number);
                    }
        }
    for (number = next_optimizer_symbol(region_motion_sets[8]); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        canonical = reg->canonical;
        wide = canonical->type == 0x1008 || canonical->type == 0x2008;
        for (variant = canonical; variant; variant = variant->variant_next) {
            if (wide) {
                add_register_to_set(region_motion_sets[4], number);
                add_register_to_set(region_motion_sets[4], variant->number);
                continue;
            }
            if (reg != variant &&
                ((long)(variant->value_bytes + (unsigned)variant->displacement) <= reg->displacement ||
                 (long)(reg->value_bytes + (unsigned)reg->displacement) <= variant->displacement)) continue;
            if (reg->displacement == variant->displacement) continue;
            add_register_to_set(region_motion_sets[4], number);
            add_register_to_set(region_motion_sets[2], number);
            if (!global_type_has_register_class(variant->type)) continue;
            add_register_to_set(region_motion_sets[4], variant->number);
            add_register_to_set(region_motion_sets[2], variant->number);
            for (other = canonical; other; other = other->variant_next)
                if ((other == variant ||
                    (variant->displacement < (long)(other->value_bytes + (unsigned)other->displacement) &&
                     other->displacement < (long)(variant->value_bytes + (unsigned)variant->displacement))) &&
                    global_type_has_register_class(other->type)) {
                    add_register_to_set(region_motion_sets[4], other->number);
                    add_register_to_set(region_motion_sets[2], other->number);
                }
        }
    }
}

int __fastcall global_operand_is_restricted(C2Expression *operand);
unsigned __fastcall test_and_remove_region_register(C2RegisterSet *set, unsigned number);

void __fastcall region_candidate_pass_3d127(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *output;
    unsigned opcode, number;
    int changed, left_candidate, left_invariant, right_candidate, right_invariant, remove;

    copy_optimizer_symbol_set_into(region_motion_sets[10], region_motion_sets[9]);
    for (block = region->header; block != region->last->previous; block = block->previous) {
        if (block->block_scope != region) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            opcode = record->opcode;
            if (opcode == 0x131) {
                if (record->precision_flags == 1 || record->precision_flags == 14) continue;
            } else if (opcode != 0x141 && opcode != 0x142) continue;
            if (((1U << (record->type >> 12)) & 0xe) && record->tail.right->kind == 2)
                add_register_to_set(region_motion_sets[9], record->tail.right->value.reg->number);
        }
    }
    subtract_optimizer_symbol_sets(region_motion_sets[9], region_motion_sets[4]);
    do {
        changed = 0;
        for (block = region->header; block != region->last->previous; block = block->previous)
            for (record = (C2Expression *)block->records->next;
                 record != (C2Expression *)block->last_record; record = record->next) {
                opcode = record->opcode;
                remove = 0;
                if (opcode == 0x131) {
                    if (global_operand_is_restricted(record->tail.right) &&
                        !global_operand_is_restricted(record->left)) remove = 1;
                } else if (opcode == 0x141 || opcode == 0x142) {
                    if (global_operand_is_restricted(record->tail.right)) {
                        left_candidate = global_operand_is_restricted(record->left);
                        left_invariant = region_operand_is_invariant(record->left);
                        right_candidate = global_operand_is_restricted(record->left->next);
                        right_invariant = region_operand_is_invariant(record->left->next);
                        if ((!left_candidate && !left_invariant) ||
                            (!right_candidate && !right_invariant) ||
                            (left_invariant && right_invariant) ||
                            (left_candidate && right_candidate) ||
                            (opcode == 0x142 && !left_candidate)) remove = 1;
                    }
                } else if (record->flags & 1) {
                    output = record->tail.right;
                    if (output && output->kind == 2 && !output->value.reg->value &&
                        test_and_remove_region_register(region_motion_sets[9], output->value.reg->number))
                        changed = 1;
                }
                if (remove) {
                    number = record->tail.right->value.reg->number;
                    if (!optimizer_set_contains(region_motion_sets[10], number)) {
                        remove_optimizer_symbol(region_motion_sets[9], number);
                        changed = 1;
                    }
                }
            }
    } while (changed);
}

unsigned __fastcall replace_region_register_variant(C2Expression *operand, C2Expression *owner);
unsigned __fastcall replace_region_register_variant(C2Expression *operand, C2Expression *owner)
{
    C2FunctionRegister *reg = operand->value.reg, *variant;
    C2Expression *copy, *conversion;
    for (variant = reg->canonical; variant; variant = variant->variant_next) {
        if (variant->displacement > reg->displacement ||
            (long)(variant->value_bytes + (unsigned)variant->displacement) <
            (long)(reg->value_bytes + (unsigned)reg->displacement)) continue;
        if (!optimizer_set_contains(region_motion_sets[9], variant->number) || variant == reg) continue;
        copy = copy_input_expression(operand);
        retarget_global_operand(copy, variant->type);
        conversion = new_linked_global_expression(0x135, operand->type, copy,
            prepend_input_record, (C2Record *)owner);
        operand->value.reg = conversion->tail.right->value.reg;
        return 1;
    }
    return 0;
}

unsigned __fastcall region_candidate_pass_3d37b(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *operand;
    unsigned changed, record_changed;
    changed = 0;
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            record_changed = 0;
            for (operand = record->left; operand; operand = operand->next)
                if (operand->kind == 2 && operand->value.reg && operand->value.reg->canonical->symbol)
                    record_changed |= replace_region_register_variant(operand, record);
            if (record_changed) {
                update_global_record_registers(record, 0);
                changed = 1;
            }
        }
    return changed;
}

void __fastcall add_region_register_use(C2FunctionRegister *reg, C2Expression *record);
void __fastcall add_region_register_definition(C2FunctionRegister *reg, C2Expression *record);

void __fastcall add_region_register_use(C2FunctionRegister *reg, C2Expression *record)
{
    C2OptimizerRecordLink *previous = reg->global_uses, *link;
    link = (C2OptimizerRecordLink *)c2_pool_allocate(3, 8);
    link->record = record;
    link->next = previous;
    reg->global_uses = link;
}

void __fastcall add_region_register_definition(C2FunctionRegister *reg, C2Expression *record)
{
    C2OptimizerRecordLink *previous = reg->global_definitions, *link;
    link = (C2OptimizerRecordLink *)c2_pool_allocate(3, 8);
    link->record = record;
    link->next = previous;
    reg->global_definitions = link;
}

void __fastcall region_candidate_pass_3ba96(C2FlowRegion *region)
{
    C2FunctionRegister *reg;
    C2BasicBlock *block;
    C2Expression *record, *operand;
    unsigned number;
    if (!region_motion_sets[9]->links.first) return;
    for (number = next_optimizer_symbol(region_motion_sets[9]); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        reg->global_uses = NULL;
        reg->global_definitions = NULL;
    }
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1) || record->precision_flags == 2 || record->precision_flags == 1)
                continue;
            for (operand = record->left; operand; operand = operand->next)
                if (global_operand_is_restricted(operand)) add_region_register_use(operand->value.reg, record);
            for (operand = record->tail.right; operand; operand = operand->next)
                if (global_operand_is_restricted(operand))
                    add_region_register_definition(operand->value.reg, record);
        }
}

void __fastcall finish_region_motion_candidates(C2FlowRegion *region, C2Function *function)
{
    unsigned changes;
    if (region->entry->flags & 0x2000000) return;
    region_candidate_pass_3caf9(region);
    region_candidate_pass_3ccb2(region);
    region_candidate_pass_3d127(region);
    changes = region_candidate_pass_3d37b(region);
    region_candidate_pass_3ba96(region);
    if (region_candidate_pass_3d4a4(region, function)) {
        global_substitution_check_enabled = 1;
        changes |= region_candidate_pass_3d93c(region, function);
        region_candidate_pass_3de8b(region, function, changes);
        region_candidate_pass_3c245(region, function);
        region_candidate_pass_3c33f(region);
        region_candidate_pass_3c3a0(region, function);
        region_candidate_pass_3c4a5(region, function);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3d93c(region, function);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3c87c(region, function);
        region_candidate_pass_3c3a0(region, function);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3bbb3(region);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3e5ed(region, function);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3e73e(region, function);
        region_candidate_pass_3e91d(region);
        region_candidate_pass_3e9fe(region, function);
        region_candidate_pass_3ea57(region, function);
        region_candidate_pass_3ec06(region, function);
        region_candidate_pass_3ec67(region, function);
        global_substitution_check_enabled = 0;
    }
    region_candidate_pass_3edd3(region);
}

void __fastcall optimize_region_motion(C2FlowRegion *region, C2Function *function)
{
    C2FlowRegion *child;
    C2BasicBlock *block;
    for (child = region->children; child; child = child->next)
        optimize_region_motion(child, function);
    validate_global_region(region);
    if (region->flags & 9) return;
    if ((int)(short)region->last->number84 - (int)(short)region->header->number84 + 1 <=
        global_region_size_limit) {
        coalesce_region_assignments(region);
        prepare_region_motion_assignments(region, 1);
    }
    build_region_motion_sets(region);
    global_region_definitions = region->rewrite_entry->global_definitions;
    set_region_motion_anchor(region);
    global_available_additions_suppressed = 1;
    for (block = region->header; block != region->last->previous; block = block->previous)
        if (!(block->flags & 0x2000000)) move_global_region_block(block, region, function);
    global_available_additions_suppressed = 0;
    if ((int)(short)region->last->number84 - (int)(short)region->header->number84 + 1 <=
        global_region_size_limit)
        finish_region_motion_candidates(region, function);
}

void __fastcall finish_region_register_sets(C2Function *function)
{
    C2Expression *record, *next, *end, *output, *consumer, *operand;
    end = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != end; record = next) {
        next = record->next;
        if (!(record->flags & 1)) continue;
        if (record->opcode == 0x131) {
            output = record->tail.right;
            if (((1U << (output->kind & 31)) & 6) && output->value.reg->value &&
                global_assignment_restored(record)) continue;
        }
        if (record->opcode == 0x151) {
            consumer = follow_optimizer_result(record, &operand);
            if (consumer->kind == 15) continue;
        } else if (record->kind == 15)
            record = record->left->value.reg->value;
        update_global_record_registers(record, 1);
    }
}

void __fastcall prepare_region_motion(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    C2Expression *record, *output;
    C2GlobalDependency *link;
    C2FunctionRegister *reg;
    C2FlowEdge *edge;

    end = last->previous;
    for (block = first; block != end; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (record->opcode != 0x131 || record->precision_flags != 7) continue;
            output = record->tail.right;
            if (!((1U << (output->kind & 31)) & 6)) continue;
            for (link = output->value.reg->global_dependents; link; link = link->next_dependent) {
                reg = link->dependent;
                if (!reg->number) continue;
                for (edge = first->incoming; edge; edge = edge->next_incoming)
                    remove_optimizer_symbol(edge->source->global_definitions, reg->number);
            }
        }
}

void __fastcall validate_global_region(C2FlowRegion *region)
{
    C2Expression *record;
    if (region->flags & 1) return;
    if (!find_flow_edge(region->last, region->header)) region->flags |= 1;
    record = (C2Expression *)region->rewrite_entry->records;
    while (record && record->kind != 25) {
        if (record->flags & 1) {
            if (record->kind == 16 && !record->detail.data && record->opcode != 0x15c)
                region->flags |= 1;
            return;
        }
        record = record->previous;
    }
}

void __fastcall prepare_region_register_sets(C2Function *function)
{
    C2Expression *record, *operand;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        for (operand = record->left; operand; operand = operand->next)
            operand->location.expression.comparison &= 0x7f;
        for (operand = record->tail.right; operand; operand = operand->next)
            operand->location.expression.comparison &= 0x7f;
    }
}

C2BasicBlock *__fastcall find_block_backedge_target(C2BasicBlock *block)
{
    C2FlowEdge *edge;
    C2BasicBlock *target = NULL;
    for (edge = block->outgoing; edge; edge = edge->next_outgoing)
        if ((short)edge->destination->number84 <= (short)block->number84)
            target = edge->destination;
    return target;
}
C2BasicBlock *__fastcall find_global_loop_header(C2BasicBlock *block)
{
    C2BasicBlock *header = find_block_backedge_target(block), *target;
    while (block != header) {
        if (global_block_has_backedge(block)) {
            target = find_block_backedge_target(block);
            if ((short)target->number84 < (short)header->number84) header = target;
        }
        block = block->next;
    }
    return header;
}

void initialize_global_optimizer(void)
{
    unsigned number, bytes, bucket;
    C2OptimizerUse *use, *saved, *current;
    C2FunctionRegister *reg;

    reset_input_pool(14);
    global_expression_count = 0;
    for (bucket = 0; bucket < 101; ++bucket) global_expression_buckets[bucket] = NULL;
    global_expression_cursor = 0;
    global_cached_expression_registers = (C2FunctionRegister *)-1;
    bytes = optimizer_final_index_limit * 4 + 4;
    global_index_registers = c2_pool_allocate(14, bytes);
    global_symbol_registers = c2_pool_allocate(14, bytes);
    global_index_direct_uses = c2_pool_allocate(14, bytes);
    global_index_variant_uses = c2_pool_allocate(14, bytes);
    for (number = next_optimizer_symbol(optimizer_symbol_indices);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        global_symbol_registers[number] = new_register_set(14);
        global_index_registers[number] = new_register_set(14);
    }
    for (number = next_optimizer_symbol(optimizer_value_indices);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        global_index_registers[number] = new_register_set(14);
        global_index_direct_uses[number] = new_register_set(14);
        global_index_variant_uses[number] = new_register_set(14);
        use = get_global_index_uses(number);
        saved = NULL;
        while (use) {
            reg = use->reg;
            current = use;
            if (reg->kind == 12) {
                saved = use;
                current = reg->optimizer_uses;
                if (!current) {
                    use = use->next;
                    continue;
                }
                reg = current->reg;
            }
            add_register_to_set(global_index_direct_uses[number], reg->number);
            for (; reg; reg = reg->variant_next)
                add_register_to_set(global_index_variant_uses[number], reg->number);
            use = current->next;
            if (!use && saved) {
                use = saved->next;
                saved = NULL;
            }
        }
    }
    global_pending_expressions = new_register_set(14);
}

C2RegisterSet *global_initial_registers; /* 48fa4c; captured 48e2c0 owner. */
C2RegisterSet *global_temporary_registers; /* 491034 */
unsigned global_register_state; /* 48fa34: encountered reset DWORD only. */

void __fastcall initialize_global_registers(C2Function *function)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg;
    C2Expression *record;
    for (block = first_function_register_block; block; block = block->next)
        for (reg = block->records; reg < block->cursor; ++reg)
            if (reg->kind != 16) {
                memset(&reg->dependent_uses, 0,
                    sizeof(*reg) - offsetof(C2FunctionRegister, dependent_uses));
                reg->next_free = NULL;
            }
    global_initial_registers = secondary_register_set;
    global_temporary_registers = new_register_set(13);
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        record->control_flags = 0;
        record->pending13 = 0;
    }
    global_register_state = 0;
}

void __fastcall prepare_global_blocks(C2Function *function)
{
    C2BasicBlock *block, *next;
    for (block = function->flow->last; block; block = next) {
        next = block->previous;
        if (!(block->flags & 0x02000000U)) prepare_global_block(block, function);
    }
}

C2Expression *global_next_record; /* 48fa8c; child may update traversal continuation. */
void __fastcall prepare_global_block(C2BasicBlock *block, C2Function *function)
{
    C2Expression *record = ((C2Expression *)block->records)->next;
    while (record != (C2Expression *)block->last_record) {
        global_next_record = record->next;
        if (record->kind == 11 ||
            (record->opcode == 0x15d && record->left->kind == 7))
            prepare_global_record(record, block, function);
        record = global_next_record;
    }
}

/* Encountered switch-range prefix: only links, signed bounds and target are
 * consumed here. Neither the reserved bytes nor an allocation extent is claimed. */
typedef struct C2GlobalSwitchRange {
    struct C2GlobalSwitchRange *next;
    unsigned char pending04[8];
    long low, high;
    C2Expression *target;
} C2GlobalSwitchRange;
typedef char C2GlobalSwitchLowOffset[offsetof(C2GlobalSwitchRange, low) == 12 ? 1 : -1];
typedef char C2GlobalSwitchTargetOffset[offsetof(C2GlobalSwitchRange, target) == 20 ? 1 : -1];
C2Expression *__fastcall truncate_global_integer(unsigned short type,
    C2Expression *operand)
{
    unsigned __int64 previous;
    C2Real *real;
    union { unsigned bits; int signed_value; } low;

    if ((1U << (type >> 12)) & 0xe) {
        previous = operand->integer.value;
        operand->integer.value = normalize_integer_value(type, previous);
        if (previous != operand->integer.value)
            operand->location.expression.comparison |= 0x40;
    } else {
        real = (C2Real *)c2_pool_allocate(0, 12);
        /* The converter uses the saved original low DWORD, even though the
         * recursive call also truncates and marks the input expression. */
        low.bits = operand->integer.low;
        if ((operand->type & 0xf000) == 0x1000) {
            truncate_global_integer(0x1004, operand);
            c2_real_from_int(&low.signed_value, real);
        } else {
            truncate_global_integer(0x2004, operand);
            c2_real_from_unsigned(&low.bits, real);
        }
        operand = new_real_expression(type, real);
    }
    operand->type = type;
    return operand;
}

void __fastcall optimize_global_assignment(C2Expression *record, C2BasicBlock *block)
{
    C2Expression *first, *second, *variable, *constant, *consumer, *use;
    C2Expression *scan, *previous_value, *added, *input;
    C2BranchTarget *target;
    C2BasicBlock *destination;
    C2GlobalSwitchRange *range;
    C2Record *anchor;
    unsigned __int64 value;
    unsigned short type;

    if (!block->outgoing) return;
    if (record->opcode == 0x151) {
        first = record->left;
        second = first->next;
        if ((((1U << (first->kind & 31)) & 6) && first->value.reg->value) ||
            (((1U << (second->kind & 31)) & 6) && second->value.reg->value)) return;
        consumer = follow_optimizer_result(record, &use);
        if (consumer->kind != 16 ||
            (consumer->convention != 1 && consumer->convention != 2)) return;
        if ((1U << (first->kind & 31)) & 0x180) {
            variable = second;
            constant = first;
            if ((1U << (variable->kind & 31)) & 0x180) return;
        } else {
            variable = first;
            constant = second;
        }
        if (constant->kind != 7 || !((1U << (variable->kind & 31)) & 6) ||
            variable->value.reg->value) return;
        if (consumer->convention == 1) {
            target = ((C2InputSymbol *)consumer->left->value.symbol)->branch_target;
            if (!target->block) target = get_symbol_branch_target(target, 24);
            destination = target->block;
            anchor = (C2Record *)target;
        } else {
            /* The predecessor's statement marker and the branch target both
             * carry their basic-block owner at offset20. Retain the actual
             * marker as the insertion anchor, rather than its block. */
            anchor = block->previous->records;
            destination = anchor->owner.block;
        }
        if (!destination->incoming || destination->incoming->next_incoming ||
            !block->outgoing->next_outgoing) return;
        value = constant->integer.value;
        previous_value = NULL;
        for (scan = consumer->previous; scan != record; scan = scan->previous) {
            if (scan->opcode == 0x131) {
                if (compare_input_operands(variable, scan->tail.right)) return;
                previous_value = scan->left;
            } else if (scan->opcode == 0x141 || scan->opcode == 0x142) {
                if (compare_input_operands(previous_value, scan->tail.right)) return;
                input = scan->left;
                if (compare_input_operands(variable, input)) return;
                input = input->next;
                if (input->kind != 7) return;
                if (scan->opcode == 0x141) value += input->integer.value;
                else value -= input->integer.value;
            } else return;
        }
        if (value != constant->integer.value) {
            constant = copy_input_expression(constant);
            constant->integer.value = value;
        }
        added = new_binary_expression(0x131, record->type, variable, constant,
            insert_input_record, anchor);
        added->precision_flags = 1;
        finish_global_assignment(added, 1);
    } else {
        variable = record->left;
        if (((1U << (variable->kind & 31)) & 6) && variable->value.reg->kind == 3) {
            scan = record->previous;
            if (scan->opcode == 0x131 &&
                !compare_input_operands(scan->tail.right, variable)) variable = scan->left;
        }
        if ((((1U << (variable->kind & 31)) & 6) && variable->value.reg->value) ||
            variable->kind == 7) return;
        type = variable->type;
        for (range = (C2GlobalSwitchRange *)record->aggregate_value; range; range = range->next) {
            if (range->low != range->high) continue;
            target = (C2BranchTarget *)range->target;
            destination = target->block;
            if (!destination->incoming || destination->incoming->next_incoming ||
                destination->outgoing->next_outgoing) continue;
            value = normalize_integer_value(type, (unsigned __int64)(__int64)range->low);
            constant = new_integer_expression(type, value);
            added = new_binary_expression(0x131, type, variable, constant,
                insert_input_record, (C2Record *)target);
            added->precision_flags = 1;
            finish_global_assignment(added, 1);
        }
    }
}

C2Expression *__fastcall fold_integer_operation(C2Expression *record, unsigned short type,
    C2Expression *left, C2Expression *right);
C2Expression *__fastcall fold_real_operation(C2Expression *record, unsigned short type,
    C2Expression *left, C2Expression *right);
int __fastcall compare_input_operands(C2Expression *left, C2Expression *right);
void __fastcall remove_statement_expression(C2Expression *statement, C2Expression *record);
int c2_real_compare(int condition, C2Real *left, C2Real *right);

C2Expression *__fastcall prepare_global_record(C2Expression *record,
    C2BasicBlock *block, C2Function *function)
{
    C2Expression *left, *right, *value, *branch, *operand, *replacement, *target;
    C2FunctionRegister *reg;
    C2InputSymbol *symbol;
    C2BranchTarget *label;
    C2BasicBlock *destination;
    C2GlobalSwitchRange *range;
    unsigned __int64 first, second;
    unsigned char condition;
    int signed_compare, result;

    if (record->opcode != 0x15d && input_has_side_effects(record)) {
        if (record->opcode != 0x16c || record->left->kind != 7) return record;
        remove_global_record(record);
        return NULL;
    }
    value = fold_integer_operation(record, record->type, record->left, record->left->next);
    if (!value) value = fold_real_operation(record, record->type, record->left, record->left->next);
    if (value) {
        if (record->left->next) remove_input_operand(record, record->left->next);
        if (value != record->left) attach_input_expression(record, record->left, value);
        record->opcode = 0x131;
        return record;
    }
    if (!((1U << ((record->type >> 12) & 31)) & 0x0e) &&
        record->opcode != 0x135 && record->opcode != 0x151) return record;
    left = record->left;
    right = left->next;
    if (record->opcode == 0x141) {
        if (left->kind == 7) {
            swap_global_operands(record);
            left = record->left;
            right = left->next;
        }
        if (left->kind == 3) {
            if (right->kind != 7) goto compare_or_constant;
            if ((long)((unsigned)left->value.reg->displacement + right->integer.low) >= 0) {
                left->value.reg = new_address_register(left->value.reg, 0x5000, 0,
                    (long)right->integer.low);
                goto assignment;
            }
        }
        if (right->kind == 7 && !right->integer.value) goto assignment;
    } else if (record->opcode == 0x143) {
        if (left->kind == 7) {
            swap_global_operands(record);
            left = record->left;
            right = left->next;
        }
        if (right->kind == 7) {
            if (!right->integer.value) {
                swap_global_operands(record);
                right = record->left->next;
                goto assignment;
            }
            if (right->integer.low == 1 && !right->integer.high) goto assignment;
        }
    } else if (record->opcode == 0x131 &&
               !compare_input_operands(record->tail.right, left)) {
        remove_global_record(record);
        return NULL;
    }
compare_or_constant:
    if (record->opcode == 0x151) {
        if (!compare_input_operands(left, right)) {
            first = second = 1;
            signed_compare = (record->type & 0xf000) == 0x4000 ||
                             (record->type & 0xf000) == 0x1000;
            goto evaluate_branch;
        }
        if (left->kind == 8 && right->kind == 8) {
            condition = global_expression_condition(record);
            signed_compare = 0;
            switch (condition) {
            case 1:
                /* The original performs this comparison, then continues. */
                c2_real_compare(1, left->value.real, right->value.real);
                break;
            case 2:
                first = (__int64)c2_real_compare(1, left->value.real, right->value.real);
                second = 1;
                goto evaluate_branch;
            case 3:
                second = (__int64)c2_real_compare(4, left->value.real, right->value.real);
                first = !second;
                goto evaluate_branch;
            case 5:
                second = (__int64)c2_real_compare(5, left->value.real, right->value.real);
                first = !second;
                goto evaluate_branch;
            case 6:
                /* Both calls are observable in the original function graph. */
                c2_real_compare(3, left->value.real, right->value.real);
            case 4:
                first = (__int64)c2_real_compare(2, left->value.real, right->value.real);
                second = !first;
                goto evaluate_branch;
            }
        }
        if (left->kind == 3 && right->kind == 3 &&
            left->value.reg->canonical == right->value.reg->canonical) {
            signed_compare = 0;
            condition = global_expression_condition(record);
            first = (__int64)left->value.reg->displacement;
            second = (__int64)right->value.reg->displacement;
            goto evaluate_branch;
        }
        if ((record->type & 0xf000) == 0x2000) {
            signed_compare = 0;
            condition = global_expression_condition(record);
            switch (condition) {
            case 3: case 6:
                if (right->kind == 7 && !right->integer.value) {
                    first = 1; second = 0;
                    goto evaluate_branch;
                }
                break;
            case 4: case 5:
                if (left->kind != 7) return record;
                if (!left->integer.value) {
                    first = 0; second = 1;
                    goto evaluate_branch;
                }
                break;
            }
        }
    }
    if (left->kind != 7 || (right && right->kind != 7)) return record;
    first = left->integer.value;
    second = right ? right->integer.value : 0;
    signed_compare = (record->type & 0xf000) == 0x1000;
    if (record->opcode != 0x151) {
        if (record->opcode != 0x15d) return record;
        for (range = (C2GlobalSwitchRange *)record->aggregate_value; range; range = range->next) {
            if (evaluate_integer_comparison(signed_compare, record->type & 0xfff,
                    (__int64)range->low, first, 5) &&
                evaluate_integer_comparison(signed_compare, record->type & 0xfff,
                    first, (__int64)range->high, 5)) break;
            remove_global_successor(block, ((C2BranchTarget *)range->target)->block, function);
        }
        target = record->detail.data;
        if (range) {
            remove_global_successor(block, ((C2BranchTarget *)target)->block, function);
            target = range->target;
        }
        new_call_expression(0x15a, 0, target, NULL, NULL,
            insert_input_record, (C2Record *)record);
        release_global_record(record);
        return NULL;
    }
evaluate_branch:
    branch = follow_optimizer_result(record, &operand);
    if (branch->kind == 15) {
        result = evaluate_integer_comparison(signed_compare, record->type & 0xfff,
            first, second, (unsigned char)branch->detail.intrinsic);
        target = branch->left->next;
        if (!result) target = target->next;
        replacement = new_binary_expression(0x131, branch->type, branch->tail.right,
            target, insert_input_record, (C2Record *)branch);
        if ((1U << (replacement->tail.right->kind & 31)) & 6) {
            reg = replacement->tail.right->value.reg;
            if (reg->value) {
                if (!reg->canonical->variant_next) reg->value = replacement;
                else bind_input_temporary(reg, replacement);
            }
        }
        remove_global_record(record);
        remove_global_record(branch);
        global_next_record = replacement->next;
        return replacement;
    }
    if (!global_motion_enabled) return record;
    result = evaluate_integer_comparison(signed_compare, record->type & 0xfff,
        first, second, branch->convention);
    remove_statement_expression(branch, record);
    remove_global_record(record);
    if (!result) {
        symbol = (C2InputSymbol *)branch->left->value.symbol;
        label = symbol->branch_target;
        destination = label->block;
        if (!destination) destination = get_symbol_branch_target(label, 24)->block;
        if (global_next_record == branch) global_next_record = branch->next;
        remove_global_record(branch);
    } else {
        destination = block->previous;
        branch->opcode = 0x15a;
        remove_input_operand(branch, operand);
        branch->convention = 0;
    }
    remove_global_successor(block, destination, function);
    return NULL;
assignment:
    if (right) remove_input_operand(record, right);
    record->opcode = 0x131;
    record->left->type = record->type;
    if (record->left->kind == 7)
        truncate_global_integer(record->left->type, record->left);
    return record;
}

typedef char C2GlobalNextRegisterOffset[
    offsetof(C2FunctionRegister, global_next) == 44 ? 1 : -1];
typedef char C2GlobalRegisterFollowersOffset[
    offsetof(C2FunctionRegister, global_followers) == 68 ? 1 : -1];
typedef char C2GlobalRegisterDefinitionsOffset[
    offsetof(C2FunctionRegister, global_definitions) == 72 ? 1 : -1];
typedef char C2GlobalRegisterUsesOffset[
    offsetof(C2FunctionRegister, global_uses) == 76 ? 1 : -1];
extern unsigned diagnostic_position;
void __fastcall retain_restored_aggregate(C2Expression *owner, C2Expression *operand);
#define GLOBAL_REGISTER_OPERAND(operand) ((1U << ((operand)->kind & 31)) & 6)

void __fastcall initialize_global_flow_analysis(C2Function *function)
{
    C2Expression *record, *cursor, *end, *operand, *result, *temporary, *inserted;
    C2FunctionRegister *destination, *source, *reg;
    C2OptimizerRecordLink *link;
    C2OptimizerUse *use;
    unsigned short type;
    int first_insert;

    end = (C2Expression *)function->flow->first->last_record;
    cursor = ((C2Expression *)function->flow->last->records)->next;
    while ((record = cursor) != end) {
        cursor = record->next;
        if (!(record->flags & 1) || record->control_flags) continue;
        record->control_flags = 1;
        if (record->location.line) {
            record_line_delta = record->location.line;
            diagnostic_position = function->source_line_base + record->location.line;
        }
        for (operand = record->left; operand; operand = operand->next) {
            if (((1U << (operand->kind & 31)) & 0x60) && operand->opcode != 0x122) {
                temporary = new_integer_expression(0x2004,
                    (__int64)operand->detail.aggregate_displacement);
                inserted = new_assignment_expression(0x133, 0x2004, temporary,
                    prepend_input_record, (C2Record *)record);
                operand->opcode = 0x122;
                operand->detail.aggregate_displacement = 0;
                operand->aggregate_value = copy_input_expression(inserted->tail.right);
                operand->aggregate_value->location.expression.properties |= 0x20;
                operand->aggregate_value->location.expression.properties |= 0x10;
                retain_restored_aggregate(record, operand);
            }
        }
        for (operand = record->tail.right; operand; operand = operand->next) {
            if (((1U << (operand->kind & 31)) & 0x60) && operand->opcode != 0x122) {
                temporary = new_integer_expression(0x2004,
                    (__int64)operand->detail.aggregate_displacement);
                inserted = new_assignment_expression(0x133, 0x2004, temporary,
                    prepend_input_record, (C2Record *)record);
                operand->opcode = 0x122;
                operand->detail.aggregate_displacement = 0;
                operand->aggregate_value = copy_input_expression(inserted->tail.right);
                operand->aggregate_value->location.expression.properties |= 0x20;
                operand->aggregate_value->location.expression.properties |= 0x10;
                retain_restored_aggregate(record, operand);
            }
        }
        if (record->opcode == 0x13f) {
            record->kind = 11;
            type = record->type & 0xfff;
            record->opcode = 0x131;
            if (type && (type < 3 || type == 4)) {
                type |= 0x2000;
                retarget_global_operand(record->tail.right, type);
                operand = record->tail.right;
                if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value)
                    materialize_global_register_operand(operand, operand->value.reg);
                retarget_global_operand(record->left, type);
                operand = record->left;
                if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value)
                    materialize_global_register_operand(operand, operand->value.reg);
                record->type = type;
            }
        }
        if (!((1U << (record->kind & 31)) & 0x1000) && record->kind != 19) {
            if (record->opcode == 0x131) {
                result = record->tail.right;
                operand = record->left;
                if (GLOBAL_REGISTER_OPERAND(result) && result->value.reg->kind == 3 &&
                    GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->kind == 3)
                    result->value.reg->flags |= operand->value.reg->flags & 0x40;
                if (result->kind == 2 || operand->kind == 2) {
                    destination = NULL;
                    if (GLOBAL_REGISTER_OPERAND(result) && result->value.reg->kind == 3 &&
                        !result->value.reg->value) {
                        destination = result->value.reg->canonical;
                        if (!destination->global_next) {
                            destination->global_next = global_pending_registers;
                            global_pending_registers = destination;
                        }
                        link = c2_pool_allocate(3, sizeof(*link));
                        link->record = record;
                        link->next = destination->global_definitions;
                        destination->global_definitions = link;
                    }
                    source = NULL;
                    if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->kind == 3 &&
                        !operand->value.reg->value) {
                        source = operand->value.reg->canonical;
                        if (!source->global_next) {
                            source->global_next = global_pending_registers;
                            global_pending_registers = source;
                        }
                        link = c2_pool_allocate(3, sizeof(*link));
                        link->record = record;
                        link->next = source->global_uses;
                        source->global_uses = link;
                    }
                    if (destination && source) {
                        use = c2_pool_allocate(3, sizeof(*use));
                        use->reg = destination;
                        use->next = source->global_followers;
                        source->global_followers = use;
                    }
                }
            } else {
                result = record->tail.right;
                if (result && GLOBAL_REGISTER_OPERAND(result) &&
                    !result->value.reg->value && result->value.reg->kind == 3) {
                    type = result->type;
                    if ((type & 0xf000) != 0x5000) {
                        temporary = new_temporary_operand(type);
                        inserted = new_binary_expression(0x131, type, result, temporary,
                            insert_input_record, (C2Record *)record);
                        replace_input_result(record, result, temporary);
                        temporary->value.reg->value = record;
                        inserted->flags |= 0x40;
                        cursor = inserted;
                    }
                }
            }
            first_insert = 1;
            for (operand = record->left; operand; operand = operand->next) {
                if ((operand->location.expression.properties & 0x20) &&
                    (operand->location.expression.properties & 0x40) &&
                    (!GLOBAL_REGISTER_OPERAND(operand) || !operand->value.reg->value)) {
                    inserted = new_assignment_expression(0x133, operand->type, operand,
                        prepend_input_record, (C2Record *)record);
                    operand = attach_input_expression(record, operand, inserted->tail.right);
                    inserted->control_flags = 1;
                }
                if (record->opcode != 0x131 && GLOBAL_REGISTER_OPERAND(operand) &&
                    !operand->value.reg->value && operand->value.reg->kind == 3) {
                    type = operand->type;
                    if ((type & 0xf000) != 0x5000) {
                        temporary = new_temporary_operand(type);
                        inserted = new_binary_expression(0x131, type, temporary, operand,
                            prepend_input_record, (C2Record *)record);
                        operand = attach_input_expression(record, operand, temporary);
                        temporary->value.reg->value = inserted;
                        if (first_insert) {
                            first_insert = 0;
                            cursor = inserted;
                        }
                        inserted->flags |= 0x40;
                    }
                }
            }
            result = record->tail.right;
            if (record->opcode == 0x131 && GLOBAL_REGISTER_OPERAND(result) &&
                result->value.reg->value && record->left->kind == 6)
                record->opcode = 0x133;
            else {
                for (operand = record->left; operand; operand = operand->next) {
                    if (((1U << (operand->kind & 31)) & 0x7e) &&
                        (operand->kind == 6 ||
                         ((operand->location.expression.properties & 0x40) &&
                          (!GLOBAL_REGISTER_OPERAND(operand) || !operand->value.reg->value)))) {
                        inserted = new_assignment_expression(0x133, operand->type, operand,
                            prepend_input_record, (C2Record *)record);
                        operand = attach_input_expression(record, operand, inserted->tail.right);
                        inserted->control_flags = 1;
                        mark_global_precision(inserted);
                    }
                }
            }
            mark_global_precision(record);
        }
        operand = record->left;
        if (record->opcode == 0x16c && operand && GLOBAL_REGISTER_OPERAND(operand)) {
            reg = operand->value.reg;
            if (reg->value && reg->value->opcode == 0x151) reg->value->opcode = 0x142;
        }
        if (record->kind == 17 && record->detail.intrinsic > 0xab &&
            record->detail.intrinsic < 0xae)
            append_global_attribute(record, 0x2001, 1);
    }
}

extern int c2_basic_mode;
void __fastcall mark_global_precision(C2Expression *record)
{
    C2Expression *value, *operand;
    if (record->opcode == 0x138) record->precision_flags = 3;
    else if (record->opcode == 0x135) {
        if ((record->type & 0xf000) == 0x4000 &&
            (record->left->type & 0xf000) == 0x4000 &&
            (record->left->type & 0xfff) <= (record->type & 0xfff)) {
            record->precision_flags = 3;
            if (GLOBAL_REGISTER_OPERAND(record->tail.right) &&
                record->tail.right->value.reg->value) {
                value = follow_optimizer_result(record, &operand);
                if (value && value->opcode == 0x130 &&
                    follow_optimizer_result(value, &operand)->opcode == 0x158)
                    record->precision_flags = 0;
            }
        }
    } else if (record->opcode == 0x133) {
        if ((record->type & 0xf000) == 0x4000) record->precision_flags = 3;
        if (GLOBAL_REGISTER_OPERAND(record->tail.right) &&
            record->tail.right->value.reg->value) {
            value = follow_optimizer_result(record, &operand);
            if (value && value->opcode == 0x130) record->precision_flags = 0;
        }
    } else if (record->opcode == 0x14c)
        record->left->value.reg->value->precision_flags = 3;
    if ((record->type & 0xf000) == 0x4000 && c2_basic_mode)
        record->precision_flags = 3;
}

void __fastcall build_global_flow_analysis(C2Function *function)
{
    C2FunctionRegister *pending, *reg, *next, *temporary_reg, *source_reg;
    C2OptimizerUse *use;
    C2OptimizerRecordLink *link;
    C2Expression *source, *definition, *record, *following, *end;
    C2Expression *value, *operand, *replacement;
    unsigned short type, destination_type, saved_type;
    int deferred, valid, compatible;

    pending = global_pending_registers;
    if (pending == (C2FunctionRegister *)-1) return;
    do {
        global_pending_registers = (C2FunctionRegister *)-1;
        for (reg = pending; reg != (C2FunctionRegister *)-1; reg = next) {
            next = reg->global_next;
            deferred = 0;
            for (use = reg->global_followers; use; use = use->next)
                if (use->reg->global_next) {
                    deferred = 1;
                    break;
                }
            if (deferred) {
                reg->global_next = global_pending_registers;
                global_pending_registers = reg;
                continue;
            }
            link = reg->global_uses;
            if (link && !link->next && reg->global_definitions &&
                (source = link->record) != NULL && source->tail.right->kind == 2 &&
                !source->tail.right->value.reg->value && !input_has_side_effects(source)) {
                type = source->type;
                valid = 1;
                for (link = reg->global_definitions; link; link = link->next) {
                    definition = link->record;
                    if (!global_rewrite_order_is_valid(source, definition) ||
                        global_range_changes_operand(definition->next, source->previous,
                            source->tail.right) ||
                        global_range_changes_operand(definition->next, source->previous,
                            source->left)) {
                        valid = 0;
                        break;
                    }
                    destination_type = definition->tail.right->type;
                    if (!register_types_match(destination_type, type)) {
                        compatible = 0;
                        switch (definition->left->kind) {
                        case 1: case 2: case 6: case 7:
                            if (destination_type == type) compatible = 1;
                            else if ((1U << ((destination_type >> 12) & 31)) & 0x0e) {
                                if (((1U << ((type >> 12) & 31)) & 0x0e) ||
                                    (type & 0xf000) == 0x5000)
                                    compatible = (type & 0xfff) <= (destination_type & 0xfff);
                            } else if ((destination_type & 0xf000) == 0x5000)
                                compatible = (type & 0xfff) <= (destination_type & 0xfff);
                            break;
                        }
                        if (!compatible) {
                            valid = 0;
                            break;
                        }
                    }
                }
                if (valid) {
                    for (link = reg->global_definitions; link; link = link->next) {
                        definition = link->record;
                        replace_input_result(definition, definition->tail.right,
                            source->tail.right);
                        operand = definition->left;
                        if (operand->kind && operand->kind < 3)
                            operand->value.reg = new_address_register(operand->value.reg,
                                type, 0, 0);
                        else if (operand->kind == 7) truncate_global_integer(type, operand);
                        definition->left->type = type;
                        definition->type = type;
                    }
                    unlink_and_release_input_record((C2Record *)source);
                }
            }
            reg->global_next = NULL;
            reg->global_uses = NULL;
            reg->global_definitions = NULL;
            reg->global_followers = NULL;
        }
        pending = global_pending_registers;
    } while (pending != (C2FunctionRegister *)-1);
    global_pending_registers = (C2FunctionRegister *)-1;
    end = (C2Expression *)function->flow->first->last_record;
    for (record = ((C2Expression *)function->flow->last->records)->next;
         record != end; record = following) {
        following = record->next;
        if (!(record->flags & 0x40)) continue;
        record->flags &= ~0x40;
        operand = record->tail.right;
        if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value) {
            temporary_reg = operand->value.reg;
            value = follow_optimizer_result(record, &operand);
            if (temporary_reg == operand->value.reg) {
                saved_type = operand->type;
                replacement = attach_input_expression(value, operand, record->left);
                replacement->type = saved_type;
                unlink_and_release_input_record((C2Record *)record);
                release_function_register(temporary_reg);
            }
        } else {
            operand = record->left;
            if (GLOBAL_REGISTER_OPERAND(operand)) {
                source_reg = operand->value.reg;
                value = source_reg->value;
                if (value && value->kind != 13) {
                    operand = value->tail.right;
                    temporary_reg = operand->value.reg;
                    if (temporary_reg == source_reg) {
                        saved_type = operand->type;
                        replacement = replace_input_result(value, operand, record->tail.right);
                        replacement->type = saved_type;
                        unlink_and_release_input_record((C2Record *)record);
                        release_function_register(temporary_reg);
                    }
                }
            }
        }
    }
}

void __fastcall rewrite_global_function(C2Function *function, unsigned mode)
{
    C2BasicBlock *block;

    reset_input_pool(6);
    for (block = function->flow->last; block; block = block->previous) {
        if (mode & 1) {
            block->rewrite_mode1[0] = new_register_set(6);
            block->rewrite_mode1[1] = new_register_set(6);
            block->rewrite_mode1[2] = new_register_set(6);
            block->rewrite_mode1[3] = new_register_set(6);
            block->rewrite_mode1[4] = new_register_set(6);
            block->rewrite_mode1[5] = new_register_set(6);
            block->rewrite_mode1[6] = new_register_set(6);
            block->rewrite_mode1[7] = new_register_set(6);
        } else {
            block->rewrite_mode1[0] = NULL;
            block->rewrite_mode1[1] = NULL;
            block->rewrite_mode1[2] = NULL;
            block->rewrite_mode1[3] = NULL;
            block->rewrite_mode1[4] = NULL;
            block->rewrite_mode1[5] = NULL;
            block->rewrite_mode1[6] = NULL;
            block->rewrite_mode1[7] = NULL;
        }
        if (mode & 2) {
            block->rewrite_mode2[1] = new_register_set(6);
            block->rewrite_mode2[0] = new_register_set(6);
            block->rewrite_mode2[2] = new_register_set(6);
            block->rewrite_mode2[3] = new_register_set(6);
            block->rewrite_mode2[4] = new_register_set(6);
            block->rewrite_mode2[5] = new_register_set(6);
        } else {
            block->rewrite_mode2[1] = NULL;
            block->rewrite_mode2[0] = NULL;
            block->rewrite_mode2[2] = NULL;
            block->rewrite_mode2[3] = NULL;
            block->rewrite_mode2[4] = NULL;
            block->rewrite_mode2[5] = NULL;
        }
        if (mode & 4) {
            block->rewrite_mode4[0] = new_register_set(6);
            block->rewrite_mode4[1] = new_register_set(6);
            block->rewrite_mode4[2] = new_register_set(6);
            block->rewrite_mode4[3] = new_register_set(6);
            block->rewrite_mode4[4] = new_register_set(6);
            block->rewrite_mode4[5] = new_register_set(6);
            block->rewrite_mode4[6] = new_register_set(6);
        } else {
            block->rewrite_mode4[0] = NULL;
            block->rewrite_mode4[1] = NULL;
            block->rewrite_mode4[2] = NULL;
            block->rewrite_mode4[3] = NULL;
            block->rewrite_mode4[4] = NULL;
            block->rewrite_mode4[5] = NULL;
            block->rewrite_mode4[6] = NULL;
        }
    }
}

/* Actual DWORD 48fa54, reset before propagation; no pointer role is inferred. */
int global_rewrite_state;

void __fastcall finish_global_function_rewrite(C2Function *function)
{
    C2BasicBlock *first, *last;

    first = function->flow->last;
    last = function->flow->first;
    global_rewrite_state = 0;
    clear_global_rewrite_marks(first, last);
    propagate_global_rewrites(first, last);
}

void __fastcall clear_global_rewrite_marks(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *end;

    end = first->next;
    for (; last != end; last = last->next)
        last->flags &= ~0x04000000;
}

C2FunctionRegister *global_rewrite_registers; /* 48fa44, next_free-linked work list. */
C2RegisterSet *global_rewrite_exclusions;     /* 48fa48. */

void __fastcall propagate_global_rewrites(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end, *header;
    C2FlowEdge *edge;
    C2Expression *record, *operand, *before, *previous;
    C2RegisterSet *inputs, *outputs;
    C2FunctionRegister *reg;
    C2FlowRegion *region;
    int has_assignments;

    if (!last->previous)
        copy_optimizer_symbol_set_into(last->rewrite_mode1[3], global_initial_registers);
    end = first->next;
    for (block = last; block != end; block = block->next)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->left; operand; operand = operand->next) {
                if ((1U << (operand->kind & 31)) & 6) {
                    operand->value.reg->rewrite_seen = 0;
                    operand->value.reg->rewrite_queued = 0;
                    operand->location.expression.comparison &= ~0x10;
                } else if (operand->kind == 6 && operand->value.reg) {
                    operand->value.reg->rewrite_seen = 0;
                    operand->value.reg->rewrite_queued = 0;
                }
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if ((1U << (operand->kind & 31)) & 6) {
                    operand->value.reg->rewrite_seen = 0;
                    operand->value.reg->rewrite_queued = 0;
                    operand->location.expression.comparison &= ~0x10;
                } else if (operand->kind == 6 && operand->value.reg) {
                    operand->value.reg->rewrite_seen = 0;
                    operand->value.reg->rewrite_queued = 0;
                }
            }
        }
    prepare_global_alias_uses();
    global_rewrite_registers = NULL;
    inputs = new_register_set(6);
    outputs = new_register_set(6);
    global_rewrite_exclusions = new_global_exclusion_set(6);
    for (block = last; block != end; block = block->next) {
        if (global_block_has_backedge(block) && !(block->flags & 0x04000000)) {
            header = find_global_loop_header(block);
            region = header->block_scope;
            if (region && region->header == header && region->rewrite_entry)
                header = region->rewrite_entry;
            prepare_global_loop(header, block);
        }
        edge = block->outgoing;
        if (edge) {
            copy_optimizer_symbol_set_into(block->rewrite_mode1[3],
                edge->destination->rewrite_mode1[2]);
            copy_optimizer_symbol_set_into(block->rewrite_mode1[7],
                edge->destination->rewrite_mode1[6]);
            for (edge = edge->next_outgoing; edge; edge = edge->next_outgoing) {
                merge_optimizer_symbol_sets(block->rewrite_mode1[3],
                    edge->destination->rewrite_mode1[2]);
                intersect_optimizer_symbol_sets(block->rewrite_mode1[7],
                    edge->destination->rewrite_mode1[6]);
            }
        }
        copy_optimizer_symbol_set_into(inputs, block->rewrite_mode1[3]);
        copy_optimizer_symbol_set_into(outputs, block->rewrite_mode1[7]);
        while ((reg = global_rewrite_registers) != NULL) {
            reg->rewrite_queued = 0;
            global_rewrite_registers = reg->next_free;
        }
        before = (C2Expression *)block->records;
        has_assignments = 0;
        for (record = ((C2Expression *)block->last_record)->previous;
             record != before; record = previous) {
            previous = record->previous;
            if (!(record->flags & 1)) continue;
            if (record->kind == 11) has_assignments = 1;
            rewrite_global_record(record, inputs, outputs);
        }
        intersect_optimizer_symbol_sets(outputs, global_rewrite_exclusions);
        copy_optimizer_symbol_set_into(block->rewrite_mode1[2], inputs);
        copy_optimizer_symbol_set_into(block->rewrite_mode1[6], outputs);
        if (has_assignments && !next_global_code_record(before))
            global_aliases_changed = 1;
    }
    release_optimizer_symbol_set(inputs);
    release_optimizer_symbol_set(outputs);
    release_optimizer_symbol_set(global_rewrite_exclusions);
}

void prepare_global_alias_uses(void)
{
    C2RegisterSet *variants, *uses;
    C2FunctionRegister *reg, *variant;
    C2OptimizerConflict *conflict;
    unsigned number;

    variants = new_register_set(6);
    for (number = next_optimizer_symbol(optimizer_live_registers);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        clear_register_set(variants);
        for (variant = reg; variant; variant = variant->variant_next)
            add_register_to_set(variants, variant->number);
        for (conflict = optimizer_conflicts[
                 ((C2InputSymbol *)reg->symbol)->detail.optimizer_number];
             conflict; conflict = conflict->next) {
            uses = global_index_variant_uses[conflict->number];
            if (uses) merge_optimizer_symbol_sets(uses, variants);
        }
    }
    release_optimizer_symbol_set(variants);
}

C2RegisterSet *__fastcall new_global_exclusion_set(int pool)
{
    C2RegisterSet *result;
    C2FunctionRegister *reg, *canonical;
    unsigned number;

    result = new_register_set(pool);
    for (number = next_optimizer_symbol(tracked_register_set);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        canonical = reg->canonical;
        if (canonical != reg &&
            (canonical->displacement < reg->displacement ||
             (long)(canonical->value_bytes + canonical->displacement) <
             (long)(reg->value_bytes + reg->displacement)))
            add_register_to_set(result, reg->number);
    }
    for (number = next_optimizer_symbol(secondary_register_set);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        canonical = reg->canonical;
        if (canonical != reg &&
            (canonical->displacement < reg->displacement ||
             (long)(canonical->value_bytes + canonical->displacement) <
             (long)(reg->value_bytes + reg->displacement)))
            add_register_to_set(result, reg->number);
    }
    return result;
}

C2FunctionRegister *__fastcall lookup_function_register(unsigned number)
{
    return &function_register_blocks[number >> 5]->records[number & 31];
}

int __fastcall global_block_has_backedge(C2BasicBlock *block)
{
    C2FlowEdge *edge;

    for (edge = block->outgoing; edge; edge = edge->next_outgoing)
        if ((short)edge->destination->number84 <= (short)block->number84)
            return 1;
    return 0;
}

C2RegisterSet *global_flow_temporary; /* Actual 491020 writer and set consumers. */

void __fastcall finish_global_flow_analysis(C2Function *function)
{
    C2BasicBlock *first, *last, *block, *end;

    first = function->flow->last;
    last = function->flow->first;
    global_flow_temporary = new_register_set(6);
    end = first->next;
    for (block = last; block != end; block = block->next)
        initialize_global_block_sets(block);
    release_optimizer_symbol_set(global_flow_temporary);
    solve_global_block_sets(first, last);
    finish_global_record_sets(first, last);
}

C2RegisterSet *global_flow_kills; /* 491024, current block+96. */
C2RegisterSet *global_flow_uses;  /* 491028, current block+100. */

void __fastcall initialize_global_block_sets(C2BasicBlock *block)
{
    C2Expression *record;

    global_flow_kills = block->rewrite_mode2[0];
    global_flow_uses = block->rewrite_mode2[1];
    clear_register_set(global_flow_temporary);
    clear_register_set(global_flow_uses);
    clear_register_set(global_flow_kills);
    clear_register_set(block->rewrite_mode2[2]);
    clear_register_set(block->rewrite_mode2[3]);
    clear_register_set(block->rewrite_mode2[4]);
    clear_register_set(block->rewrite_mode2[5]);
    for (record = ((C2Expression *)block->last_record)->previous;
         record != (C2Expression *)block->records; record = record->previous)
        if (record->flags & 1) collect_global_record_sets(record);
}

void __fastcall solve_global_block_sets(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    int changed;

    end = first->next;
    do {
        changed = 0;
        for (block = last; block != end; block = block->next)
            update_global_block_sets(block, &changed);
    } while (changed);
}

void __fastcall update_global_block_sets(C2BasicBlock *block, int *changed)
{
    C2RegisterSet *temporary, *scratch, *common, *different, *uses, *result, *input;
    C2FlowEdge *edge;
    C2BasicBlock *successor;

    temporary = new_register_set(6);
    scratch = new_register_set(6);
    common = block->rewrite_mode2[4];
    clear_register_set(common);
    different = block->rewrite_mode2[5];
    clear_register_set(different);
    edge = block->outgoing;
    if (edge) {
        successor = edge->destination;
        copy_global_set_intersection(common, successor->rewrite_mode1[2],
            successor->rewrite_mode2[2]);
        merge_optimizer_symbol_sets(different, successor->rewrite_mode2[3]);
        for (edge = edge->next_outgoing; edge; edge = edge->next_outgoing) {
            successor = edge->destination;
            copy_global_set_intersection(temporary, successor->rewrite_mode1[2],
                successor->rewrite_mode2[2]);
            copy_global_set_intersection(scratch, temporary, common);
            merge_optimizer_symbol_sets(different, scratch);
            merge_optimizer_symbol_sets(different, successor->rewrite_mode2[3]);
            xor_global_register_sets(common, temporary);
        }
        subtract_optimizer_symbol_sets(common, different);
    }
    copy_optimizer_symbol_difference(temporary, common, block->rewrite_mode2[0]);
    uses = block->rewrite_mode2[1];
    result = block->rewrite_mode2[3];
    copy_global_set_intersection(result, temporary, uses);
    copy_optimizer_symbol_difference(scratch, different, block->rewrite_mode2[0]);
    merge_optimizer_symbol_sets(result, scratch);
    copy_optimizer_symbol_difference(scratch, block->rewrite_mode1[0], uses);
    merge_optimizer_symbol_sets(result, scratch);
    input = block->rewrite_mode2[2];
    copy_optimizer_symbol_difference(scratch, block->rewrite_mode1[2], input);
    merge_optimizer_symbol_sets(result, scratch);
    xor_global_register_sets(temporary, uses);
    subtract_optimizer_symbol_sets(temporary, result);
    if (!*changed && !optimizer_symbol_sets_equal(temporary, input)) *changed = 1;
    copy_optimizer_symbol_set_into(input, temporary);
    release_optimizer_symbol_set(temporary);
    release_optimizer_symbol_set(scratch);
}

void __fastcall copy_global_set_intersection(C2RegisterSet *destination,
    C2RegisterSet *source, C2RegisterSet *mask)
{
    C2RegisterSetNode *input, *spare, *node, *next;

    input = source->links.first;
    spare = destination->links.first;
    node = spare;
    while (input) {
        if (spare) {
            node = spare;
            spare = spare->next;
        } else node = insert_register_set_node(destination, node);
        node->group = input->group;
        node->bits = input->bits;
        input = input->next;
    }
    while (spare) {
        next = spare->next;
        remove_register_set_node(destination, spare);
        spare = next;
    }
    intersect_optimizer_symbol_sets(destination, mask);
}

void __fastcall xor_global_register_sets(C2RegisterSet *destination, C2RegisterSet *source)
{
    C2RegisterSetNode *input, *node, **link;
    unsigned group;

    for (input = source->links.first; input; input = input->next) {
        group = input->group & ~31U;
        link = &destination->links.first;
        for (node = *link; node && node->group < group; node = node->next)
            link = &node->next;
        if (!node || node->group != group) {
            node = new_register_set_node(destination);
            node->group = group;
            node->next = *link;
            *link = node;
        }
        node->bits ^= input->bits;
        if (!node->bits) remove_register_set_node(destination, node);
    }
}

/* Original497220: the current block's available expression-register set. */
C2RegisterSet *global_available_registers;
typedef char C2GlobalAvailableInOffset[
    offsetof(C2BasicBlock, global_available_in) == 80 ? 1 : -1];

void __fastcall optimize_global_block(C2BasicBlock *block, C2Function *function)
{
    C2FlowEdge *edge;
    C2Expression *record, *last;
    int operations;

    if (block->flags & 0x02000000U) return;
    edge = block->incoming;
    if (edge) {
        copy_optimizer_symbol_set_into(block->global_available_in, edge->source->global_definitions);
        for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
            intersect_optimizer_symbol_sets(block->global_available_in, edge->source->global_definitions);
    }
    global_available_registers = block->global_definitions;
    copy_optimizer_symbol_set_into(global_available_registers, block->global_available_in);
    last = (C2Expression *)block->last_record;
    operations = 0;
    for (record = ((C2Expression *)block->records)->next; record != last; record = global_next_record) {
        global_next_record = record->next;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        if (record->flags & 1) {
            if (record->kind == 11) operations = 1;
            optimize_global_record(record, block, function);
        }
    }
    if (operations && !find_next_global_operation((C2Expression *)block->records))
        global_aliases_changed = 1;
}

/* Original49721c is an initialized NULL expression pointer, maintained by
 * the global record rewriting helpers. */
C2Expression *global_pending_assignment;
typedef char C2GlobalDefinitionRecordOffset[
    offsetof(C2FunctionRegister, global_definition_record) == 72 ? 1 : -1];

void __fastcall optimize_global_record(C2Expression *record,
    C2BasicBlock *block, C2Function *function)
{
    C2Expression *result, *selected, *operand, *prior, *replacement;
    C2FunctionRegister *reg;
    C2BasicBlock *destination;
    C2InputSymbol *symbol;
    C2BranchTarget *label;
    C2RegisterSet *set;
    int classification, take_first, rewrite;

    if (global_region_limited || (global_block_state && record->opcode == 0x151)) {
        if (record->opcode == 0x131) {
            if (global_assignment_is_redundant(record, record->left) &&
                !input_has_side_effects(record)) {
                if ((record->type & 0xf000) != 0x5000 ||
                    !((1U << (record->tail.right->kind & 31)) & 6)) {
                    remove_global_record(record);
                    record = NULL;
                } else if (record->tail.right->value.reg->kind != 3) {
                    remove_global_record(record);
                    record = NULL;
                }
            }
        } else if (record->opcode == 0x151 || record->opcode == 0x15e) {
            result = record->tail.right;
            if (record->opcode == 0x15e &&
                ((1U << (result->kind & 31)) & 6) &&
                result->value.reg->global_expression && !result->value.reg->value &&
                optimizer_set_contains(global_available_registers, result->value.reg->number)) {
                if (!input_has_side_effects(record)) {
                    remove_global_record(record);
                    record = NULL;
                }
            } else if (record->value.reg) {
                classification = classify_global_register(global_available_registers, record->value.reg);
                if (classification == 1 || classification == 2) {
                    take_first = classification == 1;
                    if (record->opcode == 0x15e) {
                        selected = record;
                        prior = record->left->value.reg->value;
                    } else {
                        selected = follow_optimizer_result(record, &operand);
                        /* The original retains its incoming function context
                         * in this slot until the 15e branch replaces it. */
                        prior = (C2Expression *)function;
                    }
                    if (selected->kind == 16) {
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
                        /* Original defect: the 15e path bypasses the fresh
                         * local operand writer, but this subpath reads it. */
                        if (record->opcode == 0x15e && take_first)
                            recon_unsupported("uninitialized global branch operand");
#endif
                        remove_statement_expression(selected, record);
                        remove_global_record(record);
                        if (take_first) {
                            destination = block->previous;
                            selected->opcode = 0x15a;
                            remove_input_operand(selected, operand);
                            selected->convention = 0;
                        } else {
                            symbol = (C2InputSymbol *)selected->left->value.symbol;
                            label = symbol->branch_target;
                            destination = label->block;
                            if (!destination) destination = get_symbol_branch_target(label, 24)->block;
                            if (global_next_record == selected) global_next_record = selected->next;
                            remove_global_record(selected);
                        }
                        remove_global_successor(block, destination, function);
                        record = NULL;
                    } else if (selected->kind == 15) {
                        if (take_first) replacement = selected->left->next;
                        else replacement = selected->left->next->next;
                        record = new_binary_expression(0x131, selected->type,
                            selected->tail.right, replacement, insert_input_record, (C2Record *)selected);
                        finish_global_assignment(record, 1);
                        result = record->tail.right;
                        if (((1U << (result->kind & 31)) & 6) && result->value.reg->value) {
                            reg = result->value.reg;
                            if (!reg->canonical->variant_next) reg->value = record;
                            else bind_input_temporary(reg, record);
                        }
                        remove_global_record(prior);
                        remove_global_record(selected);
                        global_next_record = record->next;
                    }
                }
            }
        } else {
            result = record->tail.right;
            if (result && ((1U << (result->kind & 31)) & 6) &&
                result->value.reg->global_expression &&
                global_register_is_available(result->value.reg) && !input_has_side_effects(record)) {
                if (record->precision_flags != 3) {
                    remove_global_record(record);
                    record = NULL;
                } else if (global_pending_assignment != record) record = NULL;
            }
        }
        if (!record) return;
        substitute_available_global_operands(record);
    }
    rewrite = global_record_needs_rewrite(record);
    if (rewrite || global_pending_assignment == record) {
        if (global_pending_assignment == record) global_pending_assignment = NULL;
        record = prepare_global_record(record, block, function);
        if (!record) return;
        update_global_record_registers(record, 0);
        if (global_region_limited || (global_block_state && record->opcode == 0x151)) {
            record = optimize_limited_global_record(record, block, function);
            if (!record) return;
        }
    } else if (record->opcode == 0x151) {
        record = prepare_global_record(record, block, function);
        if (!record) return;
        update_global_record_registers(record, 0);
    }
    if (global_region_motion_enabled && record->opcode == 0x131 &&
        record->precision_flags != 1 && ((1U << (record->tail.right->kind & 31)) & 6)) {
        record->value.reg->global_definition_record = record;
        if (record->precision_flags == 7 || record->precision_flags == 8) {
            set = new_temporary_register_set();
            copy_optimizer_symbol_set_into(set, global_available_registers);
            prepend_input_record((C2Record *)record, new_global_register_set_record(set));
        }
    }
    update_global_available_registers(record);
}

void __fastcall finish_global_block_pass(C2Function *function)
{
    C2Record *record, *next, *last;

    last = function->flow->first->last_record;
    record = function->flow->last->records->next;
    while (record != last) {
        next = record->next;
        if (record->kind == 26) {
            release_optimizer_symbol_set(record->owner.registers);
            remove_global_record((C2Expression *)record);
        }
        record = next;
    }
}

void __fastcall clear_global_block_marks(C2Function *function)
{
    C2Expression *record, *next, *last;

    last = (C2Expression *)function->flow->first->last_record;
    record = (C2Expression *)function->flow->last->records->next;
    while (record != last) {
        next = record->next;
        if (record->precision_flags == 1) unlink_and_release_input_record((C2Record *)record);
        record = next;
    }
}

void __fastcall clear_global_register_marks(C2Function *function)
{
    C2Expression *record, *next, *last;

    last = (C2Expression *)function->flow->first->last_record;
    record = (C2Expression *)function->flow->last->records->next;
    while (record != last) {
        next = record->next;
        if (record->precision_flags == 2) unlink_and_release_input_record((C2Record *)record);
        record = next;
    }
}

void __fastcall solve_global_register_sets(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    C2FlowEdge *edge;
    int changed;

    end = last->previous;
    for (block = first; block != end; block = block->previous)
        initialize_global_available_sets(block);
    for (block = first; block != end; block = block->previous) {
        if (!block->block_scope || (block->block_scope->flags & 1)) continue;
        for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
            if ((short)block->number84 >= (short)edge->destination->number84) {
                fill_global_available_set(block->global_definitions, next_function_register_number);
                break;
            }
        }
    }
    do {
        changed = 0;
        for (block = first; block != end; block = block->previous)
            update_global_available_sets(block, &changed);
    } while (changed);
}

/* These three current-block pointers are assigned before each traversal. */
C2RegisterSet *global_available_generations; /* 49722c, block+68. */
C2RegisterSet *global_available_kills;       /* 497224, block+72. */
C2RegisterSet *global_available_uses;        /* 497228, block+76. */

void __fastcall initialize_global_available_sets(C2BasicBlock *block)
{
    C2Expression *record;

    global_available_generations = block->rewrite_mode4[0];
    global_available_kills = block->rewrite_mode4[1];
    global_available_uses = block->rewrite_mode4[2];
    clear_register_set(global_available_generations);
    clear_register_set(global_available_kills);
    clear_register_set(global_available_uses);
    for (record = (C2Expression *)block->records->next;
         record != (C2Expression *)block->last_record; record = record->next)
        if (record->flags & 1) collect_global_available_record(record, block);
}

void __fastcall collect_global_available_record(C2Expression *record, C2BasicBlock *block)
{
    C2Expression *output, *operand;
    C2FunctionRegister *reg, *dependent, *canonical, *variant, *saved;
    C2GlobalDependency *link;
    C2InputSymbol *symbol;
    C2RegisterSet *effects;
    unsigned was_available, now_available, number, opcode;
    int keep_assignment;

    ++global_register_state;
    opcode = record->opcode;
    saved = NULL;
    if (opcode == 0x131 && record->tail.right->kind == 6) {
        operand = record->tail.right->aggregate_value;
        if (!GLOBAL_REGISTER_OPERAND(operand) || !operand->value.reg->value) {
            saved = operand->value.reg;
            was_available = optimizer_set_contains(global_available_generations, saved->number);
            add_register_to_set(global_available_generations, saved->number);
            remove_optimizer_symbol(global_available_kills, saved->number);
        }
    }
    for (output = record->tail.right; output; output = output->next) {
        switch (output->kind) {
        case 1: case 2:
            if (record->precision_flags == 1) break;
            reg = output->value.reg;
            if (!reg->global_expression || (reg->rewrite_flags & 1) || record->precision_flags == 14)
                invalidate_global_available_register(reg, NULL);
            else {
                for (link = reg->global_dependents; link; link = link->next_dependent) {
                    dependent = link->dependent;
                    if (dependent->global_expression &&
                        (dependent->global_expression->opcode == 0x131 ||
                         operation_is_comparison(dependent->global_expression->opcode))) {
                        number = dependent->number;
                        add_register_to_set(global_available_kills, number);
                        add_register_to_set(global_available_uses, number);
                        remove_optimizer_symbol(global_available_generations, number);
                    }
                }
                canonical = reg->canonical;
                for (variant = canonical; variant; variant = variant->variant_next) {
                    if (variant != reg &&
                        variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                        reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes))
                        invalidate_global_available_variants(variant, canonical);
                }
            }
            if (reg->kind == 7 || reg->kind == 8 || (reg->view_flags & 2)) {
                symbol = (C2InputSymbol *)reg->canonical->symbol;
                effects = global_symbol_registers[symbol->detail.optimizer_number];
                merge_optimizer_symbol_sets(global_available_uses, effects);
                merge_optimizer_symbol_sets(global_available_kills, effects);
                subtract_optimizer_symbol_sets(global_available_generations, effects);
            }
            break;
        case 6:
            effects = global_index_registers[output->immediate];
            merge_optimizer_symbol_sets(global_available_uses, effects);
            merge_optimizer_symbol_sets(global_available_kills, effects);
            subtract_optimizer_symbol_sets(global_available_generations, effects);
            number = output->value.reg->number;
            add_register_to_set(global_available_generations, number);
            remove_optimizer_symbol(global_available_kills, number);
            break;
        case 9:
            c2_report_internal("E:\\utc\\src\\\\P2\\globdf.c", 0xf3d);
            break;
        case 10:
            effects = global_index_registers[output->value.optimizer_index];
            merge_optimizer_symbol_sets(global_available_uses, effects);
            merge_optimizer_symbol_sets(global_available_kills, effects);
            subtract_optimizer_symbol_sets(global_available_generations, effects);
            break;
        }
    }
    output = record->tail.right;
    keep_assignment = 1;
    if (output) {
        if (GLOBAL_REGISTER_OPERAND(output) && output->value.reg->global_expression) {
            number = output->value.reg->number;
            add_register_to_set(global_available_generations, number);
            remove_optimizer_symbol(global_available_kills, number);
        } else if (saved) {
            number = saved->number;
            now_available = optimizer_set_contains(global_available_generations, number);
            keep_assignment = now_available != 0;
            if (!was_available && now_available) {
                add_register_to_set(global_available_kills, number);
                add_register_to_set(global_available_uses, number);
                remove_optimizer_symbol(global_available_generations, number);
            }
        }
    }
    if (opcode == 0x131 && keep_assignment) {
        number = record->value.reg->number;
        add_register_to_set(global_available_generations, number);
        remove_optimizer_symbol(global_available_kills, number);
    }
}

void __fastcall update_global_available_sets(C2BasicBlock *block, int *changed)
{
    C2FlowEdge *edge;

    edge = block->incoming;
    if (edge) {
        copy_optimizer_symbol_set_into(block->global_available_in, edge->source->global_definitions);
        for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
            intersect_optimizer_symbol_sets(block->global_available_in, edge->source->global_definitions);
    }
    update_available_register_transfer(block->global_definitions, block->rewrite_mode4[0],
        block->global_available_in, block->rewrite_mode4[1], changed);
}

void __fastcall update_available_register_transfer(C2RegisterSet *destination,
    C2RegisterSet *generated, C2RegisterSet *input, C2RegisterSet *killed, int *changed)
{
    C2RegisterSet *saved;
    C2RegisterSetNode *source, *spare, *node, *next;

    if (*changed) {
        update_optimizer_alias_set(destination, input, killed, generated);
        return;
    }
    saved = new_register_set(destination->pool);
    source = destination->links.first;
    spare = saved->links.first;
    node = spare;
    while (source) {
        if (spare) {
            node = spare;
            spare = spare->next;
        } else node = insert_register_set_node(saved, node);
        node->group = source->group;
        node->bits = source->bits;
        source = source->next;
    }
    while (spare) {
        next = spare->next;
        remove_register_set_node(saved, spare);
        spare = next;
    }
    update_optimizer_alias_set(destination, input, killed, generated);
    *changed = !optimizer_symbol_sets_equal(destination, saved);
    release_optimizer_symbol_set(saved);
}

/* Eleven contiguous temporary-set pointer owners at 497230..49725c.
 * Their initializer overwrites each cell before the region pass uses it. */
C2RegisterSet *region_motion_sets[11];

void initialize_region_motion_sets(void)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg;

    for (block = first_function_register_block; block; block = block->next) {
        for (reg = block->records; reg < block->cursor; ++reg) {
            if (reg->kind == 16) continue;
            reg->rewrite_flags &= ~6;
            reg->global_definitions = NULL;
            reg->global_uses = NULL;
        }
    }
    region_motion_sets[9] = new_temporary_register_set();
    region_motion_sets[5] = new_temporary_register_set();
    region_motion_sets[0] = new_temporary_register_set();
    region_motion_sets[4] = new_temporary_register_set();
    region_motion_sets[2] = new_temporary_register_set();
    region_motion_sets[3] = new_temporary_register_set();
    region_motion_sets[10] = new_temporary_register_set();
    region_motion_sets[7] = new_temporary_register_set();
    region_motion_sets[1] = new_temporary_register_set();
    region_motion_sets[6] = new_temporary_register_set();
    region_motion_sets[8] = new_temporary_register_set();
}

void release_region_motion_sets(void)
{
    release_optimizer_symbol_set(region_motion_sets[9]);
    release_optimizer_symbol_set(region_motion_sets[5]);
    release_optimizer_symbol_set(region_motion_sets[0]);
    release_optimizer_symbol_set(region_motion_sets[3]);
    release_optimizer_symbol_set(region_motion_sets[10]);
    release_optimizer_symbol_set(region_motion_sets[4]);
    release_optimizer_symbol_set(region_motion_sets[2]);
    release_optimizer_symbol_set(region_motion_sets[7]);
    release_optimizer_symbol_set(region_motion_sets[1]);
    release_optimizer_symbol_set(region_motion_sets[6]);
    release_optimizer_symbol_set(region_motion_sets[8]);
}

void __fastcall finish_region_motion(C2Function *function)
{
    C2Expression *record;
    C2BasicBlock *block;

    /* The original carries the incoming function pointer until a block marker
     * supplies the current block. Do not invent a NULL default for it. */
    block = (C2BasicBlock *)function;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (record->kind == 24) {
            block = ((C2Record *)record)->owner.block;
        } else if (record->opcode == 0x151) finish_region_selection(record, block);
    }
}

void __fastcall prepare_global_function_cleanup(C2Function *function)
{
    C2FlowGraph *flow;

    flow = function->flow;
    global_rewrite_state = 0;
    clear_global_rewrite_marks(flow->last, flow->first);
    global_rewrite_exclusions = new_global_exclusion_set(6);
    prepare_global_loop(flow->last, flow->first);
    prepare_global_cleanup_records(function);
    release_optimizer_symbol_set(global_rewrite_exclusions);
}

void __fastcall prepare_global_loop(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;

    end = first->next;
    for (block = last; block != end; block = block->next)
        if (!(block->flags & 0x04000000U)) initialize_global_loop_block(block);
    solve_global_loop_sets(first, last);
}

/* Current block views, assigned by the initializer; original 49720c..49721c. */
C2RegisterSet *global_loop_record_sets[4];

void __fastcall initialize_global_loop_block(C2BasicBlock *block)
{
    C2Expression *record;

    global_loop_record_sets[1] = block->rewrite_mode1[0];
    global_loop_record_sets[2] = block->rewrite_mode1[1];
    global_loop_record_sets[0] = block->rewrite_mode1[4];
    global_loop_record_sets[3] = block->rewrite_mode1[5];
    clear_register_set(global_loop_record_sets[1]);
    clear_register_set(global_loop_record_sets[2]);
    clear_register_set(global_loop_record_sets[0]);
    clear_register_set(global_loop_record_sets[3]);
    clear_register_set(block->rewrite_mode1[2]);
    clear_register_set(block->rewrite_mode1[3]);
    clear_register_set(block->rewrite_mode1[6]);
    clear_register_set(block->rewrite_mode1[7]);
    if (!block->previous)
        copy_optimizer_symbol_set_into(block->rewrite_mode1[3], global_initial_registers);
    for (record = ((C2Expression *)block->last_record)->previous;
         record != (C2Expression *)block->records; record = record->previous)
        if (record->flags & 1) collect_global_loop_record(record, block);
    intersect_optimizer_symbol_sets(global_loop_record_sets[0], global_rewrite_exclusions);
    block->flags |= 0x04000000U;
}

void __fastcall collect_global_loop_record(C2Expression *record, C2BasicBlock *block)
{
    C2Expression *operand;
    C2FunctionRegister *reg, *variant;
    C2InputSymbol *symbol;
    C2RegisterSet *direct, *variants;
    unsigned number, index;

    if ((global_rewrite_state && record->precision_flags == 4) ||
        record->precision_flags == 2 || record->precision_flags == 1) return;
    for (operand = record->tail.right; operand; operand = operand->next) {
        if (!operand->kind || operand->kind >= 3 ||
            (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value)) continue;
        reg = operand->value.reg;
        for (variant = reg->canonical; variant; variant = variant->variant_next) {
            if (reg->displacement <= variant->displacement &&
                (long)((unsigned)variant->displacement + variant->value_bytes) <=
                (long)((unsigned)reg->displacement + reg->value_bytes)) {
                number = variant->number;
                add_register_to_set(global_loop_record_sets[2], number);
                remove_optimizer_symbol(global_loop_record_sets[1], number);
                add_register_to_set(global_loop_record_sets[0], number);
                remove_optimizer_symbol(global_loop_record_sets[3], number);
            }
        }
        if (reg->global_expression && reg->global_expression->opcode == 0x122)
            mark_global_loop_dependents(reg);
    }
    for (operand = record->left; operand; operand = operand->next) {
        switch (operand->kind) {
        case 1: case 2:
            if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value) break;
            reg = operand->value.reg;
            number = reg->number;
            add_register_to_set(global_loop_record_sets[1], number);
            remove_optimizer_symbol(global_loop_record_sets[2], number);
            for (variant = reg->canonical; variant; variant = variant->variant_next) {
                if (variant == reg ||
                    (variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                     reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes))) {
                    number = variant->number;
                    remove_optimizer_symbol(global_loop_record_sets[0], number);
                    add_register_to_set(global_loop_record_sets[3], number);
                }
            }
            break;
        case 3:
            if (record->opcode != 0x130) break;
            reg = operand->value.reg;
            symbol = (C2InputSymbol *)reg->canonical->symbol;
            if (!symbol || symbol->symbol.kind != 1 || (symbol->attributes & 0x70) != 0x10 ||
                symbol->symbol.name || !(symbol->attributes & 0x40000)) break;
            for (variant = reg->canonical; variant; variant = variant->variant_next) {
                if (reg->displacement <= variant->displacement &&
                    (long)((unsigned)variant->displacement + variant->value_bytes) <=
                    (long)((unsigned)reg->displacement + reg->value_bytes)) {
                    number = variant->number;
                    add_register_to_set(global_loop_record_sets[2], number);
                    remove_optimizer_symbol(global_loop_record_sets[1], number);
                    add_register_to_set(global_loop_record_sets[0], number);
                    remove_optimizer_symbol(global_loop_record_sets[3], number);
                }
            }
            if (reg->global_expression && reg->global_expression->opcode == 0x122)
                mark_global_loop_dependents(reg);
            break;
        case 6: case 10:
            index = operand->kind == 6 ? operand->immediate : operand->value.optimizer_index;
            direct = global_index_direct_uses[index];
            variants = global_index_variant_uses[index];
            merge_optimizer_symbol_sets(global_loop_record_sets[1], direct);
            subtract_optimizer_symbol_sets(global_loop_record_sets[2], direct);
            merge_optimizer_symbol_sets(global_loop_record_sets[3], variants);
            subtract_optimizer_symbol_sets(global_loop_record_sets[0], variants);
            break;
        }
    }
}

void __fastcall solve_global_loop_sets(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    C2FlowEdge *edge;
    int changed;

    end = first->next;
    for (block = last; block != end; block = block->next) {
        if (!block->block_scope || (block->block_scope->flags & 1)) continue;
        for (edge = block->incoming; edge; edge = edge->next_incoming) {
            if ((short)block->number84 <= (short)edge->source->number84 &&
                edge->source->outgoing->next_outgoing) {
                copy_optimizer_symbol_set_into(block->rewrite_mode1[6], global_rewrite_exclusions);
                break;
            }
        }
    }
    do {
        changed = 0;
        for (block = last; block != end; block = block->next)
            update_global_loop_sets(block, &changed);
    } while (changed);
}

void __fastcall update_global_loop_sets(C2BasicBlock *block, int *changed)
{
    C2FlowEdge *edge;

    edge = block->outgoing;
    if (edge) {
        copy_optimizer_symbol_set_into(block->rewrite_mode1[3], edge->destination->rewrite_mode1[2]);
        copy_optimizer_symbol_set_into(block->rewrite_mode1[7], edge->destination->rewrite_mode1[6]);
        for (edge = edge->next_outgoing; edge; edge = edge->next_outgoing) {
            merge_optimizer_symbol_sets(block->rewrite_mode1[3], edge->destination->rewrite_mode1[2]);
            intersect_optimizer_symbol_sets(block->rewrite_mode1[7], edge->destination->rewrite_mode1[6]);
        }
    }
    update_available_register_transfer(block->rewrite_mode1[2], block->rewrite_mode1[0],
        block->rewrite_mode1[3], block->rewrite_mode1[1], changed);
    update_available_register_transfer(block->rewrite_mode1[6], block->rewrite_mode1[4],
        block->rewrite_mode1[7], block->rewrite_mode1[5], changed);
}

extern int global_rewrite_required;

void __fastcall prepare_global_cleanup_records(C2Function *function)
{
    C2RegisterBlock *register_block;
    C2FunctionRegister *reg;
    C2BasicBlock *block;
    C2Expression *record, *previous;
    C2RegisterSet *available, *work, *temporary;

    for (register_block = first_function_register_block; register_block;
         register_block = register_block->next) {
        for (reg = register_block->records; reg < register_block->cursor; ++reg) {
            if (reg->kind == 16) continue;
            reg->rewrite_seen = 0;
            reg->rewrite_queued = 0;
            reg->global_definitions = NULL;
            reg->global_uses = NULL;
        }
    }
    global_rewrite_required = 0;
    available = new_register_set(6);
    work = new_register_set(6);
    temporary = new_register_set(6);
    for (block = function->flow->first; block; block = block->next) {
        copy_optimizer_symbol_set_into(available, block->rewrite_mode1[3]);
        clear_register_set(temporary);
        record = ((C2Expression *)block->last_record)->previous;
        while (record != (C2Expression *)block->records) {
            previous = record->previous;
            if ((record->flags & 1) && record->kind != 20)
                rewrite_global_cleanup_record(record, available, work, temporary);
            record = previous;
        }
    }
}

void __fastcall restore_global_function(C2Function *function)
{
    C2Expression *record, *next, *last, *source, *result;

    last = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != last; record = next) {
        next = record->next;
        if (record->opcode == 0x133) {
            record->opcode = 0x131;
            source = record->left;
            if (source->kind == 7 || (source->location.expression.properties & 0x40)) continue;
        } else if (record->opcode == 0x138) {
            result = record->tail.right;
            source = record->left;
            if (GLOBAL_REGISTER_OPERAND(result) && result->value.reg->value &&
                GLOBAL_REGISTER_OPERAND(source) && source->value.reg->global_expression)
                record->opcode = 0x131;
        }
        if (record->opcode == 0x131 && !global_assignment_restored(record))
            restore_global_assignment(record);
    }
}

void release_global_optimizer(void)
{
    release_global_expression_cache();
    reset_input_pool(14);
}

void __fastcall fill_global_available_set(C2RegisterSet *set, unsigned count)
{
    C2RegisterSetNode *node = set->links.first, *previous = NULL;
    unsigned groups = (count + 31) >> 5;
    unsigned group, number = 0;

    for (group = 1; group < groups; ++group) {
        if (!node) node = insert_register_set_node(set, previous);
        node->group = number;
        number += 32;
        node->bits = ~0U;
        previous = node;
        node = node->next;
    }
    if (number < count) {
        if (!node) node = insert_register_set_node(set, previous);
        node->group = number;
        /* The original ORs the last partial group, retaining its other bits
         * and every later node. This is a fill operation, not a reset. */
        do { node->bits |= 1U << (number & 31); } while (++number < count);
    }
}

void __fastcall invalidate_global_available_register(C2FunctionRegister *reg,
    C2FunctionRegister *canonical)
{
    C2FunctionRegister *head, *variant;
    if (reg->global_invalidation_epoch == global_register_state) return;
    if (reg->global_dependency_set) {
        merge_optimizer_symbol_sets(global_available_uses, reg->global_dependency_set);
        merge_optimizer_symbol_sets(global_available_kills, reg->global_dependency_set);
        subtract_optimizer_symbol_sets(global_available_generations, reg->global_dependency_set);
        reg->global_invalidation_epoch = global_register_state;
    }
    head = reg->canonical;
    if (head == canonical) return;
    for (variant = head; variant; variant = variant->variant_next) {
        if (variant != reg &&
            variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
            reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes))
            invalidate_global_available_register(variant, head);
    }
}
