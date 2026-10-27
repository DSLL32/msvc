#include "global-optimizer.h"
#include "optimizer-translate.h"
#include "inlining.h"
#include "registers.h"
#include <recon.h>

/* Register equivalence chains and cached values reuse the actual+44/+48/+52
 * scratch fields. Heads begin NULL; every exposed link/value field is written
 * by this pass or its genuine child, with no static initial-default claim. */
#define CACHE_REAL_REGISTER(reg) do { \
    if ((reg) == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) \
        { __asm { int 3 } } \
} while (0)
#define CACHE_KNOWN_ROOT(reg) do { \
    if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[0] && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[5] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } \
} while (0)
#define CACHE_ROOT(reg) ((reg) == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT) ? \
    machine_register_storage.sentinel.canonical : (reg)->canonical)
#define CACHE_KNOWN_RANGE(reg) do { \
    if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[0] && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } \
} while (0)
#define CACHE_KNOWN_NUMBER(reg) do { \
    if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[0] && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[31] && \
        (reg) != (C2FunctionRegister *)&machine_registers[32] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } \
} while (0)

/* Only the opcode's write-window BYTE column is consumed.0/1/2 limit the
 * result walk;100 is the original sentinel for all ordinary output operands.
 * Other columns and neighboring data are not represented. */
#define CACHE_OUTPUT_LIMIT(op, count) do { \
    if ((op) >= 0x10b && (op) <= 0x11a) (count) = 1; \
    else if ((op) == 0x6c || (op) == 0x6e || (op) == 0x70) (count) = 1; \
    else if (((op) >= 0x45 && (op) <= 0x93) || \
             ((op) >= 0xd1 && (op) <= 0xd8) || \
             ((op) >= 0x102 && (op) <= 0x104)) (count) = 0; \
    else switch (op) { \
    case 0xf: case 0x10: case 0x2f: case 0x33: case 0xa8: case 0xa9: \
    case 0xae: case 0xc6: case 0xc7: case 0x107: case 0x108: case 0x10a: \
        (count) = 0; break; \
    case 1: case 0xa: case 0xb: case 0xd: case 0x12: case 0x1e: case 0x21: \
    case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x28: \
    case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: \
    case 0x30: case 0x31: case 0x32: case 0x34: case 0x94: case 0x99: \
    case 0xa3: case 0xac: case 0xad: case 0xba: case 0xc1: case 0xc2: \
    case 0xc3: case 0xc4: case 0xc5: case 0xca: case 0xcb: case 0xd0: case 0x109: \
        (count) = 1; break; \
    case 4: case 6: case 9: case 0xc: case 0x1f: case 0x20: case 0x22: \
    case 0x9f: case 0xa7: case 0xb9: case 0xd9: case 0xda: case 0xde: case 0xdf: \
        (count) = 2; break; \
    default: (count) = 100; break; \
    } \
} while (0)

void __fastcall initialize_final_value_reuse(void);
void __fastcall finish_final_value_reuse(void);
C2Expression *__fastcall select_final_cached_value(C2Expression *record, C2FunctionRegister **reg);
C2FunctionRegister *__fastcall add_final_cached_value(C2FunctionRegister *groups,
    C2FunctionRegister *reg, C2Expression *value);
int __fastcall final_cached_operand_aliases(C2Expression *value, C2Expression *output);
int __fastcall fold_final_cached_output(C2Expression *record, C2FunctionRegister *reg);
int __fastcall replace_final_cached_constant(C2Expression *record, C2Expression *input,
    C2FunctionRegister *reg);
int __fastcall final_cached_input_is_usable(C2Expression *record, C2Expression *input);
void __fastcall note_final_cached_memory_use(C2Expression *record, C2Expression *value);
void __fastcall remove_control_record(C2Expression *record);
void __fastcall redirect_allocation_branch(C2Expression *branch, C2Expression *label, int copy);
extern int c2_favor_speed;

/* This pass now encounters the modeled byte-view canonical/range columns.
 * Their numbers, links and unrelated payload remain unavailable. */
#define REUSE_KNOWN_ROOT(reg) do { \
    if ((reg) != (C2FunctionRegister *)&machine_registers[9] && \
        (reg) != (C2FunctionRegister *)&machine_registers[17] && \
        (reg) != (C2FunctionRegister *)&machine_registers[20] && \
        (reg) != (C2FunctionRegister *)&machine_registers[31] && \
        (reg) != (C2FunctionRegister *)&machine_registers[32] && \
        (reg) != (C2FunctionRegister *)&machine_registers[33] && \
        (reg) != (C2FunctionRegister *)&machine_registers[34] && \
        (reg) != (C2FunctionRegister *)&machine_registers[35]) CACHE_KNOWN_ROOT(reg); \
} while (0)
#define REUSE_KNOWN_RANGE(reg) do { \
    if ((reg) != (C2FunctionRegister *)&machine_registers[9] && \
        (reg) != (C2FunctionRegister *)&machine_registers[17] && \
        (reg) != (C2FunctionRegister *)&machine_registers[20]) CACHE_KNOWN_RANGE(reg); \
} while (0)
void __fastcall optimize_final_value_reuse(C2Function *function)
{
    C2Expression *record, *next, *input, *output, *value, *value_next, *previous_value;
    C2Expression *replacement, *candidate, *scan, *branch, *previous;
    C2FunctionRegister *groups = NULL, *group, *member, *next_member, *next_group;
    C2FunctionRegister *group_head, *previous_group, *previous_member, *sibling;
    C2FunctionRegister *source, *destination, *source_group;
    C2FunctionRegister *destination_group, *reg, *original, *root;
    unsigned limit, position, opcode, condition;
    int copy = 0, matched, killed;

    initialize_final_value_reuse();
    record = (C2Expression *)function->flow->last->records;
    while (record) {
        next = record->next;
        if (!(record->flags & 1)) {
            if (record->kind == 25) groups = NULL;
            goto next_record;
        }
        if (record->opcode == 1 && record->tail.right->kind == 1 && record->left->kind == 1) {
            source = record->left->original_register;
            CACHE_REAL_REGISTER(source);
            source_group = NULL;
            for (group = groups; group; group = group->cache_group_next) {
                for (member = group; member && member != source; member = member->cache_equivalent_next) ;
                if (member) { source_group = group; break; }
            }
            destination = record->tail.right->original_register;
            CACHE_REAL_REGISTER(destination);
            destination_group = NULL;
            for (group = groups; group; group = group->cache_group_next) {
                for (member = group; member && member != destination; member = member->cache_equivalent_next) ;
                if (member) { destination_group = group; break; }
            }
            if (source_group && source_group == destination_group) {
                unlink_and_release_input_record((C2Record *)record);
                goto next_record;
            }
            copy = 1;
        }
        if (groups) {
            for (input = record->left; input; input = input->next) {
                switch (input->kind) {
                case 2: case 6:
                    for (group = groups; group; group = group->cache_group_next) {
                        for (value = group->cache_values; value; value = value->cached_next)
                            if (!compare_input_operands(value, input)) break;
                        if (value) break;
                    }
                    if (value && !(input->location.expression.properties & 0x40) &&
                        final_cached_input_is_usable(record, input)) {
                        replacement = new_temporary_register_expression(0x11f, input->type, group, group);
                        attach_input_expression(record, input, replacement);
                        note_final_cached_memory_use(record, value);
                        next = record;
                        goto next_record;
                    }
                    break;
                case 3:
                    for (group = groups; group; group = group->cache_group_next) {
                        for (value = group->cache_values; value; value = value->cached_next)
                            if (value->kind == 3 && value->value.pointer == input->value.pointer) break;
                        if (value) break;
                    }
                    if (value) {
                        replacement = new_temporary_register_expression(0x11f, input->type, group, group);
                        attach_input_expression(record, input, replacement);
                        next = record;
                        goto next_record;
                    }
                    break;
                case 7:
                    for (group = groups; group; group = group->cache_group_next) {
                        for (value = group->cache_values; value; value = value->cached_next)
                            if (value->kind == 7 && value->integer.low == input->integer.low) break;
                        if (value) break;
                    }
                    if (value && replace_final_cached_constant(record, input, group)) {
                        next = record;
                        goto next_record;
                    }
                    break;
                }
            }
        }

        previous_group = NULL;
        for (group = groups; group; group = next_group) {
            next_group = group->cache_group_next;
            previous_value = NULL;
            for (value = group->cache_values; value; value = value_next) {
                value_next = value->cached_next;
                killed = 0;
                if (value->kind != 3 && value->kind != 7)
                    for (output = record->tail.right; output; output = output->next)
                        if (final_cached_operand_aliases(value, output)) { killed = 1; break; }
                if (killed) {
                    if (previous_value) previous_value->cached_next = value_next;
                    else for (sibling = group; sibling; sibling = sibling->cache_equivalent_next)
                        sibling->cache_values = value_next;
                } else previous_value = value;
            }
            group_head = group;
            previous_member = NULL;
            for (member = group; member; member = next_member) {
                next_member = member->cache_equivalent_next;
                limit = 100;
                opcode = record->opcode;
                CACHE_OUTPUT_LIMIT(opcode, limit);
                output = record->tail.right;
                for (position = 0; output && position < limit; ++position, output = output->next) {
                    if (output->kind == 1) {
                        original = output->original_register;
                        REUSE_KNOWN_ROOT(original);
                        REUSE_KNOWN_ROOT(member);
                        if (CACHE_ROOT(original) == CACHE_ROOT(member)) {
                            if (original == member) break;
                            REUSE_KNOWN_RANGE(original);
                            REUSE_KNOWN_RANGE(member);
                            if ((long)((unsigned)member->value_bytes + (unsigned)member->displacement) > original->displacement &&
                                (long)((unsigned)original->value_bytes + (unsigned)original->displacement) > member->displacement)
                                break;
                        }
                    }
                    if (output->kind == 9) {
                        REUSE_KNOWN_ROOT(member);
                        root = CACHE_ROOT(member);
                        CACHE_KNOWN_NUMBER(root);
                        if (optimizer_set_contains(output->value.registers, root->number)) break;
                    }
                }
                if (!output || position >= limit) {
                    previous_member = member;
                    continue;
                }
                if (output == record->tail.right && fold_final_cached_output(record, member)) {
                    if (next->kind == 16 && !next->detail.data && next->opcode != 0x15c &&
                        next->left->kind == 4 &&
                        ((C2InputSymbol *)next->left->value.symbol)->input_operand) {
                        previous = next->previous;
                        if (previous->kind == 16 && (previous->detail.data || previous->opcode == 0x15c) &&
                            next->next->kind == 25 &&
                            ((C2InputSymbol *)previous->left->value.symbol)->input_operand == next->next) {
                            condition = previous->convention;
                            if (condition > 16) { __asm { int 3 } }
                            previous->convention = input_inverse_condition[condition];
                            redirect_allocation_branch(previous,
                                ((C2InputSymbol *)next->left->value.symbol)->input_operand, 0);
                            remove_control_record(next);
                            next = previous->next;
                        }
                    }
                    goto next_record;
                }
                if (previous_member) previous_member->cache_equivalent_next = next_member;
                else {
                    group_head = next_member;
                    if (group_head) group_head->cache_group_next = next_group;
                    if (previous_group) previous_group->cache_group_next = group_head ? group_head : next_group;
                    else groups = group_head ? group_head : next_group;
                }
            }
            if (!group_head) continue;
            if (!group_head->cache_values && !group_head->cache_equivalent_next) {
                if (previous_group) previous_group->cache_group_next = next_group;
                else groups = next_group;
                if (copy && source_group == group_head) source_group = NULL;
            } else previous_group = group_head;
        }

        if (copy) {
            copy = 0;
            if (!source_group) {
                source->cache_group_next = groups;
                source->cache_equivalent_next = destination;
                source->cache_values = NULL;
                destination->cache_equivalent_next = NULL;
                destination->cache_values = NULL;
                groups = source;
            } else {
                sibling = source->cache_equivalent_next;
                source->cache_equivalent_next = destination;
                destination->cache_equivalent_next = sibling;
                destination->cache_values = source->cache_values;
            }
            goto next_record;
        }
        if (record->opcode == 1 && record->tail.right->kind == 1 &&
            (record->left->kind == 2 || record->left->kind == 6)) {
            if (!final_cached_operand_aliases(record->left, record->tail.right) &&
                !(record->left->location.expression.properties & 0x40)) {
                reg = record->tail.right->original_register;
                CACHE_REAL_REGISTER(reg);
                reg->cache_group_next = groups;
                reg->cache_equivalent_next = NULL;
                record->left->cached_next = NULL;
                reg->cache_values = record->left;
                groups = reg;
            }
            goto next_record;
        }
        candidate = select_final_cached_value(record, &reg);
        if (!candidate) {
            if (record->opcode == 1 && record->left->kind == 1 &&
                (record->tail.right->kind == 2 || record->tail.right->kind == 6) &&
                !(record->tail.right->location.expression.properties & 0x40)) {
                groups = add_final_cached_value(groups, record->left->original_register, record->tail.right);
            }
            goto next_record;
        }
        if (!(candidate->location.expression.properties & 0x10)) {
            matched = 0;
            for (group = groups; group; group = group->cache_group_next) {
                for (value = group->cache_values; value; value = value->cached_next)
                    if (value == candidate) { matched = 1; break; }
                if (matched) break;
            }
            if (matched) {
                replacement = copy_input_expression(candidate);
                input = candidate->next;
                candidate->next = replacement;
                replacement->next = input;
                candidate = replacement;
            }
        }
        CACHE_REAL_REGISTER(reg);
        groups = add_final_cached_value(groups, reg, candidate);
next_record:
        record = next;
    }
    finish_final_value_reuse();
}

/* Separate unreconstructed genuine entries. */
/*4920d0: this pass owns a list of integer operands used by its selector.
 * The real initializer establishes the head before any consumer runs. */
C2Expression *final_cached_constants;

#undef REUSE_KNOWN_ROOT
#undef REUSE_KNOWN_RANGE

void __fastcall initialize_final_value_reuse(void)
{
    final_cached_constants = new_integer_expression(0x1004, 0);
}
void __fastcall finish_final_value_reuse(void)
{
    C2Expression *value = final_cached_constants, *next;
    while (value) {
        next = value->next;
        release_small_node(value);
        value = next;
    }
}
C2Expression * __fastcall select_final_cached_value(C2Expression *record, C2FunctionRegister **reg)
{
    C2Expression *input, *value, *definition;
    C2FunctionRegister *source;

    switch (record->opcode) {
    case 1:
        if (record->tail.right->kind != 1) break;
        input = record->left;
        if (input->kind != 7 && input->kind != 3) break;
        *reg = record->tail.right->original_register;
        return input;
    case 0xc:
        if (record->tail.right->kind == 1 && record->previous->opcode == 0xd) {
            input = record->previous->left;
            if (input->kind == 7) {
                *reg = record->tail.right->original_register;
                return input;
            }
        }
        break;
    case 0x2f:
        input = record->left;
        if (input->kind != 1) break;
        value = input->next;
        if (value->kind == 7 && record->next->opcode == 0xf &&
            record->next->convention == 2 && !(record->flags & 4)) {
            *reg = input->original_register;
            return value;
        }
        break;
    case 0x33:
        input = record->left;
        if (input->kind != 1) break;
        value = input->next;
        if (value->kind != 1) break;
        if (input->original_register != value->original_register) {
            source = value->value.reg;
            CACHE_REAL_REGISTER(source);
            /* Of the static rows, only flags row66 has a modeled definition.
             * Dynamic register definitions retain their actual typed field. */
            if ((unsigned long)source >= (unsigned long)machine_registers &&
                (unsigned long)source < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
                source != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } }
            definition = source->value;
            if ((unsigned char)definition->opcode != 0xd) break;
            value = definition->aggregate_second;
            if (value->kind != 7 || value->integer.low) break;
        }
        if (record->next->opcode == 0xf && record->next->convention == 2 && !(record->flags & 4)) {
            *reg = input->original_register;
            return final_cached_constants;
        }
        break;
    case 0x34:
        input = record->left;
        if (input->kind == 1 && input->next->kind == 1 &&
            input->original_register == input->next->original_register) {
            *reg = record->tail.right->original_register;
            return final_cached_constants;
        }
        break;
    }
    return NULL;
}
C2FunctionRegister * __fastcall add_final_cached_value(C2FunctionRegister *groups, C2FunctionRegister *reg, C2Expression *value)
{
    C2FunctionRegister *group, *member;
    CACHE_REAL_REGISTER(reg);
    for (group = groups; group; group = group->cache_group_next) {
        CACHE_REAL_REGISTER(group);
        for (member = group; member && member != reg; member = member->cache_equivalent_next)
            CACHE_REAL_REGISTER(member);
        if (member) break;
    }
    if (!group) {
        reg->cache_group_next = groups;
        reg->cache_equivalent_next = NULL;
        reg->cache_values = value;
        value->cached_next = NULL;
        return reg;
    }
    value->cached_next = group->cache_values;
    for (member = group; member; member = member->cache_equivalent_next) {
        CACHE_REAL_REGISTER(member);
        member->cache_values = value;
    }
    return groups;
}
int __fastcall final_cached_operand_aliases(C2Expression *value, C2Expression *output)
{
    C2FunctionRegister *root, *reg;
    if (output->kind != 1) return input_operands_may_alias(output, value);
    if (value->kind != 6) return 0;
    reg = output->original_register;
    if (reg != (C2FunctionRegister *)&machine_registers[17] &&
        reg != (C2FunctionRegister *)&machine_registers[20]) CACHE_KNOWN_ROOT(reg);
    root = CACHE_ROOT(reg);
    if (value->aggregate_value) {
        reg = value->aggregate_value->original_register;
        if (reg != (C2FunctionRegister *)&machine_registers[17] &&
        reg != (C2FunctionRegister *)&machine_registers[20]) CACHE_KNOWN_ROOT(reg);
        if (CACHE_ROOT(reg) == root) return 1;
    }
    if (value->aggregate_second) {
        reg = value->aggregate_second->original_register;
        if (reg != (C2FunctionRegister *)&machine_registers[17] &&
        reg != (C2FunctionRegister *)&machine_registers[20]) CACHE_KNOWN_ROOT(reg);
        if (CACHE_ROOT(reg) == root) return 1;
    }
    return 0;
}
C2Expression *__fastcall find_following_flags_use(C2Expression *record);
void __fastcall remove_input_operand(C2Expression *owner, C2Expression *operand);
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);
int __fastcall fold_final_cached_output(C2Expression *record, C2FunctionRegister *reg)
{
    C2Expression *value, *previous, *input, *copy, *next;
    C2FunctionRegister *next_reg;
    unsigned constant;
    long difference;

    CACHE_REAL_REGISTER(reg);
    for (value = reg->cache_values; value; value = value->cached_next)
        if (value->kind == 7) break;
    if (!value || record->tail.right->kind != 1 || record->tail.right->original_register != reg)
        return 0;
    constant = value->integer.low;
    switch (record->opcode) {
    case 1:
        if (record->left->kind != 7 || c2_favor_speed || find_following_flags_use(record)) break;
        difference = (long)(record->left->integer.low - constant);
        if (difference < -128 || difference >= 128) break;
        remove_input_operand(record, record->left);
        copy = copy_input_expression(record->tail.right);
        copy->next = record->left;
        record->left = copy;
        if (difference == 1 || difference == -1) {
            record->opcode = difference == 1 ? 0x21 : 0x1e;
            attach_optimizer_output(record, new_fixed_register_operand(0x43, 0x8000));
        } else {
            record->opcode = 0x2d;
            attach_optimizer_output(record, new_fixed_register_operand(0x42, 0x8000));
            input = new_integer_expression(record->type, difference);
            copy = record->left;
            input->next = copy->next;
            copy->next = input;
        }
        break;
    case 0xc:
        previous = record->previous;
        if (previous->opcode == 0xd) {
            input = previous->left;
            if ((input->kind == 7 && input->integer.low == constant) ||
                (input->kind == 1 && input->original_register == reg)) {
                unlink_and_release_input_record((C2Record *)previous);
                unlink_and_release_input_record((C2Record *)record);
                return 1;
            }
        }
        break;
    case 0x2d:
        if (!constant && !find_following_flags_use(record)) {
            record->opcode = 1;
            remove_input_operand(record, record->left);
        }
        break;
    case 0x34:
        if (constant || record->left->next->kind != 1 ||
            record->left->next->original_register != reg) break;
        next = record->tail.right->next;
        if (next && next->kind == 1) {
            next_reg = next->value.reg;
            CACHE_REAL_REGISTER(next_reg);
            if ((unsigned long)next_reg >= (unsigned long)machine_registers &&
                (unsigned long)next_reg < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
                next_reg != (C2FunctionRegister *)&machine_registers[1] &&
                next_reg != (C2FunctionRegister *)&machine_registers[2] &&
                next_reg != (C2FunctionRegister *)&machine_registers[3] &&
                next_reg != (C2FunctionRegister *)&machine_registers[6] &&
                next_reg != (C2FunctionRegister *)&machine_registers[7] &&
                next_reg != (C2FunctionRegister *)&machine_registers[18] &&
                next_reg != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } }
            if (next_reg->kind != 1) break;
        }
        unlink_and_release_input_record((C2Record *)record);
        return 1;
    }
    return 0;
}
C2Expression *__fastcall find_following_flags_use(C2Expression *record)
{ __asm { int 3 } return NULL; }
int __fastcall replace_final_cached_constant(C2Expression *record, C2Expression *input, C2FunctionRegister *reg)
{ __asm { int 3 } return 0; }
int __fastcall final_cached_input_is_usable(C2Expression *record, C2Expression *input)
{
    unsigned opcode;

    if ((record->type & 0xf000) == 0x4000) return 0;
    opcode = record->opcode;
    if (opcode > 0 && opcode < 0x11b) {
        /* The existing property columns are represented by their proved
         * semantic ranges, without reading the other packed WORD bits. */
        if ((opcode >= 0x45 && opcode <= 0x94) ||
            (opcode >= 0xd1 && opcode <= 0xd7)) return 0;
        if (opcode == 9 || opcode == 0x1e || opcode == 0x21 ||
            (opcode >= 0x23 && opcode <= 0x2e) ||
            (opcode >= 0x30 && opcode <= 0x32) || opcode == 0x34 ||
            (opcode >= 0x45 && opcode <= 0x51) ||
            (opcode >= 0x54 && opcode <= 0x55) ||
            (opcode >= 0x57 && opcode <= 0x59) ||
            (opcode >= 0x76 && opcode <= 0x77) || opcode == 0x8c ||
            opcode == 0x9f || opcode == 0xc1 ||
            (opcode >= 0xca && opcode <= 0xcb) ||
            (opcode >= 0xd6 && opcode <= 0xd7) ||
            (opcode >= 0xd9 && opcode <= 0xda))
            return input != record->left;
    }
    return 1;
}
void __fastcall note_final_cached_memory_use(C2Expression *record, C2Expression *value)
{ __asm { int 3 } }
