#include "global-optimizer.h"
#include "global-expressions.h"
#include "optimizer-translate.h"
#include "optimizer-sets.h"
#include "inlining.h"
#include "small-nodes.h"
#include "registers.h"
#include "symbol-storage.h"
#include <recon.h>

extern unsigned global_register_state;
extern int global_rewrite_state, c2_basic_mode, c2_global_optimize;
extern C2FunctionRegister *global_rewrite_registers;
extern C2Expression *global_next_record;

/* Exact DWORD owners used by the backward operand walk. */
int global_rewrite_required; /* 48fa40 */
int global_removed_register_use; /* 48fa5c */

/* The default path shares the exact default-matherr RET0 body.  This
 * zero-stack view consumes no register or exception object and adds no code. */
int __fastcall final_default_input_bits(void);

#define REGISTER_OPERAND(value) ((1U << ((value)->kind & 31)) & 6)
#define REGISTER_END(reg) ((long)((reg)->value_bytes + (reg)->displacement))
#define REGISTER_SEEN(reg, set) ((reg)->value ? (reg)->rewrite_seen : \
    optimizer_set_contains((set), (reg)->number))

void __fastcall rewrite_global_record(C2Expression *record,
    C2RegisterSet *inputs, C2RegisterSet *outputs)
{
    C2Expression *output, *operand, *scan, *candidate, *temporary, *inserted;
    C2FunctionRegister *reg, *variant, *work, *kept, *next, *saved_reg, *whole;
    C2OptimizerUse *use, *saved_use, *current;
    C2InputSymbol *symbol;
    unsigned opcode, number, live;
    int defined;
    unsigned char comparison;

    if (record->precision_flags == 2 || record->precision_flags == 1) return;
    ++global_register_state;
    output = record->tail.right;
    if (output) {
        if (record->opcode == 0x131 && !compare_input_operands(output, record->left) &&
            !input_has_side_effects(record)) goto remove_record;
        global_rewrite_required = 0;
        if (REGISTER_OPERAND(output)) {
            reg = output->value.reg;
            live = reg->value ? reg->rewrite_queued :
                optimizer_set_contains(outputs, reg->number);
            if (!live)
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg ||
                        (variant->displacement < REGISTER_END(reg) &&
                         reg->displacement < REGISTER_END(variant))) {
                        if (REGISTER_SEEN(variant, inputs)) {
                            global_rewrite_required = 1;
                            goto write_outputs;
                        }
                    }
            if (global_rewrite_state && record->opcode == 0x131 &&
                record->precision_flags == 4) {
                reg = record->left->value.reg;
                if (REGISTER_SEEN(reg, inputs)) goto write_outputs;
            }
            if (!input_has_side_effects(record)) goto remove_record;
            if (record->kind == 13 || record->kind == 17 || record->kind == 20 ||
                record->opcode == 0x13f) {
                if (!input_has_side_effects(output)) remove_input_result(record, output);
                else if (REGISTER_OPERAND(output) && output->value.reg->kind == 3) {
                    reg = output->value.reg;
                    if (!reg->canonical->variant_next) reg->value = NULL;
                    else bind_input_temporary(reg, NULL);
                    if (output->kind == 1) make_global_register_operand(output);
                }
            } else if (!input_has_side_effects(output) && record->kind == 11) {
                reg = new_temporary_register(output->type);
                reg->value = record;
                remove_input_result(record, output);
                temporary = new_temporary_register_expression(0x11f, reg->type, reg, reg);
                attach_optimizer_output(record, temporary);
                inserted = new_input_expression(0x16c, reg->type);
                copy_input_operand(inserted, temporary);
                insert_input_record((C2Record *)record, (C2Record *)inserted);
                if (record->opcode == 0x151) record->opcode = 0x142;
            }
        } else if (output->kind == 6 && output->value.reg &&
                   output->value.reg->rewrite_queued) {
            /* Find the first subsequent aggregate definition before testing
             * the intervening records. The initial candidate is the actual
             * carried outputs pointer, even if the scan has no iterations. */
            candidate = (C2Expression *)outputs;
            for (scan = record->next; scan; scan = scan->next) {
                if (!(scan->flags & 1)) continue;
                for (candidate = scan->tail.right; candidate; candidate = candidate->next)
                    if (candidate->kind == 6 && !compare_input_operands(candidate, output))
                        goto found_definition;
            }
found_definition:
            if (!global_range_uses_operand(record->next, scan, candidate) &&
                !input_has_side_effects(record)) goto remove_record;
        }
    }

write_outputs:
    work = global_rewrite_registers;
    for (operand = record->tail.right; operand; operand = operand->next) {
        if (operand->kind && operand->kind < 3) {
            reg = operand->value.reg;
            defined = reg->value != NULL;
            live = defined ? reg->rewrite_seen : optimizer_set_contains(inputs, reg->number);
            if (!defined) work = global_rewrite_registers;
            if (!live && !global_rewrite_required)
                operand->location.expression.comparison |= 0x10;
            if (!defined) {
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (reg->displacement <= variant->displacement &&
                        REGISTER_END(variant) <= REGISTER_END(reg)) {
                        remove_optimizer_symbol(inputs, variant->number);
                        add_register_to_set(outputs, variant->number);
                        work = global_rewrite_registers;
                    }
            } else {
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (reg->displacement <= variant->displacement &&
                        REGISTER_END(variant) <= REGISTER_END(reg)) {
                        variant->rewrite_seen = 0;
                        variant->rewrite_queued = 1;
                    }
            }
            if (work && record->opcode == 0x131) {
                if (!reg) {
                    while (work) {
                        global_rewrite_registers = work->next_free;
                        work->rewrite_queued = 0;
                        work = global_rewrite_registers;
                    }
                } else {
                    prune_global_rewrite_registers(reg);
                    kept = NULL;
                    while ((reg = global_rewrite_registers) != NULL) {
                        global_rewrite_registers = reg->next_free;
                        if (reg->rewrite_queued) {
                            reg->next_free = kept;
                            kept = reg;
                        }
                    }
                    global_rewrite_registers = kept;
                    work = kept;
                }
            }
        } else if (operand->kind == 6 && (reg = operand->value.reg) != NULL &&
                   !reg->rewrite_queued) {
            reg->next_free = work;
            reg->rewrite_queued = 1;
            global_rewrite_registers = work = reg;
        }
    }

    opcode = record->opcode;
    operand = record->tail.right;
    if (opcode == 0x160 && operand && REGISTER_OPERAND(operand) &&
        (operand->type & 0xf000) == 0x5000 && operand->next &&
        REGISTER_OPERAND(operand->next) && (operand->next->type & 0xf000) == 0x5000) {
        saved_reg = operand->value.reg;
        reg = operand->next->value.reg;
        whole = saved_reg->canonical;
        if (whole == reg->canonical && saved_reg->displacement == 0 &&
            saved_reg->value_bytes == 4 && reg->displacement == 4 &&
            reg->value_bytes == 4 && whole->value_bytes == 8) {
            comparison = operand->location.expression.comparison & ~0x10;
            operand->value.reg = whole;
            operand->location.expression.comparison = comparison;
            defined = whole->value != NULL;
            live = defined ? whole->rewrite_seen : optimizer_set_contains(inputs, whole->number);
            if (!live && !global_rewrite_required)
                operand->location.expression.comparison = comparison | 0x10;
            if (!defined) {
                for (variant = whole->canonical; variant; variant = variant->variant_next)
                    if (whole->displacement <= variant->displacement &&
                        REGISTER_END(variant) <= REGISTER_END(whole)) {
                        remove_optimizer_symbol(inputs, variant->number);
                        add_register_to_set(outputs, variant->number);
                    }
            } else {
                for (variant = whole->canonical; variant; variant = variant->variant_next)
                    if (whole->displacement <= variant->displacement &&
                        REGISTER_END(variant) <= REGISTER_END(whole)) {
                        variant->rewrite_seen = 0;
                        variant->rewrite_queued = 1;
                    }
            }
            operand->value.reg = saved_reg;
        }
    }

    for (operand = record->left; operand; operand = operand->next) {
        switch (operand->kind) {
        case 1: case 2:
            reg = operand->value.reg;
            if (!reg->value) {
                if (!optimizer_set_contains(inputs, reg->number)) {
                    operand->location.expression.comparison |= 0x10;
                    add_register_to_set(inputs, reg->number);
                }
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg ||
                        (variant->displacement < REGISTER_END(reg) &&
                         reg->displacement < REGISTER_END(variant)))
                        remove_optimizer_symbol(outputs, variant->number);
            } else {
                if (!reg->rewrite_seen) {
                    reg->rewrite_seen = 1;
                    operand->location.expression.comparison |= 0x10;
                }
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg ||
                        (variant->displacement < REGISTER_END(reg) &&
                         reg->displacement < REGISTER_END(variant)))
                        variant->rewrite_queued = 0;
            }
            break;
        case 3:
            if (opcode != 0x130) break;
            reg = operand->value.reg;
            symbol = (C2InputSymbol *)reg->canonical->symbol;
            if (symbol && symbol->symbol.kind == 1 &&
                (symbol->attributes & 0x70) == 0x10 && !symbol->symbol.name &&
                (symbol->attributes & 0x40000)) {
                defined = reg->value != NULL;
                live = defined ? reg->rewrite_seen : optimizer_set_contains(inputs, reg->number);
                if (!live && !global_rewrite_required)
                    operand->location.expression.comparison |= 0x10;
                if (!defined) {
                    for (variant = reg->canonical; variant; variant = variant->variant_next)
                        if (reg->displacement <= variant->displacement &&
                            REGISTER_END(variant) <= REGISTER_END(reg)) {
                            remove_optimizer_symbol(inputs, variant->number);
                            add_register_to_set(outputs, variant->number);
                        }
                } else {
                    for (variant = reg->canonical; variant; variant = variant->variant_next)
                        if (reg->displacement <= variant->displacement &&
                            REGISTER_END(variant) <= REGISTER_END(reg)) {
                            variant->rewrite_seen = 0;
                            variant->rewrite_queued = 1;
                        }
                }
            }
            break;
        case 6:
            number = operand->immediate;
            use = get_global_index_uses(number);
            saved_use = NULL;
            while (use) {
                reg = use->reg;
                current = use;
                if (reg->kind == 12) {
                    saved_use = use;
                    current = reg->optimizer_uses;
                    if (!current) {
                        use = use->next;
                        continue;
                    }
                    reg = current->reg;
                }
                if (!REGISTER_SEEN(reg, inputs)) add_register_to_set(inputs, reg->number);
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    remove_optimizer_symbol(outputs, variant->number);
                use = current->next;
                if (!use && saved_use) {
                    use = saved_use->next;
                    saved_use = NULL;
                }
            }
            if (!number) {
                while ((reg = global_rewrite_registers) != NULL) {
                    reg->rewrite_queued = 0;
                    global_rewrite_registers = reg->next_free;
                }
            } else {
                kept = NULL;
                for (reg = global_rewrite_registers; reg; reg = next) {
                    next = reg->next_free;
                    if (!optimizer_indices_overlap(number,
                        reg->global_expression->operands->next->immediate)) {
                        reg->next_free = kept;
                        kept = reg;
                    } else reg->rewrite_queued = 0;
                }
                global_rewrite_registers = kept;
            }
            break;
        case 9:
            c2_report_internal("E:\\utc\\src\\\\P2\\globdf.c", 0xb80);
            break;
        case 10:
            use = get_global_index_uses(operand->value.optimizer_index);
            saved_use = NULL;
            while (use) {
                reg = use->reg;
                current = use;
                if (reg->kind == 12) {
                    saved_use = use;
                    current = reg->optimizer_uses;
                    if (!current) {
                        use = use->next;
                        continue;
                    }
                    reg = current->reg;
                }
                if (!REGISTER_SEEN(reg, inputs)) add_register_to_set(inputs, reg->number);
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    remove_optimizer_symbol(outputs, variant->number);
                use = current->next;
                if (!use && saved_use) {
                    use = saved_use->next;
                    saved_use = NULL;
                }
            }
            while ((reg = global_rewrite_registers) != NULL) {
                reg->rewrite_queued = 0;
                global_rewrite_registers = reg->next_free;
            }
            break;
        }
    }
    return;

remove_record:
    if (record->precision_flags != 4)
        for (operand = record->left; operand; operand = operand->next)
            if (REGISTER_OPERAND(operand) && (operand->value.reg->rewrite_flags & 1))
                global_removed_register_use = 1;
    remove_global_record(record);
}

C2Expression *__fastcall next_global_code_record(C2Expression *record)
{
    for (record = record->next; record; record = record->next) {
        if (record->flags & 1) return record;
        if (record->kind == 24) return NULL;
    }
    return NULL;
}

void __fastcall collect_global_record_sets(C2Expression *record)
{
    C2Expression *operand;
    C2FunctionRegister *reg, *variant;
    C2OptimizerUse *use, *saved, *current;
    unsigned number;
    int used;

    if (record->precision_flags == 2 || record->precision_flags == 1) return;
    for (operand = record->tail.right; operand; operand = operand->next) {
        if (!operand->kind || operand->kind >= 3) continue;
        reg = operand->value.reg;
        if (REGISTER_OPERAND(operand) && reg->value) continue;
        if (!input_has_side_effects(operand) && record->opcode == 0x131) {
            if (!optimizer_set_contains(global_flow_uses, reg->number)) {
                record->precision_flags = 8;
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg ||
                        (variant->displacement < REGISTER_END(reg) &&
                         reg->displacement < REGISTER_END(variant))) {
                        if (optimizer_set_contains(global_flow_temporary, variant->number)) {
                            record->precision_flags = 0;
                            break;
                        }
                    }
            } else record->precision_flags = 7;
        }
        for (variant = reg->canonical; variant; variant = variant->variant_next)
            if (reg->displacement <= variant->displacement &&
                REGISTER_END(variant) <= REGISTER_END(reg)) {
                number = variant->number;
                remove_optimizer_symbol(global_flow_temporary, number);
                add_register_to_set(global_flow_kills, number);
                remove_optimizer_symbol(global_flow_uses, number);
            }
        if (reg->global_expression && reg->global_expression->opcode == 0x122)
            collect_global_definition_sets(reg);
    }
    for (operand = record->left; operand; operand = operand->next) {
        switch (operand->kind) {
        case 1: case 2:
            reg = operand->value.reg;
            if (reg->value) break;
            number = reg->number;
            used = 0;
            for (variant = reg->canonical; variant; variant = variant->variant_next)
                if (variant == reg ||
                    (variant->displacement < REGISTER_END(reg) &&
                     reg->displacement < REGISTER_END(variant))) {
                    if (optimizer_set_contains(global_flow_temporary, variant->number)) {
                        remove_optimizer_symbol(global_flow_uses, number);
                        used = 1;
                        break;
                    }
                }
            if (!used) add_register_to_set(global_flow_uses, number);
            add_register_to_set(global_flow_temporary, number);
            for (variant = reg->canonical; variant; variant = variant->variant_next)
                if (variant != reg && variant->displacement < REGISTER_END(reg) &&
                    reg->displacement < REGISTER_END(variant)) {
                    number = variant->number;
                    add_register_to_set(global_flow_kills, number);
                    remove_optimizer_symbol(global_flow_uses, number);
                }
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
                number = reg->number;
                add_register_to_set(global_flow_kills, number);
                remove_optimizer_symbol(global_flow_uses, number);
                add_register_to_set(global_flow_temporary, number);
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant != reg && variant->displacement < REGISTER_END(reg) &&
                        reg->displacement < REGISTER_END(variant)) {
                        number = variant->number;
                        add_register_to_set(global_flow_kills, number);
                        remove_optimizer_symbol(global_flow_uses, number);
                    }
                use = current->next;
                if (!use && saved) {
                    use = saved->next;
                    saved = NULL;
                }
            }
            break;
        case 10:
            use = get_global_index_uses(operand->value.optimizer_index);
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
                number = reg->number;
                add_register_to_set(global_flow_kills, number);
                remove_optimizer_symbol(global_flow_uses, number);
                add_register_to_set(global_flow_temporary, number);
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant != reg && variant->displacement < REGISTER_END(reg) &&
                        reg->displacement < REGISTER_END(variant)) {
                        number = variant->number;
                        add_register_to_set(global_flow_kills, number);
                        remove_optimizer_symbol(global_flow_uses, number);
                    }
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

/* The seven original classifiers select the same operand view at distinct
 * sites. Expanding it here preserves those sites without helper vertices. */
#define GLOBAL_VALUE(value) ((value) && (value)->kind >= 1 && (value)->kind <= 8 && \
    ((value)->kind >= 3 || !((value)->location.expression.properties & 0x20)) ? (value) : NULL)

void __fastcall prepare_global_expressions(C2Function *function)
{
    C2Expression *first, *record, *operand, *constant, *replacement, *owner, *result;
    C2Expression *left, *right, *third, *fourth, *type_constant, *intrinsic_constant;
    C2Expression *definition;
    C2FunctionRegister *reg;
    unsigned short type;

    first = (C2Expression *)function->flow->last->records;
    for (record = first; record; record = record->next) {
        if (record->flags & 1) {
            for (operand = record->left; operand; operand = operand->next)
                if ((1U << (operand->kind & 31)) & 0x60) {
                    constant = new_integer_expression(0x1004, (unsigned __int64)operand->immediate);
                    reg = new_global_expression_register(operand->opcode, operand->type,
                        operand->aggregate_value, constant);
                    if (!(constant->location.expression.properties & 0x10))
                        release_small_node(constant);
                    operand->value.reg = reg;
                }
            for (operand = record->tail.right; operand; operand = operand->next)
                if ((1U << (operand->kind & 31)) & 0x60) {
                    constant = new_integer_expression(0x1004, (unsigned __int64)operand->immediate);
                    reg = new_global_expression_register(operand->opcode, operand->type,
                        operand->aggregate_value, constant);
                    if (!(constant->location.expression.properties & 0x10))
                        release_small_node(constant);
                    operand->value.reg = reg;
                }
        }
        switch (record->kind) {
        case 11:
            if (record->opcode == 0x130) break;
            if (record->opcode == 0x131) {
                operand = record->tail.right;
                if (!REGISTER_OPERAND(operand) || !operand->value.reg->value) break;
                owner = follow_optimizer_result(record, &result);
                left = GLOBAL_VALUE(record->left);
                replacement = new_global_value_expression(0x13b, record->type, left, NULL);
                reg = record->tail.right->value.reg;
                replace_input_result(record, record->tail.right, replacement);
                type = result->type;
                operand = attach_input_expression(owner, result, replacement);
                set_input_expression_type(operand, type);
                release_function_register(reg->canonical);
            } else if (record->opcode == 0x151) prepare_global_comparison(record);
            else if (!input_has_side_effects(record)) {
                left = GLOBAL_VALUE(record->left);
                right = left ? GLOBAL_VALUE(record->left->next) : NULL;
                replacement = new_global_value_expression(record->opcode, record->type, left, right);
                replace_global_expression(replacement, record);
            }
            break;
        case 12: case 13: case 20:
detach_result:
            operand = record->tail.right;
            if (operand && REGISTER_OPERAND(operand) && operand->value.reg->value) {
                follow_optimizer_result(record, &result);
                if (result->location.expression.properties & 0x20) detach_global_result(record);
            }
            break;
        case 15:
            definition = record->left->value.reg->value;
            operand = definition->left;
            left = GLOBAL_VALUE(operand);
            right = GLOBAL_VALUE(operand->next);
            operand = record->left->next;
            third = GLOBAL_VALUE(operand);
            fourth = GLOBAL_VALUE(third->next);
            type = record->type;
            type_constant = new_integer_expression(0x1004, (unsigned __int64)definition->type);
            intrinsic_constant = new_integer_expression(0x1004,
                (unsigned __int64)(unsigned char)record->detail.intrinsic);
            reg = new_global_call_register(0x15e, type, 6, left, right, third, fourth,
                intrinsic_constant, type_constant);
            replacement = new_typed_register_expression(0x120, type, reg);
            replace_global_expression(replacement, record);
            break;
        case 17:
            if (!(intrinsics[record->detail.intrinsic].flags & 0x20) ||
                (c2_basic_mode && (record->type & 0xf000) == 0x4000)) goto detach_result;
            reg = new_global_intrinsic_register(record);
            replacement = new_typed_register_expression(0x120, record->type, reg);
            replace_global_expression(replacement, record);
            break;
        }
    }
    for (record = first; record; record = record->next)
        if (record->opcode == 0x131) finish_global_assignment(record, 1);
    finish_global_expression_cache();
}

void __fastcall finish_global_record_sets(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    C2Expression *record, *output;
    C2FunctionRegister *reg, *variant;
    C2RegisterSet *seen;
    unsigned number;

    seen = new_register_set(6);
    end = last->previous;
    for (block = first; block != end; block = block->previous) {
        clear_register_set(seen);
        for (record = ((C2Expression *)block->last_record)->previous;
             record != (C2Expression *)block->records; record = record->previous) {
            if (record->opcode != 0x131) continue;
            output = record->tail.right;
            if (!output->kind || output->kind >= 3 ||
                (REGISTER_OPERAND(output) && output->value.reg->value)) continue;
            reg = output->value.reg;
            number = reg->number;
            if (test_and_add_global_register(seen, number)) continue;
            if (record->precision_flags == 7) {
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg ||
                        (variant->displacement < REGISTER_END(reg) &&
                         reg->displacement < REGISTER_END(variant))) {
                        if (optimizer_set_contains(block->rewrite_mode1[3], variant->number)) {
                            record->precision_flags = 0;
                            break;
                        }
                    }
            } else if (record->precision_flags == 8 &&
                (!optimizer_set_contains(block->rewrite_mode2[4], number) ||
                 !optimizer_set_contains(block->rewrite_mode1[3], number)))
                record->precision_flags = 0;
        }
    }
    release_optimizer_symbol_set(seen);
}

C2Expression *__fastcall new_global_value_expression(unsigned opcode,
    unsigned short type, C2Expression *left, C2Expression *right)
{
    C2FunctionRegister *reg;

    reg = new_global_expression_register(opcode, type, left, right);
    return new_typed_register_expression(0x120, type, reg);
}

void __fastcall retarget_global_operand(C2Expression *operand, unsigned short type)
{
    set_input_expression_type(operand, type);
}

unsigned char __fastcall global_expression_condition(C2Expression *record)
{
    C2Expression *consumer, *operand;
    consumer = follow_optimizer_result(record, &operand);
    if (consumer->kind == 15) return consumer->detail.condition;
    return consumer->convention;
}

unsigned __fastcall global_condition_opcode(unsigned char condition)
{
    switch (condition) {
    case 1: return 0x152;
    case 2: return 0x153;
    case 3: return 0x154;
    case 4: return 0x155;
    case 5: return 0x156;
    case 6: return 0x157;
    default: return 0x11c;
    }
}

void __fastcall prepare_global_comparison(C2Expression *record)
{
    C2Expression *operand, *consumer;
    consumer = find_register_use(record, record->tail.right->value.reg, &operand);
    if (consumer && consumer->kind == 16)
        record->value.reg = new_global_expression_register(
            global_condition_opcode(consumer->convention), record->type,
            record->left, record->left->next);
}

C2FunctionRegister *__fastcall new_global_expression_register(unsigned opcode,
    unsigned short type, C2Expression *left, C2Expression *right)
{
    set_global_expression_operands(2, left, right);
    return lookup_global_expression_register(opcode, type);
}

void __fastcall replace_global_expression(C2Expression *replacement,
    C2Expression *record)
{
    C2Expression *result, *owner, *operand, *attached;
    C2FunctionRegister *canonical;
    unsigned short type;

    result = record->tail.right;
    if (REGISTER_OPERAND(result) && result->value.reg->value) {
        canonical = result->value.reg->canonical;
        owner = follow_optimizer_result(record, &operand);
        replace_input_result(record, result, replacement);
        type = operand->type;
        attached = attach_input_expression(owner, operand, replacement);
        if (type != 0x8000) set_input_expression_type(attached, type);
        release_function_register(canonical);
    } else {
        new_binary_expression(0x131, record->type, result, replacement,
            insert_input_record, (C2Record *)record);
        replace_input_result(record, result, replacement);
    }
}

void __fastcall prepare_global_assignments(C2Function *function)
{
    C2Expression *record;
    C2BasicBlock *block;

    /* The incoming context is retained until the first statement marker,
     * whose existing owner field supplies the actual basic block. */
    block = (C2BasicBlock *)function;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (record->kind == 24) block = ((C2Record *)record)->owner.block;
        if (record->opcode == 0x151 || record->opcode == 0x15d)
            optimize_global_assignment(record, block);
    }
}

int __fastcall global_record_needs_rewrite(C2Expression *record)
{
    C2Expression *operand, *next, *replacement, *attached, *first, *selected;
    C2FunctionRegister *reg, *source;
    C2GlobalExpression *expression;
    unsigned short type;
    unsigned width, properties, intrinsic;
    long displacement;
    int changed, preserve_flag;

    changed = 0;
    for (operand = record->left; operand; operand = next) {
        next = operand->next;
        replacement = find_available_global_operand(record, operand);
        if (!replacement) continue;
        if (!(operand->location.expression.properties & 0x20) || replacement->kind != 3) {
            preserve_flag = 0;
            if (REGISTER_OPERAND(operand)) {
                reg = operand->value.reg;
                if (reg->kind == 3 && !reg->value && (reg->canonical->flags & 0x40))
                    preserve_flag = 1;
            }
            type = operand->type;
            if (replacement->kind == 7) replacement = truncate_global_integer(type, replacement);
            attached = attach_input_expression(record, operand, replacement);
            attached->type = type;
            if (REGISTER_OPERAND(attached)) {
                reg = attached->value.reg;
                if (reg->kind == 3 && !reg->value && preserve_flag)
                    reg->canonical->flags |= 0x40;
            }
            if (!(attached->location.expression.properties & 0x20)) next = attached->next;
            else {
                prepare_global_operand_links(record);
                next = record->left;
            }
        } else {
            operand = find_aggregate_operand_owner(record, operand, &first);
            properties = operand->location.expression.properties;
            type = operand->type;
            if (operand->kind == 6) {
                mark_first_input_expression(replacement);
                source = replacement->value.reg->canonical;
                displacement = (long)((unsigned)replacement->value.reg->displacement - (unsigned)source->displacement);
                width = type & 0xfff;
                if (!width) width = source->value_bytes - (unsigned)displacement;
                replacement->value.reg = new_address_register(source, type, width, displacement);
            }
            replacement->type = type;
            replacement->location.expression.properties =
                (replacement->location.expression.properties & ~0x40) | (properties & 0x40);
            if (first == record->left) {
                attach_input_expression(record, operand, replacement);
                next = record->left;
            } else replace_input_result(record, operand, replacement);
        }
        changed = 1;
        if (record->opcode == 0x133 && record->left->kind != 6) {
            record->opcode = 0x131;
            if (REGISTER_OPERAND(record->left)) {
                reg = record->left->value.reg;
                source = record->tail.right->value.reg;
                expression = source->global_expression;
                if (source != reg && expression && expression->operands->kind == 6)
                    merge_global_register_identity(reg, expression->operands->value.reg);
            }
        }
    }
    if (changed && record->opcode == 0x130) {
        selected = follow_optimizer_result(record, &attached);
        if (selected->kind == 17) {
            intrinsic = selected->detail.intrinsic;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
            if (intrinsic >= C2_INTRINSIC_COUNT)
                recon_unsupported("unmodeled global intrinsic row");
#endif
            if ((intrinsics[intrinsic].flags & 0x20) &&
                (!c2_basic_mode || (selected->type & 0xf000) != 0x4000))
                global_pending_assignment = selected;
        }
    }
    return changed;
}

extern int global_region_limited;

void __fastcall update_global_available_registers(C2Expression *record)
{
    C2Expression *operand, *output;
    C2FunctionRegister *reg, *dependent, *canonical, *variant, *saved;
    C2GlobalDependency *link;
    C2InputSymbol *symbol;
    int was_available, now_available, keep_assignment;

    ++global_register_state;
    clear_register_set(global_expression_registers);
    saved = NULL;
    if (record->opcode == 0x131 && record->tail.right->kind == 6) {
        operand = record->tail.right->aggregate_value;
        if (!REGISTER_OPERAND(operand) || !operand->value.reg->value) {
            saved = operand->value.reg;
            was_available = !saved->value &&
                optimizer_set_contains(global_available_registers, saved->number) != 0;
            mark_global_register_available(saved);
        }
    }
    for (output = record->tail.right; output; output = output->next) {
        switch (output->kind) {
        case 1: case 2:
            if (record->precision_flags != 1) {
                reg = output->value.reg;
                if (!reg->global_expression || (reg->rewrite_flags & 1) || record->precision_flags == 14)
                    invalidate_global_register(reg, NULL);
                else {
                    for (link = reg->global_dependents; link; link = link->next_dependent) {
                        dependent = link->dependent;
                        if (dependent->global_expression &&
                            (dependent->global_expression->opcode == 0x131 ||
                             operation_is_comparison(dependent->global_expression->opcode))) {
                            remove_optimizer_symbol(global_available_registers, dependent->number);
                            add_register_to_set(global_expression_registers, dependent->number);
                        }
                    }
                    canonical = reg->canonical;
                    for (variant = canonical; variant; variant = variant->variant_next) {
                        if (variant != reg &&
                            variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                            reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes))
                            invalidate_global_assignment_uses(variant, canonical);
                    }
                }
                reg = output->value.reg;
                if (reg->kind == 7 || reg->kind == 8 || (reg->view_flags & 2)) {
                    symbol = (C2InputSymbol *)reg->canonical->symbol;
                    subtract_optimizer_symbol_sets(global_available_registers,
                        global_symbol_registers[symbol->detail.optimizer_number]);
                }
            }
            break;
        case 6:
            subtract_optimizer_symbol_sets(global_available_registers,
                global_index_registers[output->immediate]);
            mark_global_register_available(output->value.reg);
            break;
        case 9:
            c2_report_internal("E:\\utc\\src\\\\P2\\globopt.c", 0x1ccd);
            break;
        case 10:
            subtract_optimizer_symbol_sets(global_available_registers,
                global_index_registers[output->value.optimizer_index]);
            break;
        }
    }
    output = record->tail.right;
    keep_assignment = 1;
    if (output) {
        if (!REGISTER_OPERAND(output) || !output->value.reg->global_expression) {
            if (saved) {
                now_available = !saved->value &&
                    optimizer_set_contains(global_available_registers, saved->number) != 0;
                if (!now_available) keep_assignment = 0;
                if (!was_available && now_available) {
                    remove_optimizer_symbol(global_available_registers, saved->number);
                    add_register_to_set(global_expression_registers, saved->number);
                }
            }
        } else if (record->precision_flags == 1) {
            operand = record->left;
            if (REGISTER_OPERAND(operand)) {
                reg = operand->value.reg;
                if (reg->global_expression && !reg->value &&
                    optimizer_set_contains(global_available_registers, reg->number))
                    mark_global_register_available(output->value.reg);
            }
        } else mark_global_register_available(output->value.reg);
    }
    if (record->opcode == 0x131 && keep_assignment && record->value.reg)
        mark_global_register_available(record->value.reg);
    if (record->kind == 19 || record->opcode == 0x15b) {
        switch (record->opcode) {
        case 0x15b: case 0x163: case 0x164: case 0x165:
        case 0x167: case 0x169: case 0x172:
            if (global_region_limited) clear_register_set(global_available_registers);
            else subtract_optimizer_symbol_sets(global_available_registers, global_pending_expressions);
            break;
        }
    }
}

void __fastcall invalidate_global_register(C2FunctionRegister *reg,
    C2FunctionRegister *canonical)
{
    C2FunctionRegister *head, *variant;

    if (reg->global_invalidation_epoch == global_register_state) return;
    if (reg->global_dependency_set) {
        subtract_optimizer_symbol_sets(global_available_registers, reg->global_dependency_set);
        reg->global_invalidation_epoch = global_register_state;
    }
    head = reg->canonical;
    if (head == canonical) return;
    for (variant = head; variant; variant = variant->variant_next) {
        if (variant != reg &&
            variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
            reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes))
            invalidate_global_register(variant, head);
    }
}

void __fastcall remove_global_record(C2Expression *record)
{
    if (record == global_pending_assignment) global_pending_assignment = NULL;
    if (record->opcode == 0x131 && record->value.reg)
        record->value.reg->global_definition_record = NULL;
    unlink_and_release_input_record((C2Record *)record);
}

void __fastcall update_global_record_registers(C2Expression *record, int mode)
{
    C2Expression *selected, *operand, *result, *first, *second, *third, *fourth;
    C2Expression *type_constant, *condition_constant;
    C2FunctionRegister *reg;
    unsigned opcode, intrinsic;
    unsigned short type;

    prepare_global_operand_links(record);
    switch (record->kind) {
    case 11:
        opcode = record->opcode;
        if (opcode == 0x130) break;
        if (opcode == 0x131) {
            finish_global_assignment(record, 1);
            return;
        }
        if (opcode == 0x151) {
            if (record->value.reg) {
                opcode = global_condition_opcode(global_expression_condition(record));
                record->value.reg = new_global_expression_register(opcode, record->type,
                    record->left, record->left->next);
                return;
            }
            selected = follow_optimizer_result(record, &operand);
            if (!input_has_side_effects(record) && !input_has_side_effects(selected)) {
                opcode = global_condition_opcode(selected->detail.condition);
                reg = new_global_expression_register(opcode, record->type,
                    record->left, record->left->next);
                type = selected->type;
                opcode = selected->opcode;
                selected->value.reg = reg;
                first = get_global_value_operand(record->left);
                second = get_global_value_operand(record->left->next);
                third = get_global_value_operand(selected->left->next);
                fourth = get_global_value_operand(selected->left->next->next);
                type_constant = new_integer_expression(0x1004, record->type);
                condition_constant = new_integer_expression(0x1004, selected->detail.condition);
                reg = new_global_call_register(opcode, type, 6, first, second, third,
                    fourth, condition_constant, type_constant);
                bind_global_record_register(reg, selected, type, selected);
                return;
            }
        } else if (!input_has_side_effects(record)) {
            result = record->tail.right;
            if (!REGISTER_OPERAND(result) || !result->value.reg->value || mode) {
                type = result->type;
                first = get_global_value_operand(record->left);
                second = first ? get_global_value_operand(record->left->next) : NULL;
                reg = new_global_expression_register(opcode, type, first, second);
                bind_global_record_register(reg, record, type, NULL);
            }
        }
        break;
    case 17:
        intrinsic = record->detail.intrinsic;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
        if (intrinsic >= C2_INTRINSIC_COUNT)
            recon_unsupported("unmodeled global register intrinsic row");
#endif
        if ((intrinsics[intrinsic].flags & 0x20) && record->tail.right &&
            !input_has_side_effects(record->tail.right) &&
            (!c2_basic_mode || (record->type & 0xf000) != 0x4000)) {
            reg = new_global_intrinsic_register(record);
            bind_global_record_register(reg, record, record->type, NULL);
        }
        /* Fall through to aggregate-result ownership. */
    case 12: case 13: case 20:
        result = record->tail.right;
        if (result && REGISTER_OPERAND(result) && result->value.reg->value) {
            follow_optimizer_result(record, &operand);
            if (operand->location.expression.properties & 0x20) {
                detach_global_result(record);
                return;
            }
        }
        break;
    }
}

void __fastcall prepare_global_operand_links(C2Expression *record)
{
    C2Expression *operand, *constant;
    C2FunctionRegister *reg;

    for (operand = record->left; operand; operand = operand->next) {
        if (!((1U << (operand->kind & 31)) & 0x60)) continue;
        constant = new_integer_expression(0x1004, (unsigned __int64)operand->immediate);
        reg = new_global_expression_register(operand->opcode, operand->type,
            operand->aggregate_value, constant);
        if (!(constant->location.expression.properties & 0x10)) release_small_node(constant);
        if (operand->value.reg != reg) {
            operand->value.reg = reg;
            merge_global_register_identity(operand->aggregate_value->value.reg, reg);
        }
    }
    for (operand = record->tail.right; operand; operand = operand->next) {
        if (!((1U << (operand->kind & 31)) & 0x60)) continue;
        constant = new_integer_expression(0x1004, (unsigned __int64)operand->immediate);
        reg = new_global_expression_register(operand->opcode, operand->type,
            operand->aggregate_value, constant);
        if (!(constant->location.expression.properties & 0x10)) release_small_node(constant);
        if (operand->value.reg != reg) {
            operand->value.reg = reg;
            merge_global_register_identity(operand->aggregate_value->value.reg, reg);
        }
    }
}

C2Expression *__fastcall get_global_value_operand(C2Expression *operand)
{
    if (operand) {
        switch (operand->kind) {
        case 1: case 2:
            if (!(operand->location.expression.properties & 0x20)) return operand;
            break;
        case 3: case 4: case 5: case 6: case 7: case 8:
            return operand;
        }
    }
    return NULL;
}

void __fastcall bind_global_record_register(C2FunctionRegister *reg,
    C2Expression *record, unsigned short type, C2Expression *prior)
{
    C2Expression *output, *temporary, *selected, *operand, *inserted;
    C2FunctionRegister *canonical;
    unsigned short operand_type;

    output = record->tail.right;
    link_global_expression_dependencies(reg);
    if (output->value.reg == reg) return;
    temporary = new_typed_register_expression(0x120, type, reg);
    if (REGISTER_OPERAND(output) && output->value.reg->value) {
        canonical = output->value.reg->canonical;
        selected = follow_optimizer_result(record, &operand);
        replace_input_result(record, output, temporary);
        operand_type = operand->type;
        operand = attach_input_expression(selected, operand, temporary);
        retarget_global_operand(operand, operand_type);
        release_function_register(canonical);
        return;
    }
    temporary = new_typed_register_expression(0x120, type, reg);
    inserted = new_binary_expression(0x131, type, output, temporary,
        insert_input_record, (C2Record *)record);
    replace_input_result(record, output, temporary);
    if (operation_is_comparison(record->opcode)) inserted->precision_flags = 1;
    if (!prior) global_next_record = inserted;
    output = inserted->tail.right;
    if (REGISTER_OPERAND(output) && output->value.reg->global_expression &&
        output->value.reg->global_dependency_set &&
        optimizer_set_contains(output->value.reg->global_dependency_set, reg->number)) {
        if (find_register_input(record, output->value.reg)) inserted->precision_flags = 14;
    }
    finish_global_assignment(inserted, 1);
}

C2Expression *__fastcall find_next_global_operation(C2Expression *record)
{
    for (record = record->next; record; record = record->next) {
        if (record->kind == 11) return record;
        if (record->kind == 24) break;
    }
    return NULL;
}

int __fastcall global_register_is_available(C2FunctionRegister *reg)
{
    return !reg->value && optimizer_set_contains(global_available_registers, reg->number) != 0;
}

void __fastcall substitute_available_global_operands(C2Expression *record)
{
    C2Expression *output, *source, *first, *second, *old_value, *new_value, *inserted;
    C2GlobalExpression *expression;
    C2GlobalDependency *link;
    C2FunctionRegister *dependent, *replacement;
    unsigned opcode;
    unsigned short type;
    int changed;

    if (record->flags & 0x20) return;
    type = record->type & 0xf000;
    if (type == 0x4000 || type == 0x5000) return;
    output = record->tail.right;
    if (record->opcode == 0x131) {
        if (!REGISTER_OPERAND(output) || output->value.reg->value) return;
        source = record->left;
        if (!REGISTER_OPERAND(source)) return;
        for (link = source->value.reg->global_dependents; link; link = link->next_dependent) {
            dependent = link->dependent;
            expression = dependent->global_expression;
            if (!expression || expression->opcode == 0x131 || expression->opcode == 0x122 ||
                expression->opcode == 0x13b || dependent->value ||
                !optimizer_set_contains(global_available_registers, dependent->number)) continue;
            first = expression->operands;
            second = first->next;
            changed = 0;
            if (!compare_input_operands(first, source)) {
                first = output;
                changed = 1;
            }
            if (second && !compare_input_operands(second, source)) {
                second = output;
                changed = 1;
            }
            if (!changed) continue;
            type = expression->type;
            opcode = expression->opcode;
            replacement = new_global_expression_register(opcode, type, first, second);
            link_global_expression_dependencies(replacement);
            if (!replacement->value &&
                optimizer_set_contains(global_available_registers, replacement->number)) continue;
            if (dependent == replacement) continue;
            old_value = new_typed_register_expression(0x120, type, dependent);
            new_value = new_typed_register_expression(0x120, type, replacement);
            inserted = new_binary_expression(0x131, type, new_value, old_value,
                insert_input_record, (C2Record *)record);
            if (operation_is_comparison(opcode)) inserted->precision_flags = 1;
            inserted->flags |= 0x20;
            finish_global_assignment(inserted, 0);
        }
        return;
    }
    if (!output || !REGISTER_OPERAND(output) || !output->value.reg->global_expression ||
        record->opcode == 0x15f) return;
    expression = output->value.reg->global_expression;
    first = record->left;
    if (!expression->operands->next) {
        if (REGISTER_OPERAND(first))
            substitute_global_assignment(record, expression, first, first, first->next);
    } else if (!expression->operands->next->next) {
        second = first->next;
        if (!compare_input_operands(first, second)) {
            if (REGISTER_OPERAND(first))
                substitute_global_assignment(record, expression, first, first, first);
            return;
        }
        if (REGISTER_OPERAND(first))
            substitute_global_assignment(record, expression, first, first, second);
        first = record->left;
        second = first->next;
        if (REGISTER_OPERAND(second))
            substitute_global_assignment(record, expression, second, first, second);
    }
}

void __fastcall substitute_global_assignment(C2Expression *record,
    C2GlobalExpression *expression, C2Expression *operand,
    C2Expression *first, C2Expression *second)
{
    C2GlobalDependency *link;
    C2GlobalExpression *assignment;
    C2FunctionRegister *dependent, *replacement;
    C2Expression *left, *right, *old_value, *new_value, *inserted;
    unsigned opcode;
    unsigned short type;

    for (link = operand->value.reg->global_dependents; link; link = link->next_dependent) {
        dependent = link->dependent;
        assignment = dependent->global_expression;
        if (!assignment || assignment->opcode != 0x131 || dependent->value ||
            !optimizer_set_contains(global_available_registers, dependent->number)) continue;
        left = assignment->operands;
        right = left->next;
        if (compare_input_operands(operand, right) || !compare_input_operands(left, right)) continue;
        left = operand == first ? assignment->operands : first;
        right = operand == second ? assignment->operands : second;
        opcode = expression->opcode;
        type = expression->type;
        replacement = new_global_expression_register(opcode, type, left, right);
        link_global_expression_dependencies(replacement);
        if ((!replacement->value &&
             optimizer_set_contains(global_available_registers, replacement->number)) ||
            expression->reg == replacement) continue;
        old_value = new_typed_register_expression(0x120, type, expression->reg);
        new_value = new_typed_register_expression(0x120, type, replacement);
        inserted = new_binary_expression(0x131, type, new_value, old_value,
            insert_input_record, (C2Record *)record);
        global_next_record = inserted;
        if (operation_is_comparison(opcode)) inserted->precision_flags = 1;
        inserted->flags |= 0x20;
        finish_global_assignment(inserted, 0);
    }
}

#define REGISTER_CONTAINS(outer, inner) ((outer)->displacement <= (inner)->displacement && \
    REGISTER_END(inner) <= REGISTER_END(outer))
#define REGISTER_OVERLAPS(left, right) ((left)->displacement < REGISTER_END(right) && \
    (right)->displacement < REGISTER_END(left))

void __fastcall rewrite_global_cleanup_record(C2Expression *record,
    C2RegisterSet *available, C2RegisterSet *work, C2RegisterSet *temporary)
{
    C2Expression *output, *operand, *value, *owner, *last_operand;
    C2FunctionRegister *reg, *variant, *replacement;
    C2OptimizerUse *use, *saved, *current;
    unsigned live, number;
    unsigned short type;
    long displacement;

    output = record->tail.right;
    if (output && REGISTER_OPERAND(output)) {
        reg = output->value.reg;
        if (reg->kind == 3 && !reg->value && !(reg->flags & 0x40)) {
            replacement = find_global_replacement_register(available, temporary, reg->canonical);
            if (replacement && !global_replacement_conflicts(record, replacement)) {
                owner = replacement->global_definition_record;
                last_operand = replacement->global_last_input_operand;
                live = reg->value ? reg->rewrite_seen : optimizer_set_contains(available, reg->number);
                if (!live && !global_rewrite_required) output->location.expression.use_flags |= 0x10;
                for (variant = reg->canonical; variant; variant = variant->variant_next) {
                    if (!REGISTER_CONTAINS(reg, variant)) continue;
                    if (!reg->value) {
                        remove_optimizer_symbol(available, variant->number);
                        add_register_to_set(work, variant->number);
                    } else {
                        variant->rewrite_seen = 0;
                        variant->rewrite_queued = 1;
                    }
                }
                replacement->global_definition_record = NULL;
                replacement->global_last_input_operand = NULL;
                remove_optimizer_symbol(temporary, replacement->number);
                value = new_temporary_operand(output->type);
                value = replace_input_result(record, output, value);
                value->value.reg->value = record;
                value->value.reg->rewrite_seen = 1;
                type = last_operand->type;
                value = attach_input_expression(owner, last_operand, value);
                value->type = type;
                if (type != value->value.reg->type) {
                    displacement = (long)((unsigned)replacement->displacement -
                        (unsigned)replacement->canonical->displacement);
                    value->value.reg = new_address_register(value->value.reg, type, type & 0xfff, displacement);
                }
                value->location.expression.use_flags |= 0x10;
            }
        }
        for (output = record->tail.right; output; output = output->next) {
            if (!output->kind || output->kind >= 3) continue;
            output->location.expression.use_flags &= ~0x10;
            reg = output->value.reg;
            live = reg->value ? reg->rewrite_seen : optimizer_set_contains(available, reg->number);
            if (!live && !global_rewrite_required) output->location.expression.use_flags |= 0x10;
            for (variant = reg->canonical; variant; variant = variant->variant_next) {
                if (!REGISTER_CONTAINS(reg, variant)) continue;
                if (!reg->value) {
                    remove_optimizer_symbol(available, variant->number);
                    add_register_to_set(work, variant->number);
                } else {
                    variant->rewrite_seen = 0;
                    variant->rewrite_queued = 1;
                }
            }
            reg->global_definition_record = NULL;
            reg->global_last_input_operand = NULL;
        }
    }
    for (operand = record->left; operand; operand = operand->next) {
        switch (operand->kind) {
        case 1: case 2:
            operand->location.expression.use_flags &= ~0x10;
            reg = operand->value.reg;
            number = reg->number;
            add_register_to_set(temporary, number);
            live = reg->value ? reg->rewrite_seen : optimizer_set_contains(available, number);
            if (!live) {
                reg->global_last_input_operand = operand;
                reg->global_definition_record = record;
            } else {
                reg->global_definition_record = NULL;
                reg->global_last_input_operand = NULL;
            }
            if (!reg->value) {
                if (!optimizer_set_contains(available, number)) {
                    operand->location.expression.use_flags |= 0x10;
                    add_register_to_set(available, number);
                }
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg || REGISTER_OVERLAPS(variant, reg))
                        remove_optimizer_symbol(work, variant->number);
            } else {
                if (!reg->rewrite_seen) {
                    reg->rewrite_seen = 1;
                    operand->location.expression.use_flags |= 0x10;
                }
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg || REGISTER_OVERLAPS(variant, reg)) variant->rewrite_queued = 0;
            }
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
                if (!global_register_seen(reg, available)) add_register_to_set(available, reg->number);
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    remove_optimizer_symbol(work, variant->number);
                use = current->next;
                if (!use && saved) {
                    use = saved->next;
                    saved = NULL;
                }
            }
            break;
        case 10:
            mark_global_index_inputs(operand->value.optimizer_index, available, work);
            break;
        }
    }
}

void __fastcall mark_global_index_inputs(unsigned index, C2RegisterSet *available,
    C2RegisterSet *work)
{
    C2OptimizerUse *use, *saved, *current;
    C2FunctionRegister *reg, *variant;
    int seen;

    use = get_global_index_uses(index);
    saved = NULL;
    while (use) {
        reg = use->reg;
        current = use;
        if (reg->kind == 12) {
            saved = use;
            current = reg->optimizer_uses;
            if (!current) {
                use = saved->next;
                continue;
            }
            reg = current->reg;
        }
        seen = reg->value ? reg->rewrite_seen :
            optimizer_set_contains(available, reg->number);
        if (!seen) add_register_to_set(available, reg->number);
        for (variant = reg->canonical; variant; variant = variant->variant_next)
            remove_optimizer_symbol(work, variant->number);
        use = current->next;
        if (!use && saved) {
            use = saved->next;
            saved = NULL;
        }
    }
}

C2FunctionRegister *__fastcall find_global_replacement_register(C2RegisterSet *available,
    C2RegisterSet *temporary, C2FunctionRegister *canonical)
{
    C2FunctionRegister *selected;

    selected = NULL;
    for (; canonical; canonical = canonical->variant_next) {
        if (!optimizer_set_contains(available, canonical->number)) continue;
        if (selected || !optimizer_set_contains(temporary, canonical->number) ||
            !canonical->global_definition_record) return NULL;
        selected = canonical;
    }
    return selected;
}

int __fastcall global_replacement_conflicts(C2Expression *record, C2FunctionRegister *reg)
{
    C2Expression *owner, *first, *operand;

    owner = reg->global_definition_record;
    if (global_record_skips_replacement_check(owner)) return 0;
    if (owner->opcode == 0x131 && (owner->type & 0xf000) == 0x5000) return 0;
    first = record->next;
    if (first == owner) return 0;
    for (operand = record->left; operand; operand = operand->next) {
        if ((1U << (operand->kind & 31)) & 0x180) continue;
        if (REGISTER_OPERAND(operand) && operand->value.reg->value) continue;
        if (global_range_overwrites_operand(first, owner->previous, operand)) return 1;
    }
    return 0;
}

int __fastcall global_record_skips_replacement_check(C2Expression *record)
{
    return record->kind >= 12 && record->kind <= 19 && record->kind != 15;
}

int __fastcall global_range_overwrites_operand(C2Expression *first, C2Expression *last,
    C2Expression *operand)
{
    C2Expression *record, *output, *end;

    end = last->next;
    for (record = first; record != end; record = record->next) {
        if (!(record->flags & 1)) continue;
        for (output = record->tail.right; output; output = output->next)
            if (input_operands_may_alias(output, operand)) return 1;
    }
    return 0;
}

extern int optimizer_input_changed;

int __fastcall input_operands_may_alias(C2Expression *left, C2Expression *right)
{
    C2FunctionRegister *reg, *other;
    unsigned index;

    if (left == right) return 1;
    switch (left->kind) {
    case 1:
        reg = left->original_register;
        if (reg->kind != 2) {
            if (right->kind == 1) {
                other = right->original_register;
                if (reg->canonical != other->canonical) return 0;
                if (reg != other && !REGISTER_OVERLAPS(reg, other)) return 0;
                return 1;
            }
            if (right->kind != 9 || reg->kind != 1) return 0;
            return optimizer_set_contains(right->value.registers, reg->canonical->number);
        }
        /* Address-class original registers share the kind2 alias rules. */
    case 2:
        switch (right->kind) {
        case 1:
            if (right->original_register->kind != 2) return 0;
        case 2:
            return function_registers_overlap(left->value.reg, right->value.reg);
        case 6:
            index = right->immediate;
            break;
        case 10:
            index = right->value.optimizer_index;
            break;
        default:
            return 0;
        }
        if (optimizer_input_changed) return optimizer_register_may_alias_index(left->value.reg, index);
        return register_may_alias_index(left->value.reg, index);
    case 6: case 10:
        index = left->kind == 6 ? left->immediate : left->value.optimizer_index;
        switch (right->kind) {
        case 1:
            if (right->original_register->kind != 2) return 0;
        case 2:
            if (optimizer_input_changed) return optimizer_register_may_alias_index(right->value.reg, index);
            return register_may_alias_index(right->value.reg, index);
        case 6:
            return optimizer_indices_overlap(index, right->immediate);
        case 10:
            return optimizer_indices_overlap(index, right->value.optimizer_index);
        default:
            return 0;
        }
    case 9:
        if (right->kind == 1 && right->original_register->kind == 1)
            return optimizer_set_contains(left->value.registers, right->original_register->canonical->number);
        if (right->kind != 9) return 0;
        return optimizer_symbol_sets_intersect(left->value.registers, right->value.registers);
    }
    return 0;
}

int __fastcall function_registers_overlap(C2FunctionRegister *left, C2FunctionRegister *right)
{
    C2FunctionRegister *left_root, *right_root;
    long left_position, right_position;

    left_root = left->canonical;
    right_root = right->canonical;
    if (left_root == right_root) {
        if (left == right) return 1;
        return REGISTER_OVERLAPS(left, right);
    }
    if (!optimizer_input_changed || left->kind < 3 || left->kind > 6 ||
        right->kind < 3 || right->kind > 6) return 0;
    left_position = left->kind == 3 ? left_root->frame_displacement : left_root->symbol->parameter_offset;
    left_position = (long)((unsigned)left_position + (unsigned)left->displacement -
        (unsigned)left_root->displacement);
    right_position = right->kind == 3 ? right_root->frame_displacement : right_root->symbol->parameter_offset;
    right_position = (long)((unsigned)right_position + (unsigned)right->displacement -
        (unsigned)right_root->displacement);
    return left_position < (long)((unsigned)right_position + right->value_bytes) &&
        right_position < (long)((unsigned)left_position + left->value_bytes);
}

/* 4969fc: detached diagnostic records are carried by their existing register
 * nodes. The producer writes the record at40 and the next node at44. */
C2FunctionRegister *global_pending_diagnostics;

void __fastcall finish_global_function(C2Function *function)
{
    C2Expression *record, *next, *last, *operand, *source, *definition, *temporary, *inserted;
    C2FunctionRegister *reg;
    unsigned short type;
    unsigned bytes;
    int temporary_output;

    global_pending_diagnostics = NULL;
    last = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != last; record = next) {
        next = record->next;
        if (record->opcode == 0x131) {
            if (!compare_input_operands(record->tail.right, record->left) &&
                !input_has_side_effects(record)) {
                remove_global_record(record);
                continue;
            }
            if ((record->type & 0xf000) == 0x5000) {
                record->kind = 12;
                record->opcode = 0x13f;
            }
        }
        if (record->flags & 1) {
            if (record->kind == 17 || record->kind == 13) normalize_global_call_operands(record);
            for (operand = record->left; operand; operand = operand->next) {
                operand->location.expression.use_flags &= ~0x80;
                if (REGISTER_OPERAND(operand)) {
                    reg = operand->value.reg;
                    if (record->kind == 11 && !(operand->location.expression.properties & 0x20) &&
                        record->opcode != 0x135 &&
                        (!global_operation_preserves_second_type(record) || operand != record->left->next))
                        operand->type = record->type;
                    type = operand->type;
                    if (!register_types_match(reg->type, type))
                        operand->value.reg = new_address_register(reg, type, type & 0xfff, 0);
                } else if (operand->kind == 7) {
                    if (record->kind == 11 && record->opcode != 0x135 &&
                        (!global_operation_preserves_second_type(record) || operand != record->left->next))
                        type = record->type;
                    else type = operand->type;
                    operand->integer.value = normalize_integer_value(type, operand->integer.value);
                    operand->type = type;
                } else if (((1U << (operand->kind & 31)) & 0x60) && record->kind == 11 &&
                    record->opcode != 0x135 &&
                    (!global_operation_preserves_second_type(record) || operand != record->left->next))
                    operand->type = record->type;
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                operand->location.expression.use_flags &= ~0x80;
                if (operand->kind == 2 && operand->value.reg->global_expression) {
                    reg = operand->value.reg;
                    type = operand->type;
                    if (!register_types_match(reg->type, type))
                        operand->value.reg = new_address_register(reg, type, type & 0xfff, 0);
                }
            }
        } else if (record->opcode == 0x184) {
            unlink_input_record((C2Record *)record);
            finish_global_detached_record(record);
            continue;
        }
        if (record->opcode == 0x16c && (record->type & 0xf000) == 0x4000 &&
            REGISTER_OPERAND(record->left) && record->left->value.reg->value &&
            record->left->value.reg->value->opcode == 0x131) {
            definition = record->left->value.reg->value;
            source = definition->left;
            type = (source->type & 0xfff) | 0x2000;
            if (source->kind == 2)
                source->value.reg = new_address_register(source->value.reg, type, type & 0xfff, 0);
            source->type = type;
            inserted = new_assignment_expression(0x131, type, source,
                prepend_input_record, (C2Record *)record);
            attach_input_expression(record, record->left, inserted->tail.right);
            record->type = type;
            remove_global_record(definition);
        }
        temporary_output = 0;
        operand = record->tail.right;
        if (record->kind == 15 && operand &&
            (!REGISTER_OPERAND(operand) || !operand->value.reg->value)) temporary_output = 1;
        else if (record->kind == 17 && input_has_side_effects(record) && operand &&
            REGISTER_OPERAND(operand) && !operand->value.reg->value) temporary_output = 1;
        if (temporary_output) {
            temporary = new_temporary_operand(operand->type);
            next = new_binary_expression(0x131, operand->type, operand, temporary,
                insert_input_record, (C2Record *)record);
            replace_input_result(record, operand, temporary);
            temporary->value.reg->value = record;
        }
        if (record->kind == 17) {
            if (record->detail.intrinsic == 0xac) {
                type = global_intrinsic_argument_type(record, &bytes);
                if (!bytes) {
                    record->detail.intrinsic = 0x96;
                    rewrite_global_intrinsic_type(record, type);
                }
            } else if (record->detail.intrinsic == 0xad) {
                type = global_intrinsic_argument_type(record, &bytes);
                if (!bytes) {
                    record->detail.intrinsic = 0x97;
                    rewrite_global_intrinsic_type(record, type);
                }
            }
        }
    }
    emit_global_pending_diagnostics(function);
}

int __fastcall global_operation_preserves_second_type(C2Expression *record)
{
    return record->opcode == 0x145 || record->opcode == 0x14b ||
        (record->opcode == 0x15f && record->detail.intrinsic >= 0xe9 &&
         record->detail.intrinsic <= 0xeb);
}

void __fastcall emit_global_pending_diagnostics(C2Function *function)
{
    C2FunctionRegister *reg;
    C2Expression *record;
    char *name;

    for (reg = global_pending_diagnostics; reg; reg = reg->diagnostic_next) {
        record = reg->diagnostic_record;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        /* The existing formatter accepts this symbol's common symbol prefix. */
        name = set_diagnostic_function((C2FunctionSymbol *)reg->symbol, 1);
        if (record->left) compiler_warning(4, 0x2bd, name);
        else compiler_warning(1, 0x2bc, name);
        release_small_node((C2SmallNode *)record);
    }
}

void __fastcall rewrite_function_records_backward(C2Function *function)
{
    C2Expression *record, *first;

    first = (C2Expression *)function->flow->last->records;
    for (record = (C2Expression *)function->flow->first->last_record;
         record != first; record = record->previous) {
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        rewrite_final_input_record(record);
    }
}

/* Only this encountered DWORD is modeled; neighboring initialized cells have
 * not been given an array layout or defaults. Both original reads pass it as
 * the replacement shift. */
unsigned final_rewrite_default_shift; /* 495fc0, initialized zero */

void __fastcall rewrite_final_input_record(C2Expression *record)
{
    C2Expression *operand, *next, *definition, *first, *inner, *replacement;
    C2Expression *first_replacement, *second_replacement, *selected;
    unsigned first_flags, second_flags, third_flags;
    unsigned short type;
    int first_range;
    C2Expression *shift_record;

    if (!(record->flags & 1) || ((1U << (record->kind & 31)) & 0x1000) ||
        (record->type & 0xf000) == 0x4000 ||
        (record->kind != 16 && (record->type & 0xfff) >= 5)) return;
    for (operand = record->left; operand; operand = next) {
        next = operand->next;
        if (operand->kind != 1 || operand->value.reg->kind != 3 ||
            !operand->value.reg->value) continue;
        definition = operand->value.reg->value;
        first = definition->left;
        switch (definition->opcode) {
        case 0x141:
        case 0x147:
            if (!REGISTER_OPERAND(first) || !first->value.reg->value ||
                !REGISTER_OPERAND(first->next) || !first->next->value.reg->value) break;
            second_flags = classify_final_shift(first->next, 0, 0);
            if (!(second_flags & 0xff00)) break;
            first_flags = classify_final_shift(first, 0, 0);
            if ((signed char)(first_flags >> 8) == -(signed char)(second_flags >> 8)) {
                type = definition->type;
                replacement = build_final_shift(first, NULL, type, 1, first_flags);
                replacement = build_final_shift(first->next, replacement, type, 1, second_flags);
                unlink_and_release_input_record((C2Record *)definition);
                replace_final_input(record, operand, replacement, attach_input_expression);
            } else {
                inner = first->value.reg->value;
                if (inner->opcode != definition->opcode) break;
                third_flags = classify_final_shift(inner->left->next, 0, 0);
                if ((signed char)(third_flags >> 8) != -(signed char)(second_flags >> 8)) break;
                replacement = build_final_shift(inner->left->next, NULL,
                    definition->type, 1, third_flags);
                replacement = build_final_shift(first->next, replacement,
                    definition->type, 1, second_flags);
                unlink_and_release_input_record((C2Record *)definition);
                replace_final_input(inner, inner->left->next, replacement, attach_input_expression);
                replace_final_input(record, operand, inner->tail.right, attach_input_expression);
                unlink_input_record((C2Record *)inner);
                prepend_input_record((C2Record *)record, (C2Record *)inner);
            }
            break;
        case 0x145:
        case 0x14b:
            first_flags = classify_final_shift(operand, 0, 1);
            if (definition->opcode == 0x145) {
                follow_optimizer_result(definition, &selected);
                if ((selected->type & 0xfff) == 1) break;
            }
            if (!(first_flags & 0xff00)) break;
            replacement = build_final_shift(operand, NULL, definition->type, 1,
                final_rewrite_default_shift);
            replace_final_input(record, operand, replacement, attach_input_expression);
            break;
        case 0x146:
            first_flags = classify_final_shift(operand, 1, 0) & 0xff00;
            if (!first_flags || !REGISTER_OPERAND(first) || !first->value.reg->value) break;
            if (!(first_flags == 0x100 && first->value.reg->value->opcode == 0x145 &&
                  input_integer_equals(first->value.reg->value->left->next, 8)) &&
                !get_final_shift_range(operand, &first_range, &shift_record)) break;
            replacement = build_final_shift(operand, NULL, definition->type, 1,
                final_rewrite_default_shift);
            replace_final_input(record, operand, replacement, attach_input_expression);
            break;
        case 0x151:
            if (!REGISTER_OPERAND(first) || !first->value.reg->value ||
                !REGISTER_OPERAND(first->next) || !first->next->value.reg->value ||
                (definition->type & 0xfff) <= 8) break;
            first_flags = classify_final_shift(first, 0, 0);
            second_flags = classify_final_shift(first->next, 0, 0);
            if (!(first_flags & 0xff00) || !(second_flags & 0xff00)) break;
            first_replacement = build_final_shift(first, NULL, 0x1001, 1, first_flags);
            second_replacement = build_final_shift(first->next, NULL, 0x1001, 0, second_flags);
            attach_input_expression(definition, first, first_replacement);
            attach_input_expression(definition, definition->left->next, second_replacement);
            definition->type = 0x2001;
            break;
        }
    }
}

int __fastcall integer_constant_differs(C2Expression *expression, unsigned __int64 value);

unsigned __fastcall classify_final_shift(C2Expression *operand, int direction, unsigned flags)
{
    C2Expression *definition, *first;
    unsigned result;
    int byte_count;
    C2Expression *masked_record;
    int first_range;
    C2Expression *shift_record;
    unsigned short type;

    result = flags & 0xffff0000;
    if (!REGISTER_OPERAND(operand) || !operand->value.reg->value) return result;
    operand = unwrap_final_conversion(operand, direction);
    if (!flags && get_final_byte_operand(operand)) {
        result |= 0xff00;
        if (!get_final_shift_range(operand, &first_range, &shift_record) &&
            (operand->type & 0xfff) == 1) result |= 1;
        return result;
    }
    if (!REGISTER_OPERAND(operand) || !operand->value.reg->value) return result;
    definition = operand->value.reg->value;
    switch (definition->opcode) {
    case 0x11d:
        /* This opcode's +24 value is the integer immediate, not an operand. */
        if (!(definition->immediate & 0xff)) return result | 0x100;
        if (!(definition->immediate & 0xff00)) return result | 0xff00;
        return result;
    case 0x145:
        if (!input_integer_equals(definition->left->next, 8)) return result;
        type = definition->type & 0xfff;
        if (type == 2) return result | 0x100;
        if (type != 4) return result;
        first = definition->left;
        if (!REGISTER_OPERAND(first) || !first->value.reg->value) return result;
        first = unwrap_final_conversion(first, 0);
        if (get_final_byte_operand(first)) return result | 0x100;
        return result;
    case 0x146:
        first = definition->left->next;
        if (first->kind != 7) return result;
        if (!integer_constant_differs(first, 0xff)) return result | 0xff00;
        if (!integer_constant_differs(definition->left->next, 0xff00)) return result | 0x100;
        return result;
    case 0x14b:
        return classify_final_shift_mask(definition, &byte_count, &masked_record);
    }
    return result;
}

C2Expression *__fastcall unwrap_final_conversion(C2Expression *operand, unsigned flags)
{
    C2Expression *definition, *source;
    unsigned short type;

    if (!REGISTER_OPERAND(operand) || !operand->value.reg->value) return operand;
    while (REGISTER_OPERAND(operand) && (definition = operand->value.reg->value) != NULL &&
           definition->opcode == 0x135) {
        source = definition->left;
        type = source->type;
        if ((definition->type & 0xfff) <= (type & 0xfff)) return operand;
        if (!(flags & 1) && (type & 0xf000) != 0x2000) return operand;
        if (!((1U << (type >> 12)) & 0xe)) return operand;
        if (flags & 2) {
            source = copy_input_expression(source);
            unlink_and_release_input_record((C2Record *)operand->value.reg->value);
        }
        operand = source;
    }
    return operand;
}

unsigned __fastcall classify_final_shift_mask(C2Expression *record,
    int *byte_count, C2Expression **masked_record)
{
    C2Expression *first, *second, *inner;
    unsigned result, shift, mask, shifted;
    unsigned short type;
    int direction, count;

    /* The original retains the incoming output pointer's upper half in EAX;
     * the caller consumes the classification byte, but the full result is
     * preserved here rather than inventing zero upper bits. */
    result = (unsigned)masked_record & 0xffff0000;
    first = record->left;
    second = first->next;
    if (second->kind != 7 || (second->integer.low & 7)) return result;
    shift = second->integer.low;
    if (shift == 8 && (record->type & 0xfff) == 2 &&
        (record->type & 0xf000) == 0x2000) {
        *byte_count = 1;
        *masked_record = record;
        return result | 0xff00;
    }
    type = record->type;
    if ((type & 0xfff) != 4 || !REGISTER_OPERAND(first) ||
        !first->value.reg->value) return result;
    inner = first->value.reg->value;
    if (inner->opcode != 0x146) return result;
    first = inner->left;
    second = first->next;
    if (second->kind != 7) return result;
    mask = second->integer.low;
    shifted = mask >> (shift & 31);
    direction = 0;
    if (shifted == 0xff) direction = -1;
    else if (shifted == 0xff00) direction = 1;
    *masked_record = inner;
    count = ((direction + 1) >> 1) + (shift >> 3);
    *byte_count = count;
    if ((count > 1 && first->kind == 1) ||
        ((type & 0xf000) != 0x2000 && (mask & 0x80000000))) direction = 0;
    return result | ((direction & 0xff) << 8);
}

int __fastcall input_integer_equals(C2Expression *operand, long value)
{
    if (operand->kind != 7) return 0;
    /* The original comparison intentionally uses the normalized low DWORD. */
    return operand->integer.low == (unsigned)normalize_integer_value(operand->type, (__int64)value);
}

C2Expression *__fastcall get_final_byte_operand(C2Expression *operand)
{
    C2Expression *definition, *first;

    if (operand->kind == 1 && operand->value.reg->kind == 3 &&
        (definition = operand->value.reg->value) != NULL && definition->opcode == 0x146) {
        first = definition->left;
        if (input_integer_equals(first->next, 0xff)) return first;
    }
    if ((operand->type & 0xfff) <= 1) {
        if (operand->kind >= 1 && operand->kind <= 2)
            return operand->value.reg->displacement == 0 ? operand : NULL;
        if (operand->kind == 6) return operand;
    }
    return NULL;
}

void __fastcall materialize_final_register_operands(C2Function *function)
{
    C2Expression *record, *operand;
    C2FunctionRegister *reg, *canonical;
    int side;

    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        for (side = 0; side < 2; ++side) {
            for (operand = side ? record->left : record->tail.right;
                 operand; operand = operand->next) {
                if (operand->kind != 2 || (operand->location.expression.properties & 0x40)) continue;
                reg = operand->value.reg;
                canonical = reg->canonical;
                if (!canonical->symbol && !(reg->kind == 3 && !reg->value)) continue;
                if ((canonical->view_flags & 0x40) || reg->kind == 10) continue;
                if (!global_type_has_register_class(operand->type)) continue;
                if (record->kind == 13 && operand == record->left) continue;
                if ((operand->type & 0xf000) == 0x4000) continue;
                /* The original uses exactly the proven table end as an
                 * address identity here. No row or payload exists at it. */
                materialize_global_register_operand(operand,
                    (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT));
            }
        }
    }
}

/* 48d268: one DWORD Boolean for each class returned by final_type_register_class.
 * The class domain ends at25; neighboring packed data is not part of this table.
 */
static const int final_type_register_classes[26] = {
    0,                  /* unclassified */
    1, 1, 1, 0, 0,      /* classes1..5 */
    1, 1, 1, 0, 0,      /* classes6..10 */
    1, 1, 1,            /* classes11..13 */
    1, 0,               /* classes14..15 */
    1, 1, 1, 0, 0,      /* classes16..20 */
    1, 0, 1, 1, 0       /* classes21..25 */
};

int __fastcall global_type_has_register_class(unsigned short type)
{
    return final_type_register_classes[final_type_register_class(type)];
}

unsigned __fastcall final_type_register_class(unsigned short type)
{
    switch (type >> 12) {
    case 1:
        switch (type & 0xfff) {
        case 1: return 1;
        case 2: return 2;
        case 4: return 3;
        case 6: return 4;
        case 8: return 5;
        }
        break;
    case 2:
        switch (type & 0xfff) {
        case 1: return 6;
        case 2: return 7;
        case 4: return 8;
        case 6: return 9;
        case 8: return 10;
        }
        break;
    case 3:
        if ((type & 0xfff) == 4) return 14;
        if ((type & 0xfff) == 6) return 15;
        break;
    case 4:
        if ((type & 0xfff) == 4) return 11;
        if ((type & 0xfff) == 8) return 12;
        if ((type & 0xfff) == 10) return 13;
        break;
    case 5:
        switch (type & 0xfff) {
        case 1: return 16;
        case 2: return 17;
        case 4: return 18;
        case 8: return 19;
        default: return 20;
        }
    case 6: return 25;
    case 7: return 22;
    case 8: return 23;
    case 9: return 24;
    case 10: return 21;
    }
    return 0;
}

C2Expression *__fastcall materialize_global_register_operand(C2Expression *operand,
    C2FunctionRegister *reg)
{
    operand->kind = 1;
    operand->opcode = 0x11f;
    operand->original_register = reg;
    return operand;
}

void __fastcall rewrite_final_assignments(C2Function *function)
{
    C2Expression *record, *next, *result;

    for (record = (C2Expression *)function->flow->last->records; record; record = next) {
        next = record->next;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        if (record->opcode != 0x131) continue;
        result = record->tail.right;
        if (result->kind == 1 && result->value.reg->kind == 3 &&
            result->value.reg->value) continue;
        rewrite_final_assignment(record);
    }
}

void __fastcall simplify_final_blocks(C2Function *function)
{
    C2RegisterBlock *register_block;
    C2FunctionRegister *reg;
    C2BasicBlock *block;

    for (register_block = first_function_register_block; register_block;
         register_block = register_block->next) {
        for (reg = register_block->records; reg < register_block->cursor; ++reg)
            if (reg->kind != 16) memset(&reg->pending2c, 0, 36);
    }
    for (block = function->flow->last; block; block = block->previous)
        simplify_final_block(block);
}

extern int c2_favor_speed; /* Existing -Os/-Ot owner at48fb60. */

void __fastcall simplify_final_block(C2BasicBlock *block)
{
    unsigned changed;
    int iterations = 0;

    do {
        changed = simplify_final_block_forward(block);
        changed |= simplify_final_block_backward(block);
        if (iterations++ >= 50) c2_report_internal("E:\\utc\\src\\\\P2\\sizeopt.c", 67);
    } while (changed);
    if (!c2_favor_speed) compact_final_block(block);
}

unsigned __fastcall simplify_final_block_forward(C2BasicBlock *block)
{
    C2Expression *record, *next, *last, *first, *result, *selected, *inserted;
    C2FunctionRegister *reg;
    unsigned changed = 0, bits;
    int insert_assignment;

    last = (C2Expression *)block->last_record;
    for (record = (C2Expression *)block->records->next; record != last; record = next) {
        next = record->next;
        switch (record->kind) {
        case 12: case 14: case 17: case 18: case 19: case 20:
        case 21: case 22: case 23: case 24: case 25: case 26:
            continue;
        case 15:
            break;
        case 16:
            if (record->opcode == 0x15c) continue;
            break;
        default:
            if ((record->type & 0xf000) == 0x4000) continue;
            if ((record->type & 0xfff) > 4 && record->opcode == 0x143)
                rewrite_wide_final_multiply(record);
            if ((record->type & 0xfff) > 4) continue;
            first = record->left;
            if (first && (first->type & 0xfff) > 4) continue;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
            /* Original defect: the width gate permits NULL but its next
             * machine instruction still reads that absent operand's type. */
            if (!first) recon_unsupported("final simplifier reads absent operand type");
#endif
            if ((first->type & 0xf000) == 0x4000) continue;
        }
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = current_function->source_line_base + record->location.line;
        }
        selected = NULL;
        if (record->kind == 16) {
            if (!record->detail.data && record->opcode != 0x15c) continue;
            selected = record->left->next;
        } else if (record->kind == 15) selected = record->left;
        if (selected) {
            if (simplify_final_record(record, selected)) changed = 1;
            continue;
        }
        first = record->left;
        result = record->tail.right;
        insert_assignment = 0;
        if (record->opcode == 0x131 && REGISTER_OPERAND(result)) {
            reg = result->value.reg;
            if (reg->kind >= 3 && reg->kind <= 6 && !reg->value &&
                ((1U << (first->kind & 31)) & 0x60) && (record->type & 0xfff) == 2)
                insert_assignment = 1;
        }
        if (!insert_assignment && (record->opcode == 0x146 || record->opcode == 0x14b) &&
            (!REGISTER_OPERAND(first) || !first->value.reg->value) &&
            REGISTER_OPERAND(result) && result->value.reg->value) insert_assignment = 1;
        if (insert_assignment) {
            inserted = new_assignment_expression(0x131, record->type, first,
                prepend_input_record, (C2Record *)record);
            next = inserted;
            attach_input_expression(record, record->left, inserted->tail.right);
        }
        result = record->tail.right;
        if (REGISTER_OPERAND(result) && result->value.reg->value) {
            if ((record->type & 0xfff) < 2) continue;
            bits = final_record_zero_bits(record);
            propagate_final_zero_bits(record, bits);
            if (final_record_sign_extends_word(record))
                record->tail.right->value.reg->extension_flags |= 1;
            if (final_record_sign_extends_byte(record))
                record->tail.right->value.reg->extension_flags |= 2;
        }
        if ((record->type & 0xfff) != 4 || record->opcode != 0x135) continue;
        first = record->left;
        if (first->type == 0x2002) {
            if (!REGISTER_OPERAND(first) || !first->value.reg->value ||
                !final_register_has_zero_bits(first->value.reg, 16)) continue;
        } else {
            if (first->type != 0x2001 || !REGISTER_OPERAND(first) ||
                !first->value.reg->value || !final_register_has_zero_bits(first->value.reg, 24)) continue;
        }
        remove_final_conversion(record);
        changed = 1;
    }
    return changed;
}

unsigned __fastcall simplify_final_block_backward(C2BasicBlock *block)
{
    C2Expression *record, *previous, *first, *last, *operand, *result;
    C2FunctionRegister *reg, *variant;
    unsigned changed = 0, bits, mask;
    long width;

    last = (C2Expression *)block->records;
    for (record = (C2Expression *)block->last_record; record != last; record = previous) {
        previous = record->previous;
        if (record->kind == 12 || (record->kind > 15 && record->kind <= 26) ||
            (record->type & 0xf000) == 0x4000 || (record->type & 0xfff) > 4) continue;
        first = record->left;
        if (first && (first->type & 0xfff) > 4) continue;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
        /* Same original absent-operand type read as in the forward pass. */
        if (!first) recon_unsupported("backward final simplifier reads absent operand type");
#endif
        if ((first->type & 0xf000) == 0x4000) continue;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = current_function->source_line_base + record->location.line;
        }
        for (operand = first; operand; operand = operand->next) {
            if (!REGISTER_OPERAND(operand) || !operand->value.reg->value ||
                !((1U << (operand->type >> 12)) & 0xe)) continue;
            reg = operand->value.reg;
            width = (long)((operand->type & 0xfff) + reg->displacement);
            if (width >= 3 || record->opcode == 0x14c) continue;
            for (variant = reg->canonical; variant; variant = variant->variant_next)
                if (!variant->displacement) variant->unused_high_bits = width == 1 ? 24 : 16;
        }
        bits = final_first_input_unused_bits(record);
        if (bits && REGISTER_OPERAND(record->left))
            for (variant = record->left->value.reg->canonical; variant; variant = variant->variant_next)
                if (!variant->displacement) variant->unused_high_bits = (unsigned char)bits;
        bits = final_second_input_unused_bits(record);
        if (bits && REGISTER_OPERAND(record->left->next))
            for (variant = record->left->next->value.reg->canonical; variant; variant = variant->variant_next)
                if (!variant->displacement) variant->unused_high_bits = (unsigned char)bits;
        if (record->opcode == 0x135) {
            first = record->left;
            result = record->tail.right;
            if (((1U << (first->type >> 12)) & 0xe) &&
                (first->type & 0xfff) == 2 && REGISTER_OPERAND(result) &&
                result->value.reg->value && result->value.reg->unused_high_bits >= 16) {
                remove_final_conversion(record);
                changed = 1;
            } else if (((1U << (first->type >> 12)) & 0xe) &&
                (first->type & 0xfff) == 1 && REGISTER_OPERAND(result) &&
                result->value.reg->value && result->value.reg->unused_high_bits >= 24) {
                remove_final_conversion(record);
                changed = 1;
            }
        }
        if (!record->tail.right) continue;
        if (record->opcode == 0x131) previous = move_final_assignment(record);
        result = record->tail.right;
        if (REGISTER_OPERAND(result) && result->value.reg->value &&
            (record->type & 0xfff) > 1 && final_record_can_narrow(record, 1) &&
            final_record_supports_width(record, 1)) {
            rewrite_final_record_width(record, 1);
            changed = 1;
        }
        if ((record->type & 0xfff) < 4 && final_record_can_narrow(record, 4) &&
            final_record_supports_width(record, 4)) {
            rewrite_final_record_width(record, 4);
            changed = 1;
        }
        result = record->tail.right;
        if (REGISTER_OPERAND(result) && result->value.reg->value &&
            record->opcode == 0x146 && record->left->next->kind == 7) {
            bits = result->value.reg->unused_high_bits;
            mask = bits >= 24 ? 0xff : bits >= 16 ? 0xffff : 0xffffffff;
            if ((record->left->next->integer.low & mask) == mask) {
                remove_final_conversion(record);
                changed = 1;
                continue;
            }
        }
        if (!REGISTER_OPERAND(result) || !result->value.reg->value ||
            !result->value.reg->unused_high_bits) continue;
        bits = required_final_input_bits(record);
        if (!bits) continue;
        for (operand = record->left; operand; operand = operand->next) {
            if ((operand->location.expression.properties & 0x20) || !REGISTER_OPERAND(operand)) continue;
            reg = operand->value.reg;
            if (!reg->value || reg->displacement || (int)reg->unused_high_bits >= (int)bits) continue;
            for (variant = reg->canonical; variant; variant = variant->variant_next)
                if (!variant->displacement) variant->unused_high_bits = (unsigned char)bits;
        }
    }
    return changed;
}

unsigned __fastcall final_record_zero_bits(C2Expression *record)
{
    C2Expression *first, *second;
    unsigned first_bits, second_bits, added_bits;

    switch (record->opcode) {
    case 0x135:
        first = record->left;
        if ((record->type & 0xfff) <= (first->type & 0xfff)) return 0;
        added_bits = ((record->type & 0xfff) - (first->type & 0xfff)) * 8;
        first_bits = final_operand_zero_bits(first);
        if ((first->type & 0xf000) == 0x2000 || (int)first_bits > 0)
            return first_bits + added_bits;
        return 0;
    case 0x146:
        first = record->left;
        second = first->next;
        first_bits = final_operand_zero_bits(first);
        second_bits = final_operand_zero_bits(second);
        if ((int)first_bits <= (int)second_bits) return final_operand_zero_bits(second);
        return final_operand_zero_bits(first);
    case 0x147:
        first = record->left;
        second = first->next;
        first_bits = final_operand_zero_bits(first);
        second_bits = final_operand_zero_bits(second);
        if ((int)first_bits < (int)second_bits) return final_operand_zero_bits(first);
        return final_operand_zero_bits(second);
    case 0x14b:
        first = record->left;
        first_bits = final_operand_zero_bits(first);
        second = first->next;
        if (second->kind == 7 && ((int)first_bits > 0 || (record->type & 0xf000) == 0x2000))
            return first_bits + second->integer.low;
        return first_bits;
    }
    return 0;
}

void __fastcall propagate_final_zero_bits(C2Expression *record, unsigned bits)
{
    C2Expression *result = record->tail.right;
    C2FunctionRegister *variant;
    unsigned bytes = result->type & 0xfff;
    long extra_bytes;
    int zero_bits;

    for (variant = result->value.reg->canonical; variant; variant = variant->variant_next) {
        extra_bytes = (long)(bytes - variant->value_bytes);
        if (!variant->displacement && extra_bytes >= 0) {
            zero_bits = (int)(bits - ((unsigned)extra_bytes << 3));
            variant->known_zero_bits = zero_bits > 0 ? (unsigned char)zero_bits : 0;
        }
    }
}

int __fastcall final_record_sign_extends_word(C2Expression *record)
{
    if (record->opcode > 0 && record->opcode < 0x11b)
        return record->opcode == 0xc4 && (record->type & 0xfff) == 4 &&
            (record->left->type & 0xfff) == 2;
    return record->opcode == 0x135 && (record->type & 0xfff) == 4 &&
        record->left->type == 0x1002;
}

int __fastcall final_record_sign_extends_byte(C2Expression *record)
{
    if (record->opcode > 0 && record->opcode < 0x11b)
        return record->opcode == 0xc4 && (record->type & 0xfff) == 4 &&
            (record->left->type & 0xfff) == 1;
    return record->opcode == 0x135 && (record->type & 0xfff) == 4 &&
        record->left->type == 0x1001;
}

unsigned __fastcall final_operand_zero_bits(C2Expression *operand)
{
    if (operand->kind >= 1 && operand->kind <= 2) {
        if (operand->value.reg->value) return operand->value.reg->known_zero_bits;
    } else if (operand->kind == 7) return count_integer_leading_zero_bits(operand);
    return 0;
}

unsigned __fastcall final_first_input_unused_bits(C2Expression *record)
{
    C2Expression *first, *second, *result;
    unsigned width = record->type & 0xfff;
    unsigned bits;

    if (width > 4) return 0;
    first = record->left;
    if (record->opcode == 0x135 && width < (first->type & 0xfff))
        return ((first->type & 0xfff) - width) * 8;
    if (!first || !(second = first->next) || second->kind != 7) return 0;
    if (record->opcode == 0x146)
        return final_operand_zero_bits(second) + (4 - width) * 8;
    result = record->tail.right;
    if (record->opcode == 0x14b && REGISTER_OPERAND(result) && result->value.reg->value) {
        bits = result->value.reg->unused_high_bits;
        if ((int)second->integer.low < (int)bits) return bits - second->integer.low;
    }
    return 0;
}

unsigned __fastcall final_second_input_unused_bits(C2Expression *record)
{
    if (record->opcode == 0x14b || record->opcode == 0x145 ||
        final_record_is_machine_shift(record)) return (record->type & 0xfff) * 8 - 8;
    if (record->opcode == 0x146 && record->left->kind == 7)
        return final_operand_zero_bits(record->left);
    return 0;
}

int __fastcall final_record_is_machine_shift(C2Expression *record)
{
    return record->opcode >= 0x29 && record->opcode <= 0x2b;
}

int __fastcall final_record_can_narrow(C2Expression *record, int bytes)
{
    C2Expression *result = record->tail.right, *first, *second;
    C2FunctionRegister *reg, *canonical;
    C2InputSymbol *symbol;
    unsigned width, required;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
    int computed = 0;
#endif

    if (!result) return 0;
    if (REGISTER_OPERAND(result) && result->value.reg->value) {
        if (!final_operand_fits_word(result)) return 0;
        width = record->type & 0xfff;
        required = (4U - (unsigned)(bytes < (int)width ? bytes : (int)width)) << 3;
        if ((int)required > result->value.reg->unused_high_bits) return 0;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
        computed = 1;
#endif
    } else {
        canonical = REGISTER_OPERAND(result) && result->value.reg ? result->value.reg->canonical : NULL;
        if (!canonical || (int)canonical->value_bytes > 4 ||
            canonical->kind < 3 || canonical->kind > 6) return 0;
        symbol = (C2InputSymbol *)canonical->symbol;
        if (symbol) {
            if ((symbol->attributes & 15) == 6 ||
                symbol->symbol.value > (long)result->value.reg->value_bytes) return 0;
        }
        /* Original defect: PUSH ECX at entry fills the nominal local slot
         * with this defined record address; the fallback reloads that value
         * as its required bit count. Preserve it in the Win32 implementation. */
        required = (unsigned)record;
    }
    first = record->left;
    switch (record->opcode) {
    case 0x131: case 0x136: case 0x137: case 0x145:
        return final_operand_supports_width(first, bytes) != 0;
    case 0x141: case 0x142: case 0x143: case 0x146: case 0x147: case 0x148:
        if (!final_operand_supports_width(first, bytes)) return 0;
        return final_operand_supports_width(first->next, bytes) != 0;
    case 0x14b:
        if (!final_operand_supports_width(first, bytes)) return 0;
        second = first->next;
        if (second->kind != 7) return 0;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
        if (!computed) recon_unsupported("record-address fallback used as final shift bit count");
#endif
        return (int)result->value.reg->unused_high_bits >= (int)(second->integer.low + required);
    }
    return 0;
}

int __fastcall final_operand_fits_word(C2Expression *operand)
{
    return final_operand_supports_width(operand, 4);
}

int __fastcall final_operand_supports_width(C2Expression *operand, int bytes)
{
    C2FunctionRegister *reg, *canonical;
    C2InputSymbol *symbol;
    C2Expression *result;
    long end, limit;
    unsigned storage;

    switch (operand->kind) {
    case 1:
        reg = operand->value.reg;
        if (reg->value && bytes < (int)(operand->type & 0xfff)) {
            result = find_final_overlapping_result(reg->value, reg);
            return result->value.reg->displacement < bytes;
        }
        if (!reg || bytes <= (int)(operand->type & 0xfff)) return 1;
        if (reg->kind >= 3 && reg->kind <= 6) return 1;
        canonical = reg->canonical;
        symbol = (C2InputSymbol *)canonical->symbol;
        limit = (long)canonical->value_bytes;
        end = (long)((unsigned)bytes + (unsigned)reg->displacement);
        if (end > limit) {
            if (symbol) {
                storage = symbol->attributes & 0x780;
                if (storage == 0x180 || storage == 0x100) return 0;
            }
            if ((unsigned)local_storage_alignment < 4) return 0;
        }
        if ((reg->displacement & 3) && (c2_favor_speed || end > limit)) return 0;
        if (operand->location.expression.properties & 0x40) return 0;
        return 1;
    case 2:
        if (operand->opcode == 0x120) return 1;
        return bytes <= (int)(operand->type & 0xfff);
    case 6:
        return bytes <= (int)(operand->type & 0xfff);
    case 7:
        return 1;
    }
    return 0;
}

C2Expression *__fastcall find_final_overlapping_result(C2Expression *record,
    C2FunctionRegister *reg)
{
    C2Expression *result;
    C2FunctionRegister *other;

    for (result = record->tail.right; result; result = result->next) {
        if (!REGISTER_OPERAND(result)) continue;
        other = result->value.reg;
        if (other->canonical != reg->canonical) continue;
        if (other == reg ||
            (other->displacement < REGISTER_END(reg) && reg->displacement < REGISTER_END(other)))
            return result;
    }
    return NULL;
}

int __fastcall final_record_supports_width(C2Expression *record, int bytes)
{
    C2Expression *first, *second, *definition, *selected;
    unsigned width, opcode, type;

    width = record->type & 0xfff;
    if (width == 2) return 1;
    if (width != 4) return 0;
    opcode = record->opcode;
    switch (opcode) {
    case 1:
    case 0x131:
        return record->left->kind == 7 && record->left->integer.low != 0;
    case 0x146:
        if (bytes == 2 && !c2_favor_speed) {
            second = record->left->next;
            if (second->kind == 7 &&
                (int)(signed char)second->integer.low != (int)second->integer.low) return 1;
        }
        if (bytes != 1) return 0;
        first = record->tail.right;
        if (!REGISTER_OPERAND(first) || !first->value.reg->value) return 0;
        selected = record;
        definition = follow_optimizer_result(record, &selected);
        if (definition->opcode != 0x151) return 0;
        second = definition->left->next;
        return second->kind == 7 && second->integer.low == 0;
    case 0x2f:
    case 0x151:
        if (bytes == 1) return 1;
        if (bytes != 2) return 0;
        first = record->left;
        if (REGISTER_OPERAND(first) && first->value.reg->value) {
            definition = first->value.reg->value;
            if (definition->opcode == 0x135) {
                type = definition->left->type;
                if (((1U << (type >> 12)) & 0xe) && (type & 0xfff) == 2) return 1;
            }
            if ((definition->type & 0xfff) == 2 && definition->opcode == 0x131) return 1;
        }
        second = first->next;
        if (REGISTER_OPERAND(second) && second->value.reg->value) {
            definition = second->value.reg->value;
            if (definition->opcode == 0x135) {
                type = definition->left->type;
                if (((1U << (type >> 12)) & 0xe) && (type & 0xfff) == 2) return 1;
            }
            if ((definition->type & 0xfff) == 2 && definition->opcode == 0x131) return 1;
        }
        return 0;
    }
    return 0;
}

unsigned __fastcall required_final_input_bits(C2Expression *record)
{
    switch (record->opcode) {
    case 0x131:
    case 0x136:
    case 0x137:
    case 0x141:
    case 0x142:
    case 0x143:
    case 0x146:
    case 0x147:
    case 0x148:
        return record->tail.right->value.reg->unused_high_bits;
    case 0x145:
    case 0x149:
    case 0x14b:
        return 0;
    }
    return final_default_input_bits();
}

C2Expression *__fastcall move_final_assignment(C2Expression *record)
{
    return record->previous;
}

int __fastcall simplify_final_transfers_once(C2Function *function);

void __fastcall simplify_final_transfers(C2Function *function)
{
    while (simplify_final_transfers_once(function)) {
    }
}

C2Expression *__fastcall next_final_marker(C2Expression *record);
void __fastcall redirect_final_symbol_uses(C2Expression *record, C2Expression *target);
int __fastcall prune_after_final_transfer(C2Expression *record);
int __fastcall fold_final_branch_target(C2Expression *record);
int __fastcall invert_final_branch(C2Expression *record);

int __fastcall simplify_final_transfers_once(C2Function *function)
{
    C2BasicBlock *block, *next;
    C2Expression *first, *last, *marker, *target;
    int changed, total = 0;

    for (block = function->flow->last->previous; block; block = next) {
        next = block->previous;
        changed = 0;
        first = (C2Expression *)block->records->next;
        if (first->kind == 25) {
            if (!symbol_range_boundary(first) && simplify_symbol_record(first)) changed = 1;
            else if (first->tail.right) {
                marker = next_final_marker(first);
                if (marker && marker->kind == 16 && !marker->detail.data &&
                    marker->opcode != 0x15c && marker->left->kind == 4) {
                    target = ((C2InputSymbol *)marker->left->value.symbol)->input_operand;
                    if (target && target != first && marker->opcode != 0x15b) {
                        redirect_final_symbol_uses(first, target);
                        changed = 1;
                    }
                }
            }
        }
        last = (C2Expression *)block->last_record->previous;
        if (last->kind == 16) {
            if (!last->detail.data && last->opcode != 0x15c && prune_after_final_transfer(last))
                ++changed;
            else if (last->left->kind == 4 &&
                ((C2InputSymbol *)last->left->value.symbol)->input_operand &&
                last->opcode != 0x15c && last->opcode != 0x15b) {
                if (fold_final_branch_target(last)) ++changed;
                else if (c2_global_optimize && (last->detail.data || last->opcode == 0x15c) &&
                    invert_final_branch(last)) ++changed;
            }
        }
        if (changed) {
            total += changed;
            next = block;
        }
    }
    return total;
}

/* The next actual code record, skipping non-code list markers. */
C2Expression *__fastcall next_final_marker(C2Expression *record)
{
    for (record = record->next; record; record = record->next)
        if (record->flags & 1) return record;
    return NULL;
}
void __fastcall redirect_allocation_branch(C2Expression *branch, C2Expression *label, int mode);
void __fastcall redirect_final_symbol_uses(C2Expression *record, C2Expression *target)
{
    C2SymbolInputUse *use, *next;

    for (use = record->tail.symbol_uses; use; use = next) {
        next = use->next;
        redirect_allocation_branch(use->expression, target, 0);
    }
    if (!symbol_range_boundary(record)) remove_symbol_record(record);
}
void __fastcall remove_control_record(C2Expression *record);
int __fastcall prune_after_final_transfer(C2Expression *record)
{
    C2Expression *current, *next;
    int removed = 0;
    for (current = record->next; current; current = next) {
        next = current->next;
        switch (current->kind) {
        case 11: case 12: case 13: case 14: case 17: case 19:
            unlink_and_release_input_record((C2Record *)current);
            ++removed;
            break;
        case 16:
            if (record->kind != 14) {
                remove_control_record(current);
                ++removed;
            }
            break;
        case 18:
            release_global_record(current);
            ++removed;
            break;
        case 20: case 23: case 25:
            return removed;
        }
    }
    return removed;
}
void __fastcall remove_branch_record(C2Expression *record);

int __fastcall fold_final_branch_target(C2Expression *record)
{
    C2Expression *next;
    for (next = record->next; next && !(next->flags & 1); next = next->next) {
        if (next->kind == 25 && next ==
            ((C2InputSymbol *)record->left->value.symbol)->input_operand) {
            remove_branch_record(record);
            return 1;
        }
    }
    return 0;
}
void __fastcall redirect_allocation_branch(C2Expression *branch, C2Expression *label, int copy);

int __fastcall invert_final_branch(C2Expression *record)
{
    C2Expression *branch, *next;
    unsigned condition;

    for (branch = record->next; branch; branch = branch->next) {
        if (branch->kind == 16 && !branch->detail.data && branch->opcode != 0x15c &&
            branch->left->kind == 4 &&
            ((C2InputSymbol *)branch->left->value.symbol)->input_operand &&
            branch->opcode != 0x15b) break;
        if ((branch->flags & 1) || branch->kind == 25) return 0;
    }
    if (!branch) return 0;
    for (next = branch->next; next; next = next->next) {
        if (next->kind == 25 && next ==
            ((C2InputSymbol *)record->left->value.symbol)->input_operand) {
            condition = record->condition_code;
            if (condition > 16) { __asm { int 3 } }
            record->condition_code = input_inverse_condition[condition];
            redirect_allocation_branch(record,
                ((C2InputSymbol *)branch->left->value.symbol)->input_operand, 0);
            record->location.attributes = branch->location.attributes;
            remove_control_record(branch);
            return 1;
        }
        if (next->flags & 1) return 0;
    }
    return 0;
}

void __fastcall fold_final_branch_pair(C2Expression *record);

void __fastcall fold_final_conditional_branches(C2Function *function)
{
    C2Expression *record, *next;
    for (record = (C2Expression *)function->flow->last->records; record; record = next) {
        next = record->next;
        if (record->kind == 16 && record->detail.data && record->opcode != 0x15c)
            fold_final_branch_pair(record);
    }
}

int __fastcall input_operand_has_effects(C2Expression *operand);
int __fastcall evaluate_comparison_rewrite(unsigned short type, C2Expression *result,
    C2Expression *left, C2Expression *right, unsigned condition, int mode);
C2Expression *__fastcall new_select_expression(unsigned opcode, unsigned short type,
    C2Expression *condition, C2Expression *left, C2Expression *right,
    unsigned char comparison, C2InputLink link, C2Record *prior);
C2Expression *__fastcall resolve_final_single_use_value(C2Expression *record);

C2Expression *__fastcall simplify_statement_expressions(C2Record *record);

void __fastcall fold_final_branch_pair(C2Expression *record)
{
    C2Expression *label, *condition_value, *branch, *target, *before, *after;
    C2Expression *output, *left, *right, *current, *optional_branch = NULL;
    C2Expression *stop, *next, *selection;
    C2SymbolInputUse *use;
    unsigned condition, inverse;

    if (record->left->kind != 4) return;
    label = ((C2InputSymbol *)record->left->value.symbol)->input_operand;
    if (!label) return;
    condition_value = record->left->next;
    condition = record->condition_code;
    if (condition > 16) { __asm { int 3 } }
    inverse = input_inverse_condition[condition];
    if (condition_value->value.reg->value != record->previous) return;
    for (use = label->tail.symbol_uses; use; use = use->next)
        if (use->expression != record) return;
    for (branch = label->previous; branch->kind == 24 || branch->kind == 21;
         branch = branch->previous) { }
    if (branch->kind != 16 || branch->detail.data || branch->opcode == 0x15c) return;
    target = ((C2InputSymbol *)branch->left->value.symbol)->input_operand;
    before = branch->previous;
    while (before->kind == 24 || before->kind == 21) before = before->previous;
    if (before->opcode == 0x131) output = before->tail.right;
    else if (before->opcode == 0x140) output = NULL;
    else return;
    left = before->left;
    current = before->previous;
    while (current->kind == 24 || current->kind == 21) current = current->previous;
    if (current != record) return;
    after = label->next;
    while (after->kind == 24 || after->kind == 21) after = after->next;
    if (after->opcode == 0x131) {
        if (output && compare_input_operands(output, after->tail.right)) return;
    } else if (after->opcode != 0x140 || output) return;
    right = after->left;
    current = after->next;
    while (current->kind == 24 || current->kind == 21) current = current->next;
    stop = target;
    if (current != target) {
        if (current->kind != 16 || current->detail.data || current->opcode == 0x15c ||
            ((C2InputSymbol *)current->left->value.symbol)->input_operand != target) return;
        stop = optional_branch = current;
    }
    if (input_operand_has_effects(left) || input_operand_has_effects(right)) return;
    if (left->type != right->type) set_input_expression_type(right, left->type);
    if (inverse > 16) { __asm { int 3 } }
    if (!evaluate_comparison_rewrite(left->type, condition_value, left, right,
                                    input_inverse_condition[inverse], 1)) return;
    selection = new_select_expression(0x15e, left->type, condition_value, left, right,
                                     (unsigned char)inverse, prepend_input_record, (C2Record *)record);
    if (output)
        new_binary_expression(0x131, left->type, output, selection->tail.right,
                              insert_input_record, (C2Record *)selection);
    else
        new_transfer_expression(0x140, left->type, NULL, selection->tail.right,
                                insert_input_record, (C2Record *)selection);
    current = record->next;
    stop = stop->next;
    remove_control_record(record);
    remove_control_record(branch);
    while (current != stop) {
        next = current->next;
        if (current->kind != 24 && current->kind != 25 && current != optional_branch)
            unlink_and_release_input_record((C2Record *)current);
        current = next;
    }
    if (selection->previous == selection->left->value.reg->value)
        simplify_statement_expressions((C2Record *)resolve_final_single_use_value(selection));
}

struct C2FinalMultiplyRecipe {
    struct C2FinalMultiplyRecipe *operand;
    int multiplier, cost, recipe_kind;
    struct C2FinalMultiplyRecipe *hash_next;
};
typedef char C2FinalMultiplyRecipeSizeCheck[sizeof(C2FinalMultiplyRecipe) == 20 ? 1 : -1];
/* These 511 pointer heads are defined by the actual clear below.  Native
 * reserved initial zeros do not establish a pre-clear target default. */
static C2FinalMultiplyRecipe *final_multiply_heads[511]; /* 496000 */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
int final_multiply_table_initialized;
#endif
static int final_multiply_negation_cost = 1; /* 495fcc */
static int final_multiply_kind2_cost = 2; /* 495fd0: shifted value plus operand. */
static int final_multiply_kind3_cost = 2; /* 495fd4: shifted value minus operand. */
static int final_multiply_kind4_cost = 3; /* 495fd8: operand minus shifted value. */
static int final_multiply_two_cost = 1; /* 495fe8 */
static int final_multiply_three_cost = 1; /* 495fec */
static int final_multiply_four_cost = 1; /* 495ff0 */
static int final_multiply_five_cost = 1; /* 495ff4 */
static int final_multiply_eight_cost = 1; /* 495ff8 */
static int final_multiply_nine_cost = 1; /* 495ffc */
C2FinalMultiplyRecipe *__fastcall find_final_multiplier_recipe(int multiplier);

void initialize_final_arithmetic(void);
C2Expression *__fastcall find_final_register_definition(C2Expression *record, C2FunctionRegister *reg);
int __fastcall lower_final_constant_multiply(C2Expression *record);
void __fastcall lower_final_constant_division(C2Expression *record);

void __fastcall lower_final_constant_arithmetic(C2Function *function)
{
    C2Expression *record, *next, *first, *second, *result, *definition;
    unsigned type;

    initialize_final_arithmetic();
    for (record = (C2Expression *)function->flow->last->records; record; record = next) {
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        next = record->next;
        if (!(record->flags & 1)) continue;
        type = record->type;
        if (record->opcode == 0x143) {
            if (!((1U << (type >> 12)) & 0xe) || type == 0x1008) continue;
            first = record->left;
            second = first->next;
            if ((first->kind == 7) == (second->kind == 7)) continue;
            result = record->tail.right;
            if (c2_basic_mode && REGISTER_OPERAND(result) && result->value.reg->kind == 3) {
                definition = find_final_register_definition(record, result->value.reg);
                if (definition && definition->opcode == 0x14c) continue;
            }
            if (first->kind == 7) swap_global_operands(record);
            lower_final_constant_multiply(record);
        } else if (record->opcode == 0x149 && c2_favor_speed &&
            ((1U << (type >> 12)) & 0xe) && (type & 0xfff) != 8) {
            first = record->left;
            if (first->next->kind == 7 && first->kind != 7)
                lower_final_constant_division(record);
        }
    }
}

void initialize_final_arithmetic(void)
{
    C2FinalMultiplyRecipe *one, *recipe;

    memset(final_multiply_heads, 0, sizeof(final_multiply_heads));
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
    final_multiply_table_initialized = 1;
#endif
    one = find_final_multiplier_recipe(1);
    one->operand = one;
    one->recipe_kind = 0;
    one->cost = 0;
    recipe = find_final_multiplier_recipe(-1);
    recipe->operand = one;
    recipe->cost = final_multiply_negation_cost;
    recipe->recipe_kind = 1;
    recipe = find_final_multiplier_recipe(2);
    recipe->operand = one;
    recipe->recipe_kind = 8;
    recipe->cost = final_multiply_two_cost;
    recipe = find_final_multiplier_recipe(4);
    recipe->operand = one;
    recipe->cost = final_multiply_four_cost;
    recipe->recipe_kind = 10;
    recipe = find_final_multiplier_recipe(8);
    recipe->operand = one;
    recipe->recipe_kind = 12;
    recipe->cost = final_multiply_eight_cost;
    recipe = find_final_multiplier_recipe(3);
    recipe->operand = one;
    recipe->cost = final_multiply_three_cost;
    recipe->recipe_kind = 9;
    recipe = find_final_multiplier_recipe(5);
    recipe->operand = one;
    recipe->recipe_kind = 11;
    recipe->cost = final_multiply_five_cost;
    recipe = find_final_multiplier_recipe(9);
    recipe->operand = one;
    recipe->recipe_kind = 13;
    recipe->cost = final_multiply_nine_cost;
}

C2FinalMultiplyRecipe *__fastcall find_final_multiplier_recipe(int multiplier)
{
    C2FinalMultiplyRecipe *recipe;
    int magnitude, bucket;

#if defined(RECON_NATIVE_TEST) && defined(__linux__)
    if (!final_multiply_table_initialized)
        recon_unsupported("final multiply heads before their real clear");
    if ((unsigned)multiplier == 0x80000000U)
        recon_unsupported("negative multiply bucket reads unmodeled preceding data");
#endif
    magnitude = multiplier < 0 ? (int)(0U - (unsigned)multiplier) : multiplier;
    bucket = magnitude % 511;
    for (recipe = final_multiply_heads[bucket]; recipe; recipe = recipe->hash_next)
        if (recipe->multiplier == multiplier) return recipe;
    recipe = (C2FinalMultiplyRecipe *)c2_pool_allocate(1, 20);
    recipe->multiplier = multiplier;
    recipe->operand = NULL;
    recipe->cost = 500;
    recipe->hash_next = final_multiply_heads[bucket];
    final_multiply_heads[bucket] = recipe;
    return recipe;
}

C2Expression *__fastcall find_final_register_definition(C2Expression *record, C2FunctionRegister *reg)
{
    return find_register_use(record, reg, NULL);
}

void __fastcall remove_input_operand(C2Expression *owner, C2Expression *operand);
unsigned __fastcall final_integer_log2(unsigned value);
int __fastcall strip_multiplier_twos(int multiplier);
C2FinalMultiplyRecipe *__fastcall choose_final_multiply_recipe(int multiplier, unsigned limit);
int __fastcall emit_final_multiply_recipe(C2FinalMultiplyRecipe *recipe,
    C2Expression **operand, C2Expression *record);
void __fastcall emit_final_multiply_shift(int multiplier, int odd_multiplier,
    C2Expression **operand, C2Expression *record);

int __fastcall lower_final_constant_multiply(C2Expression *record)
{
    C2Expression *operand = record->left, *assignment, *replacement;
    C2FinalMultiplyRecipe *recipe;
    int multiplier = (int)operand->next->integer.low, odd_multiplier;
    unsigned limit;
    int cost;
    if (!c2_favor_speed) limit = 2;
    else if (c2_cpu_generation <= 4) {
        cost = final_integer_log2((unsigned)multiplier);
        if (cost <= 3) cost = 3;
        limit = (unsigned)cost + 6;
    } else limit = 10;
    if (!multiplier || (record->type & 0xfff) != 4) return 0;
    if (!(multiplier & 1))
        recipe = choose_final_multiply_recipe(strip_multiplier_twos(multiplier), limit - 1);
    else recipe = choose_final_multiply_recipe(multiplier, limit);
    if (!recipe->operand || (unsigned)recipe->cost >= limit) return 0;
    if (operand->kind != 1 || operand->value.reg->kind != 3 || !operand->value.reg->value) {
        assignment = new_assignment_expression(0x131, operand->type,
            operand, prepend_input_record, (C2Record *)record);
        attach_input_expression(record, record->left, assignment->tail.right);
    }
    replacement = record->left;
    odd_multiplier = emit_final_multiply_recipe(recipe, &replacement, record);
    if (!(multiplier & 1))
        emit_final_multiply_shift(multiplier, odd_multiplier, &replacement, record);
    remove_input_operand(record, record->left->next);
    attach_input_expression(record, record->left, replacement);
    record->opcode = 0x131;
    return 1;
}
void __fastcall lower_final_constant_division(C2Expression *record)
{
    __asm { int 3 }
}

void clear_final_register_definitions(void);
int __fastcall final_operand_needs_expansion(C2Expression *operand);
void __fastcall expand_final_operand(C2Expression *record, C2Expression *definition,
    C2Expression *operand, long displacement, C2ExpressionReplacer replace);

void __fastcall lower_final_operands(C2Function *function)
{
    C2Expression *record, *operand, *base, *definition;
    C2FunctionRegister *reg;
    unsigned type;

    if (c2_basic_mode) clear_final_register_definitions();
    for (record = (C2Expression *)function->flow->first->last_record;
         record; record = record->previous) {
        if (!(record->flags & 1)) continue;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        operand = record->tail.right;
        if (operand) {
            if (final_operand_needs_expansion(operand)) {
                base = operand->aggregate_value;
                if (base && REGISTER_OPERAND(base) && base->value.reg->value)
                    expand_final_operand(record, base->value.reg->value, operand,
                        operand->detail.aggregate_displacement, replace_input_result);
            } else {
                type = record->type;
                if (((1U << (type >> 12)) & 0xe) && (type & 0xfff) == 4 &&
                    operand->kind == 1 &&
                    (!c2_basic_mode || !operand->value.reg->global_definition_record))
                    expand_final_operand(record, record, NULL, 0, NULL);
            }
        }
        operand = record->left;
        if (operand) {
            if (final_operand_needs_expansion(operand)) {
                base = operand->aggregate_value;
                if (base && REGISTER_OPERAND(base) && base->value.reg->value)
                    expand_final_operand(record, base->value.reg->value, operand,
                        operand->detail.aggregate_displacement, attach_input_expression);
            }
            operand = record->left->next;
            if (operand && final_operand_needs_expansion(operand)) {
                base = operand->aggregate_value;
                if (base && REGISTER_OPERAND(base) && base->value.reg->value)
                    expand_final_operand(record, base->value.reg->value, operand,
                        operand->detail.aggregate_displacement, attach_input_expression);
            }
        }
        if (record->opcode == 0x14c) {
            operand = record->left;
            if (REGISTER_OPERAND(operand) && operand->value.reg->value) {
                reg = operand->value.reg;
                definition = reg->value;
                if ((record->type & 0xf000) == 0x4000) {
                    if (definition->opcode == 0x131 && REGISTER_OPERAND(definition->left) &&
                        definition->left->value.reg->value) {
                        attach_input_expression(record, operand, definition->left);
                        unlink_and_release_input_record((C2Record *)definition);
                    }
                } else reg->global_definition_record = record;
            }
        }
    }
}

void clear_final_register_definitions(void)
{
    __asm { int 3 }
}
int __fastcall final_operand_needs_expansion(C2Expression *operand)
{
    unsigned width;
    if (operand->kind != 6 || operand->opcode == 0x128) return 0;
    width = operand->type & 0xfff;
    return width <= 4 && width != 3;
}


void __fastcall make_global_register_operand(C2Expression *operand)
{
    unsigned kind = operand->value.reg->kind;

    /* Unknown semantic rows are unsupported in both reconstructed builds. */
    if (kind != 3 && kind != 4 && kind != 5 && kind != 8) { __asm { int 3 } }
    operand->left = NULL;
    operand->kind = 2;
    operand->opcode = input_register_opcodes.values[kind];
}

int __fastcall global_assignment_is_redundant(C2Expression *record, C2Expression *operand)
{
    C2FunctionRegister *reg;
    C2GlobalExpression *expression;
    if (record->tail.right->kind != 6 || !REGISTER_OPERAND(operand)) return 0;
    reg = operand->value.reg;
    if (!reg->global_expression || reg->value ||
        !optimizer_set_contains(global_available_registers, reg->number)) return 0;
    expression = reg->global_expression;
    if (expression->opcode != 0x133) return 0;
    return !compare_input_operands(record->tail.right, expression->operands);
}

int __fastcall global_assignment_restored(C2Expression *record)
{
    C2Expression *output = record->tail.right, *input = record->left;
    C2Expression *following, *use, *replacement;
    C2FunctionRegister *reg;
    unsigned short type;
    if (!REGISTER_OPERAND(output) || !output->value.reg->value) return 0;
    following = follow_optimizer_result(record, &use);
    if (following != next_global_code_record(record) ||
        !register_types_match(record->type, use->type) || following->opcode == 0x16c)
        return 0;
    reg = use->value.reg;
    if (reg != use->original_register || output->value.reg != reg ||
        reg->canonical != reg) return 0;
    if ((use->location.expression.properties & 0x20) && input->kind != 1) return 0;
    type = use->type;
    replacement = attach_input_expression(following, use, input);
    if (replacement->type != type) retarget_global_operand(replacement, type);
    remove_global_record(record);
    release_function_register(reg);
    return 1;
}

C2Expression *__fastcall previous_global_code_record(C2Expression *record);

int __fastcall restore_global_assignment(C2Expression *record)
{
    C2Expression *input = record->left, *output, *producer, *result, *replacement;
    C2FunctionRegister *reg, *replacement_register;
    if (input->kind != 1) return 0;
    output = record->tail.right;
    if (!REGISTER_OPERAND(output) ||
        (!output->value.reg->global_expression && !output->value.reg->value)) return 0;
    producer = input->value.reg->value;
    if (!producer) return 0;
    if (!output->value.reg->value && producer != previous_global_code_record(record)) return 0;
    if (!register_types_match(record->type, producer->type) || producer->kind != 11) return 0;
    result = producer->tail.right;
    reg = result->value.reg;
    if (reg != result->original_register || input->value.reg != reg || reg->canonical != reg)
        return 0;
    replacement = replace_input_result(producer, result, output);
    replacement_register = replacement->value.reg;
    if (replacement_register->value) {
        if (replacement_register->canonical->variant_next)
            bind_input_temporary(replacement_register, producer);
        else replacement_register->value = producer;
    }
    retarget_global_operand(replacement, producer->type);
    remove_global_record(record);
    release_function_register(reg);
    return 1;
}

int __fastcall get_final_shift_range(C2Expression *operand, int *byte_count,
    C2Expression **shift_record)
{
    C2Expression *byte_operand, *definition, *first, *constant;
    unsigned mask, shift;
    int result, count;
    if (!REGISTER_OPERAND(operand) || !operand->value.reg->value) return 0;
    byte_operand = get_final_byte_operand(operand);
    if (byte_operand) {
        if (REGISTER_OPERAND(operand) && operand->value.reg->value &&
            operand->value.reg->value->opcode == 0x14b &&
            (classify_final_shift_mask(byte_operand->value.reg->value,
                byte_count, shift_record) & 0xff00) == 0xff00) return -1;
        mask = 0xff;
        result = -1;
    } else {
        definition = operand->value.reg->value;
        if (definition->opcode != 0x146 || (definition->type & 0xfff) != 4) return 0;
        byte_operand = definition->left;
        constant = byte_operand->next;
        if (constant->kind != 7) return 0;
        mask = constant->integer.low;
        if (mask == 0xff) result = -1;
        else if (mask == 0xff00) result = 1;
        else return 0;
    }
    if (!REGISTER_OPERAND(byte_operand) || !byte_operand->value.reg->value) return 0;
    definition = byte_operand->value.reg->value;
    if (definition->opcode != 0x14b) return 0;
    first = definition->left;
    constant = first->next;
    if (constant->kind != 7) return 0;
    shift = constant->integer.low;
    if ((shift & 7) || !(mask << (shift & 31))) return 0;
    *shift_record = definition;
    count = (int)((shift >> 3) + ((result + 1) >> 1));
    *byte_count = count;
    if (count > 1 && first->kind == 1) return 0;
    return result;
}

/* This search's actual scratch owners. The controller writes its inputs before
 * searching; the pending successful search must publish owner/operand before
 * the controller reads them. No pre-search payload/default is relied on. */
C2Expression *assignment_rewrite_target;       /* 496910 */
C2Expression *assignment_rewrite_output;       /* 496914 */
C2Expression *assignment_rewrite_owner;        /* 496918 */
C2Expression *assignment_rewrite_operand;      /* 49691c */
int assignment_rewrite_negated;               /* 496920 */
unsigned assignment_rewrite_opcode;           /* 496924 */

void __fastcall rewrite_final_binary_assignment(C2Expression *record,
    C2Expression *first, C2Expression *second, C2Expression *definition);
int __fastcall find_assignment_rewrite(C2Expression *record, int negate);
unsigned __int64 __fastcall assignment_identity_value(C2Expression *record)
{
    unsigned __int64 unwritten;
    switch (record->opcode) {
    case 0x141: case 0x142: case 0x145: case 0x147: case 0x148: case 0x14b:
        return 0;
    case 0x143: case 0x149:
        return 1;
    case 0x146:
        return ~(unsigned __int64)0;
    default:
        /* Original returns its fresh, unwritten eight-byte automatic cell.
         * This is an internal identity value, not undefined output padding. */
#ifdef __linux__
        recon_unsupported("assignment identity incidental stack value");
#endif
        return unwritten;
    }
}
C2Expression *__fastcall assignment_identity_operand(C2Expression *record)
{
    unsigned __int64 value = assignment_identity_value(record);
    return new_integer_expression(record->type, value);
}
C2Expression *__fastcall simplify_statement_expressions(C2Record *record);
C2Expression *__fastcall simplify_assignment_rewrite(C2Expression *record)
{
    C2Expression *operand = record->left->next, *definition;
    if (((1U << (operand->kind & 31)) & 6) && operand->value.reg->value) {
        definition = simplify_statement_expressions((C2Record *)operand->value.reg->value);
        if (record->opcode == 0x141 && definition->opcode == 0x137 &&
            register_types_match(operand->type, definition->type)) {
            attach_input_expression(record, operand, definition->left);
            unlink_and_release_input_record((C2Record *)definition);
            record->opcode = 0x142;
        }
    }
    return record;
}
void __fastcall replace_peephole_record(C2Expression *record, C2Expression *replacement);

/* Fold a matching memory/result operand directly into a binary assignment.
 * An independently held definition result keeps its compensating copy. */
void __fastcall rewrite_final_binary_assignment(C2Expression *record,
    C2Expression *first, C2Expression *second, C2Expression *definition)
{
    C2Expression *input = record->left, *replacement;

    if (input->kind != 1 || input->value.reg->kind != 3 || !input->value.reg->value)
        new_binary_expression(0x131, definition->type, definition->tail.right,
            record->tail.right, insert_input_record, (C2Record *)record);
    replacement = new_global_ternary_expression(definition->opcode, record->type,
        record->tail.right, first, second, NULL, NULL);
    replace_peephole_record(record, replacement);
    unlink_and_release_input_record((C2Record *)definition);
}

void __fastcall rewrite_final_assignment(C2Expression *record)
{
    C2Expression *input, *definition, *output, *first, *second, *replacement;
    unsigned short type = record->type;
    if ((type & 0xf000) == 0x4000) return;
    input = record->left;
    if (input->kind == 1 && input->value.reg->kind == 3 && input->value.reg->value)
        definition = input->value.reg->value;
    else {
        definition = record->previous;
        if (!REGISTER_OPERAND(input) || !(definition->flags & 1)) return;
        output = definition->tail.right;
        if (!output || !REGISTER_OPERAND(output) || output->value.reg != input->value.reg) return;
    }
    assignment_rewrite_target = record;
    switch (definition->opcode) {
    case 0x142:
        assignment_rewrite_opcode = 0x141;
        break;
    case 0x141: case 0x146: case 0x147:
        assignment_rewrite_opcode = definition->opcode;
        break;
    case 0x148:
        first = definition->left;
        second = first->next;
        if (type == second->type && !compare_input_operands(record->tail.right, second)) {
            rewrite_final_binary_assignment(record, second, first, definition);
            return;
        }
        /* Fall through to the same first-input test as the shift operations. */
    case 0x145: case 0x14b:
        first = definition->left;
        if (type != first->type || compare_input_operands(record->tail.right, first)) return;
        if ((type & 0xfff) == 8 && definition->opcode != 0x148) return;
        rewrite_final_binary_assignment(record, first, first->next, definition);
        return;
    default:
        return;
    }
    output = record->tail.right;
    assignment_rewrite_output = output;
    assignment_rewrite_negated = 0;
    if (!find_assignment_rewrite(definition, 0)) return;
    if (input->kind != 1 || input->value.reg->kind != 3 || !input->value.reg->value) {
        input = new_temporary_operand(input->type);
        new_binary_expression(0x131, definition->type, definition->tail.right,
            output, insert_input_record, (C2Record *)record);
        replace_input_result(definition, definition->tail.right, input);
        bind_input_temporary(input->value.reg, definition);
    }
    replacement = new_global_ternary_expression(definition->opcode, record->type,
        output, output, input, NULL, NULL);
    replace_peephole_record(record, replacement);
    if (definition->opcode == 0x142 || assignment_rewrite_negated)
        replacement->opcode = 0x141;
    first = assignment_identity_operand(assignment_rewrite_owner);
    attach_input_expression(assignment_rewrite_owner, assignment_rewrite_operand, first);
    simplify_assignment_rewrite(replacement);
}

int __fastcall find_assignment_rewrite(C2Expression *record, int negate)
{
    C2Expression *first, *second;
    unsigned opcode;
    if ((1U << (record->kind & 31)) & 0x7fe) {
        if (record->kind != 1 || record->value.reg->kind != 3 ||
            !record->value.reg->value) return 0;
        record = record->value.reg->value;
        if (!record->left || !record->left->next) return 0;
    }
    opcode = record->opcode;
    if (opcode == 0x141) negate = 1;
    if (opcode != assignment_rewrite_opcode &&
        !(assignment_rewrite_opcode == 0x141 && opcode == 0x142)) return 0;
    first = record->left;
    if (opcode != 0x149 && opcode != 0x142) {
        second = first->next;
        if (assignment_rewrite_target->type == second->type &&
            !compare_input_operands(assignment_rewrite_output, second)) {
            assignment_rewrite_operand = second;
            assignment_rewrite_owner = record;
            assignment_rewrite_negated = negate;
            return 1;
        }
    }
    if (assignment_rewrite_target->type == first->type &&
        !compare_input_operands(assignment_rewrite_output, first)) {
        assignment_rewrite_operand = first;
        assignment_rewrite_owner = record;
        assignment_rewrite_negated = negate;
        return 1;
    }
    if (find_assignment_rewrite(first, negate)) return 1;
    if (opcode == 0x149 || opcode == 0x142) return 0;
    return find_assignment_rewrite(first->next, negate) != 0;
}

void __fastcall consider_final_multiply_recipe(int multiplier,
    C2FinalMultiplyRecipe *recipe, unsigned kind);

C2FinalMultiplyRecipe *__fastcall choose_final_multiply_recipe(int multiplier, unsigned limit)
{
    C2FinalMultiplyRecipe *recipe = find_final_multiplier_recipe(multiplier);
    int scale, divisor;
    unsigned kind = (unsigned)multiplier;
    if (recipe->operand || recipe->cost != 500) return recipe;
    recipe->cost = (int)limit;
    if (multiplier > 0) {
        for (scale = 8; scale >= 2; scale >>= 1) {
            divisor = scale + 1;
            if (scale > multiplier || multiplier % divisor) continue;
            switch (divisor) {
            case 2: kind = 8; break;
            case 3: kind = 9; break;
            case 4: kind = 10; break;
            case 5: kind = 11; break;
            case 8: kind = 12; break;
            case 9: kind = 13; break;
            }
            consider_final_multiply_recipe(multiplier / divisor, recipe, kind);
        }
        consider_final_multiply_recipe(strip_multiplier_twos((int)((unsigned)multiplier - 1)),
            recipe, 2);
    } else {
        for (scale = 8; scale >= 2; scale >>= 1) {
            divisor = scale + 1;
            if (scale < multiplier || multiplier % divisor) continue;
            switch (divisor) {
            case 2: kind = 8; break;
            case 3: kind = 9; break;
            case 4: kind = 10; break;
            case 5: kind = 11; break;
            case 8: kind = 12; break;
            case 9: kind = 13; break;
            }
            consider_final_multiply_recipe(multiplier / divisor, recipe, kind);
        }
        consider_final_multiply_recipe(strip_multiplier_twos((int)(1U - (unsigned)multiplier)),
            recipe, 4);
    }
    consider_final_multiply_recipe(strip_multiplier_twos((int)((unsigned)multiplier + 1)),
        recipe, 3);
    return recipe;
}

int __fastcall strip_multiplier_twos(int multiplier)
{
    /* Division rounds negative odd values toward zero. The original always
     * divides once and loops forever on zero; neither behavior is a default. */
    do {
        multiplier /= 2;
    } while (!(multiplier & 1));
    return multiplier;
}

void __fastcall consider_final_multiply_recipe(int multiplier,
    C2FinalMultiplyRecipe *recipe, unsigned kind)
{
    C2FinalMultiplyRecipe *candidate;
    unsigned remaining;
    int cost;
    /* Only these semantic cost rows have been recovered; no neighboring rows
     * or zero defaults are implied by the reserved native representation. */
    if (kind == 1) cost = final_multiply_negation_cost;
    else if (kind == 2) cost = final_multiply_kind2_cost;
    else if (kind == 3) cost = final_multiply_kind3_cost;
    else if (kind == 4) cost = final_multiply_kind4_cost;
    else if (kind == 8) cost = final_multiply_two_cost;
    else if (kind == 9) cost = final_multiply_three_cost;
    else if (kind == 10) cost = final_multiply_four_cost;
    else if (kind == 11) cost = final_multiply_five_cost;
    else if (kind == 12) cost = final_multiply_eight_cost;
    else if (kind == 13) cost = final_multiply_nine_cost;
    else { __asm { int 3 } return; }
    remaining = (unsigned)recipe->cost - (unsigned)cost;
    candidate = choose_final_multiply_recipe(multiplier, remaining);
    if (candidate->operand && (unsigned)candidate->cost < remaining) {
        recipe->operand = candidate;
        recipe->recipe_kind = (int)kind;
        recipe->cost = (int)((unsigned)cost + (unsigned)candidate->cost);
    }
}

void __fastcall duplicate_final_multiply_operand(C2Expression **operand,
    C2Expression **copy, int retain);

int __fastcall emit_final_multiply_recipe(C2FinalMultiplyRecipe *recipe,
    C2Expression **operand, C2Expression *record)
{
    C2Expression *saved, *operation, *constant;
    int multiplier = recipe->multiplier, odd_multiplier, factor;
    unsigned kind = (unsigned)recipe->recipe_kind, opcode;
    switch (kind) {
    case 0:
        return multiplier;
    case 1:
        emit_final_multiply_recipe(recipe->operand, operand, record);
        operation = new_assignment_expression(0x137, record->type, *operand,
            prepend_input_record, (C2Record *)record);
        *operand = operation->tail.right;
        return multiplier;
    case 2: case 3: case 4:
        duplicate_final_multiply_operand(operand, &saved, 1);
        odd_multiplier = emit_final_multiply_recipe(recipe->operand, operand, record);
        if (kind == 2) factor = (int)((unsigned)multiplier - 1);
        else if (kind == 3) factor = (int)((unsigned)multiplier + 1);
        else factor = (int)(1U - (unsigned)multiplier);
        emit_final_multiply_shift(factor, odd_multiplier, operand, record);
        opcode = kind == 2 ? 0x141 : 0x142;
        operation = new_folded_binary_expression(opcode, record->type, *operand, saved,
            prepend_input_record, (C2Record *)record);
        *operand = operation->tail.right;
        if (kind == 4) {
            operation = new_assignment_expression(0x137, record->type, *operand,
                prepend_input_record, (C2Record *)record);
            *operand = operation->tail.right;
        }
        return multiplier;
    case 9: case 11: case 13:
        factor = kind == 9 ? 3 : kind == 11 ? 5 : 9;
        emit_final_multiply_recipe(recipe->operand, operand, record);
        duplicate_final_multiply_operand(operand, &saved, 1);
        constant = new_integer_expression(0x1004, (unsigned __int64)(__int64)(factor - 1));
        operation = new_folded_binary_expression(0x143, record->type, *operand, constant,
            prepend_input_record, (C2Record *)record);
        operation = new_folded_binary_expression(0x141, record->type, saved,
            operation->tail.right, prepend_input_record, (C2Record *)record);
        *operand = operation->tail.right;
        return multiplier;
    case 8: case 10: case 12:
        factor = kind == 8 ? 2 : kind == 10 ? 4 : 8;
        emit_final_multiply_recipe(recipe->operand, operand, record);
        constant = new_integer_expression(0x1004, (unsigned __int64)(__int64)factor);
        operation = new_folded_binary_expression(0x143, record->type, *operand, constant,
            prepend_input_record, (C2Record *)record);
        *operand = operation->tail.right;
        return multiplier;
    default:
        *operand = record->tail.right;
        return multiplier;
    }
}

void __fastcall duplicate_final_multiply_operand(C2Expression **operand,
    C2Expression **copy, int retain)
{
    C2Expression *value = *operand, *definition = value->value.reg->value;
    C2Expression *source, *temporary, *assignment;
    if (definition->opcode == 0x131) {
        source = definition->left;
        if (REGISTER_OPERAND(source) && source->value.reg->kind == 3 &&
            !source->value.reg->value && value->type == source->type) {
            assignment = new_assignment_expression(0x131, value->type, source,
                insert_input_record, (C2Record *)definition);
            *copy = assignment->tail.right;
            return;
        }
    }
    temporary = new_input_temporary(value->type);
    assignment = new_binary_expression(0x131, value->type, temporary, value,
        insert_input_record, (C2Record *)definition);
    if (retain) {
        assignment = new_assignment_expression(0x131, temporary->type, temporary,
            insert_input_record, (C2Record *)assignment);
        *operand = assignment->tail.right;
        assignment = new_assignment_expression(0x131, temporary->type, temporary,
            insert_input_record, (C2Record *)assignment);
        *copy = assignment->tail.right;
    } else {
        *copy = temporary;
        *operand = temporary;
    }
}

void __fastcall emit_final_multiply_shift(int multiplier, int odd_multiplier,
    C2Expression **operand, C2Expression *record)
{
    unsigned shifted = (unsigned)odd_multiplier, shift = 0;
    C2Expression *constant, *operation;
    do {
        shifted <<= 1;
        ++shift;
    } while (shifted != (unsigned)multiplier);
    if (shift <= 3) {
        emit_final_multiply_recipe(find_final_multiplier_recipe(1U << shift), operand, record);
    } else {
        constant = new_integer_expression(record->type, (unsigned __int64)shift);
        operation = new_folded_binary_expression(0x145, record->type, *operand, constant,
            prepend_input_record, (C2Record *)record);
        *operand = operation->tail.right;
    }
}

void __fastcall swap_global_operands(C2Expression *record)
{
    C2Expression *first = record->left;
    C2Expression *second = first->next;
    C2Expression *rest = second->next;
    record->left = second;
    second->next = first;
    first->next = rest;
}

void __fastcall normalize_global_call_operands(C2Expression *record)
{
    C2Expression *operand, *definition;
    for (operand = record->left; operand; operand = operand->next) {
        if (!((1U << (operand->kind & 31)) & 6)) continue;
        definition = operand->value.reg->value;
        if (!definition) continue;
        if ((definition->opcode == 0x130 || definition->opcode == 0x13e) && definition->next != record) {
            unlink_input_record((C2Record *)definition);
            prepend_input_record((C2Record *)record, (C2Record *)definition);
        }
        /* Every defined operand becomes the next anchor, including a
         * non-call definition that did not need moving. */
        record = definition;
    }
}


int __fastcall simplify_final_record(C2Expression *record, C2Expression *operand)
{
    C2Expression *comparison = operand->value.reg->value;
    C2Expression *left, *right;
    unsigned width;

    if (comparison->opcode != 0x151 ||
        (comparison->type & 0xf000) == 0x4000) return 0;
    width = comparison->type & 0xfff;
    if (width >= 5) return 0;
    left = comparison->left;
    if (left && (left->type & 0xfff) >= 5) return 0;
    /* Original defect: the preceding NULL alternative still reaches this
       genuine type read. It does not establish a default operand. */
    if ((left->type & 0xf000) == 0x4000 || width == 1) return 0;

    if ((int)(final_operand_zero_bits(left) +
              (4U - (left->type & 0xfff)) * 8) > 23) {
        right = left->next;
        if ((int)(final_operand_zero_bits(right) +
                  (4U - (right->type & 0xfff)) * 8) > 23 &&
            final_condition_allows_narrowing(record) &&
            final_record_supports_width(comparison, 1)) {
            rewrite_final_record_width(comparison, 1);
            return 1;
        }
    }
    left = comparison->left;
    if (final_operand_fits_signed_byte(left) ||
        (int)(final_operand_zero_bits(left) +
              (4U - (left->type & 0xfff)) * 8) > 24) {
        right = left->next;
        if ((final_operand_fits_signed_byte(right) ||
             (int)(final_operand_zero_bits(right) +
                   (4U - (right->type & 0xfff)) * 8) > 24) &&
            final_record_supports_width(comparison, 1)) {
            rewrite_final_record_width(comparison, 1);
            return 1;
        }
    }
    if ((comparison->type & 0xfff) == 2) {
        if ((comparison->type & 0xf000) == 0x2000) {
            left = comparison->left;
            if (final_operand_canonical_zero_bits(left) > 15) {
                right = left->next;
                if (final_operand_canonical_zero_bits(right) > 15 ||
                    (c2_favor_speed && right->kind == 7)) {
                    rewrite_final_record_width(comparison, 4);
                    return 1;
                }
            }
        }
    } else {
        left = comparison->left;
        if ((int)(final_operand_zero_bits(left) +
                  (4U - (left->type & 0xfff)) * 8) > 15) {
            right = left->next;
            if ((int)(final_operand_zero_bits(right) +
                      (4U - (right->type & 0xfff)) * 8) > 15 &&
                final_condition_allows_narrowing(record) &&
                final_record_supports_width(comparison, 2)) {
                rewrite_final_record_width(comparison, 2);
                return 1;
            }
        }
        left = comparison->left;
        if (final_operand_fits_signed_word(left) ||
            (int)(final_operand_zero_bits(left) +
                  (4U - (left->type & 0xfff)) * 8) > 16) {
            right = left->next;
            if ((final_operand_fits_signed_word(right) ||
                 (int)(final_operand_zero_bits(right) +
                       (4U - (right->type & 0xfff)) * 8) > 16) &&
                final_record_supports_width(comparison, 2)) {
                rewrite_final_record_width(comparison, 2);
                return 1;
            }
        }
    }
    return 0;
}

int __fastcall final_operand_fits_signed_byte(C2Expression *operand)
{
    if (operand->kind >= 1 && operand->kind <= 2)
        return (operand->value.reg->rewrite_flags >> 1) & 1;
    if (operand->kind == 7)
        return (int)(signed char)operand->integer.low == (int)operand->integer.low;
    return 0;
}

int __fastcall final_operand_fits_signed_word(C2Expression *operand)
{
    if (operand->kind >= 1 && operand->kind <= 2)
        return operand->value.reg->rewrite_flags & 1;
    if (operand->kind == 7)
        return (int)(short)operand->integer.low == (int)operand->integer.low;
    return 0;
}


int __fastcall final_condition_allows_narrowing(C2Expression *record)
{
    /* PUSH ECX seeds the original condition cell from the record pointer.
       Kinds 15/16 replace its low byte; other kinds retain that defined byte.
       Native object addresses cannot represent that Win32 numeric fallback. */
    unsigned char condition = (unsigned char)(unsigned)record;
    if (record->kind == 15) condition = record->detail.condition;
    else if (record->kind == 16) condition = record->condition_code;
#ifdef __linux__
    else recon_unsupported("final condition from Win32 record address");
#endif
    /* Encountered 48d2d0 conversion column: 3..6 become 7..10;
       equality/inequality and unsigned conditions are unchanged. */
    if (condition > 10) { __asm { int 3 } }
    return condition <= 2 || condition >= 7;
}

C2Expression *__fastcall retype_final_operand(C2Expression *, unsigned short, long);

void __fastcall rewrite_final_record_width(C2Expression *record, int bytes)
{
    C2Expression *operand, *next;
    unsigned short type = (unsigned short)((record->type & 0xf000) | bytes);
    int bound;

    operand = record->tail.right;
    record->type = type;
    if (operand->type != 0x8000) {
        bound = operand->value.reg->value != NULL;
        retype_final_operand(operand, (unsigned short)((type & 0xf000) | bytes), 0);
        if (bound) record->tail.right->value.reg->value = record;
    }
    if (record->kind == 13) return;
    for (operand = record->left; operand; operand = next) {
        next = operand->next;
        if ((operand->location.expression.properties & 0x20) || (operand->type & 0xfff) == bytes) break;
        retype_final_operand(operand, type, 0);
        if (record->opcode == 0x14b || record->opcode == 0x145 ||
            final_record_is_machine_shift(record)) break;
    }
}
