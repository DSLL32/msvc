#include "global-expressions.h"
#include "optimizer-translate.h"
#include <recon.h>

/* Original 48fae0. The count is bounded to nine values plus the NULL
 * terminator; only the written prefix participates in the current lookup.
 * The forty initialized image bytes are zero, and later calls retain tails. */
C2Expression *global_expression_operands[10];

void __cdecl set_global_expression_operands(unsigned count, ...)
{
    va_list arguments;
    C2Expression *operand;
    unsigned i;

    if (count > 9) c2_report_internal("E:\\utc\\src\\\\P2\\hash.c", 0x26a);
    va_start(arguments, count);
    for (i = 0; i < count; ++i) {
        operand = va_arg(arguments, C2Expression *);
        if (!operand) break;
        global_expression_operands[i] = operand;
    }
    global_expression_operands[i] = NULL;
    va_end(arguments);
}

C2FunctionRegister *__fastcall lookup_global_expression_register(unsigned opcode, unsigned short type)
{
    C2GlobalExpression *expression;
    unsigned hash;

    hash = hash_global_expression(opcode, type);
    expression = find_global_expression(hash, opcode, type);
    if (!expression) expression = create_global_expression(hash, opcode, type);
    return expression->reg;
}

unsigned __fastcall hash_global_expression(unsigned opcode, unsigned short type)
{
    C2Expression **operand;
    unsigned hash;

    hash = ((type & 0xfff) + opcode) % 101;
    for (operand = global_expression_operands; *operand; ++operand)
        hash = (hash + hash_global_operand(*operand)) % 101;
    return hash;
}

unsigned __fastcall hash_global_operand(C2Expression *operand)
{
    if (!operand) return 0;
    switch (operand->kind) {
    case 1: case 2: case 3: case 5: case 6:
        return operand->value.reg->number;
    case 4:
        return operand->value.symbol->serial;
    case 7:
        return operand->integer.low;
    case 8:
        return operand->value.real->significand_low;
    case 10:
        return operand->value.optimizer_index;
    }
    return 0;
}

C2GlobalExpression *__fastcall find_global_expression(unsigned hash,
    unsigned opcode, unsigned short type)
{
    C2GlobalExpression *expression;
    C2Expression *operand;
    unsigned i;

    if (!global_opcode_requires_exact_type(opcode)) {
        for (expression = global_expression_buckets[hash]; expression; expression = expression->next) {
            if (expression->opcode != opcode || !register_types_match(expression->type, type))
                continue;
            for (i = 0, operand = expression->operands; operand; ++i, operand = operand->next)
                if (!global_expression_operands[i] ||
                    compare_input_operands(operand, global_expression_operands[i])) break;
            if (!operand && !global_expression_operands[i]) return expression;
            if (operation_is_associative(opcode)) {
                operand = expression->operands;
                if (!compare_input_operands(operand, global_expression_operands[1]) &&
                    !compare_input_operands(operand->next, global_expression_operands[0]))
                    return expression;
            }
        }
    } else if (opcode == 0x135) {
        for (expression = global_expression_buckets[hash]; expression; expression = expression->next) {
            if (expression->opcode != 0x135 || !register_types_match(expression->type, type))
                continue;
            operand = expression->operands;
            if (!compare_input_operands(operand, global_expression_operands[0]) &&
                operand->type == global_expression_operands[0]->type) return expression;
        }
    } else {
        for (expression = global_expression_buckets[hash]; expression; expression = expression->next) {
            if (expression->opcode != opcode || expression->type != type) continue;
            for (i = 0, operand = expression->operands; operand; ++i, operand = operand->next)
                if (!global_expression_operands[i] ||
                    compare_input_operands(operand, global_expression_operands[i])) break;
            if (!operand && !global_expression_operands[i]) return expression;
            if (operation_is_associative(opcode)) {
                operand = expression->operands;
                if (!compare_input_operands(operand, global_expression_operands[1]) &&
                    !compare_input_operands(operand->next, global_expression_operands[0]))
                    return expression;
            }
        }
    }
    return NULL;
}

int __fastcall global_opcode_requires_exact_type(unsigned opcode)
{
    switch (opcode) {
    case 0x135: case 0x149: case 0x14a: case 0x14b: case 0x151:
    case 0x154: case 0x155: case 0x156: case 0x157:
        return 1;
    }
    return 0;
}

C2GlobalExpression *__fastcall create_global_expression(unsigned hash,
    unsigned opcode, unsigned short type)
{
    C2GlobalExpression *expression;
    C2FunctionRegister *reg;

    expression = c2_pool_allocate(14, 24);
    reg = new_global_result_register(opcode == 0x151 ? 0x8000 : type);
    expression->reg = reg;
    expression->opcode = opcode;
    reg->global_expression = expression;
    expression->type = type;
    expression->operands = clone_global_expression_operands();
    expression->next = global_expression_buckets[hash];
    ++global_expression_count;
    global_expression_buckets[hash] = expression;
    if (opcode == 0x122) {
        ++global_expression_cursor;
        reg->global_next = global_cached_expression_registers;
        global_cached_expression_registers = reg;
    }
    return expression;
}

C2FunctionRegister *__fastcall new_global_result_register(unsigned short type)
{
    C2FunctionRegister *reg;

    reg = new_function_register(15);
    reg->type = type;
    reg->kind = 3;
    reg->value_bytes = type & 0xfff;
    reg->canonical = reg;
    return reg;
}

C2Expression *clone_global_expression_operands(void)
{
    C2Expression *first, *last, *operand;
    C2Expression **next;

    first = retain_input_result(global_expression_operands[0]);
    last = first;
    for (next = global_expression_operands + 1; *next; ++next) {
        operand = retain_input_result(*next);
        last->next = operand;
        last = operand;
    }
    return first;
}

C2FunctionRegister *__fastcall finish_global_assignment(C2Expression *record, int mode)
{
    C2Expression *left, *result;
    C2GlobalExpression *expression;
    C2FunctionRegister *reg, *input, *destination;
    C2GlobalDependency *link;
    C2GlobalRegisterLink *assignment;
    unsigned hash, kind;

    result = record->tail.right;
    left = record->left;
    set_global_expression_operands(2, result, left);
    hash = hash_global_expression(record->opcode, record->type);
    expression = find_global_expression(hash, record->opcode, record->type);
    if (expression) reg = expression->reg;
    else {
        expression = create_global_expression(hash, record->opcode, record->type);
        reg = expression->reg;
        kind = 1U << (left->kind & 31);
        input = kind & (6 | 0x60) ? left->value.reg : NULL;
        destination = result->value.reg;
        if (input) {
            for (link = input->global_dependents; link; link = link->next_dependent)
                if (link->dependent == reg) break;
            if (!link) {
                link = c2_pool_allocate(13, 16);
                link->dependent = reg;
                link->next_dependent = input->global_dependents;
                input->global_dependents = link;
                link->input = input;
                link->next_input = reg->global_dependencies;
                reg->global_dependencies = link;
                if (!reg->global_dependency_set) {
                    reg->global_dependency_set = new_register_set(13);
                    add_register_to_set(reg->global_dependency_set, reg->number);
                }
                if (!input->global_dependency_set) {
                    input->global_dependency_set = new_register_set(13);
                    add_register_to_set(input->global_dependency_set, input->number);
                }
                propagate_global_dependencies(reg);
            }
            if (input->kind == 3) add_register_to_set(global_pending_expressions, reg->number);
        }
        for (link = destination->global_dependents; link; link = link->next_dependent)
            if (link->dependent == reg) break;
        if (!link) {
            link = c2_pool_allocate(13, 16);
            link->dependent = reg;
            link->next_dependent = destination->global_dependents;
            destination->global_dependents = link;
            link->input = destination;
            link->next_input = reg->global_dependencies;
            reg->global_dependencies = link;
            if (!reg->global_dependency_set) {
                reg->global_dependency_set = new_register_set(13);
                add_register_to_set(reg->global_dependency_set, reg->number);
            }
            if (!destination->global_dependency_set) {
                destination->global_dependency_set = new_register_set(13);
                add_register_to_set(destination->global_dependency_set, destination->number);
            }
            propagate_global_dependencies(reg);
        }
        if (mode && input != destination) {
            assignment = c2_pool_allocate(13, 8);
            assignment->reg = reg;
            assignment->next = destination->global_assignments;
            destination->global_assignments = assignment;
        }
    }
    record->value.reg = reg;
    return reg;
}

void __fastcall propagate_global_dependencies(C2FunctionRegister *reg)
{
    C2GlobalDependency *link;
    C2FunctionRegister *canonical, *variant, *input;
    C2RegisterSet *set;
    C2GlobalExpression *expression;
    C2OptimizerConflict *conflict;
    unsigned number;

    link = reg->global_dependencies;
    if (link) {
        set = reg->global_dependency_set;
        do {
            input = link->input;
            if (merge_global_dependency_sets(input->global_dependency_set, set))
                propagate_global_dependencies(input);
            link = link->next_input;
        } while (link);
    }
    canonical = reg->canonical;
    if (canonical->global_expression && canonical->canonical->variant_next) {
        for (variant = canonical; variant; variant = variant->variant_next) {
            if (variant == reg ||
                variant->displacement >= (long)((unsigned)reg->displacement + reg->value_bytes) ||
                reg->displacement >= (long)((unsigned)variant->displacement + variant->value_bytes))
                continue;
            if (!variant->global_dependency_set) {
                variant->global_dependency_set = new_register_set(13);
                add_register_to_set(variant->global_dependency_set, variant->number);
            }
            set = reg->global_dependency_set;
            if (merge_global_dependency_sets(variant->global_dependency_set, set))
                propagate_global_dependencies(variant);
            if (merge_global_dependency_sets(set, variant->global_dependency_set))
                propagate_global_dependencies(reg);
        }
    }
    if (!canonical->symbol) {
        expression = canonical->global_expression;
        if (expression && expression->opcode == 0x122) {
            number = expression->operands->next->immediate;
            conflict = optimizer_conflicts[number];
            if (conflict) {
                set = reg->global_dependency_set;
                do {
                    number = conflict->number;
                    if (global_symbol_registers[number])
                        merge_optimizer_symbol_sets(global_symbol_registers[number], set);
                    merge_optimizer_symbol_sets(global_index_registers[number], set);
                    conflict = conflict->next;
                } while (conflict);
            }
        }
    } else if (canonical->kind == 7 || canonical->kind == 8 ||
               (canonical->view_flags & 2)) {
        number = ((C2InputSymbol *)canonical->symbol)->detail.optimizer_number;
        conflict = optimizer_conflicts[number];
        if (conflict) {
            set = reg->global_dependency_set;
            do {
                merge_optimizer_symbol_sets(global_index_registers[conflict->number], set);
                conflict = conflict->next;
            } while (conflict);
        }
    }
}

unsigned __fastcall merge_global_dependency_sets(C2RegisterSet *destination,
    C2RegisterSet *source)
{
    C2RegisterSetNode *input, *node, **position;
    unsigned group, before, after, changed;

    changed = 0;
    for (input = source->links.first; input; input = input->next) {
        group = input->group & ~31U;
        position = &destination->links.first;
        node = *position;
        while (node && node->group < group) {
            position = &node->next;
            node = *position;
        }
        if (!node || node->group != group) {
            node = new_register_set_node(destination);
            node->group = group;
            node->next = *position;
            *position = node;
        }
        before = node->bits;
        after = before | input->bits;
        node->bits = after;
        changed |= before ^ after;
    }
    return changed;
}

void finish_global_expression_cache(void)
{
    C2GlobalExpression *expression;
    unsigned bucket;

    for (bucket = 0; bucket < 101; ++bucket)
        for (expression = global_expression_buckets[bucket]; expression; expression = expression->next)
            link_global_expression_dependencies(expression->reg);
}

void __fastcall link_global_expression_dependencies(C2FunctionRegister *reg)
{
    C2Expression *operand;
    C2FunctionRegister *input;
    C2GlobalDependency *link;
    unsigned kind;

    for (operand = reg->global_expression->operands; operand; operand = operand->next) {
        kind = 1U << (operand->kind & 31);
        if (kind & 6) {
            input = operand->value.reg;
            for (link = input->global_dependents; link; link = link->next_dependent)
                if (link->dependent == reg) break;
            if (!link) {
                link = c2_pool_allocate(13, 16);
                link->dependent = reg;
                link->next_dependent = input->global_dependents;
                input->global_dependents = link;
                link->input = input;
                link->next_input = reg->global_dependencies;
                reg->global_dependencies = link;
                if (!reg->global_dependency_set) {
                    reg->global_dependency_set = new_register_set(13);
                    add_register_to_set(reg->global_dependency_set, reg->number);
                }
                if (!input->global_dependency_set) {
                    input->global_dependency_set = new_register_set(13);
                    add_register_to_set(input->global_dependency_set, input->number);
                }
                propagate_global_dependencies(reg);
            }
        } else if (kind & 0x60) {
            input = operand->value.reg;
            for (link = input->global_dependents; link; link = link->next_dependent)
                if (link->dependent == reg) break;
            if (!link) {
                link = c2_pool_allocate(13, 16);
                link->dependent = reg;
                link->next_dependent = input->global_dependents;
                input->global_dependents = link;
                link->input = input;
                link->next_input = reg->global_dependencies;
                reg->global_dependencies = link;
                if (!reg->global_dependency_set) {
                    reg->global_dependency_set = new_register_set(13);
                    add_register_to_set(reg->global_dependency_set, reg->number);
                }
                if (!input->global_dependency_set) {
                    input->global_dependency_set = new_register_set(13);
                    add_register_to_set(input->global_dependency_set, input->number);
                }
                propagate_global_dependencies(reg);
            }
        }
    }
}

/* Original49725c: initialized zero; its two optional writers enable/disable
 * the extra operand constraint check. Their bodies remain pending. */
int global_substitution_check_enabled;
extern int global_region_motion_enabled, global_region_analysis_enabled;

C2Expression *__fastcall find_available_global_operand(C2Expression *owner,
    C2Expression *operand)
{
    C2FunctionRegister *reg, *variant, *candidate_reg;
    C2GlobalRegisterLink *assignment;
    C2Expression *candidate, *mark, *scan, *copy, *created;
    unsigned kind, candidate_kind, from_class, to_class;
    unsigned short from_type, to_type;
    unsigned char shift;
    long displacement;
    int compatible, accept_register;

    kind = 1U << (operand->kind & 31);
    reg = (kind & 6) || operand->kind == 6 ? operand->value.reg : NULL;
    if (!reg || input_has_side_effects(operand)) return NULL;
    for (variant = reg->canonical; variant; variant = variant->variant_next) {
        if (variant->displacement > reg->displacement ||
            (long)((unsigned)reg->displacement + reg->value_bytes) >
            (long)((unsigned)variant->displacement + variant->value_bytes)) continue;
        for (assignment = variant->global_assignments; assignment; assignment = assignment->next) {
            candidate_reg = assignment->reg;
            if (candidate_reg->value ||
                !optimizer_set_contains(global_available_registers, candidate_reg->number)) {
                if (global_region_motion_enabled) {
                    mark = candidate_reg->global_definition_record;
                    if (mark && (mark->precision_flags == 7 || mark->precision_flags == 8))
                        candidate_reg->global_definition_record = NULL;
                }
                continue;
            }
            candidate = candidate_reg->global_expression->operands->next;
            if (input_has_side_effects(candidate)) return NULL;
            candidate_kind = 1U << (candidate->kind & 31);
            if (global_substitution_check_enabled && (candidate_kind & 6) &&
                global_operand_is_restricted(candidate) && global_operand_rejects_substitution(operand))
                return NULL;
            if (!((kind & 6) && operand->value.reg->kind == 3 &&
                  (operand->type & 0xf000) == 0x5000) &&
                !(owner->opcode == 0x131 && (owner->type & 0xf000) != 0x5000 &&
                  (!compare_input_operands(owner->tail.right, candidate) ||
                   global_assignment_is_redundant(owner, candidate)))) {
                if ((candidate_kind & 6) && candidate->value.reg->kind == 3) {
                    mark = candidate_reg->global_definition_record;
                    if (global_region_motion_enabled && owner->precision_flags != 2 && mark &&
                        (mark->precision_flags == 7 || mark->precision_flags == 8) &&
                        candidate->value.reg->global_expression &&
                        global_operand_uses_set(candidate, (C2RegisterSet *)mark->previous->value.pointer) &&
                        (operand->type & 0xf000) != 0x5000) {
                        remove_global_record(mark);
                        restore_available_global_operand(candidate->type, candidate, owner);
                        if ((candidate->type & 0xf000) == 0x4000 &&
                            (!((1U << (candidate->kind & 31)) & 6) ||
                             !candidate->value.reg->global_expression ||
                             candidate->value.reg->global_expression->opcode != 0x138)) {
                            created = new_linked_global_expression(0x138, candidate->type,
                                candidate, prepend_input_record, (C2Record *)owner);
                            candidate = created->tail.right;
                            mark_global_precision(created);
                        }
                    } else {
                        accept_register = global_region_analysis_enabled && (kind & 6) &&
                            operand->value.reg->kind == 3 && !(operand->value.reg->rewrite_flags & 1);
                        if (!accept_register && (candidate->value.reg->rewrite_flags & 3))
                            accept_register = !reg->global_dependency_set ||
                                !optimizer_set_contains(reg->global_dependency_set, candidate->value.reg->number);
                        if (!accept_register && operand->kind == 6 &&
                            global_type_has_register_class(operand->type)) accept_register = 1;
                        if (!accept_register && reg->global_expression &&
                            reg->global_expression->opcode == 0x133) accept_register = 1;
                        if (!accept_register) {
                            if (owner->opcode != 0x151) return NULL;
                            for (scan = owner->left; scan; scan = scan->next)
                                if (!compare_input_operands(scan, candidate)) break;
                            if (!scan) return NULL;
                        }
                    }
                }
                candidate_kind = 1U << (candidate->kind & 31);
                if ((candidate_kind & 0x60) &&
                    ((candidate->type & 0xf000) != 0x5000 ||
                     (operand->location.expression.properties & 0x20) ||
                     ((operand->type ^ candidate->type) & 0xfff))) return NULL;
                if ((candidate_kind & 6) && candidate->value.reg->kind == 7 && reg->kind != 7)
                    return NULL;
                from_type = candidate->type;
                to_type = operand->type;
                from_class = from_type >> 12;
                to_class = to_type >> 12;
                compatible = from_type == to_type;
                if (!compatible) {
                    if ((1U << from_class) & 14)
                        compatible = (((1U << to_class) & 14) || to_class == 5) &&
                            (from_type & 0xfff) >= (to_type & 0xfff);
                    else if (from_class == 5)
                        compatible = (to_type & 0xfff) <= (from_type & 0xfff);
                }
                if (!compatible ||
                    ((operand->location.expression.properties & 0x20) &&
                     ((candidate_kind & 0x180) || candidate->kind == 4)) ||
                    (candidate->kind == 8 &&
                     (((to_type & 0xf000) == 0x4000 && (from_type & 0xfff) > 4) ||
                      (to_type & 0xf000) == 0x5000)) ||
                    (owner->precision_flags == 2 && (candidate_kind & 0x180) &&
                     (((1U << (owner->left->kind & 31)) & 0x180) ||
                      ((1U << (owner->left->next->kind & 31)) & 0x180)))) return NULL;
            }
            if (((1U << (candidate->kind & 31)) & 6) && candidate->value.reg->value)
                make_global_register_explicit(candidate->value.reg);
            copy = copy_input_expression(candidate);
            candidate_kind = 1U << (copy->kind & 31);
            if (candidate_kind & 6) {
                displacement = global_register_slice_displacement(variant, reg);
                copy->value.reg = new_address_register(copy->value.reg, reg->type,
                    reg->value_bytes, displacement);
                if (((1U << (copy->kind & 31)) & 6) && copy->value.reg->kind == 3 &&
                    (kind & 6) && operand->value.reg->kind == 3 &&
                    (operand->value.reg->flags & 0x40)) copy->value.reg->flags |= 0x40;
            } else if (copy->kind == 7) {
                displacement = global_register_slice_displacement(variant, reg);
                if (displacement) {
                    shift = (unsigned char)((unsigned)displacement * 8);
                    if ((reg->type & 0xf000) == 0x1000) {
                        if (shift >= 64) copy->integer.value =
                            (__int64)copy->integer.value < 0 ? ~(unsigned __int64)0 : 0;
                        else copy->integer.value = (__int64)copy->integer.value >> shift;
                    } else {
                        if (shift >= 64) copy->integer.value = 0;
                        else copy->integer.value >>= shift;
                    }
                }
            } else if (candidate_kind & 0x18) {
                if ((operand->type ^ copy->type) & 0xfff) {
                    release_small_node(copy);
                    return NULL;
                }
            } else {
                if (copy->kind == 8 || copy->kind == 6) {
                    if (!((operand->type ^ copy->type) & 0xfff)) return copy;
                    release_small_node(copy);
                }
                copy = NULL;
            }
            return copy;
        }
    }
    return NULL;
}

int __fastcall operation_is_comparison(unsigned opcode)
{
    return opcode >= 0x152 && opcode <= 0x157;
}

/* Original497260 is an initialized zero DWORD; the optional global pass
 * suppresses availability additions while this flag is set. */
int global_available_additions_suppressed;

void __fastcall mark_global_register_available(C2FunctionRegister *reg)
{
    if (!global_available_additions_suppressed)
        add_register_to_set(global_available_registers, reg->number);
}

C2RegisterSet *new_temporary_register_set(void)
{
    return new_register_set(6);
}

C2Record *__fastcall new_global_register_set_record(C2RegisterSet *set)
{
    C2Record *record;

    record = (C2Record *)allocate_small_node(26);
    record->owner.registers = set;
    record->opcode = 0x17c;
    record->line = record_line_delta;
    return record;
}

unsigned __fastcall global_operand_uses_set(C2Expression *operand, C2RegisterSet *set)
{
    C2FunctionRegister *reg;
    C2Expression *child;
    unsigned kind;

    while (operand) {
        kind = 1U << (operand->kind & 31);
        if (kind & 6) {
            reg = operand->value.reg;
            if (!reg->global_expression && reg->canonical->global_expression)
                reg = reg->canonical;
            if (reg->global_expression) {
                for (child = reg->global_expression->operands; child; child = child->next)
                    if (!global_operand_uses_set(child, set)) return 0;
                if (optimizer_set_contains(set, reg->number))
                    return reg->global_assignments == NULL;
                return 0;
            }
        }
        if (kind & 0x60) operand = operand->value.reg->global_expression->operands;
        else return !(kind & 6) || !operand->value.reg->value;
    }
    return 1;
}

void __fastcall restore_available_global_operand(unsigned short type,
    C2Expression *operand, C2Expression *owner)
{
    C2GlobalExpression *expression;
    C2Expression *child, *value, *first, *second, *third, *fourth, *fifth, *sixth;
    C2Expression *created, *comparison;
    unsigned opcode, kind;

    if (!operand) return;
    for (;;) {
        kind = 1U << (operand->kind & 31);
        if ((kind & 6) && operand->value.reg->global_expression) break;
        if (!(kind & 0x60)) return;
        operand = operand->value.reg->global_expression->operands;
        /* The original loads this type before its subsequent NULL check. */
        type = operand->type;
        if (!operand) return;
    }
    expression = operand->value.reg->global_expression;
    opcode = expression->opcode;
    if (opcode == 0x13b) return;
    if (!register_types_match(type, expression->type) || global_opcode_requires_exact_type(opcode))
        type = expression->type;
    if (opcode == 0x15f) {
        created = new_global_intrinsic_expression(0x15f, type, operand,
            NULL, NULL, NULL, NULL, (unsigned short)expression->operands->immediate, NULL, NULL);
        for (child = expression->operands->next; child; child = child->next) {
            value = child;
            if (child->kind != 10) {
                restore_available_global_operand(type, child, owner);
                value = new_assignment_expression(0x130, child->type, child,
                    prepend_input_record, (C2Record *)owner)->tail.right;
            }
            copy_input_operand(created, value);
        }
        prepend_input_record((C2Record *)owner, (C2Record *)created);
    } else {
        for (child = expression->operands; child; child = child->next)
            restore_available_global_operand(type, child, owner);
        first = expression->operands;
        second = first->next;
        if (opcode == 0x15e) {
            third = second->next;
            fourth = third->next;
            fifth = fourth->next;
            sixth = fifth->next;
            comparison = new_comparison_expression(0x151, (unsigned short)sixth->immediate,
                first, second, prepend_input_record, (C2Record *)owner);
            created = new_global_selection_expression(0x15e, type, operand,
                comparison->tail.right, third, fourth, (unsigned char)fifth->immediate,
                prepend_input_record, (C2Record *)owner);
        } else if (!second) {
            created = new_binary_expression(opcode, type, operand, first,
                prepend_input_record, (C2Record *)owner);
        } else {
            created = new_global_ternary_expression(opcode, type, operand, first, second,
                prepend_input_record, (C2Record *)owner);
        }
    }
    created->tail.right->value.reg = new_address_register(operand->value.reg, type, type & 0xfff, 0);
    created->tail.right->type = type;
    mark_global_precision(created);
    update_global_available_registers(created);
}

C2Expression *__fastcall new_global_ternary_expression(unsigned opcode,
    unsigned short type, C2Expression *result, C2Expression *first,
    C2Expression *second, C2InputLink link, C2Record *prior)
{
    C2Expression *record;

    record = (C2Expression *)allocate_small_node(11);
    record->opcode = opcode;
    record->type = type;
    record->flags |= 1;
    if ((1U << (result->kind & 31)) & 0x7fe) {
        if (result->location.expression.properties & 0x10)
            result = copy_input_expression(result);
        result->location.expression.properties |= 0x10;
    }
    record->tail.right = result;
    if ((1U << (first->kind & 31)) & 0x7fe) {
        if (first->location.expression.properties & 0x10)
            first = copy_input_expression(first);
        first->location.expression.properties |= 0x10;
    }
    record->left = first;
    if ((1U << (second->kind & 31)) & 0x7fe) {
        if (second->location.expression.properties & 0x10)
            second = copy_input_expression(second);
        second->location.expression.properties |= 0x10;
    }
    first->next = second;
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    return record;
}

long __fastcall global_register_slice_displacement(C2FunctionRegister *source, C2FunctionRegister *view)
{
    return (long)((unsigned)view->displacement - (unsigned)source->displacement);
}

void release_global_expression_cache(void)
{
    C2GlobalExpression *expression;
    C2Expression *operand, *next;
    unsigned bucket;

    for (bucket = 0; bucket < 101; ++bucket) {
        for (expression = global_expression_buckets[bucket]; expression; expression = expression->next) {
            expression->reg->global_expression = NULL;
            for (operand = expression->operands; operand; operand = next) {
                next = operand->next;
                release_small_node((C2SmallNode *)operand);
            }
        }
    }
}
