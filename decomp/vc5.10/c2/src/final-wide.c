#include "global-optimizer.h"
#include "optimizer-translate.h"
#include "code-input.h"
#include "inlining.h"
#include "registers.h"
#include <recon.h>

typedef struct C2WideOpcodeMap { unsigned input, output; } C2WideOpcodeMap;
/* High-word arithmetic and logical operations; the final key terminates the
 * lookup, whose terminator value is not consumed. */
static const C2WideOpcodeMap wide_opcodes[] = {
    {0x141,0x2c}, {0x142,0x31}, {0x148,0x34}, {0x146,0x2e},
    {0x147,0x30}, {0x136,0x24}, {0x137,0x23}, {0x11c,0}
};
void __fastcall prepare_final_record_operands(C2Expression *record);
C2Expression *__fastcall lower_final_record(C2Expression *record);
C2Expression *__fastcall split_final_wide_result(C2Expression *record,
    C2Expression *operand, C2Expression **replacement);
C2Expression *__fastcall lower_final_direct_call(C2Expression *record);
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);
C2Expression *__fastcall new_numbered_machine_operand(unsigned number, unsigned short type);
C2Expression *__fastcall new_final_move_expression(unsigned opcode, unsigned short type,
    C2Expression *input, C2Expression *result, C2Expression *owner);
C2Expression *__fastcall insert_final_constant_assignment(C2Expression *temporary,
    C2Expression *definition, long value, unsigned bind_result);
C2Expression *__fastcall new_final_stack_count(C2Record *prior);
C2InputSymbol *new_flow_label(void);
void __fastcall narrow_final_wide_conversion(C2Expression *record);
unsigned __fastcall lookup_wide_opcode(const C2WideOpcodeMap *table, unsigned opcode);
C2Expression *__fastcall simplify_final_wide_and(C2Expression *record);
C2Expression *__fastcall lower_final_wide_operation(C2Expression *record);
C2Expression *__fastcall lower_final_wide_intrinsic(unsigned opcode, C2Expression *record);

C2Expression *__fastcall widen_final_integer_record(C2Expression *record)
{
    C2Expression *returned = record, *inserted = NULL, *input, *output, *other;
    C2Expression *first_high, *second_high, *output_high, *use, *use_operand;
    C2Expression *created, *comparison, *flags, *branch, *label, *first_copy, *second_copy;
    C2Expression *definition, *extra = NULL;
    unsigned opcode, condition, first_condition, second_condition, word;
    unsigned short type;

    if ((record->type & 0xfff) != 8 &&
        (!record->left || (record->left->type & 0xfff) < 5)) return record;
    prepare_final_record_operands(record);
    opcode = record->opcode;
    switch (opcode) {
    case 0x131:
        record->opcode = 1;
        /* fall through */
    case 1:
        record->type = (record->type & 0xf000) | 4;
        input = record->left;
        if (input->kind == 6 && input->aggregate_value &&
            input->aggregate_value->kind == 1 &&
            input->aggregate_value->original_register->kind == 2 &&
            input_operands_may_alias(record->tail.right, input->aggregate_value)) {
            other = input->aggregate_value;
            created = new_assignment_expression(1, other->type, other,
                prepend_input_record, (C2Record *)record);
            attach_input_expression(record, other, created->tail.right);
        }
        first_high = split_final_wide_result(record, record->left, NULL);
        output_high = split_final_wide_result(record, record->tail.right, NULL);
        inserted = new_binary_expression(1, record->type, output_high, first_high,
            insert_input_record, (C2Record *)record);
        input = record->left;
        if (((1U << (input->kind & 31)) & 0x7e))
            inserted->left->location.expression.properties =
                (inserted->left->location.expression.properties & ~0x40) |
                (input->location.expression.properties & 0x40);
        output = inserted->tail.right;
        if (((1U << (output->kind & 31)) & 6) && output->value.reg->value)
            output->value.reg->value = inserted;
        break;

    case 0x130:
        output = record->tail.right;
        record->type = (record->type & 0xf004) | 4;
        if (output->kind == 1 && output->original_register->kind == 1) {
            first_high = split_final_wide_result(record, record->left, NULL);
            output_high = split_final_wide_result(record, record->tail.right, NULL);
            inserted = new_binary_expression(1, record->type, output_high, first_high,
                insert_input_record, (C2Record *)record);
        } else {
            first_high = split_final_wide_result(record, record->left, &extra);
            inserted = new_binary_expression(13, record->type,
                new_fixed_register_operand(5, 0x1004), first_high,
                prepend_input_record, (C2Record *)record);
            if (extra) {
                attach_optimizer_output(inserted, extra);
                extra->value.reg->value = inserted;
            }
            record->tail.right->type = (record->tail.right->type & 0xf004) | 4;
            lower_final_record(record);
            returned = inserted;
        }
        break;

    case 0x135:
        if ((record->type & 0xfff) == 8) {
            if ((record->left->type & 0xfff) != 4) {
                created = new_assignment_expression(0x135, 0x1004, record->left,
                    prepend_input_record, (C2Record *)record);
                attach_input_expression(record, record->left, created->tail.right);
                lower_final_record(created);
            }
            if ((record->left->type & 0xf000) == 0x1000) record->opcode = 0xdf;
            else {
                record->type = (record->type & 0xf004) | 4;
                record->opcode = 1;
                output_high = split_final_wide_result(record, record->tail.right, NULL);
                inserted = insert_final_constant_assignment(output_high, record, 0, 0);
                output = record->tail.right;
                if (((1U << (output->kind & 31)) & 6) && output->value.reg->value)
                    inserted->tail.right->value.reg->value = inserted;
            }
        } else narrow_final_wide_conversion(record);
        break;

    case 0x136: case 0x137:
        record->type = (record->type & 0xf004) | 4;
        first_high = split_final_wide_result(record, record->left, NULL);
        output_high = split_final_wide_result(record, record->tail.right, NULL);
        inserted = new_binary_expression(lookup_wide_opcode(wide_opcodes, record->opcode),
            record->type, output_high, first_high, insert_input_record, (C2Record *)record);
        if (record->opcode == 0x137) {
            created = new_folded_binary_expression(0x2c, record->type, first_high,
                new_integer_expression(0x1004, 0), insert_input_record, (C2Record *)record);
            attach_input_expression(inserted, first_high, created->tail.right);
            copy_input_operand(created, new_final_stack_count((C2Record *)record));
        }
        break;

    case 0x13e: case 0x13f:
        inserted = lower_final_direct_call(record);
        break;

    case 0x140:
        record->type = (record->type & 0xf000) | 4;
        first_high = split_final_wide_result(record, record->left, NULL);
        created = new_binary_expression(1, record->type,
            new_numbered_machine_operand(3, record->type | 4), first_high,
            insert_input_record, (C2Record *)record);
        created->tail.right->value.reg->value = created;
        other = new_input_expression(0x16c, record->type);
        copy_input_operand(other, copy_input_expression(created->tail.right));
        insert_input_record((C2Record *)created, (C2Record *)other);
        break;

    case 0x141: case 0x142: case 0x146: case 0x147: case 0x148:
        other = record->left->next;
        record->type = (record->type & 0xf004) | 4;
        first_high = split_final_wide_result(record, record->left, NULL);
        second_high = split_final_wide_result(record, other, NULL);
        output_high = split_final_wide_result(record, record->tail.right, NULL);
        inserted = new_global_ternary_expression(lookup_wide_opcode(wide_opcodes, record->opcode),
            record->type, output_high, first_high, second_high,
            insert_input_record, (C2Record *)record);
        output = record->tail.right;
        if (output->kind == 1 && output->value.reg->kind == 3 && output->value.reg->value)
            inserted->tail.right->value.reg->value = inserted;
        /* Only bit4 of original WORD column494400 is consumed, and only
         * these five mapped high-word operations can reach this query. */
        if (inserted->opcode != 0x2c && inserted->opcode != 0x31 &&
            inserted->opcode != 0x2e && inserted->opcode != 0x30 &&
            inserted->opcode != 0x34) { __asm { int 3 } }
        if (inserted->opcode == 0x2c || inserted->opcode == 0x31)
            copy_input_operand(inserted, new_final_stack_count((C2Record *)record));
        if (record->opcode == 0x146) {
            returned = simplify_final_wide_and(record);
            inserted = simplify_final_wide_and(inserted);
        }
        break;

    case 0x149: case 0x14a: case 0x14b:
        other = record->left->next;
        if (other->kind == 7 && !(other->integer.low | other->integer.high))
            compiler_warning(3, record->opcode == 0x149 ? 0x2d3 : 0x2d4);
        /* fall through */
    case 0x143: case 0x145:
        returned = lower_final_wide_operation(record);
        break;

    case 0x151:
        record->type = (record->type & 0xf004) | 4;
        first_high = split_final_wide_result(record, record->left, NULL);
        second_high = split_final_wide_result(record, record->left->next, NULL);
        inserted = new_final_move_expression(0x2f, 0x1004, first_high, second_high, record);
        flags = inserted->tail.right;
        flags->fixed_register = &machine_registers[66];
        use = follow_optimizer_result(record, &use_operand);
        condition = use->condition_code;
        if ((record->type & 0xf000) == 0x2000) {
            if (condition > 16) { __asm { int 3 } }
            if (condition >= 3 && condition <= 6) condition += 4;
            else if (condition > 10) condition = 0;
            use->condition_code = (unsigned char)condition;
        }
        returned = inserted;
        switch (use->condition_code) {
        case 1: case 2:
            input = record->left;
            other = input->next;
            if (other->kind == 7) {
                word = inserted->left->next->integer.low;
                if (!(other->integer.low | word) || (other->integer.low & word) == ~0U) {
                    created = new_final_move_expression(other->integer.low ? 0x2e : 0x30,
                        0x1004, copy_input_expression(input),
                        copy_input_expression(inserted->left), inserted);
                    unlink_and_release_input_record((C2Record *)inserted);
                    attach_input_expression(record, input, created->tail.right);
                    returned = inserted = created;
                    break;
                }
            }
            if (use->condition_code == 2)
                branch = new_call_expression(0x159, 2,
                    ((C2InputSymbol *)use->left->value.symbol)->input_operand,
                    inserted->tail.right, inserted, insert_input_record, (C2Record *)inserted);
            else {
                label = symbol_input_expression(new_flow_label());
                branch = new_call_expression(0x159, 2, label, inserted->tail.right,
                    inserted, insert_input_record, (C2Record *)inserted);
                insert_input_record((C2Record *)use, (C2Record *)label);
            }
            input = inserted->left;
            first_copy = copy_input_expression(input);
            second_copy = copy_input_expression(input->next);
            comparison = branch->next;
            attach_input_expression(inserted, input, comparison->left);
            attach_input_expression(inserted, inserted->left->next, comparison->left->next);
            attach_input_expression(comparison, comparison->left, first_copy);
            attach_input_expression(comparison, comparison->left->next, second_copy);
            break;
        case 3: case 5:
            first_condition = 3; second_condition = 4;
            use->condition_code += 4;
            goto relational_comparison;
        case 4: case 6:
            first_condition = 4; second_condition = 3;
            use->condition_code += 4;
            goto relational_comparison;
        case 7: case 9:
            first_condition = 7; second_condition = 8;
            goto relational_comparison;
        case 8: case 10:
            first_condition = 8; second_condition = 7;
relational_comparison:
            other = inserted->left->next;
            if (!(first_condition == 7 && other->kind == 7 && other->integer.low == 0)) {
                new_call_expression(0x159, (unsigned char)first_condition,
                    ((C2InputSymbol *)use->left->value.symbol)->input_operand,
                    flags, inserted, insert_input_record, (C2Record *)inserted);
                flags = new_fixed_register_operand(66, 0x8000);
            }
            label = symbol_input_expression(new_flow_label());
            new_call_expression(0x159, (unsigned char)second_condition, label, flags,
                inserted, insert_input_record, (C2Record *)inserted);
            insert_input_record((C2Record *)use, (C2Record *)label);
            break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\lower.c", 0x7b4);
        }
        break;

    case 0x158:
        record->type = (record->type & 0xf000) | 4;
        break;

    case 0x15f:
        switch (record->detail.intrinsic) {
        case 0xe9: opcode = 0x2a; break;
        case 0xea: opcode = 0x2b; break;
        case 0xeb: opcode = 0x29; break;
        case 0xec: case 0xed:
            input = record->left;
            returned = new_global_ternary_expression(
                (record->type & 0xf000) == 0x1000 ? 6 : 0x22,
                record->type, record->tail.right,
                input->value.reg->value->left,
                input->next->value.reg->value->left,
                prepend_input_record, (C2Record *)record);
            unlink_and_release_input_record((C2Record *)input->value.reg->value);
            unlink_and_release_input_record((C2Record *)input->next->value.reg->value);
            record->opcode = 0x11c;
            output = record->tail.right;
            if (output->kind == 1 && output->value.reg->kind == 3 && output->value.reg->value)
                output->value.reg->value = returned;
            goto wide_result;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\lower.c", 0x7d9);
            goto wide_result;
        }
        lower_final_wide_intrinsic(opcode, record);
        record->opcode = 0x11c;
        break;

    case 0x16c:
        input = record->left;
        if (((1U << (input->kind & 31)) & 6) && input->value.reg->value) {
            definition = input->value.reg->value;
            if (definition->opcode == 0x13f) {
                output = definition->tail.right;
                set_input_expression_type(output, 0x1004);
                other = definition->left;
                set_input_expression_type(other, 0x1004);
                output->original_register = output->value.reg;
                created = new_binary_expression(1, 0x1004, output, other,
                    prepend_input_record, (C2Record *)definition);
                unlink_and_release_input_record((C2Record *)definition);
                record->type = 0x1004;
                set_input_expression_type(input, 0x1004);
                input->original_register = input->value.reg;
                bind_input_temporary(input->value.reg, created);
            } else {
                record->type = (record->type & 0xf004) | 4;
                created = new_input_expression(0x16c, record->type);
                copy_input_operand(created, split_final_wide_result(record, input, NULL));
                insert_input_record((C2Record *)record, (C2Record *)created);
                inserted = NULL;
            }
        }
        break;
    }
wide_result:
    if (inserted && inserted->tail.right->kind == 1 &&
        inserted->tail.right->value.reg->kind == 3 && inserted->tail.right->value.reg->value)
        inserted->tail.right->value.reg->value = inserted;
    return returned;
}

void __fastcall narrow_final_wide_conversion(C2Expression *record)
{ __asm { int 3 } }
unsigned __fastcall lookup_wide_opcode(const C2WideOpcodeMap *table, unsigned opcode)
{ __asm { int 3 } return 0; }
C2Expression *__fastcall simplify_final_wide_and(C2Expression *record)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall lower_final_wide_operation(C2Expression *record)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall lower_final_wide_intrinsic(unsigned opcode, C2Expression *record)
{ __asm { int 3 } return NULL; }

C2Expression *__fastcall materialize_final_comparison_operand(C2Expression *operand,
    C2Expression *record);
C2Expression *__fastcall make_final_result_temporary(C2Expression *record,
    C2FunctionRegister *reg, int bind);
/* Only established static kind/number columns may participate in this
 * consumer. Dynamic function-register records retain their existing layout. */
#define WIDE_KNOWN_ORIGINAL(r) do { \
    if ((unsigned long)(r) >= (unsigned long)machine_registers && \
        (unsigned long)(r) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (r) != (C2FunctionRegister *)&machine_registers[1] && \
        (r) != (C2FunctionRegister *)&machine_registers[2] && \
        (r) != (C2FunctionRegister *)&machine_registers[3] && \
        (r) != (C2FunctionRegister *)&machine_registers[4] && \
        (r) != (C2FunctionRegister *)&machine_registers[5] && \
        (r) != (C2FunctionRegister *)&machine_registers[6] && \
        (r) != (C2FunctionRegister *)&machine_registers[7] && \
        (r) != (C2FunctionRegister *)&machine_registers[8] && \
        (r) != (C2FunctionRegister *)&machine_registers[17] && \
        (r) != (C2FunctionRegister *)&machine_registers[20] && \
        (r) != (C2FunctionRegister *)&machine_registers[66] && \
        (r) != (C2FunctionRegister *)&machine_registers[67] && \
        (r) != (C2FunctionRegister *)&machine_registers[68] && \
        (r) != (C2FunctionRegister *)&machine_registers[69] && \
        (r) != (C2FunctionRegister *)&machine_registers[70] && \
        (r) != (C2FunctionRegister *)&machine_registers[71] && \
        (r) != (C2FunctionRegister *)&machine_registers[72] && \
        (r) != (C2FunctionRegister *)&machine_registers[73]) { __asm { int 3 } } \
} while (0)

C2Expression *__fastcall split_final_wide_result(C2Expression *record,
    C2Expression *operand, C2Expression **replacement)
{
    C2Expression *result, *address, *temporary, *high_address;
    C2FunctionRegister *reg, *view;
    unsigned short type;
    unsigned __int64 value;

    if (!operand) return NULL;
    type = (operand->type & 0xf004) | 4;
    switch (operand->kind) {
    case 1:
        reg = operand->value.reg;
        if (reg->kind == 3 && reg->value && record->opcode == 0x151) {
            operand = materialize_final_comparison_operand(operand, record);
            goto split_address;
        }
        view = new_address_register(reg, type, 0, 0);
        operand->value.reg = view;
        WIDE_KNOWN_ORIGINAL(operand->original_register);
        if (operand->original_register->kind != 1)
            materialize_global_register_operand(operand, view);
        operand->type = type;
        view = new_address_register(operand->value.reg, type, 0, 4);
        result = new_temporary_register_expression(operand->opcode, type, view, view);
        if (operand->kind == 1) {
            WIDE_KNOWN_ORIGINAL(operand->original_register);
            if (operand->original_register->kind == 1 && operand->original_register->number == 1)
                result->fixed_register = &machine_registers[3];
        }
        return result;
    case 2:
split_address:
        view = new_address_register(operand->value.reg, type, 0, 0);
        operand->value.reg = view;
        operand->type = type;
        view = new_address_register(view, type, 0, 4);
        result = new_typed_register_expression(operand->opcode, type, view);
        result->location.expression.comparison =
            (result->location.expression.comparison & ~8) |
            (operand->location.expression.comparison & 8);
        break;
    case 7:
        value = operand->integer.value;
        operand->type = type;
        result = new_integer_expression(type, (__int64)value >> 32);
        operand->integer.value = normalize_integer_value(type, value);
        return result;
    case 6:
        address = operand->aggregate_value;
        operand->type = type;
        if (!address || address->kind != 1 || address->value.reg->kind != 3 ||
            !address->value.reg->value) {
            result = copy_input_expression(operand);
            result->detail.aggregate_displacement += 4;
        } else if (record->opcode == 0x151) {
            high_address = materialize_final_comparison_operand(address, record);
            result = new_aggregate_expression(operand->opcode, type,
                operand->tail.aggregate_register, operand->detail.aggregate_displacement,
                high_address, NULL, operand->immediate);
            result->detail.aggregate_displacement += 4;
        } else {
            temporary = make_final_result_temporary(record, address->value.reg, replacement == NULL);
            high_address = temporary;
            if (replacement) {
                *replacement = temporary;
                high_address = copy_input_expression(address);
                attach_input_expression(record, address, temporary);
            }
            result = new_aggregate_expression(0x122, type,
                operand->tail.aggregate_register, operand->detail.aggregate_displacement + 4,
                high_address, NULL, operand->immediate);
        }
        break;
    default:
        /* The original returns its third argument's address verbatim for
         * other kinds; it neither reads through it nor writes the holder. */
        return (C2Expression *)replacement;
    }
    result->location.expression.properties =
        (result->location.expression.properties & ~0x40) |
        (operand->location.expression.properties & 0x40);
    return result;
}
C2Expression *__fastcall materialize_final_comparison_operand(C2Expression *operand,
    C2Expression *record)
{ __asm { int 3 } return NULL; }
