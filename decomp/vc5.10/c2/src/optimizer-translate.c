#include "optimizer-translate.h"
#include "inlining.h"
#include "optimizer-sets.h"
#include <recon.h>

extern C2FunctionRegister *optimizer_memory_register, *optimizer_parameter_register;
extern unsigned optimizer_index_limit, optimizer_initial_limit, optimizer_original_limit;
extern unsigned optimizer_state_c;
extern int optimizer_context_mode, c2_basic_mode, c2_exception_mode;
extern C2OptimizerAlias *optimizer_aliases;

C2OptimizerCallState *optimizer_call_states;       /* 490ba4 */
C2OptimizerIndexMap *optimizer_index_maps;         /* 490bb8 */
C2OptimizerScopeState *optimizer_scope_states;     /* 490bbc */
unsigned optimizer_exception_index;               /* 490bc8 */
C2FunctionRegister *optimizer_unknown_alias;       /* 490bcc */
int optimizer_has_exception_record;               /* 490bd0 */
C2OptimizerRecordLink *optimizer_exception_records;/* 490bd4 */
C2OptimizerScope *optimizer_scopes;                /* 49a2e0 */

/* The original uses a masked x86 shift to recognize operand kinds 1 and 2. */
#define OPTIMIZER_REGISTER_OPERAND(p) ((1u << ((p)->kind & 31)) & 6)

unsigned __fastcall translate_optimizer_input(C2BasicBlock *block, unsigned count,
    int aliases, int no_global)
{
    C2OptimizerIndex *indices;
    C2OptimizerIndexMap *mapping;
    C2OptimizerUse *uses, *use, *new_use;
    C2OptimizerAlias *alias;
    C2OptimizerCallState *call_state;
    C2OptimizerScopeState *scope_state, *parent_state;
    C2OptimizerScope *scope, *parent;
    C2OptimizerHandler *handler;
    C2OptimizerCaptureState *capture;
    C2OptimizerCaptureRecord *captured;
    C2OptimizerRecordLink *link;
    C2DebugLink *symbols;
    C2RegisterSet *active;
    C2FunctionRegister *reg, *variant, *parameter, *previous_parameter;
    C2InputSymbol *captured_symbol;
    C2Expression *record, *next, *operand, *input, *value, *definition;
    C2Expression *scope_begin, *first, *last_exception_record;
    unsigned depth = 0, next_number = 3, number, operand_list, limit;
    long displacement;
    int changed;

    indices = c2_pool_allocate(4, count * sizeof(C2OptimizerIndex));
    indices[0].parameter = optimizer_parameter_register;
    indices[0].number = 2;
    if (aliases) optimizer_unknown_alias = c2_pool_allocate(4, 80);
    for (; block; block = block->previous) {
        active = block->alias_in;
        record = (C2Expression *)block->records;
        do {
            next = record->next;
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = current_function->source_line_base + record->location.line;
            }
            if (!(record->flags & 1)) {
                if (record->kind == 21) {
                    if (record->opcode == 0x17e) {
                        uses = NULL;
                        for (symbols = record->value.pointer; symbols; symbols = symbols->next) {
                            reg = symbols->symbol->input_register;
                            if (reg && (reg->view_flags & 2)) {
                                new_use = c2_pool_allocate(1, sizeof(C2OptimizerUse));
                                new_use->next = uses;
                                new_use->reg = reg;
                                uses = new_use;
                            }
                        }
                        if (uses) {
                            parameter = c2_pool_allocate(1, 80);
                            previous_parameter = optimizer_parameter_register;
                            reg = indices[depth].parameter;
                            optimizer_parameter_register = parameter;
                            parameter->kind = 12;
                            parameter->optimizer_uses = uses;
                            parameter->previous_parameter = previous_parameter;
                            for (use = reg->optimizer_uses; use; use = use->next) {
                                new_use = c2_pool_allocate(1, sizeof(C2OptimizerUse));
                                new_use->reg = use->reg;
                                new_use->next = parameter->optimizer_uses;
                                parameter->optimizer_uses = new_use;
                            }
                            ++depth;
                            indices[depth].parameter = parameter;
                            indices[depth].number = next_number++;
                        }
                        unlink_and_release_input_record((C2Record *)record);
                    } else if (record->opcode == 0x17f) {
                        for (symbols = record->value.pointer; symbols; symbols = symbols->next) {
                            reg = symbols->symbol->input_register;
                            if (reg && (reg->view_flags & 2)) break;
                        }
                        if (symbols) --depth;
                        unlink_and_release_input_record((C2Record *)record);
                    }
                }
                goto next_record;
            }

            if (record->opcode == 0x160) {
                for (operand = record->tail.right; operand; operand = operand->next) {
                    number = operand->optimizer_alias_number;
                    if (number) {
                        alias = &optimizer_aliases[number];
                        if (!aliases)
                            alias->uses = add_optimizer_use(alias->uses, optimizer_memory_register, 4);
                        alias->uses = add_optimizer_use(alias->uses, indices[depth].parameter, 4);
                        reg = operand->value.reg;
                        displacement = reg->displacement;
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            if (displacement <= variant->displacement &&
                                (long)(variant->value_bytes + variant->displacement) <=
                                (long)(reg->value_bytes + displacement))
                                subtract_optimizer_symbol_sets(active, variant->optimizer->aliases);
                        add_register_to_set(active, number);
                    }
                }
                scope_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
                collect_optimizer_scope_uses(record->detail.data, NULL, 1, 1, scope_state);
                value = new_optimizer_index_operand(optimizer_index_limit);
                attach_optimizer_output(record, value);
                scope_state->input_index = optimizer_index_limit++;
                value = new_optimizer_index_operand(optimizer_index_limit);
                copy_input_operand(record, value);
                scope_state->output_index = optimizer_index_limit++;
                scope_state->parameter = indices[depth].parameter;
                scope_state->next = optimizer_scope_states;
                optimizer_scope_states = scope_state;
                record = next;
                continue;
            }
            if (record->opcode == 0x16b) {
                if (!optimizer_exception_index) optimizer_exception_index = optimizer_index_limit++;
                value = new_optimizer_index_operand(optimizer_exception_index);
                attach_optimizer_output(record, value);
            }
            for (operand_list = 0; operand_list < 2; ++operand_list) {
                operand = operand_list ? record->left : record->tail.right;
                for (; operand; operand = operand->next) {
                    if (operand->kind == 6) {
                        if (!operand->aggregate_value) {
                            operand->immediate = indices[depth].number;
                            if (aliases) operand->value.reg = optimizer_unknown_alias;
                        } else {
                            if (!aliases)
                                number = resolve_optimizer_memory(operand->aggregate_value->value.reg,
                                    active, indices[depth].number, block);
                            else
                                number = resolve_optimizer_alias(operand, active, indices[depth].number);
                            operand->immediate = number;
                        }
                        if ((operand->location.expression.properties & 0x80) && !optimizer_context_mode) {
                            number = operand->immediate;
                            if (number < optimizer_initial_limit) {
                                if (!optimizer_state_c) optimizer_state_c = optimizer_index_limit++;
                                operand->immediate = optimizer_state_c;
                                if (aliases) operand->value.reg = optimizer_unknown_alias;
                            } else if (number >= optimizer_original_limit) {
                                for (mapping = optimizer_index_maps; mapping; mapping = mapping->next)
                                    if (mapping->original == number) break;
                                if (mapping) operand->immediate = mapping->replacement;
                                else {
                                    mapping = c2_pool_allocate(4, sizeof(C2OptimizerIndexMap));
                                    mapping->replacement = optimizer_index_limit;
                                    operand->immediate = optimizer_index_limit++;
                                    mapping->original = number;
                                    mapping->next = optimizer_index_maps;
                                    optimizer_index_maps = mapping;
                                }
                            }
                        }
                    } else if (operand->kind == 2 && (operand->location.expression.properties & 0x40))
                        operand->value.reg->canonical->view_flags |= 0x40;
                }
            }

            parameter = indices[depth].parameter;
            if (record->kind == 13) {
                collect_optimizer_result_uses(record, aliases, parameter);
                for (operand = record->left; operand; operand = operand->next) {
                    if (OPTIMIZER_REGISTER_OPERAND(operand)) {
                        definition = operand->value.reg->value;
                        if (definition && (definition->opcode == 0x130 || definition->opcode == 0x13e))
                            mark_optimizer_operand(definition->left, active);
                    }
                }
                if (!aliases) number = indices[depth].number;
                else if (!no_global) {
                    value = new_optimizer_index_operand(2);
                    copy_input_operand(record, value);
                    attach_optimizer_output(record, value);
                    number = indices[depth].number;
                } else {
                    number = optimizer_index_limit++;
                    call_state = c2_pool_allocate(4, sizeof(C2OptimizerCallState));
                    call_state->next = optimizer_call_states;
                    optimizer_call_states = call_state;
                    call_state->parameter = parameter;
                    call_state->number = number;
                    call_state->registers = new_register_set(6);
                    for (operand = record->left; operand; operand = operand->next) {
                        if (OPTIMIZER_REGISTER_OPERAND(operand)) {
                            definition = operand->value.reg->value;
                            if (definition && (definition->opcode == 0x130 || definition->opcode == 0x13e))
                                collect_optimizer_call_uses(definition->left, call_state, active);
                        }
                    }
                }
                value = new_optimizer_index_operand(number);
                copy_input_operand(record, value);
                attach_optimizer_output(record, value);
            } else if (record->kind == 17) {
                collect_optimizer_result_uses(record, aliases, parameter);
                switch (record->detail.intrinsic) {
                case 0xa3:
                    value = translate_optimizer_operand(record->left, active, indices[depth].number, block);
                    copy_input_operand(record, value);
                    /* Both the input and output forms are required. */
                case 0xad:
                    value = translate_optimizer_operand(record->left, active, indices[depth].number, block);
                    attach_optimizer_output(record, value);
                    break;
                case 0xa4: case 0xac:
                    value = translate_optimizer_operand(record->left, active, indices[depth].number, block);
                    attach_optimizer_output(record, value);
                    goto second_intrinsic_input;
                case 0xa5: case 0xaa:
                    value = translate_optimizer_operand(record->left, active, indices[depth].number, block);
                    copy_input_operand(record, value);
second_intrinsic_input:
                    value = translate_optimizer_operand(record->left->next, active, indices[depth].number, block);
                    copy_input_operand(record, value);
                    break;
                case 0xa6:
                    value = translate_optimizer_operand(record->left, active, indices[depth].number, block);
                    attach_optimizer_output(record, value);
                    value = translate_optimizer_operand(record->left->next, active, indices[depth].number, block);
                    copy_input_operand(record, value);
                    /* The first operand is also used as input. */
                case 0xa7: case 0x153: case 0x154: case 0x157:
                    value = translate_optimizer_operand(record->left, active, indices[depth].number, block);
                    copy_input_operand(record, value);
                    break;
                case 0x155:
                    value = translate_optimizer_operand(record->left, active, indices[depth].number, block);
                    attach_optimizer_output(record, value);
                    value = translate_optimizer_operand(record->left, active, indices[depth].number, block);
                    copy_input_operand(record, value);
                    break;
                case 0x11b: case 0x11c: case 0x11d: case 0x13e: case 0x13f: case 0x14b:
                    value = new_optimizer_index_operand(indices[depth].number);
                    copy_input_operand(record, value);
                    attach_optimizer_output(record, value);
                    break;
                case 0x14e:
                    if (!c2_basic_mode) {
                        for (scope_begin = record->previous; scope_begin && scope_begin->opcode != 0x161;
                             scope_begin = scope_begin->previous) ;
                        limit = record->left->value.reg->value->left->immediate;
                        for (scope = scope_begin->detail.data; scope && limit < scope->limit;
                             scope = lookup_optimizer_scope(optimizer_scopes, scope->serial)) {
                            if (scope->kind == 2) {
                                scope_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
                                collect_optimizer_scope_uses((C2Expression *)scope->symbol->initializer,
                                    (C2Expression *)scope->alternate->initializer, 1, 1, scope_state);
                                value = new_optimizer_index_operand(optimizer_index_limit);
                                copy_input_operand(record, value);
                                scope_state->output_index = optimizer_index_limit++;
                                value = new_optimizer_index_operand(optimizer_index_limit);
                                attach_optimizer_output(record, value);
                                scope_state->input_index = optimizer_index_limit++;
                                scope_state->next = optimizer_scope_states;
                                scope_state->parameter = parameter;
                                optimizer_scope_states = scope_state;
                                value = new_optimizer_index_operand(indices[depth].number);
                                attach_optimizer_output(record, value);
                                copy_input_operand(record, value);
                            }
                        }
                    }
                    break;
                }
            } else if (record->kind == 19) {
                handler = record->detail.data;
                switch (record->opcode) {
                case 0x163: case 0x165: case 0x167:
                    if (!c2_basic_mode) {
                        scope = record->detail.data;
                        scope_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
                        collect_optimizer_scope_uses(scope->end, record, 1, 0, scope_state);
                        value = new_optimizer_index_operand(optimizer_index_limit);
                        attach_optimizer_output(record, value);
                        scope_state->input_index = optimizer_index_limit++;
                        scope_state->parameter = indices[depth].parameter;
                        scope_state->next = optimizer_scope_states;
                        optimizer_scope_states = scope_state;
                    }
                    break;
                case 0x164:
                    if (!c2_basic_mode) {
                        scope = record->detail.data;
                        first = (C2Expression *)scope->symbol->initializer->next;
                        scope_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
                        collect_optimizer_scope_uses(first, record, 1, 1, scope_state);
                        value = new_optimizer_index_operand(optimizer_index_limit);
                        attach_optimizer_output(first, value);
                        scope_state->next = optimizer_scope_states;
                        scope_state->parameter = indices[depth].parameter;
                        scope_state->output_index = optimizer_index_limit++;
                        if (scope->flags & 2) {
                            value = new_optimizer_index_operand(optimizer_index_limit);
                            copy_input_operand(record, value);
                            scope_state->parameter = indices[depth].parameter;
                            scope_state->input_index = optimizer_index_limit++;
                            scope_state->next = optimizer_scope_states;
                        }
                        optimizer_scope_states = scope_state;
                        value = new_optimizer_index_operand(indices[depth].number);
                        copy_input_operand(record, value);
                    }
                    break;
                case 0x16d:
                    scope = handler->scope;
                    if (scope->handler_function) {
                        if (!c2_exception_mode) {
                            if ((handler->kind & 7) == 4) {
                                add_optimizer_exception_indices(record, scope,
                                    indices[depth].number, indices[depth].parameter);
                                last_exception_record = record;
                            }
                        } else {
                            if (!scope->uses) {
                                scope_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
                                first = get_optimizer_handler_body(record);
                                collect_optimizer_scope_uses(first, NULL, 0, 1, scope_state);
                                for (parent = scope->parent; parent; parent = parent->parent) {
                                    if (parent->uses) goto merge_parent_uses;
                                    if (parent->category == 4) {
                                        parent_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
                                        first = (C2Expression *)parent->handler_function->flow->last->records;
                                        collect_optimizer_scope_uses(first, NULL, 0, 1, parent_state);
                                        capture = current_optimizer_capture();
                                        if (capture)
                                            parent_state->outputs = add_optimizer_use(parent_state->outputs, capture->reg, 1);
                                        parent->optimizer_index = optimizer_index_limit;
                                        parent_state->output_index = optimizer_index_limit++;
                                        parent->uses = parent_state->outputs;
                                        parent_state->parameter = indices[depth].parameter;
                                        parent_state->next = optimizer_scope_states;
                                        optimizer_scope_states = parent_state;
                                    }
                                    if (parent->uses) {
merge_parent_uses:
                                        scope_state->outputs = merge_optimizer_uses(scope_state->outputs, parent->uses, 1);
                                        break;
                                    }
                                }
                                capture = current_optimizer_capture();
                                if (capture) scope_state->outputs = add_optimizer_use(scope_state->outputs, capture->reg, 1);
                                scope->uses = scope_state->outputs;
                                scope->optimizer_index = optimizer_index_limit;
                                scope_state->output_index = optimizer_index_limit++;
                                scope_state->parameter = indices[depth].parameter;
                                scope_state->next = optimizer_scope_states;
                                optimizer_scope_states = scope_state;
                            }
                            for (link = scope->targets; link; link = link->next) {
                                value = new_optimizer_index_operand(scope->optimizer_index);
                                copy_input_operand(link->record, value);
                            }
                        }
                    }
                    break;
                case 0x171:
                    scope_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
                    for (captured = handler->context->captures; captured; captured = captured->next)
                        if (captured->symbol)
                            scope_state->inputs = add_optimizer_use(scope_state->inputs,
                                captured->symbol->input_register->canonical, 1);
                    if (scope_state->inputs) {
                        value = new_optimizer_index_operand(optimizer_index_limit);
                        attach_optimizer_output(record, value);
                        scope_state->input_index = optimizer_index_limit++;
                        scope_state->parameter = indices[depth].parameter;
                        scope_state->next = optimizer_scope_states;
                        optimizer_scope_states = scope_state;
                    }
                    break;
                case 0x172:
                    captured_symbol = (C2InputSymbol *)handler->body->value.symbol;
                    if (!c2_exception_mode) {
                        scope_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
                        first = handler->context->first;
                        collect_optimizer_scope_uses(first,
                            (C2Expression *)((C2OptimizerHandler *)first->detail.data)->context->captures->declaration->initializer,
                            1, 0, scope_state);
                        if (captured_symbol)
                            scope_state->inputs = add_optimizer_use(scope_state->inputs,
                                captured_symbol->symbol.input_register->canonical, 1);
                        value = new_optimizer_index_operand(optimizer_index_limit);
                        attach_optimizer_output(record, value);
                        scope_state->input_index = optimizer_index_limit++;
                        scope_state->parameter = indices[depth].parameter;
                        scope_state->next = optimizer_scope_states;
                        optimizer_scope_states = scope_state;
                    } else if (captured_symbol) {
                        value = new_optimizer_index_operand(captured_symbol->detail.optimizer_number);
                        attach_optimizer_output(record, value);
                    }
                    break;
                case 0x175:
                    if (c2_exception_mode) goto ordinary_record;
                    add_optimizer_exception_indices(record, handler->scope,
                        indices[depth].number, indices[depth].parameter);
                    break;
                default:
                    goto ordinary_record;
                }
            } else if (record->opcode == 0x140 && !c2_exception_mode && optimizer_has_exception_record) {
                handler = last_exception_record->detail.data;
                add_optimizer_exception_indices(record, handler->scope,
                    indices[depth].number, indices[depth].parameter);
            } else {
ordinary_record:
                for (operand = record->tail.right; operand; operand = operand->next) {
                    number = operand->optimizer_alias_number;
                    if (!number) {
                        if (!OPTIMIZER_REGISTER_OPERAND(operand) || !operand->value.reg->value) {
                            for (input = record->left; input; input = input->next)
                                mark_optimizer_operand(input, active);
                            if (aliases) {
                                reg = NULL;
                                if (operand->kind == 2) reg = operand->value.reg->canonical;
                                else if (operand->kind == 6 && operand->value.reg != optimizer_unknown_alias)
                                    reg = operand->value.reg;
                                if (reg)
                                    for (input = record->left; input; input = input->next)
                                        compare_optimizer_alias(reg, input, active,
                                            reg->optimizer->secondary);
                            }
                        }
                    } else {
                        changed = 0;
                        alias = &optimizer_aliases[number];
                        input = record->left;
                        if (record->opcode == 0x131 && input->kind == 7 &&
                            (input->integer.low | input->integer.high)) changed = 1;
                        else {
                            for (; input; input = input->next) {
                                if (!aliases) {
                                    if (compare_optimizer_operand(input, active, alias)) changed = 1;
                                } else if (compare_optimizer_alias(operand->value.reg, input, active, alias)) changed = 1;
                            }
                        }
                        if (operand->value.reg->flags & 0x10)
                            for (input = record->left; input; input = input->next)
                                mark_optimizer_operand(input, active);
                        if (changed) {
                            if (!aliases)
                                alias->uses = add_optimizer_use(alias->uses, optimizer_memory_register, 4);
                            alias->uses = add_optimizer_use(alias->uses, indices[depth].parameter, 4);
                        }
                    }
                }
            }
            finish_optimizer_record(record, active);
            for (operand = record->tail.right; operand; operand = operand->next) {
                number = operand->optimizer_alias_number;
                if (number) {
                    reg = operand->value.reg;
                    displacement = reg->displacement;
                    for (variant = reg->canonical; variant; variant = variant->variant_next)
                        if (displacement <= variant->displacement &&
                            (long)(variant->value_bytes + variant->displacement) <=
                            (long)(reg->value_bytes + displacement))
                            subtract_optimizer_symbol_sets(active, variant->optimizer->aliases);
                    add_register_to_set(active, number);
                }
            }
next_record:
            if (record == (C2Expression *)block->last_record) break;
            record = next;
        } while (1);
        release_optimizer_symbol_set(active);
        block->alias_in = NULL;
        release_optimizer_symbol_set(block->alias_uses);
        block->alias_uses = NULL;
        release_optimizer_symbol_set(block->alias_kills);
        block->alias_kills = NULL;
        release_optimizer_symbol_set(block->alias_out);
        block->alias_out = NULL;
    }
    if (((current_function->flags & 0x8000) ||
         (current_function->symbol->additional_flags & 0x200)) &&
        optimizer_exception_records && (capture = current_optimizer_capture()) != NULL) {
        scope_state = c2_pool_allocate(4, sizeof(C2OptimizerScopeState));
        scope_state->outputs = add_optimizer_use(scope_state->outputs, capture->reg, 1);
        scope_state->output_index = optimizer_index_limit;
        scope_state->parameter = optimizer_parameter_register;
        scope_state->next = optimizer_scope_states;
        optimizer_scope_states = scope_state;
        do {
            value = new_optimizer_index_operand(optimizer_index_limit);
            copy_input_operand(optimizer_exception_records->record, value);
            optimizer_exception_records = optimizer_exception_records->next;
        } while (optimizer_exception_records);
        ++optimizer_index_limit;
        optimizer_exception_records = NULL;
    }
    return next_number;
}

/* Insert lifetime-end markers only after proving the active alias set no
 * longer contains a live overlapping view of this named register. */
void __fastcall finish_optimizer_record(C2Expression *record, C2RegisterSet *active)
{
    C2Expression *input, *definition, *output, *following, *value, *marker;
    C2FunctionRegister *reg, *variant, *other;
    C2RegisterSet *remaining;
    unsigned emit_flag;
    int branch;
    for (input = record->left; input; input = input->next) {
        if (input->kind != 2) continue;
        reg = input->value.reg;
        if (reg->kind != 4 || !(reg->view_flags & 8) ||
            !optimizer_set_contains(active, reg->optimizer->number) ||
            !reg->canonical->symbol->name) continue;
        remaining = new_register_set(6);
        for (variant = reg->canonical; variant; variant = variant->variant_next) {
            if (variant == reg ||
                (reg->displacement < (long)(variant->value_bytes + variant->displacement) &&
                 variant->displacement < (long)(reg->value_bytes + reg->displacement))) {
                merge_optimizer_symbol_sets(remaining, variant->optimizer->aliases);
                remove_optimizer_symbol(remaining, variant->optimizer->number);
            }
        }
        intersect_optimizer_symbol_sets(remaining, active);
        if (!(input->location.expression.properties & 0x20)) {
            definition = record;
            branch = record->opcode == 0x146 || record->opcode == 0x147;
            output = definition->tail.right;
            while (output && OPTIMIZER_REGISTER_OPERAND(output) && output->value.reg->value) {
                definition = follow_optimizer_result(definition, &value);
                if (definition->opcode == 0x146 || definition->opcode == 0x147) branch = 1;
                output = definition->tail.right;
            }
            if (branch && output && output->kind == 2) {
                other = output->value.reg;
                if (other->canonical == reg->canonical &&
                    (other == reg ||
                     !((long)(reg->value_bytes + reg->displacement) <= other->displacement ||
                       (long)(other->value_bytes + other->displacement) <= reg->displacement)))
                    goto release_remaining;
            }
        }
        emit_flag = 0;
        if (remaining->links.first) {
            emit_flag = 1;
            if (count_optimizer_symbols(remaining) == 1) {
                following = record->next;
                while (following->flags & 1) {
                    output = following->tail.right;
                    if (output && output->kind == 2 && output->value.reg == reg &&
                        optimizer_set_contains(remaining, output->optimizer_alias_number)) {
                        emit_flag = 0;
                        break;
                    }
                    following = following->next;
                }
            }
            for (variant = reg->canonical; variant; variant = variant->variant_next)
                if (variant->displacement <= reg->displacement &&
                    (long)(reg->value_bytes + reg->displacement) <=
                    (long)(variant->value_bytes + variant->displacement))
                    subtract_optimizer_symbol_sets(remaining, variant->optimizer->aliases);
            if (remaining->links.first) goto release_remaining;
        }
        marker = new_control_expression(0x184);
        marker->value.reg = reg->canonical;
        marker->immediate = emit_flag;
        prepend_input_record((C2Record *)record, (C2Record *)marker);
release_remaining:
        release_optimizer_symbol_set(remaining);
    }
}

int __fastcall compare_optimizer_operand(C2Expression *operand,
    C2RegisterSet *active, C2OptimizerAlias *alias)
{
    C2FunctionRegister *reg;
    C2Expression *definition, *input;
    C2RegisterSet *effects;
    int changed;
    if (operand->location.expression.properties & 0x20) return 0;
    switch (operand->kind) {
    case 1: case 2:
        reg = operand->value.reg;
        definition = reg->value;
        if (!definition) {
            if (!(reg->view_flags & 8)) return 1;
            effects = collect_optimizer_register_aliases(reg, active);
            if (!alias->effects) alias->effects = new_register_set(6);
            merge_optimizer_symbol_sets(alias->effects, effects);
            release_optimizer_symbol_set(effects);
            return 0;
        }
        if (definition->opcode == 0x151) return 0;
        if (definition->kind == 13 || definition->kind == 17) return 1;
        changed = 0;
        for (input = definition->left; input; input = input->next)
            if (compare_optimizer_operand(input, active, alias)) changed = 1;
        return changed;
    case 3:
        alias->uses = add_optimizer_use(alias->uses, operand->value.symbol->input_register, 4);
        return 0;
    case 6:
        return 1;
    }
    return 0;
}

C2RegisterSet *__fastcall collect_optimizer_register_aliases(C2FunctionRegister *reg,
    C2RegisterSet *active)
{
    C2FunctionRegister *variant;
    C2RegisterSet *result = new_register_set(6), *part;
    for (variant = reg->canonical; variant; variant = variant->variant_next) {
        if (variant == reg ||
            (reg->displacement < (long)(variant->value_bytes + variant->displacement) &&
             variant->displacement < (long)(reg->value_bytes + reg->displacement))) {
            part = copy_optimizer_symbol_set(variant->optimizer->aliases);
            intersect_optimizer_symbol_sets(part, active);
            merge_optimizer_symbol_sets(result, part);
            release_optimizer_symbol_set(part);
        }
    }
    return result;
}

void initialize_optimizer_registers(void)
{
    C2FunctionRegister *parameter;
    C2OptimizerUse **place, *use;
    for (parameter = optimizer_parameter_register; parameter;
         parameter = parameter->previous_parameter) {
        place = &parameter->optimizer_uses;
        while ((use = *place) != NULL) {
            if (use->reg->view_flags & 4) place = &use->next;
            else *place = use->next;
        }
    }
}

C2Expression *__fastcall follow_optimizer_result(C2Expression *record,
    C2Expression **operand)
{
    return find_register_use(record, record->tail.right->value.reg, operand);
}

C2Expression *__fastcall find_register_use(C2Expression *record,
    C2FunctionRegister *reg, C2Expression **operand)
{
    C2Expression *scan, *found;
    C2FunctionRegister *other;

    for (scan = record->next; scan; scan = scan->next) {
        if (scan->flags & 1) {
            found = find_register_input(scan, reg);
            if (found) {
                if (operand) *operand = found;
                return scan;
            }
        } else if (((1U << (scan->kind & 31)) & 0x7fe) &&
                   ((1U << (scan->kind & 31)) & 6)) {
            other = scan->value.reg;
            if (other == reg || (other->canonical == reg->canonical &&
                other->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                reg->displacement < (long)((unsigned)other->displacement + other->value_bytes))) {
                if (operand) *operand = scan;
                return NULL;
            }
        }
    }
    return NULL;
}

C2Expression *__fastcall find_register_input(C2Expression *record,
    C2FunctionRegister *reg)
{
    C2Expression *operand;
    C2FunctionRegister *other;

    for (operand = record->left; operand; operand = operand->next) {
        if (!((1U << (operand->kind & 31)) & 6)) continue;
        other = operand->value.reg;
        if (other == reg || (other->canonical == reg->canonical &&
            other->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
            reg->displacement < (long)((unsigned)other->displacement + other->value_bytes)))
            return operand;
    }
    return NULL;
}

/* Append a result operand, preserving the original shared-operand copy rule. */
void __fastcall attach_optimizer_output(C2Expression *record, C2Expression *value)
{
    C2Expression *scan, *previous;

    if ((1U << (value->kind & 31)) & 0x7fe) {
        if (value->location.expression.properties & 0x10)
            value = copy_input_expression(value);
        value->location.expression.properties |= 0x10;
    }
    previous = NULL;
    for (scan = record->tail.right; scan; scan = scan->next) previous = scan;
    if (previous) previous->next = value;
    else record->tail.right = value;
    if (((1U << (value->kind & 31)) & 0x60) &&
        !(value->location.expression.comparison & 0x20)) {
        if (value->aggregate_value) {
            value->aggregate_value->location.expression.properties &= ~0x10;
            copy_input_operand(record, value->aggregate_value);
        }
        if (value->aggregate_second) {
            value->aggregate_second->location.expression.properties &= ~0x10;
            copy_input_operand(record, value->aggregate_second);
        }
    }
}

extern C2FunctionRegister *optimizer_address_registers;
int __fastcall count_static_address_bases(C2FunctionRegister *reg,
    C2FunctionRegister **base)
{
    C2Expression *record = reg->value, *operand;
    int count = 0;
    if (record->kind == 13 || record->kind == 17 || record->opcode == 0x151)
        return 0;
    for (operand = record->left; operand; operand = operand->next) {
        if (operand->location.expression.properties & 0x20) continue;
        if (operand->kind == 3) {
            ++count;
            *base = operand->value.reg->canonical;
        } else if (((1U << (operand->kind & 31)) & 6) && operand->value.reg->value)
            count += count_static_address_bases(operand->value.reg, base);
    }
    return count;
}
int __fastcall resolve_active_address_base(C2FunctionRegister *reg, C2RegisterSet *active,
    C2FunctionRegister **base, C2RegisterSet **effects)
{
    C2Expression *record = reg->value, *operand;
    C2FunctionRegister *part;
    C2RegisterSet *aliases;
    int count = 0, nested;

    if (record->opcode == 0x151) return 0;
    if (record->kind == 13 || record->kind == 17) return -1;
    for (operand = record->left; operand; operand = operand->next) {
        if (operand->kind > 0 && operand->kind <= 2) {
            if (operand->location.expression.properties & 0x20) continue;
            part = operand->value.reg;
            if (((1U << (operand->kind & 31)) & 6) && part->value) {
                nested = resolve_active_address_base(part, active, base, effects);
                if (nested == -1) count = -1;
                else if (count != -1) count += nested;
            } else if (part->view_flags & 8) {
                aliases = collect_optimizer_register_aliases(part, active);
                if (!*effects) *effects = aliases;
                else {
                    merge_optimizer_symbol_sets(*effects, aliases);
                    release_optimizer_symbol_set(aliases);
                }
                if (count != -1) ++count;
                *base = part;
            } else count = -1;
        } else if (operand->kind == 6) count = -1;
    }
    return count;
}
C2OptimizerValue *__fastcall new_optimizer_memory_value(unsigned number,
    C2RegisterSet *effects, C2BasicBlock *block, C2OptimizerValue *next)
{
    C2OptimizerValue *value = c2_pool_allocate(4, sizeof(C2OptimizerValue));
    value->number = number;
    value->effects = effects;
    value->next = next;
    value->block = block;
    return value;
}
unsigned __fastcall resolve_optimizer_register_memory(C2FunctionRegister *reg,
    C2RegisterSet *effects, C2RegisterSet *active, C2BasicBlock *block);

unsigned __fastcall resolve_optimizer_memory(C2FunctionRegister *reg,
    C2RegisterSet *active, unsigned number, C2BasicBlock *block)
{
    C2RegisterSet *effects = NULL;
    C2FunctionRegister *base, *address;
    C2Expression *operand;
    int count;

    if (reg->value) {
        count = count_static_address_bases(reg, &base);
        if (count) {
            if (count == 1)
                return ((C2InputSymbol *)base->symbol)->detail.optimizer_number;
            for (operand = reg->value->left; operand; operand = operand->next)
                mark_optimizer_operand(operand, active);
            return number;
        }
        count = resolve_active_address_base(reg, active, &base, &effects);
        if (!count) return number;
        if (count == -1) {
            if (!effects) return number;
            address = c2_pool_allocate(4, sizeof(C2FunctionRegister));
            address->global_next = optimizer_address_registers;
            optimizer_address_registers = address;
            address->optimizer = c2_pool_allocate(4, sizeof(C2OptimizerRegisterInfo));
            address->optimizer->values = new_optimizer_memory_value(optimizer_index_limit,
                effects, block, NULL);
            address->optimizer_index = number;
            return optimizer_index_limit++;
        }
        if (count != 1) {
            address = c2_pool_allocate(4, sizeof(C2FunctionRegister));
            address->global_next = optimizer_address_registers;
            optimizer_address_registers = address;
            address->optimizer = c2_pool_allocate(4, sizeof(C2OptimizerRegisterInfo));
            address->optimizer->values = new_optimizer_memory_value(optimizer_index_limit,
                effects, block, NULL);
            return optimizer_index_limit++;
        }
        reg = base;
    }
    if (reg->view_flags & 8)
        return resolve_optimizer_register_memory(reg, effects, active, block);
    return number;
}

void __fastcall collect_optimizer_result_uses(C2Expression *record, int aliases,
    C2FunctionRegister *parameter)
{
    C2Expression *output;
    C2OptimizerAlias *alias;
    unsigned number;
    for (output = record->tail.right; output; output = output->next) {
        number = output->optimizer_alias_number;
        if (!number) continue;
        alias = &optimizer_aliases[number];
        if (!aliases)
            alias->uses = add_optimizer_use(alias->uses, optimizer_memory_register, 4);
        alias->uses = add_optimizer_use(alias->uses, parameter, 4);
    }
}

void __fastcall mark_optimizer_operand(C2Expression *operand, C2RegisterSet *active)
{
    C2FunctionRegister *reg;
    C2Expression *record, *part;
    C2RegisterSet *aliases;
    unsigned number;

    if (operand->location.expression.properties & 0x20) return;
    if (!operand->kind) return;
    if (operand->kind <= 2) {
        reg = operand->value.reg;
        if (((1U << (operand->kind & 31)) & 6) && reg->value) {
            record = reg->value;
            if (record->kind == 13 || record->kind == 17 || record->opcode == 0x151)
                return;
            for (part = record->left; part; part = part->next)
                mark_optimizer_operand(part, active);
        } else if (reg->view_flags & 8) {
            aliases = collect_optimizer_register_aliases(reg, active);
            for (number = next_optimizer_symbol(aliases); number != ~0U;
                 number = next_optimizer_symbol(NULL))
                optimizer_aliases[number].address_exposed = 1;
            release_optimizer_symbol_set(aliases);
        }
    } else if (operand->kind == 3 && operand->opcode == 0x120)
        operand->value.reg->canonical->view_flags |= 4;
}

C2Expression *__fastcall new_optimizer_index_operand(unsigned number)
{
    C2Expression *operand = (C2Expression *)allocate_small_node(10);
    operand->value.control = number;
    operand->opcode = 0x12f;
    operand->type = 0x6000;
    return operand;
}

int __fastcall get_optimizer_operand_range(C2Expression *operand,
    long *displacement, unsigned *bytes)
{
    C2FunctionRegister *address = operand->aggregate_value->value.reg, *base;
    C2Expression *definition = address->value, *part;
    unsigned opcode;
    if (!definition) {
        if (!(address->view_flags & 8)) return 0;
        *displacement = operand->detail.aggregate_displacement;
    } else {
        opcode = definition->opcode;
        if (opcode != 0x141 && opcode != 0x142) return 0;
        part = definition->left;
        if (part->kind != 2 || part->value.reg->value || part->next->kind != 7)
            return 0;
        base = part->value.reg;
        if (!(base->view_flags & 8)) return 0;
        *displacement = (long)part->next->integer.low;
        if (opcode == 0x142) *displacement = (long)(0U - (unsigned)*displacement);
        *displacement = (long)((unsigned)*displacement +
            (unsigned)operand->detail.aggregate_displacement);
    }
    *bytes = operand->type & 0xfff;
    return 1;
}
