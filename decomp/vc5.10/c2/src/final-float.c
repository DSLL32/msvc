#include "global-optimizer.h"
#include "code-input.h"
#include "registers.h"
#include "inlining.h"
#include "optimizer-translate.h"
#include <recon.h>

extern C2FunctionRegister *final_float_register_stack[8];
extern int final_float_stack_top;
extern int c2_favor_speed, c2_float_precision;
int c2_real_compare(int condition, C2Real *left, C2Real *right);

/* The x87 allocation stack has eight established slots. This does not model
 * adjacent globals, including the word preceding slot zero. */
#define FLOAT_SLOT_CHECK(i) do { if ((unsigned)(i) >= 8) { __asm { int 3 } } } while (0)
#define LIVE_FLOAT(x) ((x)->kind == 1 && (x)->value.reg->kind == 3 && (x)->value.reg->value)

typedef struct C2FloatOpcodeMap {
    unsigned input, output;
} C2FloatOpcodeMap;
/* Keys, mapped x86 operations and the terminating key are consumed by the
 * genuine lookup routine. The terminator's output word is not modeled. */
static const C2FloatOpcodeMap float_opcodes[] = {
    {0x141, 0x45}, {0x142, 0x47}, {0x143, 0x46}, {0x149, 0x49},
    {0x137, 0x77}, {0x138, 0x138}, {~0U, 0}
};

int __fastcall try_fold_final_float_record(C2Expression *record);
void __fastcall load_final_float_operand(C2Expression *record);
unsigned __fastcall lookup_float_opcode(const C2FloatOpcodeMap *table, unsigned opcode);
void __fastcall lower_final_float_arithmetic(C2Expression *record);
C2Expression *__fastcall lower_final_float_intrinsic(C2Expression *record);
void __fastcall finish_final_float_result(C2Expression *record);
void __cdecl compiler_error(int code, ...);
void __fastcall bind_final_fixed_result(C2Expression *record, C2MachineRegister *reg);
void __fastcall bind_final_fixed_operand(C2Expression *record, C2Expression *operand,
    C2MachineRegister *reg);
void __fastcall spill_final_float_call_stack(C2Expression *record, unsigned count, int mode);
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);
C2Expression *__fastcall new_numbered_machine_operand(unsigned number, unsigned short type);
C2Expression *__fastcall widen_final_integer_record(C2Expression *record);
C2Expression *__fastcall follow_optimizer_result(C2Expression *record, C2Expression **operand);
void __fastcall finish_final_call(C2Expression *record);
C2Symbol *__fastcall find_runtime_function(char *name);
C2FunctionRegister *__fastcall new_spill_temporary_register(unsigned short type);
C2Expression *__fastcall lower_intrinsic_runtime_call(C2Expression *record,
    unsigned opcode, C2Expression *target, unsigned char mode);

C2Expression *__fastcall lower_final_float_record(C2Expression *record)
{
    C2Expression *returned = record, *input, *output, *other, *use, *use_operand;
    C2Expression *inserted, *aggregate, *temporary, *folded, *scan;
    C2FunctionRegister *spill, *view, *whole, *top;
    unsigned short type, source_type, output_type, wide_type;
    unsigned opcode = record->opcode, condition;
    int same_precision, signed_input, round_result;

    if (c2_favor_speed && try_fold_final_float_record(record)) return record;
    switch (opcode) {
    case 0x130:
        input = record->left;
        if (LIVE_FLOAT(input)) {
            input->fixed_register = &machine_registers[31];
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = NULL;
        } else load_final_float_operand(record);
        use = follow_optimizer_result(record, &use_operand);
        if (use->opcode == 0x15f) {
            input = record->left;
            record->opcode = 0x11c;
            input->fixed_register = &machine_registers[31];
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = input->value.reg;
            use_operand->value.reg = input->value.reg;
            return returned;
        }
        if (use->call_convention == 0x18 && use_operand == use->left->next) {
            attach_input_expression(use, use_operand, record->left);
            record->opcode = 0x11c;
            --final_float_stack_top;
            return returned;
        }
        inserted = new_folded_binary_expression(0x32, 0x1004,
            new_fixed_register_operand(5, 0x1004),
            new_integer_expression(0x1004, record->type & 0xfff),
            prepend_input_record, (C2Record *)record);
        aggregate = new_aggregate_expression(0x122, record->type, NULL, 0,
            inserted->tail.right, NULL, 1);
        new_binary_expression(0x63, record->type, aggregate, record->left,
            insert_input_record, (C2Record *)inserted);
        aggregate->aggregate_value->fixed_register = &machine_registers[5];
        inserted->tail.right->fixed_register = &machine_registers[5];
        --final_float_stack_top;
        return returned;

    case 0x131:
        if (!LIVE_FLOAT(record->tail.right)) {
            input = record->left;
            record->opcode = 0x63;
            if (LIVE_FLOAT(input)) {
                input->fixed_register = &machine_registers[31];
                FLOAT_SLOT_CHECK(final_float_stack_top);
                final_float_register_stack[final_float_stack_top] = NULL;
            } else load_final_float_operand(record);
            --final_float_stack_top;
            return returned;
        }
        record->opcode = 0x60;
        goto push_result;

    case 0x135:
        input = record->left;
        if (input->kind == 7) {
            if ((input->type & 0xfff) != 8) {
                record->left = NULL;
                input->location.expression.properties &= ~0x10;
                folded = fold_input_constant(input, record->tail.right->type, NULL);
                returned = follow_optimizer_result(record, &use_operand);
                attach_input_expression(returned, use_operand, folded);
                record->opcode = 0x11c;
                return returned;
            }
            spill = new_spill_temporary_register(input->type);
            new_binary_expression(1, 0x1004,
                new_typed_register_expression(0x120, 0x1004,
                    new_address_register(spill, 0x1004, 4, 0)),
                new_integer_expression(0x1004, input->integer.value),
                prepend_input_record, (C2Record *)record);
            input = record->left;
            new_binary_expression(1, 0x1004,
                new_typed_register_expression(0x120, 0x1004,
                    new_address_register(spill, 0x1004, 4, 4)),
                new_integer_expression(0x1004, (__int64)input->integer.value >> 32),
                prepend_input_record, (C2Record *)record);
            attach_input_expression(record, record->left,
                new_typed_register_expression(0x120, spill->type, spill));
        }
        input = record->left;
        type = record->type & 0xf000;
        source_type = input->type;
        if (type == 0x4000 && (source_type & 0xf000) == 0x4000) {
            output_type = record->tail.right->type;
            same_precision = 1;
            if ((output_type & 0xfff) < (source_type & 0xfff)) {
                other = record->next;
                if (other->opcode != 0x131 || LIVE_FLOAT(other->tail.right) ||
                    !((1U << (other->left->kind & 31)) & 6) ||
                    record->tail.right->value.reg != other->left->value.reg ||
                    other->type != output_type)
                    same_precision = 0;
            }
            if (!LIVE_FLOAT(input)) {
                if (same_precision) {
                    record->opcode = 0x60;
                    record->type = (record->type & 0xf000) | (source_type & 0xfff);
                    goto push_result;
                }
                load_final_float_operand(record);
                input = record->left;
                input->fixed_register = &machine_registers[31];
                FLOAT_SLOT_CHECK(final_float_stack_top);
                final_float_register_stack[final_float_stack_top] = input->value.reg;
            }
            FLOAT_SLOT_CHECK(final_float_stack_top);
            if (!LIVE_FLOAT(record->tail.right)) {
                final_float_register_stack[final_float_stack_top] = NULL;
                --final_float_stack_top;
                record->opcode = 0x63;
                input->fixed_register = &machine_registers[31];
                return returned;
            }
            if (same_precision) {
                follow_optimizer_result(record, &use_operand);
                input = record->left;
                use_operand->value.reg = new_address_register(input->value.reg,
                    use_operand->type, use_operand->type & 0xfff, 0);
                record->opcode = 0x11c;
                use_operand->fixed_register = input->fixed_register;
                return returned;
            }
            input->fixed_register = &machine_registers[31];
            type = record->tail.right->type;
            final_float_register_stack[final_float_stack_top] = NULL;
            temporary = new_input_temporary(type);
            new_binary_expression(0x63, type, temporary, input,
                prepend_input_record, (C2Record *)record);
            record->opcode = 0x60;
            attach_input_expression(record, record->left, temporary);
            goto replace_top;
        }
        signed_input = (record->left->type & 0xf000) == 0x1000;
        if (type == 0x4000) {
            if (!signed_input && (record->left->type & 0xfff) > 4) {
                compiler_error(0x208);
                signed_input = 1;
            }
            if (c2_float_precision) {
                round_result = 1;
                output = record->tail.right;
                if (((1U << (output->kind & 31)) & 6) && output->value.reg->value &&
                    follow_optimizer_result(record, &use_operand)->opcode == 0x131)
                    round_result = 0;
            }
            input = record->left;
            if (!LIVE_FLOAT(input)) {
                if (signed_input || (input->type & 0xfff) > 4) {
                    record->opcode = 0x56;
                    record->type = (record->type & 0xf000) | (input->type & 0xfff);
                    if (final_float_stack_top == 7) spill_final_float_call_stack(record, 1, 0);
                    output = record->tail.right;
                    if (output->kind == 1) {
                        ++final_float_stack_top;
                        FLOAT_SLOT_CHECK(final_float_stack_top);
                        output->fixed_register = &machine_registers[31];
                        final_float_register_stack[final_float_stack_top] = output->value.reg;
                    } else bind_final_fixed_result(record, &machine_registers[31]);
                    if (c2_float_precision && round_result && final_float_stack_top >= 0)
                        spill_final_float_call_stack(record, 1, 0);
                    return returned;
                }
                bind_final_fixed_operand(record, input, &machine_registers[0]);
            }
            if (final_float_stack_top == 7) spill_final_float_call_stack(record, 1, 0);
            output = record->tail.right;
            if (output->kind == 1) {
                ++final_float_stack_top;
                FLOAT_SLOT_CHECK(final_float_stack_top);
                output->fixed_register = &machine_registers[31];
                final_float_register_stack[final_float_stack_top] = output->value.reg;
            } else bind_final_fixed_result(record, &machine_registers[31]);
            if (!signed_input) {
                type = (record->left->type & 0xf004) | 4;
                wide_type = (record->left->type & 0xf008) | 8;
                whole = new_spill_temporary_register(wide_type);
                view = new_address_register(whole, type, 0, 0);
            } else {
                type = wide_type = record->left->type;
                view = whole = new_spill_temporary_register(type);
            }
            returned = new_binary_expression(1, type,
                new_typed_register_expression(0x120, type, view), record->left,
                prepend_input_record, (C2Record *)record);
            if ((type & 0xfff) == 8) widen_final_integer_record(returned);
            if (!signed_input)
                new_binary_expression(1, 0x2004,
                    new_typed_register_expression(0x120, 0x2004,
                        new_address_register(view, 0x2004, 0, 4)),
                    new_integer_expression(0x2004, 0),
                    prepend_input_record, (C2Record *)record);
            attach_input_expression(record, record->left,
                new_typed_register_expression(0x120, wide_type, whole));
            record->opcode = 0x56;
            if (c2_float_precision && round_result && final_float_stack_top >= 0)
                spill_final_float_call_stack(record, 1, 0);
            return returned;
        }
        bind_final_fixed_result(record, &machine_registers[1]);
        input = record->left;
        if (LIVE_FLOAT(input)) {
            input->fixed_register = &machine_registers[31];
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = NULL;
        } else load_final_float_operand(record);
        --final_float_stack_top;
        temporary = variable_input_expression((C2InputSymbol *)find_runtime_function("__ftol"));
        temporary->value.symbol->flags |= 0x10000;
        return lower_intrinsic_runtime_call(record, 14, temporary, 0);

    case 0x137: case 0x138:
        record->opcode = lookup_float_opcode(float_opcodes, opcode);
        input = record->left;
        if (LIVE_FLOAT(input)) {
            input->fixed_register = &machine_registers[31];
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = NULL;
        } else load_final_float_operand(record);
        goto replace_top;

    case 0x140:
        input = record->left;
        if (LIVE_FLOAT(input)) {
            use = find_final_overlapping_result(input->value.reg->value, input->value.reg);
            use->value.reg = (C2FunctionRegister *)&machine_registers[31];
            record->opcode = 0x11c;
            --final_float_stack_top;
            final_float_register_stack[0] = NULL;
            return returned;
        }
        new_binary_expression(0x60, record->type,
            new_fixed_register_operand(31, 0x4008), input,
            insert_input_record, (C2Record *)record);
        record->opcode = 0x11c;
        return returned;

    case 0x149:
        input = record->left->next;
        if (input->kind == 8 && c2_real_compare(1, input->value.real, &c2_real_zero))
            compiler_warning(3, 0x2d3);
        /* fall through */
    case 0x141: case 0x142: case 0x143:
        record->opcode = lookup_float_opcode(float_opcodes, opcode);
        lower_final_float_arithmetic(record);
        return returned;

    case 0x14c:
        input = record->left;
        if (LIVE_FLOAT(input)) {
            input->fixed_register = &machine_registers[31];
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = NULL;
        } else load_final_float_operand(record);
        output = record->tail.right;
        if (output->kind == 1) {
            output->fixed_register = &machine_registers[31];
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = output->value.reg;
        } else {
            bind_final_fixed_result(record, &machine_registers[31]);
            --final_float_stack_top;
        }
        use = follow_optimizer_result(record, NULL);
        if (use->opcode == 0x135 && (use->tail.right->type & 0xf000) == 0x4000 &&
            ((1U << (use->tail.right->kind & 31)) & 6) && use->tail.right->value.reg->value)
            use = follow_optimizer_result(use, NULL);
        if (use->opcode == 0x131) {
            for (scan = record->next; scan != use && scan->kind != 13 && scan->opcode != 0x14c;
                 scan = scan->next) { }
            if (scan->opcode == 0x131) {
                temporary = new_temporary_operand(0x1004);
                temporary->fixed_register = &machine_registers[1];
                temporary->value.reg->value = record;
                attach_optimizer_output(record, temporary);
                new_global_ternary_expression(0x14c, 0x1004,
                    new_temporary_register_expression(0x11f, 0x4008,
                        (C2FunctionRegister *)&machine_registers[31], (C2FunctionRegister *)&machine_registers[31]),
                    temporary,
                    new_temporary_register_expression(0x11f, 0x8000,
                        (C2FunctionRegister *)&machine_registers[66], (C2FunctionRegister *)&machine_registers[66]),
                    insert_input_record, (C2Record *)scan);
                goto attach_flags;
            }
        }
        attach_optimizer_output(record, new_temporary_register_expression(0x11f, 0x1004,
            (C2FunctionRegister *)&machine_registers[1], (C2FunctionRegister *)&machine_registers[1]));
attach_flags:
        attach_optimizer_output(record, new_temporary_register_expression(0x11f, 0x8000,
            (C2FunctionRegister *)&machine_registers[66], (C2FunctionRegister *)&machine_registers[66]));
        return returned;

    case 0x151:
        use = follow_optimizer_result(record, &use_operand);
        condition = use->condition_code;
        /* Original BYTE column48d2d0: unsigned relational conditions. Only
         * the established0..16 domain is modeled; other rows are unknown. */
        if (condition > 16) { __asm { int 3 } }
        if (condition >= 3 && condition <= 6) condition += 4;
        else if (condition > 10) condition = 0;
        use->condition_code = (unsigned char)condition;
        input = record->left;
        if (LIVE_FLOAT(input)) {
            other = input->next;
            if (LIVE_FLOAT(other)) {
                record->opcode = 0x5e;
                FLOAT_SLOT_CHECK(final_float_stack_top);
                FLOAT_SLOT_CHECK(final_float_stack_top - 1);
                top = final_float_register_stack[final_float_stack_top];
                final_float_register_stack[final_float_stack_top] = NULL;
                final_float_register_stack[final_float_stack_top - 1] = NULL;
                if (top->canonical == other->value.reg->canonical) {
                    input->fixed_register = &machine_registers[32];
                    final_float_stack_top -= 2;
                    other->fixed_register = &machine_registers[31];
                    swap_global_operands(record);
                    condition = use->condition_code;
                    if (condition > 17) { __asm { int 3 } }
                    if (condition >= 3 && condition <= 10) condition = ((condition - 3) ^ 1) + 3;
                    else if (condition > 10) condition = 0;
                    use->condition_code = (unsigned char)condition;
                } else {
                    input->fixed_register = &machine_registers[31];
                    final_float_stack_top -= 2;
                    other->fixed_register = &machine_registers[32];
                }
            } else {
                record->opcode = 0x5d;
                input->fixed_register = &machine_registers[31];
                FLOAT_SLOT_CHECK(final_float_stack_top);
                final_float_register_stack[final_float_stack_top] = NULL;
                --final_float_stack_top;
            }
        } else {
            load_final_float_operand(record);
            other = record->left->next;
            if (LIVE_FLOAT(other)) {
                record->opcode = 0x5e;
                other->fixed_register = &machine_registers[32];
                FLOAT_SLOT_CHECK(final_float_stack_top - 1);
                final_float_register_stack[final_float_stack_top - 1] = NULL;
                final_float_stack_top -= 2;
            } else {
                record->opcode = 0x5d;
                --final_float_stack_top;
            }
        }
        temporary = new_numbered_machine_operand(0x4a, 0x8000);
        temporary->value.reg->value = record;
        inserted = new_assignment_expression(0x6e, 0x2002, temporary,
            insert_input_record, (C2Record *)record);
        output = record->tail.right;
        use = new_binary_expression(0xae, 0x2001, output, inserted->tail.right,
            insert_input_record, (C2Record *)inserted);
        replace_input_result(record, output, temporary);
        bind_final_fixed_result(inserted, &machine_registers[9]);
        use->tail.right->value.reg->value = use;
        return returned;

    case 0x158:
        record->opcode = 14;
        finish_final_call(record);
        if (record->left->opcode == 0x11d)
            bind_final_fixed_operand(record, record->left, &machine_registers[0]);
        output = record->tail.right;
        if (!output || !((1U << (output->kind & 31)) & 6) ||
            output->value.reg->kind != 3 || (output->type & 0xf000) != 0x4000) {
            finish_final_float_result(record);
            return returned;
        }
        goto push_result;

    case 0x15f:
        return lower_final_float_intrinsic(record);

    case 0x16c:
        input = record->left;
        if (input && LIVE_FLOAT(input)) {
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = NULL;
            --final_float_stack_top;
            input->fixed_register = &machine_registers[31];
            new_binary_expression(0x63, record->type,
                new_temporary_register_expression(0x11f, record->type,
                    (C2FunctionRegister *)&machine_registers[31], (C2FunctionRegister *)&machine_registers[31]),
                input, prepend_input_record, (C2Record *)record);
            record->opcode = 0x11c;
            return returned;
        }
        /* fall through */
    default:
        if (!opcode || opcode > 0x11a)
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\lowerflt.c", 0x344);
        return returned;
    }

push_result:
    if (final_float_stack_top == 7) spill_final_float_call_stack(record, 1, 0);
    output = record->tail.right;
    if (output->kind != 1) bind_final_fixed_result(record, &machine_registers[31]);
    else {
        ++final_float_stack_top;
        FLOAT_SLOT_CHECK(final_float_stack_top);
        output->fixed_register = &machine_registers[31];
        final_float_register_stack[final_float_stack_top] = output->value.reg;
    }
    return returned;

replace_top:
    output = record->tail.right;
    if (output->kind == 1) {
        output->fixed_register = &machine_registers[31];
        FLOAT_SLOT_CHECK(final_float_stack_top);
        final_float_register_stack[final_float_stack_top] = output->value.reg;
    } else {
        bind_final_fixed_result(record, &machine_registers[31]);
        --final_float_stack_top;
    }
    return returned;
}

/* Direct memory x87 forms avoid allocating an x87 stack-register result. */
void __fastcall normalize_final_move(C2Expression *record);
extern int c2_global_optimize;
int __fastcall try_fold_final_float_record(C2Expression *record)
{
    C2Expression *use, *operand, *input, *other, *scan;
    int first_constant, second_constant;

    if (((1U << (record->kind & 31)) & 0x1000) ||
        (record->type & 0xf000) != 0x4000) return 0;
    if (record->opcode == 0x130) {
        if (LIVE_FLOAT(record->left)) return 0;
        use = follow_optimizer_result(record, &operand);
        if (use->opcode != 0x158) return 0;
        if (use->call_convention == 0x18 && operand == use->left->next) return 0;
    } else if (record->opcode == 0x131) {
        if (LIVE_FLOAT(record->tail.right) || LIVE_FLOAT(record->left)) return 0;
        if (c2_global_optimize && !c2_float_precision &&
            (record->tail.right->kind == 2 || record->left->kind == 2)) return 0;
    } else if (record->opcode == 0x151) {
        use = follow_optimizer_result(record, &operand);
        if (use->condition_code != 1 && use->condition_code != 2) return 0;
        input = record->left;
        for (scan = input; scan; scan = scan->next)
            if (LIVE_FLOAT(scan)) return 0;
        other = input->next;
        first_constant = ((1U << (input->kind & 31)) & 0x180) != 0;
        second_constant = ((1U << (other->kind & 31)) & 0x180) != 0;
        if (first_constant && second_constant) return 0;
        if (!first_constant && !second_constant) return 0;
        if (first_constant) {
            if (c2_real_compare(1, input->value.real, &c2_real_zero)) return 0;
            if (c2_global_optimize && !c2_float_precision && other->kind == 2) return 0;
        }
        if (second_constant) {
            if (c2_real_compare(1, other->value.real, &c2_real_zero)) return 0;
            if (c2_global_optimize && !c2_float_precision && input->kind == 2) return 0;
        }
    } else return 0;
    normalize_final_move(record);
    return 1;
}
C2Expression *__fastcall materialize_final_address_input(unsigned opcode,
    C2Expression *record, C2Expression *operand, C2MachineRegister *reg);
void __fastcall load_final_float_operand(C2Expression *record)
{
    C2Expression *input = record->left, *result;
    unsigned opcode = 0x60;

    if (!c2_favor_speed && input->kind == 8) {
        if (c2_real_compare(1, input->value.real, &c2_real_zero)) opcode = 0x7f;
        else if (c2_real_compare(1, input->value.real, &c2_real_one)) opcode = 0x94;
    }
    result = materialize_final_address_input(opcode, record, input, &machine_registers[31]);
    if (final_float_stack_top == 7)
        spill_final_float_call_stack(record->previous, 1, 0);
    if (record->previous->tail.right->kind == 1) {
        ++final_float_stack_top;
        FLOAT_SLOT_CHECK(final_float_stack_top);
        result->fixed_register = &machine_registers[31];
        final_float_register_stack[final_float_stack_top] = result->value.reg;
    } else bind_final_fixed_result(record->previous, &machine_registers[31]);
    attach_input_expression(record, record->left, result);
    FLOAT_SLOT_CHECK(final_float_stack_top);
    final_float_register_stack[final_float_stack_top] = NULL;
}
unsigned __fastcall lookup_float_opcode(const C2FloatOpcodeMap *table, unsigned opcode)
{
    for (; table->input != ~0U; ++table)
        if (table->input == opcode) return table->output;
    c2_report_internal("E:\\utc\\src\\\\P2\\x86\\lowerflt.c", 0x49);
    return 0; /* The internal-error path does not return. */
}
void __fastcall select_float_arithmetic_form(C2Expression *record, int reverse, int pop);
void __fastcall lower_final_float_arithmetic(C2Expression *record)
{
    C2Expression *input = record->left, *other, *output;
    C2FunctionRegister *top;

    if (LIVE_FLOAT(input)) {
        other = input->next;
        if (LIVE_FLOAT(other)) {
            FLOAT_SLOT_CHECK(final_float_stack_top);
            FLOAT_SLOT_CHECK(final_float_stack_top - 1);
            top = final_float_register_stack[final_float_stack_top];
            final_float_register_stack[final_float_stack_top] = NULL;
            final_float_register_stack[final_float_stack_top - 1] = NULL;
            if (top->canonical == other->value.reg->canonical) {
                other->fixed_register = &machine_registers[31];
                input->fixed_register = &machine_registers[32];
                --final_float_stack_top;
                select_float_arithmetic_form(record, 1, 1);
            } else {
                input->fixed_register = &machine_registers[31];
                other->fixed_register = &machine_registers[32];
                --final_float_stack_top;
                select_float_arithmetic_form(record, 0, 1);
                swap_global_operands(record);
            }
        } else {
            input->fixed_register = &machine_registers[31];
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = NULL;
        }
    } else {
        other = input->next;
        if (LIVE_FLOAT(other)) {
            other->fixed_register = &machine_registers[31];
            FLOAT_SLOT_CHECK(final_float_stack_top);
            final_float_register_stack[final_float_stack_top] = NULL;
            select_float_arithmetic_form(record, 1, 0);
            swap_global_operands(record);
        } else load_final_float_operand(record);
    }
    output = record->tail.right;
    if (output->kind == 1) {
        output->fixed_register = &machine_registers[31];
        FLOAT_SLOT_CHECK(final_float_stack_top);
        final_float_register_stack[final_float_stack_top] = output->value.reg;
    } else {
        bind_final_fixed_result(record, &machine_registers[31]);
        --final_float_stack_top;
    }
}
void __fastcall select_float_arithmetic_form(C2Expression *record, int reverse, int pop)
{
    switch (record->opcode) {
    case 0x45:
        if (pop) record->opcode = 0x4b;
        break;
    case 0x46:
        if (pop) record->opcode = 0x4c;
        break;
    case 0x47:
        if (pop) record->opcode = reverse ? 0x4d : 0x4e;
        else if (reverse) record->opcode = 0x48;
        break;
    case 0x49:
        if (pop) record->opcode = reverse ? 0x4f : 0x50;
        else if (reverse) record->opcode = 0x4a;
        break;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\x86\\lowerflt.c", 0x127);
    }
}
C2Expression *__fastcall lower_final_float_intrinsic(C2Expression *record)
{ __asm { int 3 } return NULL; }
void __fastcall finish_final_float_result(C2Expression *record)
{ __asm { int 3 } }
void __cdecl compiler_error(int code, ...)
{ __asm { int 3 } }

C2Real *__fastcall find_symbol_real(C2Symbol *symbol);
C2Real *__fastcall find_final_real_value(C2Expression *operand)
{
    C2FunctionRegister *reg;
    if (operand->kind == 8) return operand->value.real;
    if (((1U << (operand->kind & 31)) & 6)) {
        reg = operand->value.reg;
        if (reg->kind == 7 || reg->kind == 8) return find_symbol_real(reg->symbol);
    }
    return NULL;
}
C2Expression *__fastcall new_final_real_bits(C2Real *value, unsigned short type)
{ __asm { int 3 } return NULL; }

/* The real-constant cache at49a038 starts with a proven NULL head. Its
 * allocated16-byte links retain a real value, byte width and created symbol. */
typedef struct C2RealSymbolCache {
    struct C2RealSymbolCache *next;
    C2Real *value;
    unsigned bytes;
    C2InputSymbol *symbol;
} C2RealSymbolCache;
C2RealSymbolCache *real_symbol_cache;

/* Initializer kind1 has this leaf view of the existing28-byte allocation.
 * The first8 bytes remain initializer linkage, not floating payload. */
#pragma pack(push, 2)
typedef struct C2RealInitializer {
    unsigned pending00[2];
    unsigned short kind, bytes;
    union { float single; double real; C2Real extended; } value;
    unsigned count;
} C2RealInitializer;
#pragma pack(pop)
typedef char C2RealInitializerSize[sizeof(C2RealInitializer) == 28 ? 1 : -1];
typedef char C2RealInitializerPayload[offsetof(C2RealInitializer, value) == 12 ? 1 : -1];
typedef char C2RealInitializerCount[offsetof(C2RealInitializer, count) == 24 ? 1 : -1];
extern int output_enabled;
C2InputSymbol *__fastcall find_real_constant_symbol(C2Real *value, unsigned bytes);
C2InputSymbol *__fastcall materialize_real_symbol(C2Expression *operand)
{
    C2Real *value = operand->value.real;
    unsigned bytes = operand->type & 0xfff;
    C2InputSymbol *symbol;
    C2RealSymbolCache *cache;
    C2InitRecord *root;
    C2RealInitializer *leaf;
    int status;

    symbol = find_real_constant_symbol(value, bytes);
    if (symbol) return symbol;
    cache = (C2RealSymbolCache *)c2_pool_allocate(c2_internal_listing != 0, 16);
    cache->next = real_symbol_cache;
    real_symbol_cache = cache;
    cache->value = value;
    cache->bytes = bytes;
    symbol = new_inline_result_symbol(5, bytes, 4, c2_internal_listing != 0, 4, 0x80);
    cache->symbol = symbol;
    root = new_initializer(0);
    root->symbol_serial = symbol->symbol.serial;
    root->offset = 0;
    leaf = (C2RealInitializer *)new_initializer(1);
    leaf->count = 1;
    root->next = (C2InitRecord *)leaf;
    leaf->kind = 5;
    switch (bytes) {
    case 4:
        leaf->bytes = 4;
        status = input_real_to_float(value, &leaf->value.single);
        if (status) compiler_warning(2, 0x2f4);
        break;
    case 8:
        leaf->bytes = 8;
        status = input_real_to_double(value, &leaf->value.real);
        if (status) compiler_warning(2, 0x2f4);
        break;
    case 10:
        leaf->bytes = 10;
        leaf->value.extended = *value;
        break;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\misc.c", 0x37b);
        break;
    }
    queue_initializer(root);
    return symbol;
}
C2InputSymbol *__fastcall find_real_constant_symbol(C2Real *value, unsigned bytes)
{
    C2RealSymbolCache *cache;

    for (cache = real_symbol_cache; cache; cache = cache->next)
        if (cache->bytes == bytes && c2_real_compare(1, value, cache->value))
            return cache->symbol;
    return NULL;
}

C2Real *__fastcall find_symbol_real(C2Symbol *symbol)
{
    C2RealSymbolCache *cache;

    for (cache = real_symbol_cache; cache; cache = cache->next)
        /* C2InputSymbol begins with its actual C2Symbol prefix. */
        if ((C2Symbol *)cache->symbol == symbol) return cache->value;
    return NULL;
}
