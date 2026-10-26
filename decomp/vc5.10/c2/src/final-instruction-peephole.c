/* Instruction peepholes after frame construction. The main dispatcher and
 * its cold continuations form one original function. Opcode tables are
 * represented by their semantic cases; unknown static register payloads
 * remain guarded at the point of use. */
#include "global-optimizer.h"
#include "optimizer-translate.h"
#include "inlining.h"
#include "registers.h"
#include <recon.h>

#define REGISTER_OPERAND(node) ((1U << ((node)->kind & 31)) & 6U)
#define IMMEDIATE_OPERAND(node) ((1U << ((node)->kind & 31)) & 0x180U)
/* Only flags-row66's actual NULL definition is known for static rows.
 * Dynamic register value is the recovered definition pointer. Other static
 * payloads stop before a read in both builds. */
#define REQUIRE_REGISTER_VALUE(reg) do { \
    if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) \
        { __asm { int 3 } } \
} while (0)

#define REGISTER_DEFINITION(reg) ((reg) == (C2FunctionRegister *)&machine_registers[66] ? \
    machine_registers[66].definition : (reg)->value)
#define REQUIRE_REGISTER_KIND(reg) do { \
    if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } \
} while (0)
#define REQUIRE_REGISTER_NUMBER(reg) do { \
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

/* Genuine pending callees retain separate named entry traps. */
C2Expression * __fastcall new_register_set_expression(C2RegisterSet *set);
void __fastcall prepare_instruction_peephole_inputs(C2Expression *record, C2Expression *input);
void __fastcall finish_instruction_peephole_record(C2Expression *record, C2FunctionRegister *flags);
int __fastcall preceding_instruction_is_arithmetic(C2Expression *record);
void __fastcall remove_input_operand(C2Expression *record, C2Expression *operand);
int __fastcall peephole_436085(C2Expression *record, C2Expression **next);
int __fastcall peephole_436275(C2Expression *first, C2Expression *second);
int __fastcall select_narrow_logical_width(C2Expression *record, unsigned *width, unsigned *displacement);
void __fastcall peephole_440fae(C2Expression *record, C2Expression *replacement);
C2Expression * __fastcall peephole_4464f0(C2Expression *record);
void __fastcall peephole_45856e(C2Expression *record, unsigned width, unsigned displacement);
void __fastcall peephole_459fcf(C2Expression *record);
C2Expression * __fastcall peephole_46133c(C2Expression *record);
C2Expression * __fastcall peephole_462dfb(C2Expression *record);
C2Expression * __fastcall peephole_463050(C2Expression *record);
C2Expression * __fastcall peephole_463a2c(C2Expression *record);
void __fastcall peephole_463b53(C2Expression *record);
void __fastcall peephole_465547(C2Expression *record);
unsigned __fastcall previous_float_register_number(unsigned number);
void __fastcall peephole_47d9b4(C2Function *function);
void __fastcall peephole_481c20(C2Function *function);
C2Expression *__fastcall final_record_uses_machine_register(C2Expression *, C2MachineRegister *);
C2Expression *__fastcall find_final_register_output(C2Expression *, C2FunctionRegister *);
C2Expression *__fastcall new_fixed_register_operand(unsigned, unsigned short);
void __fastcall select_final_machine_form(C2Expression *);
void __fastcall normalize_final_float_order(C2Expression *);
unsigned char __fastcall final_record_condition(C2Expression *);
void __fastcall set_final_record_condition(C2Expression *, unsigned char);
int __fastcall final_register_accepts_width(C2FunctionRegister *, unsigned, int);
C2Expression *__fastcall retype_final_operand(C2Expression *, unsigned short, long);
C2Expression *__fastcall next_final_marker(C2Expression *);
C2Expression *__fastcall push_function_register(unsigned, C2Expression *);
C2Expression *__fastcall pop_function_register(unsigned, C2Expression *);
C2Expression *__fastcall new_runtime_call(unsigned, unsigned short,
    C2Expression *, C2Expression *, unsigned char, C2InputLink, C2Record *);
extern int c2_favor_speed, c2_global_optimize, c2_float_precision;

void __fastcall optimize_final_instruction_forms(C2Function *function)
{
    C2Expression *record, *next, *end, *input, *output, *left, *right;
    C2Expression *previous, *temporary, *replacement, *created, *marker, *scan;
    C2Expression *last_flags_input; /* Original fresh local; no initializer. */
    C2FunctionRegister *tracked = NULL, *working;
    unsigned low, bytes, opcode, condition;
    unsigned narrow_width = 0, narrow_offset = 0;
    C2Expression *owner;
    C2RegisterSet *clobbered;
    unsigned __int64 normalized;
    int have_flags_input = 0, moved_label, hops;

    if (c2_cpu_generation == 6) {
        peephole_47d9b4(function);
        peephole_481c20(function);
    }
    record = (C2Expression *)function->flow->last->records->next;
    end = (C2Expression *)function->flow->first->last_record;
    while (record != end) {
        next = record->next;
        if (!(record->flags & 1)) goto next_record;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record_line_delta;
        }
        input = final_record_uses_machine_register(record, &machine_registers[66]);
        if (input && REGISTER_OPERAND(input)) {
            REQUIRE_REGISTER_VALUE(input->value.reg);
            if (REGISTER_DEFINITION(input->value.reg)) tracked = NULL;
        }
        output = find_final_register_output(record, (C2FunctionRegister *)&machine_registers[66]);
        if (output && REGISTER_OPERAND(output)) {
            REQUIRE_REGISTER_VALUE(output->value.reg);
            if (REGISTER_DEFINITION(output->value.reg)) tracked = output->value.reg;
        }
        working = tracked;
        if (record->left) prepare_instruction_peephole_inputs(record, record->left);
        opcode = record->opcode;
        switch (opcode) {
        case 1:
            right = record->tail.right;
            if (right->kind == 1 && !working) {
                left = record->left;
                if (!IMMEDIATE_OPERAND(left)) {
                    if (preceding_instruction_is_arithmetic(record->previous) &&
                        !left->next && left->kind == 1) {
                        REQUIRE_REGISTER_KIND(left->value.reg);
                        if (left->value.reg->kind == 3) {
                            REQUIRE_REGISTER_VALUE(left->value.reg);
                            if (REGISTER_DEFINITION(left->value.reg)) {
                                previous = record->previous;
                                if (previous->tail.right->kind == 1 && previous->left->next->kind == 1 &&
                                    right->original_register != left->original_register &&
                                    previous->tail.right->original_register == left->original_register &&
                                    right->original_register == previous->left->next->original_register &&
                                    !right->next) {
                                    swap_global_operands(previous);
                                    REQUIRE_REGISTER_NUMBER(previous->left->original_register);
                                    replacement = new_fixed_register_operand(previous->left->original_register->number,
                                        previous->tail.right->type);
                                    replace_input_result(previous, previous->tail.right, replacement);
                                    REQUIRE_REGISTER_NUMBER(previous->tail.right->original_register);
                                    replacement = new_fixed_register_operand(previous->tail.right->original_register->number,
                                        record->left->type);
                                    attach_input_expression(record, record->left, replacement);
                                    goto fold_simple_move;
                                }
                            }
                        }
                    }
                    if (right->kind == 1 && left->kind == 1) {
                        REQUIRE_REGISTER_KIND(left->value.reg);
                        if (left->value.reg->kind == 13 &&
                            left->value.reg->allocation_operand->kind == 7 &&
                            !left->value.reg->allocation_operand->integer.value &&
                            right->original_register != left->original_register) {
                            record->opcode = 0x34;
                            attach_input_expression(record, left, right);
                            copy_input_operand(record, record->tail.right);
                            select_final_machine_form(record);
                        }
                    }
                } else {
                    if (!left->integer.low) {
                        record->opcode = 0x34;
                        release_small_node(left);
                        record->left = copy_input_expression(record->tail.right);
                        record->left->next = copy_input_expression(record->tail.right);
                    } else {
                        normalized = normalize_integer_value(left->type, (unsigned __int64)-1);
                        if (left->integer.low != (unsigned)normalized) goto fold_simple_move;
                        record->opcode = 0x30;
                        record->left = copy_input_expression(record->tail.right);
                        record->left->next = left;
                    }
                    attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
                }
            } else if (right->kind != 1 && !c2_favor_speed) {
                left = record->left;
                if (!working && IMMEDIATE_OPERAND(left) &&
                    !(right->location.expression.properties & 0x40)) {
                    normalized = normalize_integer_value(left->type, (unsigned __int64)-1);
                    if (!left->integer.low || left->integer.low == (unsigned)normalized) {
                        created = new_global_ternary_expression(left->integer.low ? 0x30 : 0x2e,
                            record->type, right, right, left, NULL, NULL);
                        peephole_440fae(record, created);
                        attach_optimizer_output(created, new_fixed_register_operand(66, 0x8000));
                        record = created;
                    }
                }
            }
fold_simple_move:
            right = record->tail.right;
            left = record->left;
            if (right->kind == 1 && left->kind == 1 &&
                right->original_register == left->original_register) {
                previous = record->previous;
                unlink_and_release_input_record((C2Record *)previous->next);
                record = previous;
            } else if (right->kind == 1 && !c2_favor_speed && left->kind == 7 &&
                       (record->type & 0xfff) == 4 && left->integer.low == (unsigned)(signed char)left->integer.low) {
                temporary = new_fixed_register_operand(5, 0x1004);
                created = new_integer_expression(0x1004, left->integer.value);
                replacement = new_fixed_register_operand(5, 0x1004);
                created = new_global_ternary_expression(13, 0x1004, replacement,
                    created, temporary, insert_input_record, (C2Record *)record);
                REQUIRE_REGISTER_NUMBER(record->tail.right->original_register);
                created = pop_function_register(record->tail.right->original_register->number, created);
                unlink_and_release_input_record((C2Record *)record);
                record = created;
            }
            break;
        case 0x0e:
            if (!c2_favor_speed && next->opcode == 0x2d && next->tail.right->kind == 1 &&
                next->tail.right->original_register == (C2FunctionRegister *)&machine_registers[5] &&
                next->left->next->kind == 7) {
                bytes = next->left->next->integer.low;
                if (bytes == 4 || bytes == 8) {
                    unlink_and_release_input_record((C2Record *)next);
                    if (bytes == 8) pop_function_register(2, record);
                    next = pop_function_register(2, record);
                }
            }
            if (c2_global_optimize && !record->detail.data) {
                moved_label = 0;
                marker = next_final_marker(record);
                temporary = record;
                if (!working) {
                    while (marker && (marker->flags & 1) &&
                           ((marker->opcode == 1 && marker->tail.right->kind == 1 &&
                             marker->left->kind == 1 &&
                             marker->tail.right->original_register == marker->left->original_register) ||
                            marker->opcode == 0x16c)) {
                        temporary = marker;
                        marker = peephole_4464f0(marker);
                    }
                }
                if (marker || working) {
                    hops = 0;
                    while (marker && marker->kind == 16 && !marker->detail.data && marker->opcode != 0x15c) {
                        moved_label = 1;
                        temporary = ((C2InputSymbol *)marker->left->value.symbol)->input_operand;
                        marker = next_final_marker(temporary);
                        if (hops++ > 4) break;
                    }
                }
                for (scan = temporary->next; scan; scan = scan->next) {
                    if (scan->kind == 25) moved_label = 1;
                    else if (scan->flags & 1) break;
                }
                if (scan && scan->opcode == 8 && scan->left && scan->left->kind == 7 &&
                    !scan->left->integer.value) {
                    /* Actual42dae0 pushes this current record as the seventh
                     * argument; Ghidra's old prototype omitted it. */
                    created = new_call_expression(0x10, 0, record->left, NULL, NULL,
                        prepend_input_record, (C2Record *)record);
                    unlink_and_release_input_record((C2Record *)record);
                    if (!moved_label) unlink_and_release_input_record((C2Record *)scan);
                    record = created;
                    next = created->next;
                }
            }
            break;
        case 0x12:
            left = record->left;
            if (left->kind != 5 || left->tail.indirect || working) break;
            right = record->tail.right;
            if (left->opcode == 0x122 &&
                left->aggregate_value->original_register == right->original_register) {
                low = (unsigned)left->detail.aggregate_displacement;
                attach_input_expression(record, left, right);
                if (low == ~0U) record->opcode = 0x1e;
                else if (low == 1) record->opcode = 0x21;
                else {
                    record->opcode = 0x2d;
                    copy_input_operand(record, new_integer_expression(0x1004, (long)low));
                }
                if (peephole_436085(record, &next)) goto next_record;
                select_final_machine_form(record);
            } else if (left->opcode == 0x123 && !left->detail.data &&
                       !(left->location.expression.properties & 15)) {
                right = record->tail.right;
                if (left->aggregate_value->original_register == right->original_register) {
                    REQUIRE_REGISTER_NUMBER(left->aggregate_second->original_register);
                    low = left->aggregate_second->original_register->number;
                } else if (left->aggregate_second->original_register == right->original_register) {
                    REQUIRE_REGISTER_NUMBER(left->aggregate_value->original_register);
                    low = left->aggregate_value->original_register->number;
                } else {
                    if (next && next->opcode == 0x32 && next->left->next->kind == 1 &&
                        peephole_436275(right, next->tail.right)) {
                        output = find_final_register_output(next,
                            (C2FunctionRegister *)&machine_registers[66]);
                        if (output && REGISTER_OPERAND(output))
                            REQUIRE_REGISTER_VALUE(output->value.reg);
                        if (!output || !REGISTER_OPERAND(output) || !REGISTER_DEFINITION(output->value.reg)) {
                            if (left->aggregate_value->original_register == next->left->next->original_register)
                                working = left->aggregate_second->original_register;
                            else if (left->aggregate_second->original_register == next->left->next->original_register)
                                working = left->aggregate_value->original_register;
                            else break;
                            record->opcode = 1;
                            REQUIRE_REGISTER_NUMBER(working);
                            attach_input_expression(record, left, new_fixed_register_operand(working->number, 0x1004));
                            unlink_and_release_input_record((C2Record *)next);
                            next = record;
                        }
                    }
                    break;
                }
                attach_input_expression(record, left, right);
                record->opcode = 0x2d;
                copy_input_operand(record, new_fixed_register_operand(low, 0x1004));
                next = record;
                select_final_machine_form(record);
            } else if (left->opcode == 0x125 && !left->detail.data &&
                       (left->location.expression.properties & 15) > 1) {
                if (left->aggregate_second->original_register == right->original_register) {
                    low = left->location.expression.properties & 15;
                    attach_input_expression(record, left, right);
                    record->opcode = 0x2a;
                    copy_input_operand(record, new_integer_expression(record->type, low));
                    select_final_machine_form(record);
                } else if (!c2_favor_speed) {
                    low = left->location.expression.properties & 15;
                    new_binary_expression(1, record->type, right,
                        left->aggregate_second, prepend_input_record, (C2Record *)record);
                    attach_input_expression(record, record->left, record->tail.right);
                    record->opcode = 0x2a;
                    copy_input_operand(record, new_integer_expression(record->type, low));
                    select_final_machine_form(record);
                }
            }
            break;
        case 0x1e: case 0x21:
            if (!working && peephole_436085(record, &next)) goto next_record;
            break;
        case 0x2d: case 0x32:
            /* Opposite arithmetic on identical operands can cancel only when
             * neither output nor source is volatile and flags are unobserved. */
            if (!working && next && next->opcode == (opcode == 0x2d ? 0x32 : 0x2d) &&
                peephole_436275(record->tail.right, next->tail.right) &&
                !(((1U << (record->tail.right->kind & 31)) & 0x7eU) &&
                  (record->tail.right->location.expression.properties & 0x40)) &&
                !(((1U << (next->tail.right->kind & 31)) & 0x7eU) &&
                  (next->tail.right->location.expression.properties & 0x40)) &&
                peephole_436275(record->left->next, next->left->next) &&
                !(((1U << (record->left->next->kind & 31)) & 0x7eU) &&
                  (record->left->next->location.expression.properties & 0x40)) &&
                !(((1U << (next->left->next->kind & 31)) & 0x7eU) &&
                  (next->left->next->location.expression.properties & 0x40))) {
                output = find_final_register_output(next, (C2FunctionRegister *)&machine_registers[66]);
                if (output && REGISTER_OPERAND(output))
                    REQUIRE_REGISTER_VALUE(output->value.reg);
                if (!output || !REGISTER_OPERAND(output) || !REGISTER_DEFINITION(output->value.reg)) {
                    previous = record->previous;
                    unlink_and_release_input_record((C2Record *)record);
                    unlink_and_release_input_record((C2Record *)next);
                    next = previous->next;
                    goto next_record;
                }
            }
            if (!working && record->left->kind == 1 && record->left->next->kind == 7) {
                if (peephole_436085(record, &next)) goto next_record;
                if (record->opcode != 0x2d && record->opcode != 0x32) break;
            }
            if (!c2_favor_speed && record->left->kind == 1 && record->left->next->kind == 7 &&
                (record->type & 0xfff) == 4) {
                if (working) {
                    int wrote_flags_input = 0;
                    for (scan = record->next; scan; scan = scan->next) {
                        if (scan->flags & 1) {
                            input = find_register_input(scan, working);
                            last_flags_input = input;
                            have_flags_input = 1;
                            wrote_flags_input = 1;
                            if (input) break;
                        }
                    }
                    /* A prior defined result is carried when this scan makes
                     * no call. The original fresh-local consumer is guarded;
                     * do not invent an initial NULL or evaluate it early. */
                    if (!wrote_flags_input && !have_flags_input) { __asm { int 3 } }
                    if (last_flags_input->original_register != (C2FunctionRegister *)&machine_registers[72]) break;
                }
                peephole_459fcf(record);
            }
            if (record->opcode == 0x32 && record->tail.right->kind == 1 &&
                record->tail.right->original_register == (C2FunctionRegister *)&machine_registers[5] &&
                record->left->next->kind == 7) {
                bytes = record->left->next->integer.low;
                if (bytes == 4 || (!c2_favor_speed && bytes == 8)) {
                    created = push_function_register(2, record);
                    if (bytes == 8) next = push_function_register(2, created);
                    unlink_and_release_input_record((C2Record *)record);
                    record = created;
                }
            } else if (record->opcode == 0x2d && next->kind == 14 &&
                       record->tail.right->kind == 1 &&
                       record->tail.right->original_register == (C2FunctionRegister *)&machine_registers[5] &&
                       record->left->next->kind == 7) {
                bytes = record->left->next->integer.low;
                if (bytes == 4 || (!c2_favor_speed && bytes == 8)) {
                    created = pop_function_register(2, record);
                    if (bytes == 8) next = pop_function_register(2, created);
                    unlink_and_release_input_record((C2Record *)record);
                    record = created;
                }
            }
            break;
        case 0x2e: case 0x30: case 0x33:
            narrow_width = 0;
            if (c2_cpu_generation != 6 && select_narrow_logical_width(record, &narrow_width, &narrow_offset))
                peephole_45856e(record, narrow_width, narrow_offset);
            break;
        case 0x2f:
            /* The original's allocation-operand constant observation feeds
             * only a dead scratch pair; no result or later branch consumes it. */
            left = record->left;
            if (left->kind == 1 && left->next->kind == 7 && !left->next->integer.low) {
                record->opcode = 0x33;
                attach_input_expression(record, left->next, left);
            }
            break;
        case 0x45: case 0x46: case 0x47: case 0x48: case 0x49: case 0x4a:
            next = peephole_462dfb(record);
            next = peephole_463050(record);
            goto next_record;
        case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x50:
            next = peephole_463050(record);
            goto next_record;
        case 0x5d:
            previous = record->previous;
            if (previous->opcode != 0x60) break;
            left = previous->left;
            if (left->kind == 1) REQUIRE_REGISTER_KIND(left->original_register);
            if (left->kind == 1 && (left->original_register)->kind == 1) REQUIRE_REGISTER_NUMBER(left->original_register);
            if (left->kind == 1 && (left->original_register)->kind == 1 &&
                (left->original_register)->number == 31) {
                input = record->left->next;
                if (input->kind == 1) REQUIRE_REGISTER_KIND(input->original_register);
                if (input->kind == 1 && (input->original_register)->kind == 1) {
                    REQUIRE_REGISTER_NUMBER(input->original_register);
                    low = previous_float_register_number((input->original_register)->number);
                    replacement = new_fixed_register_operand(low, input->type);
                    attach_input_expression(record, record->left->next, replacement);
                }
                attach_input_expression(record, record->left, record->previous->left);
                record->opcode = 0x5c;
                normalize_final_float_order(record);
                unlink_and_release_input_record((C2Record *)record->previous);
            } else {
                if (record->type != previous->type || !((1U << (left->kind & 31)) & 0x60)) break;
                input = record->left;
                if (input->kind == 1) {
                    REQUIRE_REGISTER_KIND(input->original_register);
                    if (input->original_register->kind == 1) REQUIRE_REGISTER_NUMBER(input->original_register);
                }
                if (input->kind != 1 || (input->original_register)->kind != 1 ||
                    (input->original_register)->number != 31) break;
                input = input->next;
                if (input->kind == 1) {
                    REQUIRE_REGISTER_KIND(input->original_register);
                    if (input->original_register->kind == 1) REQUIRE_REGISTER_NUMBER(input->original_register);
                }
                if (input->kind != 1 || (input->original_register)->kind != 1 ||
                    (input->original_register)->number != 32) break;
                record->opcode = 0x5c;
                normalize_final_float_order(record);
                attach_input_expression(record, record->left->next, record->previous->left);
                unlink_and_release_input_record((C2Record *)record->previous);
                record = follow_optimizer_result(record, &owner);
                record = follow_optimizer_result(record, &owner);
                record = follow_optimizer_result(record, &owner);
                condition = final_record_condition(record);
                if (condition > 10) { __asm { int 3 } }
                condition = condition < 3 ? condition : ((condition - 3) ^ 1) + 3;
                set_final_record_condition(record, (unsigned char)condition);
            }
            goto next_record;
        case 0x5e:
            previous = record->previous;
            if (previous->opcode != 0x60) break;
            left = previous->left;
            if (record->type != previous->type && left->kind != 1) break;
            if (left->kind == 1) REQUIRE_REGISTER_NUMBER(left->original_register);
            if (left->kind == 1 && (left->original_register)->number != 31) break;
            if (record->next->opcode != 0x6e || record->next->next->opcode != 0xae) break;
            replacement = left;
            if (left->kind == 1) REQUIRE_REGISTER_KIND(left->original_register);
            if (left->kind == 1 && (left->original_register)->kind == 1) {
                REQUIRE_REGISTER_NUMBER(left->original_register);
                low = previous_float_register_number((left->original_register)->number);
                replacement = new_fixed_register_operand(low, record->type);
            }
            record->opcode = 0x5d;
            attach_input_expression(record, record->left->next, replacement);
            record = follow_optimizer_result(record->next->next, &owner);
            condition = final_record_condition(record);
            if (condition > 10) { __asm { int 3 } }
            condition = condition < 3 ? condition : ((condition - 3) ^ 1) + 3;
            set_final_record_condition(record, (unsigned char)condition);
            unlink_and_release_input_record((C2Record *)previous);
            goto next_record;
        case 0x5f:
            next = peephole_463a2c(record);
            goto next_record;
        case 0x60:
            previous = record->previous;
            left = record->left;
            if (!c2_float_precision && previous->opcode == 0x63) {
                right = previous->tail.right;
                if (left->type == right->type && left->kind == 2 && right->kind == 2 &&
                    !compare_input_operands(left, right) && (record->left->type & 0xfff) != 10) {
                    record->previous->opcode = 0x62;
                    normalize_final_float_order(record->previous);
                    unlink_and_release_input_record((C2Record *)record);
                    goto next_record;
                }
            }
            if (record->left->kind != 1) break;
            REQUIRE_REGISTER_KIND(record->left->original_register);
            if ((record->left->original_register)->kind != 1) break;
            next = peephole_46133c(record);
            goto next_record;
        case 0x63:
            previous = record->previous;
            scan = previous;
            while (scan->opcode == 0x32 && scan->left->kind == 1) {
                REQUIRE_REGISTER_KIND(scan->left->original_register);
                if (scan->left->original_register->kind != 1) break;
                REQUIRE_REGISTER_NUMBER(scan->left->original_register);
                if (scan->left->original_register->number != 5) break;
                scan = scan->previous;
            }
            if (scan->opcode == 0x60 && scan->left->kind == 1) {
                REQUIRE_REGISTER_KIND(scan->left->original_register);
                if (scan->left->original_register->kind == 1) REQUIRE_REGISTER_NUMBER(scan->left->original_register);
            }
            if (scan->opcode == 0x60 && scan->left->kind == 1 &&
                (scan->left->original_register)->kind == 1 &&
                (scan->left->original_register)->number == 31) {
                right = record->tail.right;
                record->opcode = 0x62;
                if (right->kind == 1) {
                    REQUIRE_REGISTER_NUMBER(right->original_register);
                    low = (right->original_register)->number;
                    /* Original494708 + number*80 is rows[number-1]. This is
                     * an address identity only; no new row payload is read. */
                    if (!low || low > C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
                    right->original_register = (C2FunctionRegister *)&machine_registers[low - 1];
                }
                normalize_final_float_order(record);
                unlink_and_release_input_record((C2Record *)scan);
            } else {
                right = record->tail.right;
                if (right->kind == 1) {
                    REQUIRE_REGISTER_KIND(right->original_register);
                    if (right->original_register->kind == 1) REQUIRE_REGISTER_NUMBER(right->original_register);
                }
                if (right->kind != 1 || (right->original_register)->kind != 1 ||
                    (right->original_register)->number != 31) break;
                if (previous->opcode == 0x6e) previous = previous->previous;
                if (previous->opcode == 0x5c &&
                    ((1U << (previous->left->next->kind & 31)) & 0x60)) {
                    previous->opcode = 0x5d;
                } else if (previous->opcode == 0x62 &&
                           ((1U << (previous->tail.right->kind & 31)) & 0x60)) {
                    previous->opcode = 0x63;
                } else break;
                output = new_temporary_register_expression(0x11f, 0x4008,
                    (C2FunctionRegister *)&machine_registers[31],
                    (C2FunctionRegister *)&machine_registers[31]);
                attach_optimizer_output(previous, output);
                unlink_and_release_input_record((C2Record *)record);
            }
            goto next_record;
        case 0xae:
            if (c2_favor_speed) peephole_463b53(record);
            break;
        case 0xc2: case 0xc3:
            right = record->tail.right;
            if ((c2_cpu_generation == 6 || c2_favor_speed) && !working &&
                right->kind == 1 && !right->next && record->left->kind == 1 &&
                !final_record_uses_machine_register(record,
                    (C2MachineRegister *)right->original_register)) {
                if (!final_register_accepts_width(record->tail.right->original_register,
                                                  record->left->type & 0xfff, 0))
                    goto next_record;
                right = record->tail.right;
                REQUIRE_REGISTER_NUMBER(right->original_register);
                temporary = new_fixed_register_operand((right->original_register)->number, right->type);
                input = new_fixed_register_operand((right->original_register)->number, right->type);
                output = new_fixed_register_operand((right->original_register)->number, right->type);
                created = new_global_ternary_expression(0x34, record->tail.right->type,
                    output, input, temporary, prepend_input_record, (C2Record *)record);
                select_final_machine_form(created);
                record->opcode = 1;
                retype_final_operand(record->tail.right, record->left->type, 0);
                record->type = record->left->type;
            }
            break;
        case 0x107:
            input = record->left->next;
            record->opcode = 0x2c;
            record->left = copy_input_expression(record->tail.right);
            record->left->next = copy_input_expression(record->tail.right);
            record->left->next->next = input;
            break;
        case 0x108:
            input = record->left;
            record->opcode = 0x31;
            record->left = copy_input_expression(record->tail.right);
            record->left->next = copy_input_expression(record->tail.right);
            record->left->next->next = input;
            if (!input) attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
            break;
        case 0x109:
            if (record->tail.right->kind != 1)
                c2_report_internal("E:\\utc\\src\\\\P2\\x86\\lower.c", 0xe45);
            record->opcode = 0x34;
            release_small_node(record->left);
            record->left = copy_input_expression(record->tail.right);
            record->left->next = copy_input_expression(record->tail.right);
            break;
        case 0x13c:
            peephole_465547(record);
            unlink_and_release_input_record((C2Record *)record);
            break;
        case 0x15b:
            input = new_integer_expression(0x1004, 0);
            created = new_transfer_expression(8, 0x1004, NULL, input,
                insert_input_record, (C2Record *)record);
            copy_input_operand(created, new_fixed_register_operand(1, 0x1004));
            attach_optimizer_output(created, new_fixed_register_operand(66, 0x8000));
            ((C2FunctionSymbol *)record->left->value.symbol)->pending3e = 1;
            input = new_scalar_expression(0x129, (unsigned)record->left->value.symbol);
            output = new_fixed_register_operand(1, 0x1004);
            new_binary_expression(1, 0x1004, output, input,
                insert_input_record, (C2Record *)record);
            unlink_and_release_input_record((C2Record *)record);
            goto next_record;
        case 0x15c:
            if (record->flags & 8) {
                input = new_scalar_expression(0x12a, (unsigned)record->left->value.symbol);
                ((C2FunctionSymbol *)input->value.symbol)->pending3e = 1;
                output = new_fixed_register_operand(66, 0x8000);
                created = new_runtime_call(14, 0x1004, output, input, 4,
                    prepend_input_record, (C2Record *)record);
                clobbered = new_register_set(8);
                fill_global_available_set(clobbered, 9);
                remove_optimizer_symbol(clobbered, 0);
                output = new_register_set_expression(clobbered);
                attach_optimizer_output(created, output);
            }
            unlink_and_release_input_record((C2Record *)record);
            goto next_record;
        case 0x16c:
            right = record->tail.right;
            if (right && REGISTER_OPERAND(right)) {
                REQUIRE_REGISTER_VALUE(right->value.reg);
                if (REGISTER_DEFINITION(right->value.reg)) {
                    created = follow_optimizer_result(record, &owner);
                    right = record->tail.right;
                    REQUIRE_REGISTER_NUMBER(right->original_register);
                    output = new_fixed_register_operand((right->original_register)->number, right->type);
                    attach_optimizer_output(created, output);
                }
            }
            /* Fall through: these bookkeeping records are removed. */
        case 0x161: case 0x164: case 0x166: case 0x169: case 0x172:
            unlink_and_release_input_record((C2Record *)record);
            goto next_record;
        case 0x171:
            record->opcode = 0x11c;
            unlink_and_release_input_record((C2Record *)record);
            goto next_record;
        default:
            break;
        }
        finish_instruction_peephole_record(record, tracked);
next_record:
        record = next;
    }
}

/* Unreconstructed genuine callees stop before their bodies execute. */
C2Expression * __fastcall new_register_set_expression(C2RegisterSet *set)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall new_final_address_expression(unsigned opcode,
    unsigned short type, C2FunctionRegister *reg, long displacement,
    C2Expression *base, C2Expression *index);

void __fastcall prepare_instruction_peephole_inputs(C2Expression *record, C2Expression *input)
{
    C2Expression *replacement, *index;
    unsigned char properties;

    if (input->opcode != 0x125) return;
    properties = input->location.expression.properties;
    if ((properties & 15) != 1 || input->tail.indirect) return;
    index = input->aggregate_second;
    if (input->kind == 5)
        replacement = new_final_address_expression(0x123, input->type, NULL,
            input->detail.aggregate_displacement, index, index);
    else
        replacement = new_aggregate_expression(0x123, input->type, NULL,
            input->detail.aggregate_displacement, index, index, input->immediate);
    replacement->location.expression.properties =
        (replacement->location.expression.properties & ~0x40) | (properties & 0x40);
    attach_input_expression(record, input, replacement);
}
int __fastcall integer_constant_differs(C2Expression *expression, unsigned __int64 value);

void __fastcall finish_instruction_peephole_record(C2Expression *record, C2FunctionRegister *flags)
{
    C2Expression *constant;
    unsigned __int64 identity;

    /* Only these four table-selected arithmetic forms have an action. Their
     * original class BYTE is2 and their destructive-input bit is set; all
     * other dispatch targets return without inspecting an operand. */
    switch (record->opcode) {
    case 0x2d: case 0x30: case 0x32: identity = 0; break;
    case 0x2e: identity = ~(unsigned __int64)0; break;
    default: return;
    }
    constant = record->left->next;
    if (constant->kind != 7) return;
    if (!integer_constant_differs(constant, identity) && !flags)
        unlink_and_release_input_record((C2Record *)record);
}
int __fastcall preceding_instruction_is_arithmetic(C2Expression *record)
{
    switch (record->opcode) {
    case 0x2d: case 0x2e: case 0x30: case 0x34: case 0xc1: return 1;
    default: return 0;
    }
}
int __fastcall peephole_436085(C2Expression *record, C2Expression **next)
{
    C2Expression *previous = record->previous, *input, *prior_input, *constant;
    C2IntegerWords amount; /* The default's high DWORD is genuinely unwritten. */
    unsigned prior_opcode;

    if ((record->type & 0xfff) != 4) return 0;
    prior_opcode = previous->opcode;
    if (!(((prior_opcode == 0x2d || prior_opcode == 0x32) &&
           previous->left->next->kind == 7) ||
          prior_opcode == 0x21 || prior_opcode == 0x1e)) return 0;
    input = record->left;
    prior_input = previous->left;
    if (input->kind != 1 || prior_input->kind != 1 ||
        input->original_register != prior_input->original_register) return 0;

    amount.low = (unsigned)next;
    switch (record->opcode) {
    case 0x1e: amount.value = ~(unsigned __int64)0; break;
    case 0x21: amount.value = 1; break;
    case 0x2d: amount.value = input->next->integer.value; break;
    case 0x32: amount.value = 0 - input->next->integer.value; break;
    default:
        /* Original defect: this path uses the saved next-pointer as the low
         * DWORD and a fresh unwritten stack DWORD as the high half. Preserve
         * that partial C local in Win32 rather than inventing a zero value. */
#ifdef __linux__
        /* Numerical Win32 object addresses and incidental stack bytes are
         * not equivalent to this host's pointer/stack representation. */
        __asm { int 3 }
#endif
        break;
    }
    switch (prior_opcode) {
    case 0x1e: --amount.value; break;
    case 0x21: ++amount.value; break;
    case 0x2d: amount.value += prior_input->next->integer.value; break;
    case 0x32: amount.value -= prior_input->next->integer.value; break;
    }
    unlink_and_release_input_record((C2Record *)previous);
    if (!amount.value) {
        *next = record->next;
        unlink_and_release_input_record((C2Record *)record);
        return 1;
    }
    if (amount.value == 1 || amount.value == ~(unsigned __int64)0) {
        if (input->next) remove_input_operand(record, input->next);
        record->opcode = amount.value == 1 ? 0x21 : 0x1e;
    } else {
        constant = new_integer_expression(record->type, amount.value);
        if (input->next)
            attach_input_expression(record, input->next, constant);
        else
            copy_input_operand(record, constant);
        record->opcode = 0x2d;
        peephole_459fcf(record);
    }
    return 0;
}
int __fastcall peephole_436275(C2Expression *first, C2Expression *second)
{ __asm { int 3 } return 0; }
int __fastcall select_narrow_logical_width(C2Expression *record,
    unsigned *width, unsigned *displacement)
{
    C2Expression *input, *output, *owner, *use;
    unsigned requested = *width, selected = 0, offset;
    unsigned constant, mask, identity, lane;
    unsigned char condition;

    if ((record->type & 0xfff) == requested ||
        (!requested && (record->type & 0xfff) == 1) ||
        record->left->next->kind != 7) return 0;
    if (record->opcode == 0x33) {
        use = follow_optimizer_result(record, &owner);
        condition = final_record_condition(use);
        if (!condition || condition > 2) return 0;
    } else {
        output = find_final_register_output(record, (C2FunctionRegister *)&machine_registers[66]);
        if (REGISTER_OPERAND(output)) {
            REQUIRE_REGISTER_VALUE(output->value.reg);
            if (REGISTER_DEFINITION(output->value.reg)) return 0;
        }
    }
    input = record->left;
    constant = input->next->integer.low;
    /* The four original mask/offset pairs mean the bytes at offsets0..3.
     * AND requires ones outside the selected byte, OR/XOR require zeros.
     * No table address escapes or neighboring data is consumed. */
    if (requested == 1 || !requested) {
        for (lane = 0; lane != 4; ++lane) {
            mask = ~(0xffU << (lane * 8));
            identity = record->opcode == 0x2e ? mask : 0;
            if ((constant & mask) == identity) {
                selected = 1;
                offset = lane;
                break;
            }
        }
    }
    if ((requested == 2 || !requested) && !selected && !c2_favor_speed) {
        identity = record->opcode == 0x2e ? ~0U : 0;
        if ((constant & 0xffff0000U) == (identity & 0xffff0000U)) {
            selected = 2;
            offset = 0;
        } else if ((constant & 0xffffU) == (identity & 0xffffU)) {
            selected = 2;
            offset = 2;
        }
    }
    if (input->kind == 1 && (long)(input->type & 0xfff) < (long)requested) {
        selected = requested;
        offset = 0;
    }
    if (!selected) return 0;
    if (input->kind == 1 &&
        !final_register_accepts_width(input->original_register, selected, offset)) return 0;
    if (!requested && selected == 1 && (record->type & 0xfff) == 4 && !offset &&
        constant == (unsigned)(signed char)constant &&
        (input->kind != 1 || input->original_register != (C2FunctionRegister *)&machine_registers[1]))
        return 0;
    *displacement = offset;
    if (!requested) *width = selected;
    return selected == *width;
}
void __fastcall peephole_440fae(C2Expression *record, C2Expression *replacement)
{ __asm { int 3 } }
C2Expression * __fastcall peephole_4464f0(C2Expression *record)
{ __asm { int 3 } return NULL; }
void __fastcall peephole_45856e(C2Expression *record, unsigned width, unsigned displacement)
{ __asm { int 3 } }
void __fastcall peephole_459fcf(C2Expression *record)
{ __asm { int 3 } }
C2Expression * __fastcall peephole_46133c(C2Expression *record)
{ __asm { int 3 } return NULL; }
C2Expression * __fastcall peephole_462dfb(C2Expression *record)
{ __asm { int 3 } return NULL; }
C2Expression * __fastcall peephole_463050(C2Expression *record)
{ __asm { int 3 } return NULL; }
C2Expression * __fastcall peephole_463a2c(C2Expression *record)
{ __asm { int 3 } return NULL; }
void __fastcall peephole_463b53(C2Expression *record)
{ __asm { int 3 } }
void __fastcall peephole_465547(C2Expression *record)
{ __asm { int 3 } }
unsigned __fastcall previous_float_register_number(unsigned number)
{ __asm { int 3 } return 0; }
void __fastcall peephole_47d9b4(C2Function *function)
{ __asm { int 3 } }
void __fastcall peephole_481c20(C2Function *function)
{ __asm { int 3 } }

int __fastcall final_register_accepts_width(C2FunctionRegister *reg,
    unsigned bytes, int mode)
{
    C2FunctionRegister *root;
    unsigned number;
    unsigned char kind;

    if (mode > 1) return 0;
    if (bytes != 1) return 1;
    if (reg == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT))
        kind = machine_register_storage.sentinel.kind;
    else {
        if (reg != (C2FunctionRegister *)&machine_registers[7])
            REQUIRE_REGISTER_KIND(reg);
        kind = reg->kind;
    }
    if (kind != 1) return 1;
    root = reg->canonical;
    REQUIRE_REGISTER_NUMBER(root);
    number = root->number;
    return number < 6 || number > 8;
}
