#include "global-optimizer.h"
#include "optimizer-translate.h"
#include "code-input.h"
#include "inlining.h"
#include "registers.h"
#include <recon.h>

extern int c2_global_optimize, c2_favor_speed;

C2Expression *__fastcall lower_final_record(C2Expression *record);
void __fastcall detach_input_operand(C2Expression *owner, C2Expression *operand);

unsigned __fastcall select_final_copy_opcode(C2Expression *operand);
C2Expression *__fastcall bind_final_call_parameter(C2Expression *record, C2Expression *parameter);
C2Expression *__fastcall copy_final_memory_count(unsigned short type,
    C2Expression *result, C2Expression *source, C2Record *prior);

void __fastcall bind_final_intrinsic_operands(C2Expression *record);
void __fastcall prepare_final_call_parameters(C2Expression *record);
C2Expression *__fastcall lower_final_indirect_call(C2Expression *record);
C2Expression *__fastcall lower_final_intrinsic_record(C2Expression *record);
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);
C2Expression *__fastcall new_numbered_machine_operand(unsigned number, unsigned short type);
C2Expression *__fastcall retype_final_operand(C2Expression *operand, unsigned short type, int mode);
C2Symbol *__fastcall find_runtime_function(char *name);
C2Expression *__fastcall prepare_final_division_operands(unsigned short type,
    C2Expression *definition, C2Expression *temporary, C2Expression **input);
void __fastcall remove_input_operand(C2Expression *owner, C2Expression *operand);
void __fastcall unlink_final_output(C2Expression *owner, C2Expression *operand);

/* 49e290 is an initialized zero DWORD. Its actual direct references only read
 * this mode; no address escapes or writer have been recovered. It is not a
 * pointer or a default for any adjacent state. */
int final_return_address_mode;

C2Expression *__fastcall new_final_intrinsic_result(unsigned opcode, unsigned short type,
    C2Expression *operand, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return 0;
}
unsigned __fastcall final_intrinsic_output_index(C2Expression *record)
{
    C2Expression *operand;

    for (operand = record->tail.right; operand; operand = operand->next)
        if (operand->kind == 10) return operand->value.optimizer_index;
    c2_report_internal("E:\\utc\\src\\\\P2\\misc.c", 0x397);
    return 0;
}
unsigned __fastcall final_intrinsic_input_index(C2Expression *record)
{
    C2Expression *operand;

    for (operand = record->left; operand; operand = operand->next)
        if (operand->kind == 10) return operand->value.optimizer_index;
    c2_report_internal("E:\\utc\\src\\\\P2\\misc.c", 0x3a6);
    return 0;
}
void __fastcall lower_final_memory_intrinsic(C2Expression *record,
    C2Expression **arguments, int width, int reverse);
void __fastcall release_final_memory_argument(C2Expression *argument)
{
    __asm { int 3 }
}
C2Expression *__fastcall prepare_final_memory_count(unsigned opcode, unsigned short type,
    C2Expression *result, C2Expression **input, C2Record *prior)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall normalize_final_memory_source(C2Expression *record,
    C2Expression *source, unsigned width, unsigned bytes, C2MachineRegister *fixed)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall materialize_final_intrinsic_operand(C2Expression *record,
    C2Expression *operand, C2MachineRegister *fixed)
{
    C2FunctionRegister *reg;
    C2Expression *definition, *temporary;

    if ((1U << (operand->kind & 31)) & 6) {
        reg = operand->value.reg;
        definition = reg->value;
        if (definition && definition->opcode == 0x130) {
            if (fixed) return bind_final_call_parameter(record, operand);
            operand->original_register = reg;
            definition->tail.right->original_register = reg;
            definition->opcode = select_final_copy_opcode(definition->left);
            return operand;
        }
    }
    if (fixed && operand->original_register != (C2FunctionRegister *)fixed) {
        /* This consumer needs only the three fixed rows actually passed by
         * the memory-transfer owner. Unknown rows remain valid original
         * inputs but their number field is not available in this model. */
        if (fixed != &machine_registers[2] && fixed != &machine_registers[7] &&
            fixed != &machine_registers[8]) { __asm { int 3 } }
        temporary = new_numbered_machine_operand(fixed->number, 0x1004);
        copy_final_memory_count(0x1004, temporary, operand, (C2Record *)record->previous);
        return temporary;
    }
    return operand;
}
C2Expression *__fastcall materialize_final_intrinsic_destination(C2Expression *record,
    C2Expression *operand, int mode)
{
    C2Expression *result = record->tail.right, *created;

    if (result && result->kind != 10) {
        if (mode) {
            prepare_final_memory_count(1, 0x1004, result, &operand, (C2Record *)record->previous);
            remove_input_result(record, result);
        } else {
            created = new_binary_expression(1, 0x1004, result, operand,
                prepend_input_record, (C2Record *)record);
            result = record->tail.right;
            result->value.reg->value = created;
            remove_input_result(record, result);
        }
    }
    return operand;
}
C2Expression *__fastcall emit_final_memory_operation(unsigned opcode, unsigned short type,
    C2Expression **destination, C2Expression **source, C2Expression **count,
    C2Record *prior, unsigned flags, unsigned output_index, unsigned input_index)
;
/* These memory operands carry the already recovered EDI/ESI/ECX views. A
 * dynamic function register has its real number writer; other static-row
 * number columns remain unavailable to this consumer. */
#define REQUIRE_MEMORY_REGISTER_NUMBER(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8]) { __asm { int 3 } } } while (0)

C2Expression *__fastcall emit_final_memory_operation(unsigned opcode, unsigned short type,
    C2Expression **destination, C2Expression **source, C2Expression **count,
    C2Record *prior, unsigned flags, unsigned output_index, unsigned input_index)
{
    C2Expression *old_destination = *destination, *old_source = *source;
    C2Expression *result, *created, *operand, *fixed_operand;
    C2FunctionRegister *reg;
    unsigned number;

    reg = old_destination->original_register;
    REQUIRE_MEMORY_REGISTER_NUMBER(reg);
    number = reg->number;
    if (flags & (1U << (number & 31)))
        result = new_numbered_machine_operand(number, 0x1004);
    else result = new_fixed_register_operand(8, 0x1004);
    created = new_global_ternary_expression(opcode, type | 0x1000, result,
        old_source, old_destination, insert_input_record, prior);
    if (flags & (1U << (number & 31))) {
        *destination = result;
        reg = result->value.reg;
        if (!reg->canonical->variant_next) reg->value = created;
        else bind_input_temporary(reg, created);
    }
    if (count) {
        operand = *count;
        copy_input_operand(created, operand);
        reg = operand->original_register;
        REQUIRE_MEMORY_REGISTER_NUMBER(reg);
        number = reg->number;
        if (flags & (1U << (number & 31))) {
            fixed_operand = new_numbered_machine_operand(number, 0x1004);
            *count = fixed_operand;
            attach_optimizer_output(created, fixed_operand);
            reg = fixed_operand->value.reg;
            if (!reg->canonical->variant_next) reg->value = created;
            else bind_input_temporary(reg, created);
        } else attach_optimizer_output(created, new_fixed_register_operand(2, 0x1004));
    }
    reg = old_source->original_register;
    REQUIRE_MEMORY_REGISTER_NUMBER(reg);
    number = reg->number;
    if (flags & (1U << (number & 31))) {
        operand = new_numbered_machine_operand(number, 0x1004);
        *source = operand;
        attach_optimizer_output(created, operand);
        reg = operand->value.reg;
        if (!reg->canonical->variant_next) reg->value = created;
        else bind_input_temporary(reg, created);
    } else if (opcode != 0x101 && opcode != 0x100 && opcode != 0x3b && opcode != 0x3a)
        attach_optimizer_output(created, new_fixed_register_operand(7, 0x1004));
    if (output_index)
        attach_optimizer_output(created, new_optimizer_index_operand(output_index));
    if (input_index) copy_input_operand(created, new_optimizer_index_operand(input_index));
    return created;
}
void __fastcall emit_final_memory_unit(C2Expression *record, int reverse,
    C2Expression **source, C2Expression **destination, unsigned short type,
    unsigned displacement, int advance, unsigned output_index, unsigned input_index)
{
    __asm { int 3 }
}
C2Expression *__fastcall emit_final_memory_count_operation(unsigned opcode,
    unsigned short type, C2Expression **result, C2Expression **input, C2Record *prior, int mode)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall new_final_stack_count(C2Record *prior)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall copy_final_memory_count(unsigned short type,
    C2Expression *result, C2Expression *source, C2Record *prior)
{
    __asm { int 3 }
    return 0;
}

C2Expression *__fastcall lower_final_memory_transfer(unsigned bytes,
    C2Expression *count, C2Expression *size, C2Expression *destination,
    C2Expression *source, int unit_count, int aligned_count, int width,
    int reverse, int destination_mode, int source_mode, C2Expression *record,
    int count_shift)
{
    C2Expression *definition, *last, *constant, *temporary;
    C2MachineRegister *fixed;
    unsigned units = bytes, type = width, source_register, fixed_register;
    unsigned output_index = 0, input_index = 0, scalar_opcode, repeat_opcode, flags;
    unsigned displacement = 0, index, stride, shift = (width ^ 3) & 31;
    int fast, short_transfer, advance;
    extern int c2_favor_speed;

    fast = c2_favor_speed && c2_cpu_generation >= 4;
    if (unit_count) {
        units = (int)bytes >> shift;
        type = 4;
    }
    short_transfer = bytes != 0 && (int)units <= (reverse != 0) + 4;
    if (!reverse) {
        source_register = fixed_register = 7;
        scalar_opcode = 0x38;
        repeat_opcode = 0xfe;
        if (c2_global_optimize) {
            output_index = final_intrinsic_output_index(record);
            input_index = final_intrinsic_input_index(record);
        }
    } else {
        source_register = fixed_register = 1;
        scalar_opcode = 0x3b;
        repeat_opcode = 0x101;
        if (c2_global_optimize) output_index = final_intrinsic_output_index(record);
    }
    if (short_transfer && fast) source_register = fixed_register = 0;
    if (reverse && (unit_count || aligned_count)) {
        fixed = source_register ? &machine_registers[source_register] : NULL;
        source = normalize_final_memory_source(record, source, source->type & 0xfff, bytes, fixed);
    }
    definition = NULL;
    if ((1U << (source->kind & 31)) & 6) definition = source->value.reg->value;
    if (!definition || fixed_register || definition->opcode != 0x130 ||
        !((1U << (definition->left->kind & 31)) & 0x18)) {
        fixed = fixed_register ? &machine_registers[fixed_register] : NULL;
        source = materialize_final_intrinsic_operand(record, source, fixed);
    } else {
        source = copy_input_expression(definition->left);
        unlink_and_release_input_record((C2Record *)definition);
    }
    definition = NULL;
    if ((1U << (destination->kind & 31)) & 6) definition = destination->value.reg->value;
    if (!definition || fixed_register || definition->opcode != 0x130 ||
        !((1U << (definition->left->kind & 31)) & 0x18)) {
        fixed = fixed_register ? &machine_registers[8] : NULL;
        destination = materialize_final_intrinsic_operand(record, destination, fixed);
    } else {
        destination = copy_input_expression(definition->left);
        unlink_and_release_input_record((C2Record *)definition);
    }
    destination = materialize_final_intrinsic_destination(record, destination, 1);
    if (bytes && (int)units <= (reverse != 0) + 4) {
        if (source_mode == -1 && !fast)
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\cgintrin.c", 0x3ec);
        unlink_and_release_input_record((C2Record *)count->value.reg->value);
        last = source;
        index = 0;
        if ((int)units > 0) {
            stride = type * (unsigned)source_mode;
            for (; (int)index < (int)units; ++index, displacement += stride) {
                if ((!unit_count || (units << shift) == bytes) && index == units - 1) {
                    advance = 0;
                    flags = 0;
                } else {
                    advance = 1;
                    flags = (1U << (fixed_register & 31)) | 0x100;
                }
                if (!fast)
                    last = emit_final_memory_operation(scalar_opcode, (unsigned short)type,
                        &destination, &source, NULL, (C2Record *)record->previous,
                        flags, output_index, input_index);
                else emit_final_memory_unit(record, reverse, &source, &destination,
                    (unsigned short)type, displacement, advance, output_index, input_index);
            }
        }
        displacement = fast ? index * type : 0;
    } else {
        count = materialize_final_intrinsic_operand(record, count, &machine_registers[2]);
        if (unit_count && !bytes) {
            constant = new_integer_expression(0x1004, (__int64)(width ^ 3));
            emit_final_memory_count_operation(0x2b, 0x1004, &count, &constant,
                (C2Record *)record->previous, 1);
        } else if (count_shift) {
            constant = new_integer_expression(0x1004, (__int64)count_shift);
            emit_final_memory_count_operation(0x2b, 0x1004, &count, &constant,
                (C2Record *)record->previous, 1);
        }
        if (!bytes) {
            if (!unit_count) flags = 0;
            else if (destination_mode && width == 2)
                flags = (1U << (fixed_register & 31)) | 0x104;
            else flags = (1U << (fixed_register & 31)) | 0x100;
        } else if (((int)bytes > 0 || size->kind == 7) &&
                   (!unit_count || (units << shift) == bytes)) flags = 0;
        else flags = (1U << (fixed_register & 31)) | 0x100;
        last = emit_final_memory_operation(repeat_opcode, (unsigned short)type,
            &destination, &source, &count, (C2Record *)record->previous,
            flags, output_index, input_index);
    }
    if (!unit_count) return last;
    if (width == 1) {
        if (!bytes) {
            if (!destination_mode) return last;
            count = new_numbered_machine_operand(2, 0x1004);
            temporary = copy_final_memory_count(0x1004, count, size, (C2Record *)record->previous);
            constant = new_integer_expression(0x1004, 3);
            temporary = emit_final_memory_count_operation(0x2e, 0x1004, &count,
                &constant, (C2Record *)temporary, 1);
            return emit_final_memory_operation(repeat_opcode, 1, &destination, &source,
                &count, (C2Record *)temporary, 0, output_index, input_index);
        }
        if (bytes & 2) {
            if (!fast || !short_transfer) {
                flags = bytes & 1 ? (1U << (fixed_register & 31)) | 0x100 : 0;
                last = emit_final_memory_operation(scalar_opcode, 2, &destination, &source,
                    NULL, (C2Record *)record->previous, flags, output_index, input_index);
            } else {
                emit_final_memory_unit(record, reverse, &source, &destination, 0x1002,
                    displacement, bytes & 1, output_index, input_index);
                displacement += 2;
            }
        }
        if (!(bytes & 1)) return last;
        if (fast && short_transfer) {
            emit_final_memory_unit(record, reverse, &source, &destination, 1,
                displacement, 0, output_index, input_index);
            return last;
        }
        type = 1;
    } else {
        if (!bytes) {
            if (!destination_mode) return last;
            constant = new_final_stack_count((C2Record *)last->previous);
            emit_final_memory_count_operation(0x107, 0x1004, &count, &constant,
                (C2Record *)record->previous, 1);
            return emit_final_memory_operation(repeat_opcode, 2, &destination, &source,
                &count, (C2Record *)record->previous, 0, output_index, input_index);
        }
        if (!(bytes & 1)) return last;
        if (fast && short_transfer) {
            emit_final_memory_unit(record, reverse, &source, &destination, 2,
                displacement, 0, output_index, input_index);
            return last;
        }
        type = 2;
    }
    return emit_final_memory_operation(scalar_opcode, (unsigned short)type,
        &destination, &source, NULL, (C2Record *)record->previous, 0, output_index, input_index);
}
C2Expression *__fastcall split_final_wide_result(C2Expression *record,
    C2Expression *result, C2Expression **replacement);
int __fastcall register_views_differ(C2Expression *operand);
void __fastcall propagate_final_result_register(C2Expression *record);
C2Expression *__fastcall materialize_final_address_input(unsigned mode,
    C2Expression *owner, C2Expression *operand, C2MachineRegister *reg);
C2Expression *__fastcall find_final_register_output(C2Expression *record,
    C2FunctionRegister *fixed);
C2Expression *__fastcall final_record_uses_machine_register(C2Expression *record,
    C2MachineRegister *fixed);
int __fastcall final_parameter_needs_copy(C2Expression *definition,
    C2Expression *record, C2FunctionRegister *fixed)
{
    C2Expression *scan;

    for (scan = definition->next; scan != record; scan = scan->next)
        if ((scan->flags & 1) &&
            (find_final_register_output(scan, fixed) ||
             final_record_uses_machine_register(scan, (C2MachineRegister *)fixed))) break;
    return scan != record;
}
/* Same encountered494400 bit0x40 column and bounded opcode domain as the
 * ordinary final-lowering consumer; no other WORD bits or data are read. */
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

C2Expression *__fastcall bind_final_parameter_register(C2Expression *record,
    C2Expression *parameter)
{
    C2Expression *definition = parameter->value.reg->value;
    C2Expression *result = definition->tail.right, *replacement;
    C2FunctionRegister *fixed;
    unsigned opcode;

    fixed = new_bitfield_register(parameter->original_register,
        (result->type & 0xfff) << 3, 0);
    if ((!register_views_differ(result) || result->original_register == fixed) &&
        !final_parameter_needs_copy(definition, record, fixed)) {
        result->original_register = fixed;
        opcode = definition->opcode;
        if (FINAL_DESTRUCTIVE_OPCODE(opcode)) propagate_final_result_register(definition);
        return parameter;
    }
    result->original_register = result->value.reg;
    replacement = materialize_final_address_input(1, record, result, (C2MachineRegister *)fixed);
    attach_input_expression(record, parameter, replacement);
    return replacement;
}
C2Expression *__fastcall bind_final_call_parameter(C2Expression *record, C2Expression *parameter)
{
    C2Expression *definition = parameter->value.reg->value;
    C2Expression *input = definition->left, *result, *high;
    unsigned width = parameter->type & 0xfff;

    if (width == 2 &&
        (((input->kind == 1 || input->kind == 2) && final_operand_fits_word(input)) ||
         (input->kind == 7 && c2_favor_speed))) {
        rewrite_final_record_width(definition, 4);
        width = 4;
    }
    result = bind_final_parameter_register(record, parameter);
    definition->opcode = select_final_copy_opcode(definition->left);
    if (width == 8) {
        high = split_final_wide_result(record, result, NULL);
        copy_input_operand(record, high);
        bind_final_parameter_register(record, high);
    }
    return result;
}

void __fastcall lower_final_memory_intrinsic(C2Expression *record,
    C2Expression **arguments, int width, int reverse)
{
    C2Expression *counter = arguments[2], *count = counter;
    C2Expression *size = counter->value.reg->value->left;
    C2Expression *first = arguments[0], *definition, *base, *address, *replacement;
    C2Expression *destination, *source;
    unsigned bytes = 0, shift;
    int unit_count = 0, aligned_count = 0, count_shift = 0;

    if (size->kind == 7) {
        bytes = size->integer.low;
        if (!bytes) {
            release_final_memory_argument(arguments[1]);
            release_final_memory_argument(counter);
            definition = arguments[0]->value.reg->value;
            destination = record->tail.right;
            if (!destination || destination->kind == 10)
                release_final_memory_argument(arguments[0]);
            else {
                definition->opcode = 1;
                replace_input_result(definition, definition->tail.right, destination);
                destination = definition->tail.right;
                if (((1U << (destination->kind & 31)) & 6) && destination->value.reg->value)
                    destination->value.reg->value = definition;
            }
            return;
        }
    }
    if (reverse && width == 1) {
        source = arguments[1];
        definition = source->value.reg->value;
        if ((definition->type & 0xfff) != 1) rewrite_final_record_width(definition, 1);
        retype_final_operand(source, 0x1001, 0);
    }
    if (width != 4 && !record->intrinsic_call_flags) {
        if (size->opcode == 0x11d) {
            unit_count = bytes != 0;
            shift = (unsigned char)(width ^ 3);
            if (shift >= 64) size->integer.value = (__int64)size->integer.value < 0 ?
                (unsigned __int64)-1 : 0;
            else size->integer.value = (__int64)size->integer.value >> shift;
        } else {
            definition = NULL;
            if ((1U << (size->kind & 31)) & 6) definition = size->value.reg->value;
            base = address = NULL;
            if (definition && definition->opcode == 1) {
                source = definition->left;
                if ((1U << (source->kind & 31)) & 6) {
                    base = source->value.reg->value;
                    if (base && base->opcode == 0x12) {
                        address = base->left;
                        if (address->opcode != 0x125 ||
                            (address->location.expression.properties & 15) !=
                                (4 >> ((unsigned)width & 31)) || address->detail.data)
                            address = NULL;
                    }
                }
            }
            if (address) {
                source = address->aggregate_second;
                aligned_count = 1;
                base->opcode = 1;
                width = 4;
                replacement = new_temporary_register_expression(0x11f, source->type,
                    source->value.reg, source->original_register);
                attach_input_expression(base, address, replacement);
            } else if (definition && definition->opcode == 0x2e &&
                       definition->left->next->kind == 7 &&
                       !(definition->left->next->integer.low & 3)) {
                aligned_count = 1;
                count_shift = width ^ 3;
                width = 4;
            } else {
                count = bind_final_call_parameter(record, counter);
                if (width == 1) {
                    size = new_temporary_operand(0x1004);
                    prepare_final_memory_count(1, 0x1004, size, &count, (C2Record *)record->previous);
                }
                unit_count = 1;
            }
        }
    }
    lower_final_memory_transfer(bytes, count, size, first, arguments[1], unit_count,
        aligned_count, width, reverse, 1, 1, record, count_shift);
}
C2Expression *__fastcall make_final_result_temporary(C2Expression *record,
    C2FunctionRegister *reg, int bind)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall lower_final_string_intrinsic(C2Expression *record,
    C2Expression *argument, C2Expression **result, int mode, unsigned short type)
{
    __asm { int 3 }
    return 0;
}
void __fastcall lower_final_intrinsic_a3(C2Expression *record,
    C2Expression **arguments, int mode)
{
    __asm { int 3 }
}
void __fastcall release_final_intrinsic_definitions(C2Expression *record)
{
    __asm { int 3 }
}
void __fastcall lower_final_intrinsic_a6(C2Expression *record, C2Expression **arguments)
{
    __asm { int 3 }
}
void __fastcall lower_final_intrinsic_a5(C2Expression *record, C2Expression **arguments)
{
    __asm { int 3 }
}
C2Expression *__fastcall prepare_final_alloca_intrinsic(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall prepare_intrinsic_runtime_call(C2Expression *record,
    unsigned opcode, C2Expression *target, unsigned char mode);
C2Expression *__fastcall new_register_set_expression(C2RegisterSet *set);
C2Expression *__fastcall lower_intrinsic_runtime_call(C2Expression *record,
    unsigned opcode, C2Expression *target, unsigned char mode)
{
    C2Expression *call;
    C2RegisterSet *clobbered;
    unsigned number;

    call = prepare_intrinsic_runtime_call(record, opcode, target, mode);
    clobbered = new_register_set(8);
    for (number = 0; number <= 8; ++number)
        if (!find_final_register_output(call, (C2FunctionRegister *)&machine_registers[number]) &&
            (call_clobbered_register_mask & (1U << number)))
            add_register_to_set(clobbered, number);
    if (clobbered->links.first)
        attach_optimizer_output(call, new_register_set_expression(clobbered));
    else release_optimizer_symbol_set(clobbered);
    return call;
}
void __fastcall replace_peephole_record(C2Expression *old_record, C2Expression *replacement);
C2Expression *__fastcall prepare_intrinsic_runtime_call(C2Expression *record,
    unsigned opcode, C2Expression *target, unsigned char mode)
{
    C2Expression *call, *output;
    C2FunctionRegister *reg;

    call = (C2Expression *)allocate_small_node(13);
    call->call_convention = mode;
    call->flags |= 1;
    call->opcode = opcode;
    call->type = record->type;
    if ((1U << (target->kind & 31)) & 0x7fe) {
        if (target->location.expression.properties & 0x10)
            target = copy_input_expression(target);
        target->location.expression.properties |= 0x10;
    }
    call->left = target;
    retain_aggregate_operands(call);
    call->location.line = record_line_delta;
    call->left->next = record->left;
    call->tail.right = record->tail.right;
    record->left = NULL;
    record->tail.right = NULL;
    for (output = call->tail.right; output; output = output->next) {
        if (((1U << (output->kind & 31)) & 6) && output->value.reg->value) {
            reg = output->value.reg;
            if (!reg->canonical->variant_next) reg->value = call;
            else bind_input_temporary(reg, call);
        }
    }
    replace_peephole_record(record, call);
    return call;
}
unsigned __fastcall lookup_final_intrinsic_opcode(unsigned intrinsic)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall lower_final_intrinsic_d8(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}
void __fastcall lower_final_intrinsic_aa(C2Expression *record)
{
    __asm { int 3 }
}
C2Expression *__fastcall prepare_setjmp_frame_arguments(C2Expression *record,
    C2Expression **first, C2Expression **second)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall lower_final_intrinsic_152(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall prepare_setjmp_scope_arguments(C2Expression *record,
    C2Expression **first, C2Expression **second, C2Expression **third)
{
    __asm { int 3 }
    return 0;
}

C2Expression *__fastcall lower_final_condition_record(C2Expression *record)
{
    C2Expression *arguments[5], **argument_slot, *operand, *head, *definition;
    C2Expression *created, *result, *temporary, *first, *second, *last;
    C2Expression *scope_first, *scope_second, *scope_third;
    C2FunctionRegister *reg;
    C2Symbol *symbol;
    C2InputSymbol *local;
    C2FinalStackLink *stack_link;
    unsigned intrinsic, opcode, bytes;
    unsigned short type;
    int value;

    if (record->detail.intrinsic == 0xa5) {
        operand = record->left->next->value.reg->value->left;
        if (((1U << (operand->kind & 31)) & 0x18) &&
            operand->value.reg->symbol && operand->value.reg->symbol->source_kind == 0x25) {
            value = operand->value.reg->symbol->value;
            created = new_assignment_expression(0x130, 0x1004,
                new_integer_expression(0x1004, (__int64)value),
                prepend_input_record, (C2Record *)record);
            record->detail.intrinsic = 0xaa;
            head = record->left->next;
            if (head->next) {
                temporary = copy_input_expression(created->tail.right);
                temporary->location.expression.properties |= 0x10;
                temporary->next = head->next;
                head->next = temporary;
            } else copy_input_operand(record, created->tail.right);
        }
    }
    bind_final_intrinsic_operands(record);
    prepare_final_call_parameters(record);
    head = record->left;
    /* The original has five local operand slots, filled up to kind10. Its
     * intrinsic arities, rather than a general unbounded list, govern them. */
    argument_slot = arguments;
    for (operand = head; operand && operand->kind != 10; operand = operand->next)
        *argument_slot++ = operand;
    intrinsic = record->detail.intrinsic;
    switch (intrinsic) {
    case 0x96: case 0x97:
        lower_final_memory_intrinsic(record, arguments, record->detail.intrinsic_count,
            intrinsic == 0x97);
        break;
    case 0xac: case 0xad:
        lower_final_memory_intrinsic(record, arguments, 1, intrinsic == 0xad);
        break;
    case 0x9f: case 0xa0: case 0xa1: case 0xa2:
        opcode = lookup_final_intrinsic_opcode(intrinsic);
        created = new_global_ternary_expression(opcode, record->type, record->tail.right,
            arguments[0]->value.reg->value->left, arguments[1]->value.reg->value->left,
            insert_input_record, (C2Record *)record);
        reg = record->tail.right->value.reg;
        if (!reg->canonical->variant_next) reg->value = created;
        else bind_input_temporary(reg, created);
        goto release_definitions;
    case 0xa3: case 0xa4:
        lower_final_intrinsic_a3(record, arguments, intrinsic == 0xa3);
        break;
    case 0xa5: lower_final_intrinsic_a5(record, arguments); break;
    case 0xa6: lower_final_intrinsic_a6(record, arguments); break;
    case 0xa7:
        temporary = lower_final_string_intrinsic(record, arguments[0], &last, 0, 0x1004);
        created = new_binary_expression(1, record->type, record->tail.right, temporary,
            prepend_input_record, (C2Record *)record);
        result = record->tail.right;
        if (((1U << (result->kind & 31)) & 6) && result->value.reg->value) {
            reg = result->value.reg;
            if (!reg->canonical->variant_next) reg->value = created;
            else bind_input_temporary(reg, created);
        }
        break;
    case 0xa8: case 0xaf: case 0xe0:
        opcode = lookup_final_intrinsic_opcode(intrinsic);
        result = record->tail.right;
        if (!result) {
            type = record->type;
            first = retype_final_operand(arguments[0]->value.reg->value->left, 0x1002, 0);
            reg = new_bitfield_register((C2FunctionRegister *)&machine_registers[1],
                (type & 0xfff) << 3, 0);
            new_binary_expression(opcode, type, new_fixed_register_operand(reg->number, type),
                first, insert_input_record, (C2Record *)record);
        } else {
            first = retype_final_operand(arguments[0]->value.reg->value->left, 0x1002, 0);
            created = new_binary_expression(opcode, record->type, result, first,
                insert_input_record, (C2Record *)record);
            reg = result->value.reg;
            if (!reg->canonical->variant_next) reg->value = created;
            else bind_input_temporary(reg, created);
        }
        goto release_definitions;
    case 0xa9: case 0xb0: case 0xe1:
        opcode = lookup_final_intrinsic_opcode(intrinsic);
        second = retype_final_operand(arguments[1]->value.reg->value->left,
            arguments[1]->type, 0);
        first = retype_final_operand(arguments[0]->value.reg->value->left, 0x1002, 0);
        created = new_global_ternary_expression(opcode, arguments[1]->type,
            arguments[0]->value.reg->value->tail.right, first, second,
            insert_input_record, (C2Record *)record);
        release_small_node((C2SmallNode *)created->tail.right);
        created->tail.right = NULL;
        result = record->tail.right;
        if (result) {
            first = created->left->next;
            if (((1U << (first->kind & 31)) & 6) && first->value.reg->value) {
                first = new_temporary_operand(first->type);
                first->value.reg->value = created;
                created->tail.right = first;
                first->location.expression.properties |= 0x10;
                first->original_register = new_bitfield_register(
                    (C2FunctionRegister *)&machine_registers[1], (first->type & 0xfff) << 3, 0);
            }
            if ((first->type ^ result->type) & 0xfff) {
                created = new_binary_expression(0x135, result->type,
                    new_temporary_operand(result->type), first, insert_input_record,
                    (C2Record *)record);
                first = created->tail.right;
                first->value.reg->value = created;
            }
            created = new_binary_expression(1, result->type, result, first,
                insert_input_record, (C2Record *)record);
            reg = result->value.reg;
            if (!reg->canonical->variant_next) reg->value = created;
            else bind_input_temporary(reg, created);
        }
        goto release_definitions;
    case 0xaa: lower_final_intrinsic_aa(record); break;
    case 0xb1: case 0xb2:
        first = new_fixed_register_operand(66, 0x8000);
        second = new_fixed_register_operand(66, 0x8000);
        created = new_binary_expression(intrinsic == 0xb1 ? 0xb2 : 0x9c, 0x8000,
            second, first, insert_input_record, (C2Record *)record);
        remove_input_operand(created, created->left);
        goto release_definitions;
    case 0xd7: case 0xd9: case 0xda: case 0x14e: case 0x14f: case 0x151:
        return lower_final_intrinsic_record(record);
    case 0xd8: return lower_final_intrinsic_d8(record);
    case 0xdb:
        if (current_function->symbol->symbol.flags & 0x1000) {
            last = prepare_setjmp_frame_arguments(arguments[0]->value.reg->value->previous,
                &scope_first, &scope_second);
            temporary = new_integer_expression(0x1004, 2);
            created = new_assignment_expression(0x130, 0x1004, temporary,
                insert_input_record, (C2Record *)last);
        } else if ((current_function->flags & 0x8000) ||
                   (current_function->symbol->additional_flags & 0x200)) {
            last = prepare_setjmp_scope_arguments(arguments[0]->value.reg->value->previous,
                &scope_first, &scope_second, &scope_third);
            temporary = new_integer_expression(0x1004, 3);
            created = new_assignment_expression(0x130, 0x1004, temporary,
                insert_input_record, (C2Record *)last);
        } else created = new_assignment_expression(0x130, 0x1004,
            new_integer_expression(0x1004, 0), prepend_input_record, (C2Record *)record);
        temporary = copy_input_expression(created->tail.right);
        temporary->next = head;
        record->left = temporary;
        lower_final_record(created);
        symbol = find_runtime_function("__setjmp3");
        ((C2InputSymbol *)symbol)->attributes &= ~0x700;
        temporary = variable_input_expression((C2InputSymbol *)symbol);
        if (current_function->symbol->symbol.flags & 0x1000)
            copy_input_operand(record, scope_first);
        else if ((current_function->flags & 0x8000) ||
                 (current_function->symbol->additional_flags & 0x200)) {
            copy_input_operand(record, scope_first);
            copy_input_operand(record, scope_second);
            scope_second = scope_third;
        }
        if ((current_function->symbol->symbol.flags & 0x1000) ||
            (current_function->flags & 0x8000) ||
            (current_function->symbol->additional_flags & 0x200))
            copy_input_operand(record, scope_second);
        if (current_function->symbol->symbol.flags & 0x1000)
            lower_final_intrinsic_record(record);
        return lower_intrinsic_runtime_call(record, 14, temporary, 0);
    case 0xde:
        last = prepare_final_alloca_intrinsic(record);
        bytes = (arguments[0]->value.reg->value->left->immediate + stack_alignment - 1) &
            ~(stack_alignment - 1);
        temporary = new_integer_expression(0x1004, bytes);
        first = new_fixed_register_operand(5, 0x1004);
        second = new_fixed_register_operand(5, 0x1004);
        created = new_global_ternary_expression(0x32, 0x1004, second, first, temporary,
            prepend_input_record, (C2Record *)last);
        for (stack_link = function_finish_state;
             stack_link && stack_link->record != record; stack_link = stack_link->next) {}
        /* The original also dereferences a missing link; no invented fallback. */
        stack_link->record = created;
        result = record->tail.right;
        record->tail.right = NULL;
        result->location.expression.properties &= ~0x10;
        created = new_binary_expression(1, 0x1004, result,
            new_fixed_register_operand(5, 0x1004), insert_input_record, (C2Record *)record);
        result->value.reg->value = created;
        if (record->left->next) {
            temporary = arguments[1]->value.reg->value->left;
            if (((1U << (temporary->kind & 31)) & 6) && temporary->value.reg->value) {
                definition = temporary->value.reg->value;
                temporary = definition->left;
                detach_input_operand(definition, temporary);
                unlink_and_release_input_record((C2Record *)definition);
            }
            new_binary_expression(1, 0x1004, temporary,
                new_fixed_register_operand(5, 0x1004), insert_input_record, (C2Record *)record);
        }
        goto release_definitions;
    case 0xdf:
        definition = follow_optimizer_result(record, &first);
        created = follow_optimizer_result(definition, &second);
        attach_input_expression(created, second, record->tail.right);
        unlink_and_release_input_record((C2Record *)definition);
        definition = arguments[0]->value.reg->value;
        first = definition->left;
        created = NULL;
        if ((1U << (first->kind & 31)) & 6) created = first->value.reg->value;
        if (!created && ((1U << (first->kind & 31)) & 6) &&
            first->value.reg->kind == 3 && !first->value.reg->value &&
            (first->value.reg->flags & 0x40)) {
            created = resolve_inline_argument(definition, first, &scope_third);
            first = created->left;
            if (((1U << (first->kind & 31)) & 6) && first->value.reg->value &&
                first->original_register == first->value.reg) {
                last = first->value.reg->value;
                unlink_and_release_input_record((C2Record *)created);
                created = last;
            }
        }
        if (created) {
            first = created->left;
            if (((1U << (first->kind & 31)) & 6) && first->value.reg->value) {
                last = first->value.reg->value;
                unlink_and_release_input_record((C2Record *)created);
                replace_input_result(last, last->tail.right, new_fixed_register_operand(1, 0x1004));
            } else unlink_and_release_input_record((C2Record *)created);
        }
        bytes = (arguments[1]->value.reg->value->left->immediate + stack_alignment - 1) &
            ~(stack_alignment - 1);
        temporary = new_integer_expression(0x1004, bytes);
        release_final_intrinsic_definitions(record);
        release_small_node((C2SmallNode *)arguments[0]);
        release_small_node((C2SmallNode *)arguments[1]);
        record->left = temporary;
        return record;
    case 0xe5:
        local = new_inline_result_symbol(input_type_alignment(record->type),
            record->type & 0xfff, 3, 1, 1, 0);
        local->symbol.name = "__$ReturnAddr";
        local->symbol.parameter_offset = 4;
        temporary = input_symbol_operand(local);
        created = new_binary_expression(1, record->tail.right->type, record->tail.right,
            temporary, insert_input_record, (C2Record *)record);
        result = record->tail.right;
        if (result->kind == 1 && result->value.reg->kind == 3 && result->value.reg->value) {
            reg = result->value.reg;
            if (!reg->canonical->variant_next) reg->value = created;
            else bind_input_temporary(reg, created);
        }
        record->opcode = 0x11c;
        current_function->flags |= 0x80000;
        return record;
    case 0x11b: case 0x11c: case 0x11d: case 0x13e: case 0x13f: case 0x14b:
        current_function->flags |= 0x10000;
        temporary = new_integer_expression(0x1004, intrinsic);
        created = new_final_intrinsic_result(0x13c, record->type, temporary,
            prepend_input_record, (C2Record *)record);
        created->location.attributes = record->location.attributes;
        result = record->tail.right;
        if (result && ((1U << (result->kind & 31)) & 6) && result->value.reg->value) {
            unlink_final_output(record, result);
            attach_optimizer_output(created, result);
            reg = result->value.reg;
            if (!reg->canonical->variant_next) reg->value = created;
            else bind_input_temporary(reg, created);
        }
        detach_input_operand(record, arguments[0]);
        copy_input_operand(created, arguments[0]);
        definition = arguments[0]->value.reg->value;
        definition->opcode = 1;
        remove_input_result(definition, definition->tail.right);
        remove_input_operand(definition, definition->left);
        attach_optimizer_output(definition, arguments[0]);
        if (intrinsic == 0x11b || intrinsic == 0x11c) {
            value = intrinsic == 0x11b ? 1 : -1;
            last = new_assignment_expression(1, 0x1004,
                new_integer_expression(0x1004, (__int64)value),
                prepend_input_record, (C2Record *)created);
            copy_input_operand(created, last->tail.right);
        } else {
            detach_input_operand(record, arguments[1]);
            copy_input_operand(created, arguments[1]);
            definition = arguments[1]->value.reg->value;
            definition->opcode = 1;
            remove_input_result(definition, definition->tail.right);
            remove_input_operand(definition, definition->left);
            attach_optimizer_output(definition, arguments[1]);
            if (intrinsic == 0x13f) {
                detach_input_operand(record, arguments[2]);
                copy_input_operand(created, arguments[2]);
                definition = arguments[2]->value.reg->value;
                definition->opcode = 1;
                remove_input_result(definition, definition->tail.right);
                remove_input_operand(definition, definition->left);
                attach_optimizer_output(definition, arguments[2]);
                arguments[2]->original_register = (C2FunctionRegister *)&machine_registers[1];
                arguments[2]->value.reg->value->tail.right->original_register =
                    (C2FunctionRegister *)&machine_registers[1];
            } else if (intrinsic == 0x11d && final_return_address_mode) {
                last = new_input_expression(0x16c, 0x1004);
                prepend_input_record((C2Record *)created, (C2Record *)last);
                temporary = new_temporary_operand(0x1004);
                temporary->original_register = (C2FunctionRegister *)&machine_registers[1];
                temporary->value.reg->value = last;
                attach_optimizer_output(last, temporary);
                copy_input_operand(created, temporary);
            }
        }
        if (c2_global_optimize) {
            attach_optimizer_output(created, new_optimizer_index_operand(final_intrinsic_output_index(record)));
            copy_input_operand(created, new_optimizer_index_operand(final_intrinsic_input_index(record)));
        }
        break;
    case 0x152: return lower_final_intrinsic_152(record);
    case 0x157:
        definition = follow_optimizer_result(record, &first);
        created = follow_optimizer_result(definition, &second);
        result = record->tail.right;
        attach_input_expression(created, second, result);
        replace_input_result(record, result, definition->tail.right);
        bind_input_temporary(record->tail.right->value.reg, record);
        unlink_and_release_input_record((C2Record *)definition);
        lower_final_indirect_call(record);
        return record;
    case 0xd: case 0xe: case 0xf:
        type = 0x1000 | (1U << (intrinsic - 0xd));
        first = record->previous->left;
        temporary = new_temporary_operand(type);
        last = prepare_final_division_operands(type, record->previous, temporary, &first);
        definition = new_folded_binary_expression(0x34, type, first, temporary,
            insert_input_record, (C2Record *)last);
        second = make_final_result_temporary(definition, temporary->value.reg, 1);
        result = record->tail.right;
        created = new_global_ternary_expression(0x32, type, result, definition->tail.right,
            second, insert_input_record, (C2Record *)definition);
        if (((1U << (result->kind & 31)) & 6) && result->value.reg->value) {
            reg = result->value.reg;
            if (!reg->canonical->variant_next) reg->value = created;
            else bind_input_temporary(reg, created);
        }
        result->original_register = (C2FunctionRegister *)&machine_registers[1];
        release_final_intrinsic_definitions(record);
        record->opcode = 0x11c;
        return last;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\x86\\cgintrin.c", 0x20c);
        break;
    }
    record->opcode = 0x11c;
    return record;

release_definitions:
    release_final_intrinsic_definitions(record);
    record->opcode = 0x11c;
    return record;
}
