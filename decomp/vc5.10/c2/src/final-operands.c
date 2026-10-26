#include "global-optimizer.h"
#include "code-input.h"
#include "flow.h"
#include "inlining.h"
#include <recon.h>

#define REGISTER_OPERAND(value) ((1U << ((value)->kind & 31)) & 6)
#define ADDRESS_OPERAND(value) (REGISTER_OPERAND(value) || (value)->kind == 6)
#define DEFINED_REGISTER(node) (REGISTER_OPERAND(node) && (node)->value.reg->value)

void __fastcall remove_input_operand(C2Expression *expression, C2Expression *operand);
void __fastcall prepare_final_address_inputs(C2Expression *record);
int __fastcall final_address_value_changes(C2Expression *first, C2Expression *last, C2Expression *owner);
int __fastcall final_address_operand_changes(C2Expression *operand, C2Expression *first,
    C2Expression *last, int mode);
int __fastcall final_address_displacement_fits(C2FunctionRegister *reg, long displacement);
C2Expression *__fastcall new_final_address_expression(unsigned opcode, unsigned short type,
    C2FunctionRegister *symbol, long displacement, C2Expression *base, C2Expression *index);

/* Collect one base, one scaled index, a symbol address and a signed DWORD
 * displacement. Each consumed definition relinquishes its real input nodes. */
void __fastcall expand_final_operand(C2Expression *record, C2Expression *definition,
    C2Expression *operand, long displacement, C2ExpressionReplacer replace)
{
    C2Expression *base = NULL, *index = NULL, *last_base = NULL, *last_index = NULL;
    C2Expression *first, *second, *old, *new_base, *new_index, *created, *value;
    C2FunctionRegister *symbol = NULL, *reg;
    int scale = 1, collect_base = 1, changed = 0, symbol_base = 0;
    int absorbed, factor, address_opcode;
    unsigned operand_bytes = operand ? operand->type & 0xfff : 0;
    unsigned bits, shift;

    while (definition) {
        switch (definition->opcode) {
        case 0x131:
            if (definition == record) goto change_component;
            first = definition->left;
            switch (first->kind) {
            case 1: case 2:
                if (collect_base) {
                    if (base) release_small_node(base);
                    base = first;
                    if (!base->value.reg->value) collect_base = 0;
                } else {
                    release_small_node(index);
                    index = first;
                    if (!index->value.reg->value) collect_base = 1;
                }
                definition->left = NULL;
                unlink_and_release_input_record((C2Record *)definition);
                changed = 1;
                goto next_definition;
            case 3:
                if (first->opcode == 0x120) {
                    if (!symbol_base && (collect_base || scale == 1)) {
                        if (collect_base) {
                            if (base) release_small_node(base);
                            base = first;
                        } else {
                            release_small_node(index);
                            index = first;
                        }
                        definition->left = NULL;
                        unlink_and_release_input_record((C2Record *)definition);
                        changed = symbol_base = 1;
                    }
                } else if (!symbol && (collect_base || scale == 1)) {
                    if (collect_base) {
                        if (base) {
                            release_small_node(base);
                            base = NULL;
                        }
                    } else {
                        release_small_node(index);
                        index = NULL;
                    }
                    symbol = first->value.reg;
                    unlink_and_release_input_record((C2Record *)definition);
                    changed = 1;
                }
                goto change_component;
            case 7:
                if (collect_base) {
                    if (base) {
                        release_small_node(base);
                        base = NULL;
                    }
                    collect_base = 0;
                    displacement = (long)((unsigned)displacement + first->integer.low);
                } else {
                    release_small_node(index);
                    index = NULL;
                    collect_base = 1;
                    displacement = (long)((unsigned)displacement + first->integer.low * (unsigned)scale);
                }
                unlink_and_release_input_record((C2Record *)definition);
                changed = 1;
                goto next_definition;
            }
            goto change_component;

        case 0x141:
            first = definition->left;
            second = first->next;
            absorbed = 0;
            new_base = base;
            new_index = index;
            if (ADDRESS_OPERAND(first)) {
                if (ADDRESS_OPERAND(second)) {
                    if (collect_base && !index) {
                        prepare_final_address_inputs(definition);
                        if (base) release_small_node(base);
                        new_base = definition->left;
                        new_index = new_base->next;
                        new_base->next = NULL;
                        definition->left = NULL;
                        if (definition != record)
                            unlink_and_release_input_record((C2Record *)definition);
                        absorbed = changed = 1;
                        if (!new_base->value.reg->value) collect_base = 0;
                    } else if (!collect_base && !base && scale == 1) {
                        prepare_final_address_inputs(definition);
                        new_base = definition->left;
                        release_small_node(index);
                        new_index = new_base->next;
                        new_base->next = NULL;
                        definition->left = NULL;
                        unlink_and_release_input_record((C2Record *)definition);
                        absorbed = changed = 1;
                        if (!new_index->value.reg->value) collect_base = 1;
                    }
                } else if (second->kind == 3) {
                    if (second->opcode == 0x120) {
                        if (collect_base && !index && !symbol_base) {
                            prepare_final_address_inputs(definition);
                            if (base) release_small_node(base);
                            new_index = definition->left;
                            new_base = new_index->next;
                            new_index->next = NULL;
                            definition->left = NULL;
                            if (definition != record)
                                unlink_and_release_input_record((C2Record *)definition);
                            absorbed = changed = symbol_base = 1;
                            collect_base = 0;
                        } else if (!collect_base && !base && scale == 1 && !symbol_base) {
                            prepare_final_address_inputs(definition);
                            new_base = definition->left->next;
                            release_small_node(index);
                            new_index = definition->left;
                            definition->left = NULL;
                            new_index->next = NULL;
                            unlink_and_release_input_record((C2Record *)definition);
                            absorbed = changed = symbol_base = 1;
                        }
                    } else if (!symbol && (collect_base || scale == 1)) {
                        prepare_final_address_inputs(definition);
                        symbol = definition->left->next->value.reg;
                        if (collect_base) {
                            if (base) release_small_node(base);
                            new_base = definition->left;
                            release_small_node(new_base->next);
                            new_base->next = NULL;
                            definition->left = NULL;
                            if (definition != record)
                                unlink_and_release_input_record((C2Record *)definition);
                            if (!new_base->value.reg->value) collect_base = 0;
                        } else {
                            release_small_node(index);
                            new_index = definition->left;
                            release_small_node(new_index->next);
                            new_index->next = NULL;
                            definition->left = NULL;
                            unlink_and_release_input_record((C2Record *)definition);
                            if (!new_index->value.reg->value) collect_base = 1;
                        }
                        absorbed = changed = 1;
                    }
                } else if (second->kind == 7) {
                    prepare_final_address_inputs(definition);
                    if (collect_base) {
                        displacement = (long)((unsigned)displacement + definition->left->next->integer.low);
                        if (base) release_small_node(base);
                        new_base = definition->left;
                        release_small_node(new_base->next);
                        new_base->next = NULL;
                        definition->left = NULL;
                        if (definition != record)
                            unlink_and_release_input_record((C2Record *)definition);
                        if (!new_base->value.reg->value) collect_base = 0;
                    } else {
                        new_index = definition->left;
                        displacement = (long)((unsigned)displacement + new_index->next->integer.low * (unsigned)scale);
                        release_small_node(new_index->next);
                        new_index->next = NULL;
                        definition->left = NULL;
                        unlink_and_release_input_record((C2Record *)definition);
                        if (!new_index->value.reg->value) collect_base = 1;
                    }
                    absorbed = changed = 1;
                } else {
                    absorbed = 1;
                    collect_base = !collect_base;
                }
            } else if (first->kind == 3) {
                if (ADDRESS_OPERAND(second)) {
                    if (first->opcode == 0x120) {
                        if (collect_base && !index && !symbol_base) {
                            prepare_final_address_inputs(definition);
                            if (base) release_small_node(base);
                            new_base = definition->left;
                            new_index = new_base->next;
                            definition->left = NULL;
                            if (definition != record)
                                unlink_and_release_input_record((C2Record *)definition);
                            absorbed = changed = symbol_base = 1;
                            collect_base = 0;
                        } else if (!collect_base && !base && scale == 1 && !symbol_base) {
                            prepare_final_address_inputs(definition);
                            release_small_node(index);
                            new_base = definition->left;
                            definition->left = NULL;
                            new_index = new_base->next;
                            new_base->next = NULL;
                            unlink_and_release_input_record((C2Record *)definition);
                            absorbed = changed = symbol_base = 1;
                        }
                    } else if (!symbol && (collect_base || scale == 1)) {
                        prepare_final_address_inputs(definition);
                        symbol = definition->left->value.reg;
                        if (collect_base) {
                            if (base) release_small_node(base);
                            new_base = definition->left->next;
                            release_small_node(definition->left);
                            definition->left = NULL;
                            if (definition != record)
                                unlink_and_release_input_record((C2Record *)definition);
                            if (!new_base->value.reg->value) collect_base = 0;
                        } else {
                            release_small_node(index);
                            new_index = definition->left->next;
                            definition->left->next = NULL;
                            unlink_and_release_input_record((C2Record *)definition);
                            if (!new_index->value.reg->value) collect_base = 1;
                        }
                        absorbed = changed = 1;
                    }
                } else if (second->kind == 7) {
                    if (first->opcode == 0x120) {
                        if (!symbol_base && (collect_base || scale == 1)) {
                            displacement = (long)((unsigned)displacement + second->integer.low);
                            if (collect_base) {
                                if (base) release_small_node(base);
                                new_base = definition->left;
                                release_small_node(new_base->next);
                                new_base->next = NULL;
                                definition->left = NULL;
                                if (definition != record)
                                    unlink_and_release_input_record((C2Record *)definition);
                                collect_base = 0;
                            } else {
                                release_small_node(index);
                                new_index = definition->left;
                                release_small_node(new_index->next);
                                new_index->next = NULL;
                                definition->left = NULL;
                                unlink_and_release_input_record((C2Record *)definition);
                                collect_base = 1;
                            }
                            absorbed = changed = symbol_base = 1;
                        }
                    } else if (!symbol && (collect_base || scale == 1)) {
                        symbol = first->value.reg;
                        displacement = (long)((unsigned)displacement + second->integer.low);
                        if (collect_base) {
                            if (base) {
                                release_small_node(base);
                                base = NULL;
                            }
                            release_small_node(definition->left->next);
                            release_small_node(definition->left);
                            definition->left = NULL;
                            if (definition != record)
                                unlink_and_release_input_record((C2Record *)definition);
                            new_base = base;
                            collect_base = 0;
                        } else {
                            release_small_node(index);
                            new_index = NULL;
                            unlink_and_release_input_record((C2Record *)definition);
                            collect_base = 1;
                        }
                        absorbed = changed = 1;
                    }
                } else {
                    absorbed = 1;
                    collect_base = !collect_base;
                }
            } else if (first->kind == 7 && ADDRESS_OPERAND(second)) {
                prepare_final_address_inputs(definition);
                if (collect_base) {
                    displacement = (long)((unsigned)displacement + definition->left->integer.low);
                    if (base) release_small_node(base);
                    new_base = definition->left->next;
                    release_small_node(definition->left);
                    definition->left = NULL;
                    if (definition != record)
                        unlink_and_release_input_record((C2Record *)definition);
                    if (!new_base->value.reg->value) collect_base = 0;
                } else {
                    displacement = (long)((unsigned)displacement + definition->left->integer.low * (unsigned)scale);
                    release_small_node(index);
                    new_index = definition->left->next;
                    definition->left->next = NULL;
                    unlink_and_release_input_record((C2Record *)definition);
                    if (!new_index->value.reg->value) collect_base = 1;
                }
                absorbed = changed = 1;
            } else {
                absorbed = 1;
                collect_base = !collect_base;
            }
            base = new_base;
            index = new_index;
            if (absorbed) goto next_definition;
            /* Both address slots are occupied. Move an invariant input around
             * this addition only after its original range checks succeed. */
            if (base && index) {
                first = definition->left;
                second = first->next;
                if (!!DEFINED_REGISTER(first) != !!DEFINED_REGISTER(second)) {
                    if (scale == 1) {
                        if (collect_base) {
                            if (!DEFINED_REGISTER(index) || last_index != index->value.reg->value ||
                                final_address_value_changes(last_index, definition, record))
                                goto change_component;
                            if (!DEFINED_REGISTER(first)) {
                                old = first->next;
                                collect_base = 0;
                                index->next = old->next;
                                old->next = NULL;
                                first->next = index;
                                index = old;
                            } else {
                                index->next = first->next;
                                definition->left = index;
                                first->next = NULL;
                                collect_base = 0;
                                index = first;
                            }
                        } else {
                            if (!DEFINED_REGISTER(base) || last_base != base->value.reg->value ||
                                final_address_value_changes(last_base, definition, record))
                                goto change_component;
                            if (!DEFINED_REGISTER(first)) {
                                old = first->next;
                                collect_base = 1;
                                base->next = old->next;
                                old->next = NULL;
                                first->next = base;
                                base = old;
                            } else {
                                base->next = first->next;
                                definition->left = base;
                                first->next = NULL;
                                collect_base = 1;
                                base = first;
                            }
                        }
                        goto next_definition;
                    }
                } else if (DEFINED_REGISTER(first) && scale == 1) {
                    if (collect_base) {
                        bits = 1U << (index->kind & 31);
                        if (DEFINED_REGISTER(index) ||
                            (!(bits & 0x180) && !(bits & 0x18) &&
                             final_address_operand_changes(index, definition, record, 0)))
                            goto change_component;
                        base = first;
                        definition->left = index;
                        index->next = base->next;
                        base->next = NULL;
                        index = new_base;
                    } else {
                        bits = 1U << (base->kind & 31);
                        if (DEFINED_REGISTER(base) ||
                            (!(bits & 0x180) && !(bits & 0x18) &&
                             final_address_operand_changes(base, definition, record, 0)))
                            goto change_component;
                        old = first->next;
                        first->next = base;
                        base = index;
                        index = old;
                    }
                    goto next_definition;
                }
            }
            goto change_component;

        case 0x142:
            first = definition->left;
            if (first->next->kind != 7 || !ADDRESS_OPERAND(first)) goto change_component;
            prepare_final_address_inputs(definition);
            if (collect_base) {
                displacement = (long)((unsigned)displacement - definition->left->next->integer.low);
                if (base) release_small_node(base);
                base = definition->left;
                release_small_node(base->next);
                base->next = NULL;
                definition->left = NULL;
                if (definition != record)
                    unlink_and_release_input_record((C2Record *)definition);
                if (!base->value.reg->value) collect_base = 0;
            } else {
                displacement = (long)((unsigned)displacement - definition->left->next->integer.low * (unsigned)scale);
                release_small_node(index);
                index = definition->left;
                release_small_node(index->next);
                index->next = NULL;
                definition->left = NULL;
                unlink_and_release_input_record((C2Record *)definition);
                if (!index->value.reg->value) collect_base = 1;
            }
            changed = 1;
            goto next_definition;

        case 0x143:
        case 0x145:
            first = definition->left;
            second = first->next;
            if (second->kind != 7 || !ADDRESS_OPERAND(first)) goto change_component;
            factor = (int)second->integer.low;
            if (definition->opcode == 0x143) {
                if (factor != 1 && factor != 2 && factor != 4 && factor != 8) goto change_component;
            } else {
                if (factor < 1 || factor > 3) goto change_component;
                factor = 1 << factor;
            }
            if ((!collect_base && scale != 1) || (collect_base && index && scale != 1))
                goto change_component;
            prepare_final_address_inputs(definition);
            if (collect_base) {
                if (base) release_small_node(base);
                old = index;
                index = definition->left;
                release_small_node(index->next);
                index->next = NULL;
                definition->left = NULL;
                if (definition != record)
                    unlink_and_release_input_record((C2Record *)definition);
                base = old;
                if (index->value.reg->value) collect_base = 0;
            } else {
                release_small_node(index);
                index = definition->left;
                release_small_node(index->next);
                index->next = NULL;
                definition->left = NULL;
                unlink_and_release_input_record((C2Record *)definition);
                if (!index->value.reg->value) collect_base = 1;
            }
            changed = 1;
            scale = factor;
            goto next_definition;
        }
change_component:
        collect_base = !collect_base;
next_definition:
        if (collect_base) {
            if (!base || !(definition = base->value.reg->value) || definition == last_base) break;
            last_base = definition;
        } else {
            if (!index || !(definition = index->value.reg->value) || definition == last_index) break;
            last_index = definition;
        }
        if ((definition->type & 0xfff) != 4) break;
    }

    if (!base) {
        if (!index) address_opcode = symbol ? 0x121 : 0x11d;
        else {
            index->location.expression.properties &= ~0x10;
            index->next = NULL;
            if (scale < 2) {
                address_opcode = 0x122;
                base = index;
                index = NULL;
            } else address_opcode = 0x125;
        }
    } else {
        base->next = NULL;
        base->location.expression.properties &= ~0x10;
        if (base->kind == 3) address_opcode = index ? 0x124 : 0x120;
        else address_opcode = index ? 0x123 : 0x122;
        if (index) {
            index->next = NULL;
            index->location.expression.properties &= ~0x10;
        }
    }
    if (!changed) return;
    shift = scale < 8 ? (unsigned)scale >> 1 : 3;
    if (operand) {
        switch (address_opcode) {
        case 0x11d:
            old = operand->aggregate_value;
            old->location.expression.properties &= ~0x20;
            remove_input_operand(record, old);
            operand->aggregate_value = NULL;
            operand->detail.aggregate_displacement = displacement;
            operand->opcode = 0x128;
            return;
        case 0x120:
            if (!final_address_displacement_fits(base->value.reg, displacement)) {
                value = operand->aggregate_value;
                created = new_global_ternary_expression(0x141, 0x1004, value, base,
                    new_integer_expression(0x1004, displacement), prepend_input_record, (C2Record *)record);
                value->value.reg->value = created;
                return;
            }
            reg = new_address_register(base->value.reg, operand->type, operand_bytes, displacement);
            created = new_typed_register_expression(0x120, operand->type, reg);
            created->location.expression.use_flags = (created->location.expression.use_flags & ~8) |
                (operand->location.expression.use_flags & 8);
            created->location.expression.properties = (created->location.expression.properties & ~0x40) |
                (operand->location.expression.properties & 0x40);
            replace(record, operand, created);
            release_small_node(base);
            return;
        case 0x121:
            if (!final_address_displacement_fits(symbol, displacement)) {
                value = operand->aggregate_value;
                base = new_register_expression(0x121, symbol);
                created = new_global_ternary_expression(0x141, 0x1004, value, base,
                    new_integer_expression(0x1004, displacement), prepend_input_record, (C2Record *)record);
                value->value.reg->value = created;
                return;
            }
            reg = new_address_register(symbol, operand->type, operand_bytes, displacement);
            created = new_typed_register_expression(0x121, operand->type, reg);
            created->location.expression.use_flags = (created->location.expression.use_flags & ~8) |
                (operand->location.expression.use_flags & 8);
            created->location.expression.properties = (created->location.expression.properties & ~0x40) |
                (operand->location.expression.properties & 0x40);
            replace(record, operand, created);
            return;
        case 0x122:
            created = new_aggregate_expression(0x122, operand->type, symbol, displacement,
                base, NULL, operand->immediate);
            created->location.expression.use_flags = (created->location.expression.use_flags & ~15) |
                (operand->location.expression.use_flags & 15);
            created->location.expression.properties = (created->location.expression.properties & ~0x40) |
                (operand->location.expression.properties & 0x40);
            replace(record, operand, created);
            return;
        case 0x123:
            if (scale < 2 && base->kind != 1 && index->kind != 1) {
                value = operand->aggregate_value;
                created = new_global_ternary_expression(0x2d, base->type, value, base, index,
                    prepend_input_record, (C2Record *)record);
                operand->tail.aggregate_register = symbol;
                value->value.reg->value = created;
                operand->detail.aggregate_displacement = displacement;
                return;
            }
            created = new_aggregate_expression(0x123, operand->type, symbol, displacement,
                base, index, operand->immediate);
            created->location.expression.properties = (created->location.expression.properties & ~15) | shift;
            created->location.expression.use_flags = (created->location.expression.use_flags & ~15) |
                (operand->location.expression.use_flags & 15);
            created->location.expression.properties = (created->location.expression.properties & ~0x40) |
                (operand->location.expression.properties & 0x40);
            replace(record, operand, created);
            return;
        case 0x124:
            created = new_aggregate_expression(0x124, operand->type, base->value.reg, displacement,
                NULL, index, operand->immediate);
            created->location.expression.properties = (created->location.expression.properties & ~15) | shift;
            created->location.expression.use_flags = (created->location.expression.use_flags & ~8) |
                (operand->location.expression.use_flags & 8);
            created->location.expression.properties = (created->location.expression.properties & ~0x40) |
                (operand->location.expression.properties & 0x40);
            replace(record, operand, created);
            release_small_node(base);
            return;
        case 0x125:
            created = new_aggregate_expression(0x125, operand->type, symbol, displacement,
                NULL, index, operand->immediate);
            created->location.expression.properties = (created->location.expression.properties & ~15) | shift;
            created->location.expression.use_flags = (created->location.expression.use_flags & ~15) |
                (operand->location.expression.use_flags & 15);
            created->location.expression.properties = (created->location.expression.properties & ~0x40) |
                (operand->location.expression.properties & 0x40);
            replace(record, operand, created);
            return;
        }
        c2_report_internal("E:\\utc\\src\\\\P2\\x86\\addr.c", 961);
        return;
    }
    switch (address_opcode) {
    case 0x11d:
        record->opcode = 1;
        copy_input_operand(record, new_integer_expression(record->type, displacement));
        return;
    case 0x120:
        record->opcode = 0x12;
        if (!displacement) {
            copy_input_operand(record, base);
            return;
        }
        if (!final_address_displacement_fits(base->value.reg, displacement)) {
            record->opcode = 0x141;
            copy_input_operand(record, base);
            copy_input_operand(record, new_integer_expression(0x1004, displacement));
            return;
        }
        reg = new_address_register(base->value.reg, base->value.reg->type, 4, displacement);
        created = new_register_expression(0x120, reg);
        copy_input_operand(record, created);
        release_small_node(base);
        return;
    case 0x121:
        record->opcode = 1;
        if (!final_address_displacement_fits(symbol, displacement)) {
            created = new_final_address_expression(0x126, record->type, symbol, displacement, NULL, NULL);
            copy_input_operand(record, created);
            return;
        }
        if (displacement) symbol = new_address_register(symbol, symbol->type, 4, displacement);
        copy_input_operand(record, new_register_expression(0x121, symbol));
        return;
    case 0x122:
        if (!symbol && !displacement) {
            record->opcode = 1;
            copy_input_operand(record, base);
            return;
        }
        value = record->tail.right;
        if (!((DEFINED_REGISTER(value) && (base->kind != 1 || base->value.reg->value)) ||
              (base->kind == 1 && base->value.reg == value->value.reg)) ||
            (symbol && !final_address_displacement_fits(symbol, displacement))) {
            record->opcode = 0x12;
            created = new_final_address_expression(0x122, record->type, symbol, displacement, base, NULL);
            copy_input_operand(record, created);
            return;
        }
        copy_input_operand(record, base);
        if (!symbol) {
            if (displacement < 1) {
                displacement = (long)(0U - (unsigned)displacement);
                record->opcode = 0x32;
            } else record->opcode = 0x2d;
            copy_input_operand(record, new_integer_expression(record->type, displacement));
            return;
        }
        record->opcode = 0x2d;
        if (displacement) symbol = new_address_register(symbol, symbol->type, 4, displacement);
        copy_input_operand(record, new_register_expression(0x121, symbol));
        return;
    case 0x123:
        value = record->tail.right;
        if (scale < 2 && !displacement && !symbol &&
            (DEFINED_REGISTER(value) || (base->kind == 1 && value->value.reg == base->value.reg)) &&
            (base->kind != 1 || base->original_register->kind != 2 ||
             index->kind != 1 || index->original_register->kind != 2 || base->value.reg == value->value.reg)) {
            record->opcode = 0x2d;
            copy_input_operand(record, base);
            copy_input_operand(record, index);
            if (DEFINED_REGISTER(index)) {
                old = index->value.reg->value;
                if (old->opcode == 1) {
                    attach_input_expression(record, index, old->left);
                    unlink_and_release_input_record((C2Record *)old);
                }
            }
            return;
        }
        record->opcode = 0x12;
        created = new_final_address_expression(0x123, record->type, symbol, displacement, base, index);
        created->location.expression.properties = (created->location.expression.properties & ~15) | shift;
        copy_input_operand(record, created);
        return;
    case 0x124:
        record->opcode = 0x12;
        created = new_final_address_expression(0x124, record->type, base->value.reg, displacement, NULL, index);
        created->location.expression.properties = (created->location.expression.properties & ~15) | shift;
        copy_input_operand(record, created);
        release_small_node(base);
        return;
    case 0x125:
        value = record->tail.right;
        if (!symbol && !displacement &&
            ((DEFINED_REGISTER(value) && (index->kind != 1 || index->value.reg->value)) ||
             (index->kind == 1 && index->value.reg == value->value.reg))) {
            record->opcode = 0x2a;
            copy_input_operand(record, index);
            copy_input_operand(record, new_integer_expression(record->type, shift));
            return;
        }
        record->opcode = 0x12;
        created = new_final_address_expression(0x125, record->type, symbol, displacement, NULL, index);
        created->location.expression.properties = (created->location.expression.properties & ~15) | shift;
        copy_input_operand(record, created);
        return;
    }
    c2_report_internal("E:\\utc\\src\\\\P2\\x86\\addr.c", 1114);
}

void __fastcall prepare_final_address_inputs(C2Expression *record)
{
    C2Expression *operand = record->left, *assignment;
    if (operand->kind == 6) {
        assignment = new_assignment_expression(1, operand->type, operand,
            prepend_input_record, (C2Record *)record);
        attach_input_expression(record, record->left, assignment->tail.right);
    }
    operand = record->left->next;
    if (operand->kind == 6) {
        assignment = new_assignment_expression(1, operand->type, operand,
            prepend_input_record, (C2Record *)record);
        attach_input_expression(record, record->left->next, assignment->tail.right);
    }
}
int __fastcall final_address_value_changes(C2Expression *first, C2Expression *last,
    C2Expression *owner)
{
    __asm { int 3 }
    return 0;
}
int __fastcall final_address_operand_changes(C2Expression *operand, C2Expression *first,
    C2Expression *last, int mode)
{
    __asm { int 3 }
    return 0;
}
int __fastcall final_address_displacement_fits(C2FunctionRegister *reg, long displacement)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall new_final_address_expression(unsigned opcode, unsigned short type,
    C2FunctionRegister *symbol, long displacement, C2Expression *base, C2Expression *index)
{
    __asm { int 3 }
    return NULL;
}
