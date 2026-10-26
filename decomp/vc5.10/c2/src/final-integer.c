#include "global-optimizer.h"
#include "optimizer-translate.h"
#include "code-input.h"
#include "inlining.h"
#include "registers.h"
#include <recon.h>

extern int c2_favor_speed, c2_basic_mode;

void __fastcall prepare_final_record_operands(C2Expression *record);
C2Expression *__fastcall widen_final_integer_record(C2Expression *record);
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);
void __fastcall lower_final_direct_call(C2Expression *record);
C2Expression *__fastcall lower_final_indirect_call(C2Expression *record);
C2Expression *__fastcall new_numbered_machine_operand(unsigned number, unsigned short type);
unsigned __fastcall final_integer_log2(unsigned value);
C2Expression *__fastcall bind_final_fixed_operand(C2Expression *owner,
    C2Expression *operand, C2MachineRegister *reg);
void __fastcall lower_final_shift_count(C2Expression *record);
C2Expression *__fastcall prepare_final_division_operands(unsigned short type,
    C2Expression *definition, C2Expression *temporary, C2Expression **input);
C2Expression *__fastcall lower_final_flags_record(C2Expression *record);
C2Expression *__fastcall lower_final_selection_record(C2Expression *record);
C2Expression *__fastcall lower_final_condition_record(C2Expression *record);
C2Expression *__fastcall lower_final_intrinsic_record(C2Expression *record);
C2Expression *__fastcall lower_final_special_record(C2Expression *record);
void __fastcall append_final_call_input(C2Expression *owner, C2Expression *operand);
void __fastcall append_final_call_output(C2Expression *owner, C2Expression *operand);
C2Expression *__fastcall find_final_register_definition(C2Expression *record,
    C2FunctionRegister *reg);
void __fastcall remove_input_operand(C2Expression *owner, C2Expression *operand);

/* The width-selected column at 494390 names existing machine-register rows.
 * Only these three encountered pointer identities are modeled. Their row
 * payloads remain unavailable; the genuine bitfield constructor is pending. */
C2MachineRegister *final_division_registers[3] = {
    &machine_registers[21], &machine_registers[11], &machine_registers[3]
};
#define DIVISION_REGISTER(type) final_division_registers[(((unsigned)(type)) & 0xffe) >> 1]

C2Expression *__fastcall lower_final_integer_record(C2Expression *record)
{
    C2Expression *operand, *result, *inserted, *temporary, *replacement;
    C2Expression *definition, *next, *marker;
    C2FunctionRegister *reg;
    C2MachineRegister *fixed;
    unsigned width, family, value, magnitude, shift, other_number;
    unsigned low, high, low_type;
    unsigned short type;
    int signed_value;

    if (!((1U << (record->kind & 31)) & 0x1000) && record->kind != 16 &&
        ((record->type & 0xfff) > 4 ||
         (record->left && (record->left->type & 0xfff) > 4)))
        record = widen_final_integer_record(record);
    switch (record->opcode) {
    case 0x130:
        result = record->tail.right;
        if (result->kind != 1 || result->original_register->kind != 1) {
            temporary = new_fixed_register_operand(5, 0x1004);
            replace_input_result(record, result, temporary);
            copy_input_operand(record, new_fixed_register_operand(5, 0x1004));
        }
        break;
    case 0x131: case 0x133: case 0x134:
        record->opcode = 1;
        break;
    case 0x135:
        operand = record->left;
        if ((1U << (operand->kind & 31)) & 0x180) {
            record->opcode = 1;
            fold_input_constant(operand, record->type, record);
            break;
        }
        type = operand->type;
        if ((type & 0xf000) != 0x2000) {
            record->opcode = (type & 0xfff) != 1 ? 0xc5 : 0xc4;
            break;
        }
        if ((record->type & 0xfff) != 2 && c2_favor_speed) {
            if (final_operand_fits_word(operand)) {
                temporary = new_integer_expression(record->type,
                    (operand->type & 0xfff) == 2 ? 0xffff : 0xff);
                copy_input_operand(record, temporary);
                type = record->type;
                operand = record->left;
                operand->value.reg = new_address_register(operand->value.reg,
                    type, type & 0xfff, 0);
                operand->type = type;
                record->opcode = 0x2e;
                break;
            }
            prepare_final_record_operands(record);
            inserted = new_assignment_expression(0x109, 0x1004,
                new_integer_expression(0x1004, 0), prepend_input_record, (C2Record *)record);
            temporary = inserted->tail.right;
            inserted = new_folded_binary_expression(1, record->left->type,
                record->left, temporary, prepend_input_record, (C2Record *)record);
            reg = temporary->original_register;
            type = record->type;
            result = inserted->tail.right;
            result->original_register = reg;
            replacement = new_temporary_register_expression(0x11f, type,
                new_address_register(result->value.reg, type, 4, 0), reg);
            attach_input_expression(record, record->left, replacement);
            record->opcode = 1;
            return inserted;
        }
        record->opcode = (type & 0xfff) != 1 ? 0xc3 : 0xc2;
        break;
    case 0x136: record->opcode = 0x24; break;
    case 0x137: record->opcode = 0x23; break;
    case 0x13e: case 0x13f:
        width = record->type & 0xfff;
        if (width && width <= 8 && !(width & (width - 1)) && !record->tail.right->next) {
            lower_final_direct_call(record);
            break;
        }
        return lower_final_indirect_call(record);
    case 0x140:
        type = record->type;
        reg = new_bitfield_register((C2FunctionRegister *)&machine_registers[1],
            (type & 0xfff) << 3, 0);
        inserted = new_binary_expression(1, type,
            new_numbered_machine_operand(reg->number, type), record->left,
            insert_input_record, (C2Record *)record);
        marker = inserted->next->next;
        inserted->tail.right->value.reg->value = inserted;
        if (!marker || marker->opcode != 0x16c) {
            marker = new_input_expression(0x16c, record->type);
            insert_input_record((C2Record *)inserted, (C2Record *)marker);
        }
        copy_input_operand(marker, copy_input_expression(inserted->tail.right));
        if (record->tail.right) {
            attach_optimizer_output(inserted, record->tail.right);
            result = record->tail.right;
            if (result->kind == 1 && result->value.reg->kind == 3 && result->value.reg->value) {
                result->value.reg->value = inserted;
                unlink_and_release_input_record((C2Record *)record);
                return inserted;
            }
        }
        unlink_and_release_input_record((C2Record *)record);
        return inserted;
    case 0x141: record->opcode = 0x2d; break;
    case 0x142: record->opcode = 0x32; break;
    case 0x143:
        operand = record->left->next;
        if (operand->kind == 7) {
            low = operand->integer.low;
            high = operand->integer.high;
            if (!(((low - 1) & low) | ((high - (low == 0)) & high))) {
                result = record->tail.right;
                definition = NULL;
                if (c2_basic_mode && ((1U << (result->kind & 31)) & 6) && result->value.reg->kind == 3)
                    definition = find_final_register_definition(record, result->value.reg);
                if (!definition || definition->opcode != 0x14c) {
                    operand = record->left->next;
                    signed_value = (int)final_integer_log2(operand->integer.low);
                    operand->integer.low = (unsigned)signed_value;
                    operand->integer.high = (unsigned)(signed_value >> 31);
                    record->opcode = 0x2a;
                    break;
                }
            }
        }
        record->opcode = (record->type & 0xfff) > 1 ? 0xc1 : 6;
        break;
    case 0x144:
        temporary = bind_final_fixed_operand(record, record->left->next, &machine_registers[1]);
        prepare_final_record_operands(temporary->value.reg->value);
        attach_input_expression(record, record->left->next, temporary);
        result = record->tail.right;
        temporary = new_numbered_machine_operand(3, result->type);
        inserted = new_binary_expression(1, record->type, result, temporary,
            insert_input_record, (C2Record *)record);
        if (((1U << (result->kind & 31)) & 6) && result->value.reg->value)
            result->value.reg->value = inserted;
        replace_input_result(record, result, temporary);
        temporary->value.reg->value = record;
        attach_optimizer_output(record, new_fixed_register_operand(1, 0x1004));
        record->opcode = (record->type & 0xf000) == 0x2000 ? 0x22 : 6;
        break;
    case 0x145:
        record->opcode = 0x2a;
        lower_final_shift_count(record);
        break;
    case 0x146: record->opcode = 0x2e; break;
    case 0x147: record->opcode = 0x30; break;
    case 0x148: record->opcode = 0x34; break;
    case 0x149:
        operand = record->left->next;
        if (operand->kind == 7 && !operand->integer.low) compiler_warning(3, 0x2d3);
        family = record->type & 0xf000;
        if (family == 0x2000 && operand->kind == 7) {
            value = operand->integer.low;
            if (value && !(value & (value - 1))) {
                signed_value = (int)final_integer_log2(value);
                operand->integer.low = (unsigned)signed_value;
                operand->integer.high = (unsigned)(signed_value >> 31);
                record->opcode = 0x2b;
                break;
            }
        }
        if (family == 0x1000 && operand->kind == 7) {
            signed_value = (int)operand->integer.low;
            magnitude = ((unsigned)signed_value ^ (unsigned)(signed_value >> 31)) -
                (unsigned)(signed_value >> 31);
            if (signed_value && (c2_favor_speed ? !(magnitude & (magnitude - 1)) : signed_value == 2)) {
                width = (record->type & 0xfff) << 3;
                reg = new_bitfield_register((C2FunctionRegister *)&machine_registers[1], width, 0);
                value = reg->number;
                reg = new_bitfield_register((C2FunctionRegister *)&machine_registers[3], width, 0);
                other_number = reg->number;
                if (value >= C2_MACHINE_REGISTER_COUNT)
                    { __asm { int 3 } }
                fixed = &machine_registers[value];
                temporary = bind_final_fixed_operand(record, record->left, fixed);
                prepare_final_record_operands(temporary->value.reg->value);
                type = record->type;
                replacement = new_temporary_operand(type);
                if (((type & 0xffe) >> 1) >= 3) { __asm { int 3 } }
                replacement->fixed_register = DIVISION_REGISTER(type);
                prepare_final_division_operands(type, temporary->value.reg->value, replacement, &temporary);
                if (magnitude == 2) {
                    inserted = new_folded_binary_expression(0x32, record->type, temporary,
                        replacement, prepend_input_record, (C2Record *)record);
                    result = inserted->tail.right;
                    result->fixed_register = fixed;
                    shift = 1;
                } else {
                    inserted = new_folded_binary_expression(0x2e, record->type, replacement,
                        new_integer_expression(record->type, (__int64)(int)(magnitude - 1)),
                        prepend_input_record, (C2Record *)record);
                    result = inserted->tail.right;
                    if (other_number >= C2_MACHINE_REGISTER_COUNT)
                        { __asm { int 3 } }
                    result->fixed_register = &machine_registers[other_number];
                    inserted = new_folded_binary_expression(0x2d, record->type, temporary,
                        result, prepend_input_record, (C2Record *)record);
                    result = inserted->tail.right;
                    result->fixed_register = fixed;
                    shift = final_integer_log2(magnitude);
                }
                if (signed_value < 1) {
                    inserted = new_folded_binary_expression(0x29, record->type, result,
                        new_integer_expression(0x1004, (__int64)(int)shift),
                        prepend_input_record, (C2Record *)record);
                    result = inserted->tail.right;
                    result->fixed_register = fixed;
                    attach_input_expression(record, record->left, result);
                    remove_input_operand(record, record->left->next);
                    record->opcode = 0x23;
                } else {
                    attach_input_expression(record, record->left, result);
                    record->opcode = 0x29;
                    record->left->next->integer.low = shift;
                }
                break;
            }
        }
        record->opcode = family == 0x2000 ? 0x1f : 0x20;
        break;
    case 0x14a:
        operand = record->left->next;
        if (operand->kind == 7 && !operand->integer.low) compiler_warning(3, 0x2d4);
        if (c2_favor_speed && (record->type & 0xf000) == 0x1000 && operand->kind == 7) {
            value = operand->integer.low;
            magnitude = (value ^ (unsigned)((int)value >> 31)) - (unsigned)((int)value >> 31);
            if (value && !(magnitude & (magnitude - 1))) {
                reg = new_bitfield_register((C2FunctionRegister *)&machine_registers[1],
                    (record->type & 0xfff) << 3, 0);
                if (reg->number >= C2_MACHINE_REGISTER_COUNT)
                    { __asm { int 3 } }
                fixed = &machine_registers[reg->number];
                temporary = bind_final_fixed_operand(record, record->left, fixed);
                prepare_final_record_operands(temporary->value.reg->value);
                type = record->type;
                /* Number is not a modeled field of these static rows yet. */
                { __asm { int 3 } }
                if (((type & 0xffe) >> 1) >= 3) { __asm { int 3 } }
                other_number = DIVISION_REGISTER(type)->number;
                replacement = new_numbered_machine_operand(other_number, type);
                prepare_final_division_operands(type, temporary->value.reg->value, replacement, &temporary);
                inserted = new_folded_binary_expression(0x34, record->type, temporary,
                    replacement, prepend_input_record, (C2Record *)record);
                inserted->tail.right->fixed_register = fixed;
                replacement = new_numbered_machine_operand(other_number, record->type);
                attach_optimizer_output(inserted, replacement);
                result = inserted->tail.right;
                replacement->value.reg->value = inserted;
                inserted = new_folded_binary_expression(0x32, record->type, result,
                    replacement, prepend_input_record, (C2Record *)record);
                inserted->tail.right->fixed_register = fixed;
                replacement = new_numbered_machine_operand(other_number, record->type);
                attach_optimizer_output(inserted, replacement);
                replacement->value.reg->value = inserted;
                inserted = new_folded_binary_expression(0x2e, record->type, inserted->tail.right,
                    new_integer_expression(record->type, (__int64)(int)(magnitude - 1)),
                    prepend_input_record, (C2Record *)record);
                result = inserted->tail.right;
                result->fixed_register = fixed;
                inserted = new_folded_binary_expression(0x34, record->type, result,
                    replacement, prepend_input_record, (C2Record *)record);
                inserted->tail.right->fixed_register = fixed;
                temporary = new_numbered_machine_operand(other_number, record->type);
                attach_optimizer_output(inserted, temporary);
                result = inserted->tail.right;
                temporary->value.reg->value = inserted;
                record->opcode = 0x32;
                attach_input_expression(record, record->left, result);
                attach_input_expression(record, record->left->next, temporary);
                break;
            }
        }
        break;
    case 0x14b:
        record->opcode = (record->type & 0xf000) == 0x2000 ? 0x2b : 0x29;
        lower_final_shift_count(record);
        break;
    case 0x151: record->opcode = 0x2f; break;
    case 0x158: record->opcode = 0xe; break;
    case 0x159:
        operand = record->left->next;
        if (operand->value.reg->kind == 3 && operand->value.reg->value &&
            (operand->value.reg->value->type & 0xf000) == 0x2000) {
            low_type = record->type & 0xff;
            if (low_type > 10) { __asm { int 3 } }
            /* The encountered conversion column maps both integer comparison
             * families to their unsigned forms; retain the high type byte. */
            switch (low_type) {
            case 0: case 1: case 2: break;
            case 3: case 4: case 5: case 6: low_type += 4; break;
            case 7: case 8: case 9: case 10: break;
            }
            record->type = (unsigned short)((record->type & 0xff00) | low_type);
        }
        record->opcode = 0xf;
        break;
    case 0x15a: record->opcode = 0x10; break;
    case 0x15d:
        if (record->branch_flags & 1) return lower_final_flags_record(record);
        break;
    case 0x15e: return lower_final_selection_record(record);
    case 0x15f: return lower_final_condition_record(record);
    case 0x161: case 0x162: case 0x163: case 0x164: case 0x165:
    case 0x166: case 0x167: case 0x168: case 0x169:
        return lower_final_intrinsic_record(record);
    case 0x172: return lower_final_special_record(record);
    case 0x174:
        inserted = new_transfer_expression(8, 0x1004, NULL,
            new_integer_expression(0x1004, 0), prepend_input_record, (C2Record *)record);
        unlink_and_release_input_record((C2Record *)inserted->next);
        return inserted;
    case 0x175:
        if (!(current_function->flags & 0x8000) &&
            !(current_function->symbol->additional_flags & 0x200)) {
            record->opcode = 0x11c;
            break;
        }
        marker = next_global_code_record(record);
        while (marker && marker->opcode == 0x16d) marker = next_global_code_record(marker);
        if (marker && marker->opcode == 0x175) {
            type = record->type;
            if (type != marker->type && (marker->type & 0xfff) < (type & 0xfff)) {
                marker->type = type;
                set_input_expression_type(marker->tail.right, type);
                set_input_expression_type(marker->left, record->type);
            }
            for (operand = record->left; operand; operand = operand->next)
                if (operand->kind == 10) append_final_call_input(marker, operand);
            for (operand = record->tail.right; operand; operand = operand->next)
                if (operand->kind == 10) append_final_call_output(marker, operand);
            record->opcode = 0x11c;
            break;
        }
        inserted = new_binary_expression(1, record->type, record->tail.right,
            record->left, prepend_input_record, (C2Record *)record);
        result = inserted->tail.right;
        next = record->tail.right->next;
        record->tail.right->next = NULL;
        result->next = next;
        operand = record->left;
        result = inserted->left;
        next = operand->next;
        operand->next = NULL;
        result->next = next;
        unlink_and_release_input_record((C2Record *)record);
        return inserted;
    }
    return record;
}

C2Expression * __fastcall widen_final_integer_record(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_fixed_register_operand(unsigned number, unsigned short type)
{
    C2FunctionRegister *reg;

    if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
    reg = (C2FunctionRegister *)&machine_registers[number];
    return new_temporary_register_expression(0x11f, type, reg, reg);
}

void __fastcall lower_final_direct_call(C2Expression *record)
{
    __asm { int 3 }
}

C2Expression * __fastcall lower_final_indirect_call(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall new_numbered_machine_operand(unsigned number, unsigned short type)
{
    C2FunctionRegister *reg;

    if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
    reg = new_temporary_register(type);
    return new_temporary_register_expression(0x11f, type, reg,
        (C2FunctionRegister *)&machine_registers[number]);
}

unsigned __fastcall final_integer_log2(unsigned value)
{
    __asm { int 3 }
    return 0;
}

C2Expression *__fastcall materialize_final_address_input(unsigned mode,
    C2Expression *owner, C2Expression *operand, C2MachineRegister *reg);
C2Expression *__fastcall find_final_register_use(C2FunctionRegister *reg, C2Expression *owner);

C2Expression * __fastcall bind_final_fixed_operand(C2Expression *owner, C2Expression *operand, C2MachineRegister *reg)
{
    C2Expression *replacement, *address, *use;

    /* Entry-domain limit: these narrow static-row numbers are unmodeled.
     * Check before the ordinary original branch or any row payload read. */
    if (reg != &machine_registers[0] && reg != &machine_registers[1] &&
        (operand->type & 0xfff) < 4) { __asm { int 3 } }
    if (reg != &machine_registers[0] && (operand->type & 0xfff) < 4) {
        reg = (C2MachineRegister *)new_bitfield_register(
            (C2FunctionRegister *)&machine_registers[reg->number],
            (operand->type & 0xfff) << 3, 0);
    }
    replacement = materialize_final_address_input(1, owner, operand, reg);
    if (operand->kind == 6) {
        address = operand->aggregate_value;
        if (address && address->kind == 1 && address->value.reg->kind == 3 &&
            address->value.reg->value && address->original_register->kind == 3) {
            use = find_final_register_use(address->original_register, owner);
            if (use) {
                use->value.reg->value = replacement->value.reg->value;
                attach_optimizer_output(replacement->value.reg->value, use);
                remove_input_result(owner, use);
            }
        }
        address = operand->aggregate_second;
        if (address && address->kind == 1 && address->value.reg->kind == 3 &&
            address->value.reg->value && address->original_register->kind == 3) {
            use = find_final_register_use(address->original_register, owner);
            if (use) {
                use->value.reg->value = replacement->value.reg->value;
                attach_optimizer_output(replacement->value.reg->value, use);
                remove_input_result(owner, use);
            }
        }
    }
    replace_final_input(owner, operand, replacement, attach_input_expression);
    return replacement;
}
C2Expression *__fastcall find_final_register_use(C2FunctionRegister *reg, C2Expression *owner)
{
    C2Expression *operand;

    for (operand = owner->tail.right; operand; operand = operand->next)
        if (operand->kind == 1 && operand->original_register == reg) break;
    return operand;
}

C2Expression *__fastcall retype_final_operand(C2Expression *operand,
    unsigned short type, long displacement);

void __fastcall lower_final_shift_count(C2Expression *record)
{
    C2Expression *count, *definition, *input;

    count = record->left->next;
    if (!((1U << (count->kind & 31)) & 6)) return;
    definition = count->value.reg->value;
    if (!definition || definition->opcode != 1) return;
    input = definition->left;
    if (input->kind == 1 && input->value.reg->kind == 3 && input->value.reg->value) return;
    retype_final_operand(count, 0x1001, 0);
    retype_final_operand(definition->tail.right, 0x1001, 0);
    retype_final_operand(definition->left, 0x1001, 0);
    definition->type = 0x1001;
}

C2Expression *__fastcall retype_final_operand(C2Expression *operand,
    unsigned short type, long displacement)
{
    __asm { int 3 }
    return NULL;
}

/* Exact width-selected columns: three DWORD opcodes at499f80 and three
 * machine-row pointers at4943a8. Their neighbors and row payloads are unmodeled. */
static unsigned final_sign_extension_opcodes[3] = {0x99, 0x9f, 0xdf};
static C2MachineRegister *final_sign_extension_registers[3] = {
    &machine_registers[17], &machine_registers[9], &machine_registers[1]
};

C2Expression *__fastcall prepare_final_division_operands(unsigned short type,
    C2Expression *definition, C2Expression *temporary, C2Expression **input)
{
    C2Expression *record, *result;
    unsigned selector = (type & 0xfff) >> 1;
    if (selector >= 3) { __asm { int 3 } }
    record = new_assignment_expression(final_sign_extension_opcodes[selector], type,
        *input, insert_input_record, (C2Record *)definition);
    if (temporary->fixed_register == &machine_registers[21]) {
        *input = retype_final_operand(record->tail.right, (record->type & 0xf002) | 2, 0);
        (*input)->value.reg->value = record;
    } else {
        temporary->fixed_register = final_division_registers[selector];
        attach_optimizer_output(record, temporary);
        temporary->value.reg->value = record;
    }
    result = record->tail.right;
    *input = result;
    selector = (result->type >> 1) & 0x7ff;
    if (selector >= 3) { __asm { int 3 } }
    result->fixed_register = final_sign_extension_registers[selector];
    return record;
}

C2Expression * __fastcall lower_final_flags_record(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall lower_final_selection_record(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall lower_final_condition_record(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall lower_final_intrinsic_record(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall lower_final_special_record(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

void __fastcall append_final_call_input(C2Expression *owner, C2Expression *operand)
{
    __asm { int 3 }
}

void __fastcall append_final_call_output(C2Expression *owner, C2Expression *operand)
{
    __asm { int 3 }
}
