#include "global-optimizer.h"
#include "code-input.h"
#include "inlining.h"
#include "registers.h"
#include "register-sets.h"
#include "function-reader.h"
#include "symbol-storage.h"
#include "optimizer-translate.h"
#include <recon.h>

/* The real initialization below owns all eight x87 stack-register slots.
 * Neighboring storage and pre-initialization contents are not modeled here. */
C2FunctionRegister *final_float_register_stack[8]; /* 496800 */
int final_float_stack_top; /* 496900, becomes -1 only at initialization */
unsigned final_lowering_state; /* 494388, scalar reset/consumer state */

void initialize_final_lowering(void);
void __fastcall prepare_final_exception_lowering(C2Function *function);
C2Expression *__fastcall lower_final_record(C2Expression *record);
void __fastcall fold_final_assignment(C2Expression *record);
void __fastcall select_final_machine_form(C2Expression *record);
void __fastcall fold_final_address_operand(C2Expression *record, C2Expression *operand);
C2InputSymbol *__fastcall materialize_real_symbol(C2Expression *operand);

void __fastcall lower_final_function_records(C2Function *function)
{
    C2Expression *first, *record, *next, *operand, *replacement;
    C2InputSymbol *symbol;
    C2FunctionRegister *reg;

    initialize_final_lowering();
    if (function->symbol->additional_flags & 0x300)
        prepare_final_exception_lowering(function);
    first = (C2Expression *)function->flow->last->records;
    for (record = first; record; record = next) {
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        next = record->next;
        if ((record->flags & 1) && ((record->type & 0xf000) == 0x4000 ||
            (record->opcode != 0x130 && record->opcode != 0x13e)))
            next = lower_final_record(record);
    }
    for (record = first; record; record = next) {
        next = record->next;
        if (!(record->flags & 1)) continue;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        fold_final_assignment(record);
        select_final_machine_form(record);
        if (record->opcode == 0x11c) {
            unlink_and_release_input_record((C2Record *)record);
            continue;
        }
        operand = record->left;
        if (operand) {
            if (operand->opcode == 0x11e) {
                symbol = materialize_real_symbol(operand);
                reg = input_symbol_register(symbol);
                replacement = new_typed_register_expression(0x121, operand->type, reg);
                attach_input_expression(record, operand, replacement);
                record->left->value.reg->view_flags |= 4;
            } else if (operand->opcode == 0x122)
                fold_final_address_operand(record, operand);
            operand = record->left->next;
            if (operand) {
                if (operand->opcode == 0x11e) {
                    symbol = materialize_real_symbol(operand);
                    reg = input_symbol_register(symbol);
                    replacement = new_typed_register_expression(0x121, operand->type, reg);
                    attach_input_expression(record, operand, replacement);
                    record->left->next->value.reg->view_flags |= 4;
                } else if (operand->opcode == 0x122)
                    fold_final_address_operand(record, operand);
            }
        }
        operand = record->tail.right;
        if (operand && operand->opcode == 0x122)
            fold_final_address_operand(record, operand);
    }
}

void initialize_final_lowering(void)
{
    memset(final_float_register_stack, 0, sizeof(final_float_register_stack));
    final_float_stack_top = -1;
    final_lowering_state = 0;
}
void __fastcall prepare_final_exception_lowering(C2Function *function)
{
    __asm { int 3 }
}
void __fastcall prepare_final_record_operands(C2Expression *record);
C2Expression *__fastcall lower_final_integer_record(C2Expression *record);
C2Expression *__fastcall finish_final_integer_record(C2Expression *record);
C2Expression *__fastcall lower_final_float_record(C2Expression *record);
void __fastcall lower_final_address_record(C2Expression *record);

C2Expression *__fastcall lower_final_record(C2Expression *record)
{
    C2Expression *next, *result;
    unsigned opcode;

    next = record->next;
    if (!(record->flags & 1)) return next;
    opcode = record->opcode;
    /* Only bit zero of the original opcode-property column is consumed here.
     * Its true rows are these two ranges; no other property bits are modeled. */
    if ((record->type & 0xf000) == 0x4000 ||
        (opcode > 0 && opcode < 0x11b &&
         ((opcode >= 0x45 && opcode <= 0x94) ||
          (opcode >= 0xd1 && opcode <= 0xd7))) ||
        (record->left && (record->left->type & 0xf000) == 0x4000)) {
        prepare_final_record_operands(record);
        result = lower_final_float_record(record);
        if (((1U << (result->type >> 12)) & 0xe) &&
            (result->opcode == 0x131 || result->opcode == 0x151 ||
             result->opcode == 0x130))
            next = result;
        else next = result->next;
    } else {
        if (opcode == 0x14c) {
            lower_final_address_record(record);
            return next;
        }
        result = lower_final_integer_record(record);
        prepare_final_record_operands(result);
        result = finish_final_integer_record(result);
        next = result->next;
    }
    if (result->opcode == 0x11c)
        unlink_and_release_input_record((C2Record *)result);
    return next;
}
void __fastcall prepare_final_operand(C2Expression *record, C2Expression *operand,
    C2ExpressionReplacer replace);

void __fastcall prepare_final_record_operands(C2Expression *record)
{
    if (record->opcode == 0x11c) return;
    prepare_final_operand(record, record->tail.right, replace_input_result);
    prepare_final_operand(record, record->left, attach_input_expression);
    if (record->left)
        prepare_final_operand(record, record->left->next, attach_input_expression);
}
C2Expression *__fastcall materialize_final_address_input(unsigned mode,
    C2Expression *owner, C2Expression *operand, C2MachineRegister *reg);
C2Expression *__fastcall new_final_address_expression(unsigned opcode,
    unsigned short type, C2FunctionRegister *reg, long displacement,
    C2Expression *base, C2Expression *index);

void __fastcall prepare_final_operand(C2Expression *record, C2Expression *operand,
    C2ExpressionReplacer replace)
{
    C2Expression *base, *index, *replacement;
    int changed = 0;

    if (!operand || !((1U << (operand->kind & 31)) & 0x60)) return;
    index = operand->aggregate_second;
    base = operand->aggregate_value;
    if (index && index->kind != 1) {
        changed = 1;
        index = materialize_final_address_input(1, record, index, &machine_registers[0]);
    }
    if (base && base->kind != 1) {
        changed = 1;
        base = materialize_final_address_input(1, record, base, &machine_registers[0]);
    }
    if (!changed) return;
    if (operand->kind == 6)
        replacement = new_aggregate_expression(operand->opcode, operand->type,
            operand->tail.aggregate_register, operand->detail.aggregate_displacement,
            base, index, operand->immediate);
    else
        replacement = new_final_address_expression(operand->opcode, operand->type,
            operand->tail.aggregate_register, operand->detail.aggregate_displacement,
            base, index);
    replacement->location.expression.properties =
        (replacement->location.expression.properties & ~0x4f) |
        (operand->location.expression.properties & 0x4f);
    replacement->location.expression.use_flags =
        (replacement->location.expression.use_flags & ~7) |
        (operand->location.expression.use_flags & 7);
    replace(record, operand, replacement);
}
unsigned __fastcall select_final_copy_opcode(C2Expression *operand);
C2Expression *__fastcall new_final_move_expression(unsigned opcode, unsigned short type,
    C2Expression *input, C2Expression *result, C2Expression *owner);
C2Expression *__fastcall widen_final_integer_record(C2Expression *record);
void __fastcall remove_input_operand(C2Expression *owner, C2Expression *operand);

C2Expression *__fastcall materialize_final_address_input(unsigned mode,
    C2Expression *owner, C2Expression *operand, C2MachineRegister *reg)
{
    C2Expression *inserted, *result;
    unsigned short type = operand->type;

    if (mode == 1) {
        mode = select_final_copy_opcode(operand);
        if (mode == 0x12 && owner->opcode == 0x12) owner->opcode = 1;
    } else if (mode == 0xc4) type = 0x1004;
    if ((((1U << (operand->kind & 31)) & 0x18) || operand->kind == 5) &&
        (operand->type & 0xfff) < 4) {
        operand = copy_input_expression(operand);
        operand->type = type = 0x1004;
    }
    inserted = new_final_move_expression(mode, type, operand, NULL, owner);
    if (mode == 0x7f || mode == 0x94) remove_input_operand(inserted, inserted->left);
    result = inserted->tail.right;
    /* Only rows zero/one/two have encountered number initializers 0/1/2.
     * A different row/variant is valid original data, but still unmodeled. */
    if (reg != &machine_registers[0] && reg != &machine_registers[1] &&
        reg != &machine_registers[2]) { __asm { int 3 } }
    if (reg->number) result->fixed_register = reg;
    if ((inserted->type & 0xfff) > 4 && mode == 1) {
        result = copy_input_expression(result);
        widen_final_integer_record(inserted);
    }
    return result;
}
unsigned __fastcall select_final_copy_opcode(C2Expression *operand)
{
    if (operand->kind == 5) return 0x12;
    if (operand->kind == 3 && operand->value.reg->kind >= 3 &&
        operand->value.reg->kind <= 6) return 0x12;
    return 1;
}
C2Expression *__fastcall prepare_final_aggregate_result(C2Expression *owner,
    C2Expression *operand, C2FunctionRegister **original);

C2Expression *__fastcall new_final_move_expression(unsigned opcode, unsigned short type,
    C2Expression *input, C2Expression *result, C2Expression *owner)
{
    C2Expression *inserted, *input_result = NULL, *output_result = NULL;
    C2FunctionRegister *input_original, *output_original;

    if (input->kind == 6)
        input_result = prepare_final_aggregate_result(owner, input, &input_original);
    if (result && result->kind == 6)
        output_result = prepare_final_aggregate_result(owner, result, &output_original);
    if (opcode == 0x2f) {
        inserted = new_comparison_expression(0x151, type, input, result,
            prepend_input_record, (C2Record *)owner);
        inserted->opcode = 0x2f;
        inserted->tail.right->fixed_register = &machine_registers[66];
    } else if (!result)
        inserted = new_assignment_expression(opcode, type, input,
            prepend_input_record, (C2Record *)owner);
    else
        inserted = new_folded_binary_expression(opcode, type, input, result,
            prepend_input_record, (C2Record *)owner);
    if (input_result) {
        attach_optimizer_output(inserted, input_result);
        input_result->value.reg->value = inserted;
    }
    if (output_result) {
        attach_optimizer_output(inserted, output_result);
        output_result->value.reg->value = inserted;
    }
    return inserted;
}
C2Expression *__fastcall prepare_final_aggregate_result(C2Expression *owner,
    C2Expression *operand, C2FunctionRegister **original)
{
    __asm { int 3 }
    return NULL;
}
/* These four DWORDs are initialized by the genuine constraint initializer.
 * Register numbers name existing static rows, without granting their payload. */
typedef struct C2FinalRegisterConstraints {
    unsigned flags, result_register, first_register, second_register;
} C2FinalRegisterConstraints;
typedef char C2FinalConstraintsSizeCheck[sizeof(C2FinalRegisterConstraints) == 16 ? 1 : -1];
void __fastcall initialize_final_constraints(C2FinalRegisterConstraints *constraints);
void __fastcall find_final_constraints(C2Expression *record, C2FinalRegisterConstraints *constraints);
C2Expression *__fastcall finish_final_record(C2Expression *record);
void __fastcall bind_final_fixed_result(C2Expression *record, C2MachineRegister *reg);
C2Expression *__fastcall bind_final_fixed_operand(C2Expression *owner,
    C2Expression *operand, C2MachineRegister *reg);
void __fastcall prepare_final_destructive_input(C2Expression *record);
unsigned char __fastcall final_record_condition(C2Expression *record);
void __fastcall set_final_record_condition(C2Expression *record, unsigned char condition);
void __fastcall prepare_final_assembly_record(C2Expression *record);
C2Expression *__fastcall final_record_uses_machine_register(C2Expression *record, C2MachineRegister *reg);
#define CONSTRAINT_REGISTER(number) (&machine_registers[(number)])

#define FINAL_DESTRUCTIVE_OPCODE(opcode) (opcode > 0 && opcode < 0x11b && \
            (opcode == 9 || opcode == 0x1e || opcode == 0x21 || \
             (opcode >= 0x23 && opcode <= 0x2e) || \
             (opcode >= 0x30 && opcode <= 0x32) || opcode == 0x34 || \
             (opcode >= 0x45 && opcode <= 0x51) || \
             (opcode >= 0x54 && opcode <= 0x55) || \
             (opcode >= 0x57 && opcode <= 0x59) || \
             (opcode >= 0x76 && opcode <= 0x77) || opcode == 0x8c || \
             opcode == 0x9f || opcode == 0xc1 || \
             (opcode >= 0xca && opcode <= 0xcb) || \
             (opcode >= 0xd6 && opcode <= 0xd7) || \
             (opcode >= 0xd9 && opcode <= 0xda)))

C2Expression *__fastcall finish_final_integer_record(C2Expression *record)
{
    C2FinalRegisterConstraints constraints;
    C2Expression *use, *owner;
    unsigned opcode;
    unsigned char condition;
    int destructive;

    if (!(record->flags & 1)) return record;
    if (record->kind == 20) {
        prepare_final_assembly_record(record);
        return record;
    }
    initialize_final_constraints(&constraints);
    find_final_constraints(record, &constraints);
    if (constraints.flags) {
        if (constraints.flags & 1) {
            if (constraints.result_register >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
            bind_final_fixed_result(record, CONSTRAINT_REGISTER(constraints.result_register));
        }
        if (constraints.flags & 2) {
            if (constraints.first_register >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
            bind_final_fixed_operand(record, record->left,
                CONSTRAINT_REGISTER(constraints.first_register));
        }
        if (constraints.flags & 4) {
            if (constraints.second_register >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
            bind_final_fixed_operand(record, record->left->next,
                CONSTRAINT_REGISTER(constraints.second_register));
        }
        opcode = record->opcode;
        /* Only the destructive-input Boolean column (bit 0x40) is used from
         * the original WORD rows. These are exactly its 48 true opcodes. */
        destructive = FINAL_DESTRUCTIVE_OPCODE(opcode);
        if ((constraints.flags & 8) || (destructive && (constraints.flags & 2)))
            prepare_final_destructive_input(record);
        if (constraints.flags & 0x10) {
            swap_global_operands(record);
            if (record->opcode == 0x2f) {
                use = follow_optimizer_result(record, &owner);
                condition = final_record_condition(use);
                if (condition > 10) { __asm { int 3 } }
                condition = condition < 3 ? condition : ((condition - 3) ^ 1) + 3;
                set_final_record_condition(use, condition);
                use = use->next;
                if (use->kind == 16 &&
                    final_record_uses_machine_register(use, &machine_registers[66])) {
                    condition = final_record_condition(use);
                    if (condition > 10) { __asm { int 3 } }
                    condition = condition < 3 ? condition : ((condition - 3) ^ 1) + 3;
                    set_final_record_condition(use, condition);
                }
            }
        }
    }
    return finish_final_record(record);
}
void __fastcall initialize_final_constraints(C2FinalRegisterConstraints *constraints)
{
    constraints->flags = 0;
    constraints->result_register = 0;
    constraints->first_register = 0;
    constraints->second_register = 0;
}
void __fastcall find_final_operand_constraints(C2Expression *record,
    C2Expression *result, C2Expression *first, C2Expression *second,
    C2FinalRegisterConstraints *constraints);

void __fastcall find_final_constraints(C2Expression *record, C2FinalRegisterConstraints *constraints)
{
    C2Expression *result, *first, *second;
    unsigned opcode = record->opcode;

    result = record->tail.right;
    if (FINAL_DESTRUCTIVE_OPCODE(opcode) && result->kind == 1 &&
        result->original_register->canonical == (C2FunctionRegister *)&machine_registers[66]) {
        first = record->left;
        result = copy_input_expression(first);
        if (first->kind != 1) {
            result->next = record->tail.right;
            result->location.expression.properties |= 0x10;
            record->tail.right = result;
        }
    }
    first = record->left;
    second = first ? first->next : NULL;
    find_final_operand_constraints(record, result, first, second, constraints);
    if (result != record->tail.right) release_small_node(result);
}
/* The original initialized pointer is NULL; its sole later writer publishes
 * a symbol returned by the genuine symbol-creation path at 47eb1f. Only
 * identity comparison is encountered here, not that symbol's payload. */
C2InputSymbol *final_special_call_symbol; /* 4943a0 */
extern int c2_favor_speed;
void __fastcall find_generic_final_constraints(C2Expression *record,
    C2Expression *result, C2Expression *first, C2Expression *second,
    C2FinalRegisterConstraints *constraints);
int __fastcall final_division_can_share_result(C2Expression *record);
int __fastcall final_flags_need_register(C2Expression *record);
#define FINAL_IMMEDIATE(operand) ((operand)->opcode == 0x11d || \
    (((1U << ((operand)->kind & 31)) & 0x18) && (operand)->opcode != 0x120))
#define FINAL_BOUND_RESULT(result) ((result)->kind == 1 && \
    (result)->value.reg->kind == 3 && (result)->value.reg->value && \
    (result)->original_register->kind == 1)

void __fastcall find_final_operand_constraints(C2Expression *record,
    C2Expression *result, C2Expression *first, C2Expression *second,
    C2FinalRegisterConstraints *constraints)
{
    C2Expression *use, *owner, *temporary, *marker;
    unsigned opcode = record->opcode;
    unsigned result_number = 1;
    unsigned kind_mask;

    if (first && first->kind == 3 && (first->type & 0xfff) < 4)
        constraints->flags |= 2;
    if (second && second->kind == 3 && (second->type & 0xfff) < 4)
        constraints->flags |= 4;
    switch (opcode) {
    case 1:
        kind_mask = (1U << (first->kind & 31)) & 0x18;
        if (kind_mask && first->opcode == 0x120 && result->kind == 1) {
            record->opcode = 0x12;
            return;
        }
        if (result->kind == 1 || first->kind == 1 || first->opcode == 0x11d) return;
        if (kind_mask && first->opcode != 0x120) return;
        constraints->flags |= 2;
        return;
    case 0x12:
        if (result->kind != 1) constraints->flags |= 2;
        return;
    case 9:
        if (FINAL_IMMEDIATE(second)) {
            constraints->flags |= 4;
            return;
        }
        break;
    case 0xa:
        if (result->kind != 1 ||
            result->original_register->canonical != (C2FunctionRegister *)&machine_registers[1]) {
            constraints->result_register = 1;
            constraints->flags |= 1;
        }
        goto byte_count;
    case 0xb:
        if (second->kind != 1 ||
            second->original_register->canonical != (C2FunctionRegister *)&machine_registers[1]) {
            constraints->second_register = 1;
            constraints->flags |= 4;
        }
byte_count:
        if (FINAL_IMMEDIATE(first) && (first->immediate & 0xff) == first->immediate) return;
        constraints->first_register = 3;
        constraints->flags |= 2;
        return;
    case 0xd:
conversion_input:
        if (first->kind == 1 && (first->type & 0xfff) < 4 &&
            first->original_register->kind == 2 &&
            (first->value.reg->kind < 3 || first->value.reg->kind > 6)) {
            constraints->flags |= 2;
            return;
        }
        if (FINAL_IMMEDIATE(first) || first->kind == 1) return;
        if (c2_favor_speed || (first->kind == 3 && first->opcode == 0x120))
            constraints->flags |= 2;
        if ((first->type & 0xfff) < 4 && first->opcode != 0x120)
            constraints->flags |= 2;
        return;
    case 0xe:
        if ((1U << (first->kind & 31)) & 0x180) constraints->flags |= 2;
        if (!result || !((1U << (result->kind & 31)) & 6)) return;
        if (record->left->kind == 4 &&
            record->left->value.symbol == (C2Symbol *)final_special_call_symbol) return;
        if ((record->type & 0xf000) == 0x4000) return;
        if (result->kind == 1 &&
            result->original_register->canonical == (C2FunctionRegister *)&machine_registers[1]) return;
        constraints->result_register = 1;
        constraints->flags |= 1;
        return;
    case 0x1e: case 0x21:
        if (first->kind == 7) constraints->flags |= 2;
        break;
    case 6: case 0x1f: case 0x20: case 0x22:
        goto divide;
    case 0x23: case 0x24:
        break;
    case 0x25: case 0x26: case 0x27: case 0x28:
    case 0x29: case 0x2a: case 0x2b:
        if (!FINAL_IMMEDIATE(second) &&
            (second->kind != 1 || second->fixed_register != &machine_registers[2])) {
            constraints->second_register = 2;
            constraints->flags |= 4;
        }
        break;
    case 0x33:
        if (first->kind == 7) {
            constraints->flags |= 0x10;
            if (second->kind == 7) constraints->flags |= 4;
            return;
        }
        if (second->kind != 2 && second->kind != 6) return;
        constraints->flags |= first->kind == 1 ? 0x10 : 4;
        return;
    case 0x99: case 0x9f: case 0xdf:
        if (first->kind != 1 || first->fixed_register != &machine_registers[1]) {
            constraints->first_register = 1;
            constraints->flags |= 2;
        }
        constraints->result_register = 1;
        constraints->flags |= 1;
        return;
    case 0xa3:
        if (first->kind != 1) constraints->flags |= 2;
        return;
    case 0xc1:
        if (first->kind != 1) constraints->flags |= 2;
        break;
    case 0xc2: case 0xc3: case 0xc4: case 0xc5:
        if (result->kind != 1) constraints->flags |= 1;
        if (FINAL_IMMEDIATE(first)) constraints->flags |= 2;
        break;
    case 0xca: case 0xcb:
        if (record->left->next->kind != 1) constraints->flags |= 4;
        break;
    case 0xcc: case 0xcd: case 0xce: case 0xcf:
        if (FINAL_IMMEDIATE(first)) constraints->flags |= 2;
        if (!FINAL_IMMEDIATE(second) && second->kind != 1) constraints->flags |= 4;
        break;
    case 0x107: case 0x108:
        if (result->kind != 1) constraints->flags |= 1;
        break;
    case 0x11c: case 0x13f: case 0x160: case 0x167: case 0x16d: case 0x175:
        return;
    case 0x130:
        if (FINAL_BOUND_RESULT(result)) return;
        goto conversion_input;
    case 0x14a:
        result_number = (record->type & 0xfff) > 1 ? 3 : 21;
divide:
        if (FINAL_IMMEDIATE(second)) constraints->flags |= 4;
        if (!final_division_can_share_result(record)) {
            constraints->first_register = 1;
            constraints->flags |= 2;
        } else if (FINAL_IMMEDIATE(first)) constraints->flags |= 2;
        if (FINAL_BOUND_RESULT(result)) return;
        constraints->result_register = result_number;
        constraints->flags |= 1;
        return;
    case 0x15d:
        if (!final_flags_need_register(record)) {
            if (record->left->kind != 1) constraints->flags |= 2;
            return;
        }
        first = record->left;
        if (c2_favor_speed) {
            if (first->kind != 1) constraints->flags |= 2;
            if (first->next) return;
            marker = new_input_expression(0x16c, 0x1001);
            prepend_input_record((C2Record *)record, (C2Record *)marker);
            temporary = new_temporary_operand(0x1001);
            marker->tail.right = temporary;
            temporary->location.expression.properties |= 0x10;
            temporary->value.reg->value = marker;
            copy_input_operand(record, temporary);
            return;
        }
        if (first->kind == 1 && first->value.reg->kind == 3 && first->value.reg->value) return;
        constraints->flags |= 2;
        return;
    case 0x15f:
        if (record->detail.intrinsic == 0xdf || record->detail.intrinsic == 0x157) return;
        break;
    default:
        if (opcode > 0x12 && opcode <= 0x2f) {
            if (FINAL_IMMEDIATE(first) && !FINAL_IMMEDIATE(second)) {
                use = follow_optimizer_result(record, &owner);
                if (final_record_condition(use)) {
                    constraints->flags |= 0x10;
                    break;
                }
            }
            if (FINAL_IMMEDIATE(first) ||
                (first->kind != 1 && !FINAL_IMMEDIATE(second) && second->kind != 1))
                constraints->flags |= 2;
        }
        break;
    }
    find_generic_final_constraints(record, result, first, second, constraints);
}
void __fastcall find_generic_final_constraints(C2Expression *record,
    C2Expression *result, C2Expression *first, C2Expression *second,
    C2FinalRegisterConstraints *constraints)
{
    unsigned opcode;
    int commutative;

    if (!result || !first) return;
    opcode = record->opcode;
    /* Other original opcodes are not invalid; their property rows have not
     * been encountered in this reconstruction. Guard before column use. */
    if (opcode > 0x175) { __asm { int 3 } }
    commutative = opcode == 0x2d || opcode == 0x2e || opcode == 0x30 ||
        opcode == 0x34 || opcode == 0xc1;
    if (FINAL_DESTRUCTIVE_OPCODE(opcode)) {
        if ((constraints->flags & 2) && result->kind != 1) constraints->flags |= 1;
        if (commutative && !compare_input_operands(result, second) && first->kind == 1) {
            constraints->flags |= 0x10;
        } else if (compare_input_operands(result, first)) {
            if (result->kind == 1 && result->value.reg->kind == 3 &&
                result->value.reg->value && result->original_register->kind == 3 &&
                result->original_register->value && first->kind == 1 &&
                first->value.reg->kind == 3 && first->value.reg->value &&
                first->original_register->kind == 3 && first->original_register->value) {
                constraints->flags |= 8;
            } else if (!(result->kind == 1 && first->kind == 1 &&
                result->original_register == first->original_register &&
                result->original_register !=
                    (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT))) {
                constraints->flags |= 2;
                if (result->kind != 1) constraints->flags |= 1;
            }
        } else if (first->kind != 1 && second && (second->kind == 2 || second->kind == 6)) {
            if (!(constraints->flags & 2)) constraints->flags |= 4;
        } else if (((1U << (result->kind & 31)) & 0x60) && second &&
                   ((1U << (second->kind & 31)) & 0x60)) {
            constraints->flags |= 3;
        }
    } else if (((1U << (first->kind & 31)) & 0x18) && first->opcode == 0x120)
        constraints->flags |= 2;
    if (second && ((1U << (second->kind & 31)) & 0x18) && second->opcode == 0x120)
        constraints->flags |= 4;
}
int __fastcall final_division_can_share_result(C2Expression *record)
{
    C2Expression *use = final_record_uses_machine_register(record, &machine_registers[1]);
    return use && use != record->left;
}
int __fastcall final_flags_need_register(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);
C2Expression *__fastcall find_final_fixed_operand(C2Expression *first, C2FunctionRegister *reg);
void __fastcall finish_final_multiply(C2Expression *record);
void __fastcall finish_final_call(C2Expression *record);
void __fastcall finish_final_condition_use(C2Expression *record, C2Expression *operand);
void __fastcall bind_final_division_result(C2Expression *record, C2MachineRegister *reg,
    unsigned unsigned_division);

/* This separate width-selected remainder column has only these three
 * encountered pointer identities. No neighboring cells or row data follow. */
static C2MachineRegister *final_remainder_registers[3] = {
    &machine_registers[17], &machine_registers[9], &machine_registers[1]
};

C2Expression *__fastcall finish_final_record(C2Expression *record)
{
    C2Expression *operand, *result, *inserted, *replacement;
    unsigned width, selector, unsigned_division;
    unsigned short type;

    switch (record->opcode) {
    case 0xc: case 0xd:
        copy_input_operand(record, new_fixed_register_operand(5, 0x1004));
        if (!find_final_fixed_operand(record->tail.right, (C2FunctionRegister *)&machine_registers[5]))
            attach_optimizer_output(record, new_fixed_register_operand(5, 0x1004));
        break;
    case 6: case 0x22:
        finish_final_multiply(record);
        if ((record->type & 0xfff) == 1) {
            result = record->tail.right;
            replacement = new_temporary_register_expression(0x11f, 0x1002,
                new_address_register(result->value.reg, 0x1002, 2, 0),
                (C2FunctionRegister *)&machine_registers[9]);
            replace_input_result(record, result, replacement);
        }
        break;
    case 0xe:
        finish_final_call(record);
        break;
    case 0xf:
        operand = record->left->next;
        if (operand->kind == 1 && operand->value.reg->kind == 3 && operand->value.reg->value)
            finish_final_condition_use(record, operand);
        break;
    case 0x1f: case 0x20:
        selector = (record->type >> 1) & 0x7ff;
        if (selector >= 3) { __asm { int 3 } }
        bind_final_division_result(record, final_division_registers[selector], record->opcode == 0x1f);
        break;
    case 0x25: case 0x26: case 0x27: case 0x28: case 0x29: case 0x2a: case 0x2b:
        operand = record->left->next;
        if (operand->kind == 1 && (operand->type & 0xfff) != 1) {
            operand->type = 0x1001;
            operand->fixed_register = &machine_registers[18];
            operand->value.reg = new_address_register(operand->value.reg, 0x1001, 1, 0);
        }
        break;
    case 0x109:
        if (record->tail.right->kind != 1) {
            operand = record->left;
            if (find_final_fixed_operand(operand, (C2FunctionRegister *)&machine_registers[66])) {
                inserted = new_assignment_expression(0x109, record->type, operand,
                    prepend_input_record, (C2Record *)record);
                record->opcode = 1;
                attach_input_expression(record, record->left, inserted->tail.right);
            } else record->opcode = 1;
        }
        break;
    case 0x130:
        if (record->tail.right->fixed_register == &machine_registers[5]) {
            operand = record->left;
            record->type = (record->type & 0xf004) | 4;
            type = (operand->type & 0xf004) | 4;
            operand->type = type;
            if (((1U << (operand->kind & 31)) & 6) && type != operand->value.reg->type)
                operand->value.reg = new_address_register(operand->value.reg, type, type & 0xfff, 0);
        }
        break;
    case 0x14a:
        unsigned_division = (record->type & 0xf000) == 0x2000;
        record->opcode = unsigned_division ? 0x1f : 0x20;
        selector = (record->type >> 1) & 0x7ff;
        if (selector >= 3) { __asm { int 3 } }
        bind_final_division_result(record, final_remainder_registers[selector], unsigned_division);
        break;
    }
    return record;
}
C2Expression *__fastcall find_final_fixed_operand(C2Expression *first, C2FunctionRegister *reg)
{
    while (first && (first->kind != 1 || first->original_register != reg)) first = first->next;
    return first;
}
void __fastcall finish_final_multiply(C2Expression *record)
{
    C2Expression *first, *next;

    first = record->left;
    if (final_record_uses_machine_register(record, &machine_registers[1]) == first) {
        next = first->next;
        copy_input_operand(record, first);
        release_small_node(record->left);
        record->left = next;
    }
}
void __fastcall finish_final_call(C2Expression *record)
{
    __asm { int 3 }
}
void __fastcall finish_final_condition_use(C2Expression *record, C2Expression *operand)
{
    __asm { int 3 }
}
C2Expression *__fastcall prepare_final_division_operands(unsigned short type,
    C2Expression *definition, C2Expression *temporary, C2Expression **input);
C2Expression *__fastcall insert_final_constant_assignment(C2Expression *temporary,
    C2Expression *definition, long value, unsigned bind_result);
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);

void __fastcall bind_final_division_result(C2Expression *record, C2MachineRegister *reg,
    unsigned unsigned_division)
{
    C2Expression *input, *definition, *temporary, *inserted, *result, *replacement, *previous;
    C2MachineRegister *fixed;
    unsigned selector;

    if (final_division_can_share_result(record)) return;
    input = record->left;
    definition = input->value.reg->value;
    temporary = new_temporary_operand(record->type);
    selector = (record->type & 0xffe) >> 1;
    if (selector >= 3) { __asm { int 3 } }
    fixed = final_division_registers[selector];
    temporary->fixed_register = fixed;
    if (fixed != &machine_registers[21]) copy_input_operand(record, temporary);
    if (!unsigned_division) {
        inserted = prepare_final_division_operands(record->type, definition, temporary, &input);
        attach_input_expression(record, record->left, input);
    } else {
        inserted = insert_final_constant_assignment(temporary, definition, 0, 1);
    }
    finish_final_multiply(record);
    if ((record->type & 0xfff) != 1) {
        /* Only these rows' number fields have been encountered. */
        if (reg != &machine_registers[0] && reg != &machine_registers[1] &&
            reg != &machine_registers[3]) { __asm { int 3 } }
        replacement = new_fixed_register_operand(reg->number, record->type);
        attach_optimizer_output(record, replacement);
        return;
    }
    result = record->tail.right;
    replacement = new_temporary_register_expression(0x11f, 0x1002,
        new_address_register(result->value.reg, 0x1002, 2, 0),
        (C2FunctionRegister *)&machine_registers[9]);
    replace_input_result(record, result, replacement);
    if (unsigned_division) {
        previous = inserted->previous;
        unlink_input_record((C2Record *)inserted);
        prepend_input_record((C2Record *)previous, (C2Record *)inserted);
        copy_input_operand(previous, inserted->tail.right);
    }
}

C2Expression *__fastcall insert_final_constant_assignment(C2Expression *temporary,
    C2Expression *definition, long value, unsigned bind_result)
{
    C2Expression *inserted;
    C2FunctionRegister *reg;

    inserted = new_binary_expression(1, temporary->type, temporary,
        new_integer_expression(temporary->type, (__int64)value),
        insert_input_record, (C2Record *)definition);
    if (bind_result) {
        reg = temporary->value.reg;
        if (!reg->canonical->variant_next) reg->value = inserted;
        else bind_input_temporary(reg, inserted);
        if (!value) inserted->opcode = 0x109;
    }
    return inserted;
}
C2Expression *__fastcall find_final_register_output(C2Expression *record, C2FunctionRegister *reg);
C2Expression *__fastcall split_final_wide_result(C2Expression *record,
    C2Expression *result, C2Expression *replacement);
C2Expression *__fastcall find_final_result_use(C2Expression *first,
    C2Expression *result, C2Expression **owner);

void __fastcall bind_final_fixed_result(C2Expression *record, C2MachineRegister *reg)
{
    C2Expression *result, *fixed_result, *temporary, *inserted, *address, *use, *owner;
    unsigned opcode;

    result = record->tail.right;
    if (result->kind != 1 || result->value.reg->kind != 3 || !result->value.reg->value) {
        temporary = new_temporary_operand(result->type);
        opcode = 1;
        if ((record->type & 0xf000) == 0x4000) {
            temporary->fixed_register = &machine_registers[31];
            opcode = 0x63;
        }
        inserted = new_binary_expression(opcode, result->type, result, temporary,
            insert_input_record, (C2Record *)record);
        result = inserted->tail.right;
        replace_input_result(record, record->tail.right, temporary);
        if (((1U << (result->kind & 31)) & 0x60)) {
            address = result->aggregate_value;
            if (address->kind == 1 && address->value.reg->kind == 3 &&
                address->value.reg->value &&
                address->original_register->canonical != address->value.reg->canonical) {
                use = find_final_register_output(record, address->original_register);
                if (use) {
                    attach_optimizer_output(inserted, use);
                    use->value.reg->value = inserted;
                    remove_input_result(record, use);
                }
            }
        }
        temporary->value.reg->value = record;
    }
    result = record->tail.right;
    if (reg != &machine_registers[0]) {
        if ((result->type & 0xfff) < 4) {
            /* Other static-row payloads are still unmodeled. */
            if (reg != &machine_registers[1]) { __asm { int 3 } }
            if (!reg->bit_displacement)
                reg = (C2MachineRegister *)new_bitfield_register(
                    (C2FunctionRegister *)&machine_registers[reg->number],
                    (result->type & 0xfff) << 3, 0);
        }
        if (result->fixed_register != reg) result->fixed_register = reg;
    }
    if ((record->type & 0xf000) != 0x4000) {
        fixed_result = result;
        inserted = new_assignment_expression(1, result->type, result,
            insert_input_record, (C2Record *)record);
        if ((record->tail.right->type & 0xfff) == 8) {
            temporary = split_final_wide_result(record, record->tail.right, NULL);
            attach_optimizer_output(record, temporary);
            result = record->tail.right;
            if (result->kind == 1 && result->value.reg->kind == 3 && result->value.reg->value)
                temporary->value.reg->value = record;
        }
        use = find_final_result_use(inserted->next, fixed_result, &owner);
        replace_final_input(owner, use, inserted->tail.right, attach_input_expression);
    }
}
C2Expression *__fastcall find_final_register_output(C2Expression *record, C2FunctionRegister *reg)
{
    C2Expression *result;
    C2FunctionRegister *root, *original, *original_root;
    long end, result_end;

    if ((unsigned long)reg >= (unsigned long)&machine_registers[0] &&
        (unsigned long)reg < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
        reg != (C2FunctionRegister *)&machine_registers[1] &&
        reg != (C2FunctionRegister *)&machine_registers[2] &&
        reg != (C2FunctionRegister *)&machine_registers[3] &&
        reg != (C2FunctionRegister *)&machine_registers[18] &&
        reg != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } }
    root = reg->canonical;
    for (result = record->tail.right; result; result = result->next) {
        if (result->kind == 1) {
            original = result->original_register;
            if ((unsigned long)original >= (unsigned long)&machine_registers[0] &&
                (unsigned long)original < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
                original != (C2FunctionRegister *)&machine_registers[0] &&
                original != (C2FunctionRegister *)&machine_registers[4] &&
                original != (C2FunctionRegister *)&machine_registers[5] &&
                original != (C2FunctionRegister *)&machine_registers[6] &&
                original != (C2FunctionRegister *)&machine_registers[7] &&
                original != (C2FunctionRegister *)&machine_registers[8] &&
                original != (C2FunctionRegister *)&machine_registers[1] &&
                original != (C2FunctionRegister *)&machine_registers[2] &&
                original != (C2FunctionRegister *)&machine_registers[3] &&
                original != (C2FunctionRegister *)&machine_registers[18] &&
                original != (C2FunctionRegister *)&machine_registers[66] &&
                original != (C2FunctionRegister *)&machine_registers[67] &&
                original != (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
            /* The table-end identity has only a typed sentinel prefix. */
            original_root = original == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)
                ? machine_register_storage.sentinel.canonical : original->canonical;
            if (original_root != root) continue;
            if (original == reg) return result;
            /* Row3 has canonical identity/number but no modeled range. */
            if (reg == (C2FunctionRegister *)&machine_registers[3] ||
                original == (C2FunctionRegister *)&machine_registers[3] ||
                original == (C2FunctionRegister *)&machine_registers[5]) { __asm { int 3 } }
            end = (long)(reg->value_bytes + (unsigned)reg->displacement);
            result_end = (long)(original->value_bytes + (unsigned)original->displacement);
            if (original->displacement < end && reg->displacement < result_end) return result;
        } else if (result->kind == 9 && optimizer_set_contains(result->value.registers, root->number)) {
            return result;
        }
    }
    return NULL;
}
C2Expression *__fastcall split_final_wide_result(C2Expression *record,
    C2Expression *result, C2Expression *replacement)
{
    __asm { int 3 }
    return NULL;
}
C2Expression *__fastcall find_final_result_use(C2Expression *first,
    C2Expression *result, C2Expression **owner)
{
#if !defined(__linux__)
    C2Expression *saved_eax;
#endif
    C2Expression *record, *use;
    C2FunctionRegister *reg;
    int searched;

#if !defined(__linux__)
    /* The original returns entry EAX unchanged if no searchable record is
     * visited. Fastcall C cannot name that defined incoming value; capture
     * it before compiler-generated C statements use EAX. No extra formal,
     * helper or wrapper is introduced. */
    __asm { mov saved_eax, eax }
#endif
    reg = result->value.reg;
    searched = 0;
    for (record = first; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        searched = 1;
        use = find_register_input(record, reg);
        if (use) {
            *owner = record;
            return use;
        }
    }
#if defined(__linux__)
    /* Host instrumentation does not supply the original entry-EAX contract.
     * Deny only this no-search-call domain, without inventing a return value. */
    if (!searched) { __asm { int 3 } }
    return NULL;
#else
    return searched ? NULL : saved_eax;
#endif
}
void __fastcall propagate_final_result_register(C2Expression *record);

void __fastcall prepare_final_destructive_input(C2Expression *record)
{
    C2Expression *result = record->tail.right;
    C2Expression *definition;

    if (result->kind == 1 && result->value.reg->kind == 3 && result->value.reg->value) {
        propagate_final_result_register(record);
        return;
    }
    definition = record->left->value.reg->value;
    attach_input_expression(record, record->left, result);
    replace_input_result(definition, definition->tail.right, result);
}
void __fastcall propagate_final_result_register(C2Expression *record)
{
    C2Expression *input, *result, *definition;
    C2FunctionRegister *fixed;
    unsigned opcode;

    result = record->tail.right;
    opcode = record->opcode;
    if (opcode > 0x175) { __asm { int 3 } }
    if (opcode == 0x14c || FINAL_DESTRUCTIVE_OPCODE(opcode)) input = record->left;
    else input = find_final_fixed_operand(record->left, result->original_register);
    if (!input) return;
    /* Static rows' kind column is not reconstructed. Do not interpret their
     * reserved backing as a register kind while following a future chain. */
    if ((unsigned long)input->original_register >= (unsigned long)&machine_registers[0] &&
        (unsigned long)input->original_register < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
        { __asm { int 3 } }
    if (input->original_register->kind == 1) {
        fixed = record->left->original_register;
        follow_optimizer_result(record, &input);
        result = NULL;
    } else fixed = record->tail.right->original_register;
    for (;;) {
        if (result && result->kind == 1 && result->value.reg->kind == 3 &&
            result->value.reg->value) result->original_register = fixed;
        if (!input || input->kind != 1 || input->value.reg->kind != 3 ||
            !input->value.reg->value) return;
        input->original_register = fixed;
        definition = input->value.reg->value;
        result = definition->tail.right;
        opcode = definition->opcode;
        if (opcode > 0x175) { __asm { int 3 } }
        if (opcode == 0x14c || FINAL_DESTRUCTIVE_OPCODE(opcode)) input = definition->left;
        else input = find_final_fixed_operand(definition->left, result->original_register);
    }
}
unsigned char __fastcall final_record_condition(C2Expression *record)
{
    switch (record->opcode) {
    case 0xf: case 0x159: return (unsigned char)record->type;
    case 0x15e: return record->detail.condition;
    case 0x10d: return 7;
    case 0x10e: return 10;
    case 0x10f: return 1;
    case 0x110: return 2;
    case 0x111: return 9;
    case 0x112: return 8;
    case 0x117: return 3;
    case 0x118: return 6;
    case 0x119: return 5;
    case 0x11a: return 4;
    default: return 0;
    }
}
void __fastcall set_final_record_condition(C2Expression *record, unsigned char condition)
{
    __asm { int 3 }
}
void __fastcall prepare_final_assembly_record(C2Expression *record)
{
    __asm { int 3 }
}
C2Expression *__fastcall final_record_uses_machine_register(C2Expression *record, C2MachineRegister *reg)
{
    C2Expression *operand;
    C2FunctionRegister *original, *original_root;
    long end, operand_end;

    /* The reached queries use the fully modeled row1 or flags-row66
     * root/range. Operand roots below are separately known identities. */
    if (reg != &machine_registers[1] && reg != &machine_registers[66])
        { __asm { int 3 } }
    for (operand = record->left; operand; operand = operand->next) {
        if (operand->kind != 1) continue;
        original = operand->original_register;
        if ((unsigned long)original >= (unsigned long)machine_registers &&
            (unsigned long)original <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
            original != (C2FunctionRegister *)&machine_registers[0] &&
            original != (C2FunctionRegister *)&machine_registers[1] &&
            original != (C2FunctionRegister *)&machine_registers[2] &&
            original != (C2FunctionRegister *)&machine_registers[3] &&
            original != (C2FunctionRegister *)&machine_registers[4] &&
            original != (C2FunctionRegister *)&machine_registers[5] &&
            original != (C2FunctionRegister *)&machine_registers[6] &&
            original != (C2FunctionRegister *)&machine_registers[7] &&
            original != (C2FunctionRegister *)&machine_registers[8] &&
            original != (C2FunctionRegister *)&machine_registers[18] &&
            original != (C2FunctionRegister *)&machine_registers[66] &&
            original != (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
        /* Compare the minimally modeled sentinel through its own view;
         * do not dereference a one-past array pointer as a full row. */
        original_root = original == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)
            ? machine_register_storage.sentinel.canonical : original->canonical;
        if (original_root != reg->canonical) continue;
        if (original == (C2FunctionRegister *)reg) return operand;
        if (original == (C2FunctionRegister *)&machine_registers[5]) { __asm { int 3 } }
        end = (long)(reg->bit_width + (unsigned)reg->bit_displacement);
        operand_end = (long)(original->value_bytes + (unsigned)original->displacement);
        if (original->displacement < end && reg->bit_displacement < operand_end) return operand;
    }
    return NULL;
}
C2Expression *__fastcall lower_final_float_record(C2Expression *record)
{
    __asm { int 3 }
    return NULL;
}
void __fastcall lower_final_address_record(C2Expression *record)
{
    __asm { int 3 }
}
int __fastcall register_views_differ(C2Expression *expression);
int __fastcall final_record_has_single_operands(C2Expression *record);
unsigned __fastcall find_final_input_position(C2Expression *record, C2FunctionRegister *reg);
int __fastcall final_input_accepts_replacement(C2Expression *record,
    unsigned position, C2Expression *replacement);

void __fastcall fold_final_assignment(C2Expression *record)
{
    C2Expression *input, *result, *scan, *use, *output;
    unsigned position;

    if (record->opcode != 1) return;
    result = record->tail.right;
    if (result->kind != 1 || result->value.reg->kind != 3 || !result->value.reg->value) return;
    input = record->left;
    if (input->kind == 1 && input->value.reg->kind == 3 && input->value.reg->value) {
        /* Only rows one/two/three have static kind bytes modeled so far. */
        if ((unsigned long)input->original_register >= (unsigned long)&machine_registers[0] &&
            (unsigned long)input->original_register < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
            input->original_register != (C2FunctionRegister *)&machine_registers[1] &&
            input->original_register != (C2FunctionRegister *)&machine_registers[2] &&
            input->original_register != (C2FunctionRegister *)&machine_registers[3])
            { __asm { int 3 } }
        if (input->original_register->kind == 1 &&
            (!register_views_differ(result) || result->original_register == input->original_register)) {
            for (scan = record->next; scan; scan = scan->next) {
                if (!(scan->flags & 1) || scan->opcode == 0x16c) continue;
                use = find_register_input(scan, result->value.reg);
                if (use) {
                    output = scan->tail.right;
                    if (output->kind != 1 || output->value.reg->kind != 3 ||
                        !output->value.reg->value || !register_views_differ(output)) {
                        if (replace_final_input(scan, use, input, attach_input_expression))
                            record->opcode = 0x11c;
                    }
                    break;
                }
                if (final_record_uses_machine_register(scan, (C2MachineRegister *)input->original_register) ||
                    find_final_register_output(scan, input->original_register)) break;
            }
        }
    }
    if (!final_record_has_single_operands(record)) return;
    result = record->tail.right;
    if (register_views_differ(result)) return;
    input = record->left;
    if (input->kind == 7 && !(input->integer.low | input->integer.high)) return;
    scan = record->next;
    if (!(scan->flags & 1)) return;
    position = find_final_input_position(scan, result->value.reg);
    if (!position || !final_input_accepts_replacement(scan, position, input)) return;
    use = scan->left;
    if (position != 1) use = use->next;
    if (replace_final_input(scan, use, input, attach_input_expression)) record->opcode = 0x11c;
}

int __fastcall final_record_has_single_operands(C2Expression *record)
{
    C2Expression *input, *remaining;

    input = record->left;
    remaining = input->next;
    if (record->tail.right->next) return 0;
    if (!remaining) return 1;
    if (((1U << (input->kind & 31)) & 0x60) &&
        !(input->location.expression.comparison & 0x20)) {
        if (input->aggregate_second) remaining = remaining->next;
        if (input->aggregate_value) remaining = remaining->next;
    }
    return remaining == NULL;
}
unsigned __fastcall find_final_input_position(C2Expression *record, C2FunctionRegister *reg)
{
    C2Expression *input = find_register_input(record, reg), *first, *second = NULL;
    unsigned opcode;
    if (!input) return 0;
    opcode = record->opcode;
    if (!opcode || opcode >= 0x11b) return 0;
    first = record->left;
    /* Only the actual consumed predicates: opcode WORD bit0x800 clear and
     * input-count BYTE equal2. No other flag bits, arities or neighboring
     * columns are modeled by this ordinary second-input classification. */
    if ((opcode >= 3 && opcode <= 5) || opcode == 9 || opcode == 0xb ||
        (opcode >= 0x25 && opcode <= 0x34) ||
        (opcode >= 0x45 && opcode <= 0x55) ||
        (opcode >= 0x57 && opcode <= 0x59) ||
        (opcode >= 0x5c && opcode <= 0x5d) || opcode == 0xc1 ||
        (opcode >= 0xca && opcode <= 0xcf) ||
        (opcode >= 0xd1 && opcode <= 0xd2) ||
        (opcode >= 0xd9 && opcode <= 0xda) || opcode == 0xe0 || opcode == 0xe3 ||
        (opcode >= 0xe6 && opcode <= 0xfc)) second = first->next;
    if (input == first) return 1;
    return input == second ? 2 : 0;
}
int __fastcall final_input_accepts_replacement(C2Expression *record,
    unsigned position, C2Expression *replacement)
{
    C2Expression *first, *second;
    C2FinalRegisterConstraints constraints;

    switch (record->opcode) {
    case 0xe: case 0x35: case 0x37: case 0x38: case 0x3b:
    case 0xfe: case 0xff: case 0x101:
        return 0;
    }
    first = second = replacement;
    if (position == 1) second = record->left->next;
    else if (position == 2) first = record->left;
    initialize_final_constraints(&constraints);
    find_final_operand_constraints(record, record->tail.right,
        first, second, &constraints);
    return constraints.flags == 0;
}
void __fastcall select_final_machine_form(C2Expression *record)
{
    C2Expression *use, *result;
    unsigned number;
    unsigned char condition;

    if (!(record->flags & 1)) return;
    condition = final_record_condition(record);
    if (condition) {
        use = final_record_uses_machine_register(record, &machine_registers[66]);
        /* The two original DWORD columns map conditions to flag-register
         * classes. Only the encountered condition domain 1..10 is modeled. */
        switch (condition) {
        case 1: case 2: number = 72; break;
        case 3: case 6: number = 68; break;
        case 4: case 5: number = 67; break;
        case 7: case 10: number = 73; break;
        case 8: case 9: number = 69; break;
        default: { __asm { int 3 } } number = 0;
        }
        use->fixed_register = &machine_registers[number];
        return;
    }
    result = find_final_register_output(record, (C2FunctionRegister *)&machine_registers[66]);
    switch (record->opcode) {
    case 0x1e: case 0x21:
        if (result) result->fixed_register = &machine_registers[67];
        else attach_optimizer_output(record, new_fixed_register_operand(67, 0x8000));
        return;
    case 0x27: case 0x28:
        attach_optimizer_output(record, new_fixed_register_operand(73, 0x8000));
        return;
    case 0x2c: case 0x31: case 0x107: case 0x108:
        use = final_record_uses_machine_register(record, &machine_registers[66]);
        use->fixed_register = &machine_registers[73];
        attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
        return;
    case 0xff:
        final_record_uses_machine_register(record, &machine_registers[66]);
        return;
    case 6: case 8: case 0xe:
    case 0x1f: case 0x20: case 0x22: case 0x23:
    case 0x29: case 0x2a: case 0x2b: case 0x2d: case 0x2e:
    case 0x30: case 0x32: case 0x33: case 0x34: case 0x35: case 0x3a:
    case 0xa3: case 0xc1: case 0xc8: case 0xc9: case 0xca: case 0xcb:
    case 0xcc: case 0xcd: case 0xce: case 0xcf: case 0xd9: case 0xda:
    case 0x105: case 0x109:
        if (!result) attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
        return;
    default:
        return;
    }
}
void __fastcall fold_final_address_operand(C2Expression *record, C2Expression *operand)
{
    __asm { int 3 }
}
C2InputSymbol *__fastcall materialize_real_symbol(C2Expression *operand)
{
    __asm { int 3 }
    return NULL;
}

int __fastcall final_register_accepts_width(C2FunctionRegister *reg, unsigned bytes, int mode);
C2Expression *__fastcall convert_final_operand(C2Expression *operand,
    unsigned short type, long alignment);
int __fastcall final_record_accepts_width(C2Expression *record, unsigned bytes);

int __fastcall replace_final_input(C2Expression *owner, C2Expression *old,
    C2Expression *replacement, C2ExpressionReplacer replace)
{
    unsigned short type = old->type;
    unsigned short replacement_type = replacement->type;
    C2Expression *overlap;
    long alignment = 0;
    unsigned bytes;

    if (type != replacement_type) {
        if (old->kind == 1 && old->value.reg->displacement < 4)
            alignment = old->value.reg->displacement;
        switch (replacement->kind) {
        case 1:
            if (replacement->value.reg->value) {
                overlap = find_final_overlapping_result(replacement->value.reg->value,
                    replacement->value.reg);
                if (overlap->value.reg->displacement >= (int)(type & 0xfff)) {
                    if ((overlap->type ^ type) & 0xfff) return 0;
                    replacement = overlap;
                }
            }
            if (!final_register_accepts_width(replacement->original_register, type & 0xfff, 0))
                return 0;
            break;
        case 2:
            if (owner->opcode == 0x120) break;
            /* Other kind-two uses share the aggregate narrowing path. */
        case 6:
            if ((replacement_type & 0xfff) < (type & 0xfff)) {
                bytes = replacement_type & 0xfff;
                if (!final_record_accepts_width(owner, bytes)) return 0;
                rewrite_final_record_width(owner, bytes);
                replace(owner, old, replacement);
                return 1;
            }
            break;
        case 3:
            if (alignment > 0 || (type & 0xfff) != 4) return 0;
            break;
        }
        replacement = convert_final_operand(replacement, type, alignment);
    }
    replace(owner, old, replacement);
    return 1;
}

C2Expression *__fastcall convert_final_operand(C2Expression *operand,
    unsigned short type, long alignment)
{
    __asm { int 3 }
    return NULL;
}
int __fastcall final_record_accepts_width(C2Expression *record, unsigned bytes)
{
    __asm { int 3 }
    return 0;
}

/* The calling convention determines stack reservation and entry copies. */
extern int c2_global_optimize;
C2Expression *__fastcall new_numbered_machine_operand(unsigned number, unsigned short type);
C2Expression *__fastcall find_next_record_kind(C2Record *prior, unsigned char kind);
void __fastcall prepend_hidden_parameter(C2Function *function, C2Expression *entry);
int get_argument_stack_offset(void);
void __fastcall assign_parameter_storage(C2Function *function, unsigned *offset,
    C2Expression *parameter, unsigned convention, int entry_parameter);
void __fastcall assign_entry_register_parameters(C2Function *function, C2Expression *entry);

void __fastcall assign_function_parameters(C2Function *function)
{
    C2Expression *entry, *parameter, *assignment, *marker, *operand;
    C2FunctionRegister *reg, *view;
    C2InputSymbol *symbol;
    C2MachineRegister *original;
    unsigned offset = 0, total, convention;
    unsigned short type;

    entry = find_next_record_kind(function->flow->last->records, 22);
    prepend_hidden_parameter(function, entry);
    total = 0;
    argument_stack_offset = get_argument_stack_offset();
    convention = current_symbol->flags & 0x1c;
    if (convention == 4 || convention == 0x14 || convention == 0x18)
        for (parameter = entry->left; parameter; parameter = parameter->next)
            assign_parameter_storage(function, &offset, parameter, convention, 1);
    assign_entry_register_parameters(function, entry);
    for (parameter = entry->left; parameter; parameter = parameter->next) {
        view = parameter->value.reg->canonical;
        symbol = (C2InputSymbol *)view->symbol;
        if (parameter->kind == 1) {
            original = parameter->fixed_register;
            if (original != &machine_registers[1] && original != &machine_registers[2] &&
                original != &machine_registers[3]) { __asm { int 3 } }
            if (original->kind == 1) {
                if (c2_global_optimize) {
                    symbol->storage.register_location.number = original->parameter_number;
                    symbol->attributes |= 0x8000;
                }
                if ((symbol->symbol.flags & 0x100) || !c2_global_optimize) {
                    symbol->symbol.flags |= 0x100;
                    symbol->attributes = (symbol->attributes & ~0x60) | 0x10;
                    for (; view; view = view->variant_next) view->kind = 4;
                    reserve_symbol_storage(&symbol->symbol);
                }
                continue;
            }
        }
        total += reserve_symbol_storage(&symbol->symbol);
    }
    ((C2FunctionSymbol *)current_symbol)->argument_stack_bytes = total;
    if ((convention != 4 && convention != 0x14) || (current_symbol->flags & 0x400)) return;
    for (parameter = entry->left; parameter; parameter = parameter->next) {
        if (parameter->kind != 1) continue;
        original = parameter->fixed_register;
        if (original != &machine_registers[1] && original != &machine_registers[2] &&
            original != &machine_registers[3]) { __asm { int 3 } }
        if (original->kind != 1) continue;
        reg = parameter->value.reg;
        symbol = (C2InputSymbol *)reg->canonical->symbol;
        if (!(symbol->symbol.flags & 0x100)) continue;
        type = parameter->type;
        assignment = new_binary_expression(1, type,
            new_typed_register_expression(0x120, type, reg),
            new_numbered_machine_operand(original->number, type), insert_input_record, (C2Record *)entry);
        assignment->location.line = entry->location.line;
        marker = new_input_expression(0x16c, parameter->type);
        operand = assignment->left;
        attach_optimizer_output(marker, copy_input_expression(operand));
        insert_input_record((C2Record *)entry, (C2Record *)marker);
        operand->value.reg->value = marker;
    }
}

C2Expression *__fastcall find_next_record_kind(C2Record *prior, unsigned char kind)
{
    C2Expression *record = (C2Expression *)prior->next;
    while (record && record->kind != kind) record = record->next;
    return record;
}
void __fastcall prepend_hidden_parameter(C2Function *function, C2Expression *entry)
{
    C2Expression *parameter;

    if (!function->return_storage) return;
    parameter = new_typed_register_expression(0x120, input_value_type,
        input_symbol_register(function->return_storage));
    parameter->next = entry->left;
    entry->left = parameter;
}
int get_argument_stack_offset(void)
{
    unsigned flags = current_symbol->flags;

    if ((flags & 0x1c) == 0xc) return 0;
    if ((flags & 2) || (!(flags & 1) && argument_layout_kind > 8)) return 12;
    return 8;
}
void __fastcall assign_parameter_storage(C2Function *function, unsigned *offset,
    C2Expression *parameter, unsigned convention, int entry_parameter)
{
    __asm { int 3 }
}
int __fastcall update_entry_parameter_flags(C2Function *function,
    C2FunctionRegister *reg, C2Expression *entry, int reset);

void __fastcall assign_entry_register_parameters(C2Function *function, C2Expression *entry)
{
    C2Expression *record, *operand;
    C2FunctionRegister *root;
    int phase;

    update_entry_parameter_flags(function, NULL, entry, 1);
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (!(record->flags & 1) || record->kind == 22) continue;
        for (phase = 2; phase > 0; --phase) {
            operand = phase == 2 ? record->tail.right : record->left;
            for (; operand; operand = operand->next) {
                root = NULL;
                switch (operand->kind) {
                case 1: case 2: case 3:
                    root = operand->value.reg->canonical;
                    break;
                case 5: case 6:
                    if (operand->tail.aggregate_register) root = operand->tail.aggregate_register->canonical;
                    break;
                }
                if (root && update_entry_parameter_flags(function, root, entry, 0))
                    root->symbol->flags |= 0x100;
            }
        }
    }
}

int __fastcall update_entry_parameter_flags(C2Function *function,
    C2FunctionRegister *reg, C2Expression *entry, int reset)
{
    C2Expression *parameter;
    C2FunctionRegister *root;

    for (parameter = entry->left; parameter; parameter = parameter->next) {
        root = parameter->value.reg->canonical;
        if (reset) {
            if (!(root->view_flags & 2) && !(root->flags & 0x10))
                root->symbol->flags &= ~0x100;
        } else if (root == reg) return 1;
    }
    return 0;
}

void __fastcall fold_final_record_inputs(C2Expression *record);
C2Expression *__fastcall fold_final_comparison(C2Expression *record);
C2Expression *__fastcall find_final_register_definition(C2Expression *record,
    C2FunctionRegister *reg);
int __fastcall integer_constant_differs(C2Expression *expression, unsigned __int64 value);
extern int c2_basic_mode;

void __fastcall fold_final_integer_forms(C2Function *function)
{
    C2Expression *record, *last, *next, *count, *output, *input;
    C2Expression *definition, *multiply, *owner, *constant;
    unsigned opcode, missing;
    int shift;

    last = (C2Expression *)function->flow->first->last_record;
    for (record = ((C2Expression *)function->flow->last->records)->next;
         record != last; record = next) {
        next = record->next;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record_line_delta;
        }
        if (!(record->flags & 1)) continue;
        fold_final_record_inputs(record);
        opcode = record->opcode;
        switch (opcode) {
        case 0x2f:
            next = fold_final_comparison(record);
            break;
        case 0x2d: case 0x32:
            count = record->left->next;
            if (count->kind != 7) break;
            output = find_final_register_output(record, (C2FunctionRegister *)&machine_registers[66]);
            if (((1U << (output->kind & 31)) & 6) && output->value.reg->value) break;
            output = record->tail.right;
            if (c2_basic_mode && (record->type & 0xf000) == 0x2000 && output->kind == 1) {
                definition = find_final_register_definition(record, output->value.reg);
                if (definition && definition->opcode == 0x14c) break;
            }
            if (count->integer.low == 1) record->opcode = opcode == 0x2d ? 0x21 : 0x1e;
            else {
                if (integer_constant_differs(count, ~(unsigned __int64)0)) break;
                record->opcode = opcode == 0x2d ? 0x1e : 0x21;
            }
            remove_input_operand(record, count);
            select_final_machine_form(record);
            break;
        case 0x2b:
            input = record->left;
            if (input->kind != 1 || input->value.reg->kind != 3 || !input->value.reg->value) break;
            count = input->next;
            if (count->kind != 7) break;
            definition = input->value.reg->value;
            owner = record;
            multiply = NULL;
            if (definition->opcode == 0x2e) multiply = definition;
            else if (definition->opcode == 1) {
                owner = definition;
                input = definition->left;
                if (input->kind == 1 && input->value.reg->kind == 3 && input->value.reg->value &&
                    input->value.reg->value->opcode == 0x2e) multiply = input->value.reg->value;
            }
            if (!multiply) break;
            constant = multiply->left->next;
            if (constant->kind != 7) break;
            shift = (short)count->integer.low;
            /* Only the proved power-of-two column's indices 0..31 are modeled. */
            if (shift < 0 || shift > 31) { __asm { int 3 } }
            missing = ~constant->integer.low;
            if ((((1U << shift) - 1) & missing) != missing) break;
            attach_input_expression(owner, owner->left, multiply->left);
            unlink_and_release_input_record((C2Record *)multiply);
            break;
        }
    }
}

int __fastcall can_forward_final_operand(C2Expression *record, C2Expression *operand);
int __fastcall final_definition_can_fold(C2Expression *record);
int __fastcall final_replacement_is_blocked(C2Expression *definition,
    C2Expression *record, C2Expression *protected_operand);

void __fastcall fold_final_record_inputs(C2Expression *record)
{
    C2Expression *operand, *definition;
    unsigned position;

    for (position = 1; position <= 2; ++position) {
        operand = record->left;
        if (position == 2) operand = operand->next;
        if (!operand) return;
        if (!can_forward_final_operand(record, operand)) continue;
        definition = operand->value.reg->value;
        if (!final_definition_can_fold(definition) ||
            final_replacement_is_blocked(definition, record, NULL) ||
            !final_input_accepts_replacement(record, position, definition->left)) continue;
        if (replace_final_input(record, operand, definition->left, attach_input_expression))
            unlink_and_release_input_record((C2Record *)definition);
    }
}
C2Expression *__fastcall fold_final_comparison(C2Expression *record)
{
    __asm { int 3 }
    return NULL;
}

int __fastcall can_forward_final_operand(C2Expression *record, C2Expression *operand)
{
    if (record->kind == 18 || record->kind == 19 || record->opcode == 0x13c ||
        (operand->location.expression.properties & 0x20) ||
        !((1U << (operand->kind & 31)) & 6) || !operand->value.reg->value) return 0;
    return find_final_register_output(record, operand->original_register) == NULL;
}
int __fastcall final_definition_can_fold(C2Expression *record)
{
    C2Expression *extra;

    if (record->opcode != 1 || record->tail.right->next) return 0;
    extra = record->left->next;
    return !extra || (extra->location.expression.properties & 0x20) != 0;
}
int __fastcall final_replacement_is_blocked(C2Expression *definition,
    C2Expression *record, C2Expression *protected_operand)
{
    C2Expression *stop = record->next;
    C2Expression *scan, *output, *input, *inputs;

    if (protected_operand) {
        for (scan = definition; scan != stop; scan = scan->next) {
            if (!(scan->flags & 1)) continue;
            for (output = scan->tail.right; output; output = output->next)
                if (input_operands_may_alias(output, protected_operand)) return 1;
        }
    } else {
        for (scan = definition; scan != stop; scan = scan->next) {
            if (!(scan->flags & 1)) continue;
            inputs = definition->left;
            for (output = scan->tail.right; output; output = output->next)
                for (input = inputs; input; input = input->next)
                    if (input_operands_may_alias(output, input)) return 1;
        }
    }
    return 0;
}
