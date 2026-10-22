#include "value-selection.h"
#include "extended.h"
#include "expression-fields.h"
#include "tree-rewrite.h"
#include "algebra.h"
#include "work-prepare.h"
extern int optimize_precision, cpu_model, code_pointer_bytes;
SymbolHeader *__fastcall require_external_symbol(char *name);
MachineOperand operand_st1 = { OPERAND_REGISTER, { 15 } }; /* 00080058 */
/* Operator descriptions and pop-form x87 operations, original 00080170.
   The null description terminates lookup; names are ordinary table literals. */
static const FloatingOpcode floating_arithmetic_ops[] = {
    {2, "fadd", MC_FADDP}, {3, "fsubr", MC_FSUBP}, {4, "fmul", MC_FMULP},
    {5, "fdiv", MC_FDIVP}, {0xb2, "fdivr", MC_FDIVRP}, {0xb1, "fsubr", MC_FSUBRP},
    {0, 0, 0}
};
/* Original [00019d00,00019d80). Integer leaves become immediate moves;
   real constants obtain a symbol in the constant pool and load the x87 stack. */
void __fastcall select_constant(Node *node, unsigned short reg)
{
    MachineOperand *destination, *source;
    SymbolHeader *symbol;
    if (node->attributes.words.type & 0x1000) {
        symbol = floating_constant_symbol(node);
        source = new_operand(OPERAND_ABSOLUTE, 0, 0, symbol);
        emit_machine_operation(MC_FLD, node->attributes.words.type, &operand_st0, source);
    } else {
        destination = operand_for_register(reg);
        source = new_operand(OPERAND_IMMEDIATE, 0, node->left.value, 0);
        emit_machine_operation(MC_MOV, node->attributes.words.type, destination, source);
    }
}

/* Original [00017e20,00019330), including the 13- and 56-entry dispatch
   tables. Operand temporaries belong to this evaluation; the result register
   survives restoration of the shared operand cursor. */
unsigned short __fastcall select_value_register(Node *node)
{
    int saved_operands = temporary_operand_count;
    unsigned opcode = node->opcode;
    unsigned short type = node->attributes.words.type, result = 0xffff, reg, value_reg;
    unsigned short source_type, source_bytes, target_bytes;
    unsigned operation, value, mask, bytes, argument_bytes, register_mask, result_flags, condition;
    int argument_count, index;
    Node *left_node, *right_node, *argument;
    MachineOperand *left, *right, *destination, *source, *high_left, *high_right;
    MachineOperand argument_operand;
    MachineOperand *argument_values[4];
    unsigned short argument_registers[4];
    MachineInstruction *instruction;
    SymbolHeader *temporary;
    LabelSymbol *label;
    const OpcodeMap *map;
    if (x87_stack_depth) spill_x87_for_node(node);
    switch (opcode) {
    case 2: case 3: case 0xb: case 0xc: case 0xd: case 0xb1: case 0xb2:
arithmetic:
        reg = select_value_register(node->left.node);
        left = operand_for_register(reg);
        reg = select_value_register(node->second.node);
        right = operand_for_register(reg);
        if (!(type & 0xf00)) {
            arrange_x87_left(node->left.node);
            operation = lookup_floating_opcode(opcode, floating_arithmetic_ops);
            emit_machine_operation(operation, type, &operand_st1, &operand_st0);
        } else {
            operation = lookup_opcode(signed_arithmetic_ops, opcode);
            emit_machine_operation(operation, type, left, right);
        }
        result = left->value.reg.words.number;
        break;
    case 4:
        if (!(type & 0xf00) || (type & 255) != 1) goto arithmetic;
        reg = select_value_register(node->left.node);
        left = operand_for_register(reg);
        reg = select_value_register(node->second.node);
        right = operand_for_register(reg);
        emit_machine_operation(MC_MOV, type, &operand_eax, left);
        emit_machine_operation(MC_IMUL, type, right);
        result = new_value_register(type);
        destination = operand_for_register(result);
        emit_machine_operation(MC_MOV, type, destination, &operand_eax);
        result = destination->value.reg.words.number;
        break;
    case 5: case 6:
        right_node = node->second.node;
        if ((type & 0xf00) && (type & 255) > 1 && right_node->opcode == 0x33) {
            value = right_node->left.value;
            mask = value - 1;
            if (value && !(value & mask)) {
                reg = select_value_register(node->left.node);
                left = operand_for_register(reg);
                if ((type & 255) == 2) emit_machine_operation(MC_MOVSXW, 0x104, &operand_eax, left);
                else emit_machine_operation(MC_MOV, type, &operand_eax, left);
                emit_machine_operation(MC_CDQ, 0);
                if (opcode == 5) {
                    if (value == 2) {
                        emit_machine_operation(MC_SUB, 0x104, &operand_eax, &operand_edx);
                        right = immediate_operand(1);
                    } else {
                        right = immediate_operand(mask);
                        emit_machine_operation(MC_AND, 0x104, &operand_edx, right);
                        emit_machine_operation(MC_ADD, 0x104, &operand_eax, &operand_edx);
                        right = immediate_operand(integer_log2(value));
                    }
                    emit_machine_operation(MC_SAR, 0x104, &operand_eax, right);
                } else {
                    emit_machine_operation(MC_XOR, 0x104, &operand_eax, &operand_edx);
                    emit_machine_operation(MC_SUB, 0x104, &operand_eax, &operand_edx);
                    right = immediate_operand(mask);
                    emit_machine_operation(MC_AND, 0x104, &operand_eax, right);
                    emit_machine_operation(MC_XOR, 0x104, &operand_eax, &operand_edx);
                    emit_machine_operation(MC_SUB, 0x104, &operand_eax, &operand_edx);
                }
                result = new_value_register(type);
                destination = operand_for_register(result);
                emit_machine_operation(MC_MOV, type, destination, &operand_eax);
                result = destination->value.reg.words.number;
                break;
            }
        }
        reg = select_value_register(node->left.node);
        left = operand_for_register(reg);
        reg = select_value_register(right_node);
        right = operand_for_register(reg);
        if (!(type & 0xf00)) {
            arrange_x87_left(node->left.node);
            emit_machine_operation(MC_FDIVP, type, &operand_st1, &operand_st0);
            result = left->value.reg.words.number;
            break;
        }
        emit_machine_operation(MC_MOV, type, &operand_eax, left);
        if (type == 0x101) emit_machine_operation(MC_CBW, 0);
        else if (!(type & 0x100)) {
            if (type == 0x201) {
                instruction = emit_machine_operation(MC_SUB, type, &operand_eax, &operand_eax);
                instruction->opcode |= 0x180000;
            } else emit_machine_operation(MC_SUB, type, &operand_edx, &operand_edx);
        } else emit_machine_operation((type & 255) == 2 ? MC_CWD : MC_CDQ, 0);
        emit_machine_operation((type & 0x200) ? MC_DIV : MC_IDIV, type, right);
        result = new_value_register(type);
        destination = operand_for_register(result);
        if (opcode == 5) emit_machine_operation(MC_MOV, type, destination, &operand_eax);
        else if ((type & 255) == 1) {
            instruction = emit_machine_operation(MC_MOV, type, destination, &operand_eax);
            instruction->opcode |= 0x100000;
        } else emit_machine_operation(MC_MOV, type, destination, &operand_edx);
        result = destination->value.reg.words.number;
        break;
    case 7:
        if (type & 0x1000) break;
        reg = select_value_register(node->left.node);
        bytes = node->attributes.words.type & 255;
        right = 0;
        if (bytes == 1 || bytes == 2) {
            value_reg = new_value_register(0x104);
            right = operand_for_register(value_reg);
            left = operand_for_register(reg);
            emit_machine_operation(bytes == 1 ? MC_MOVSX : MC_MOVSXW, 0x104, right, left);
        } else if (bytes == 4) right = operand_for_register(reg);
        emit_machine_operation(MC_PUSH, 0x104, right);
        reg = select_value_register(node->second.node);
        bytes = node->attributes.words.type & 255;
        if (bytes == 1 || bytes == 2) {
            value_reg = new_value_register(0x104);
            right = operand_for_register(value_reg);
            left = operand_for_register(reg);
            emit_machine_operation(bytes == 1 ? MC_MOVSX : MC_MOVSXW, 0x104, right, left);
        } else if (bytes == 4) right = operand_for_register(reg);
        emit_machine_operation(MC_PUSH, 0x104, right);
        temporary = require_external_symbol("___FHfexpl@8");
        right = new_operand(OPERAND_SYMBOL, 0, 0, temporary);
        emit_call(code_pointer_bytes, right, 8, 0, 1);
        result = new_value_register(node->attributes.words.type);
        destination = operand_for_register(result);
        emit_machine_operation(MC_MOV, type, destination, &operand_eax);
        break;
    case 8:
        if (type & 0x1000) {
            reg = select_value_register(node->left.node);
            left = operand_for_register(reg);
            emit_machine_operation(MC_FCHS, 0);
            result = left->value.reg.words.number;
            break;
        }
        /* Fall through to the integer unary operation. */
    case 0xe:
        operation = lookup_opcode(signed_arithmetic_ops, opcode);
        reg = select_value_register(node->left.node);
        left = operand_for_register(reg);
        emit_machine_operation(operation, type, left);
        result = left->value.reg.words.number;
        break;
    case 9: case 0xa: case 0xac: case 0xad:
        reg = select_value_register(node->left.node);
        left = operand_for_register(reg);
        map = (type & 0x200) ? unsigned_shift_ops : signed_arithmetic_ops;
        operation = lookup_opcode(map, opcode);
        right_node = node->second.node;
        reg = select_value_register(right_node);
        right = operand_for_register(reg);
        emit_machine_operation(MC_MOV, right_node->attributes.words.type, &operand_ecx, right);
        emit_machine_operation(operation, type, left, &operand_ecx);
        result = left->value.reg.words.number;
        break;
    case 0x26:
        result = new_value_register(type);
        left = operand_for_register(result);
        right = symbol_operand(node, 0);
        emit_machine_operation(MC_LEA, 0x104, left, right);
        break;
    case 0x33:
        result = new_value_register(type);
        select_constant(node, result);
        break;
    case 0x30:
        left_node = node->left.node;
        if (left_node->opcode == 0xa2) {
            reg = (unsigned short)left_node->left.value;
            source = operand_for_register(reg);
            if (node->attributes.words.flags & 0x20) source->value.reg.words.flags = 1;
            else machine_register_set = bitset_include(machine_register_set, (unsigned)(short)reg & 0xffff3fffU);
        } else {
            if (left_node->opcode == 0x26) source = symbol_operand(left_node, 1);
            else {
                reg = select_value_register(left_node);
                source = new_operand(OPERAND_BASED, reg, 0, 0);
            }
            if (node->attributes.words.flags & 0x800) source->flags |= 0x8000;
        }
        result = new_value_register(type);
        destination = operand_for_register(result);
        if (type & 0x3000) emit_machine_operation(MC_FLD, type, destination, source);
        else if ((type & 255) == 8) {
            high_left = high_dword_operand(duplicate_operand(destination));
            high_right = high_dword_operand(duplicate_operand(source));
            emit_machine_operation(MC_MOV, 4, destination, source);
            emit_machine_operation(MC_MOV, 4, high_left, high_right);
        } else emit_machine_operation(MC_MOV, type, destination, source);
        break;
    case 0x34:
        left_node = node->left.node;
        result = select_value_register(left_node);
        source = operand_for_register(result);
        source_type = left_node->attributes.words.type;
        if ((type & 0xf00) && (source_type & 0xf00)) {
            source_bytes = source_type & 255;
            target_bytes = node->attributes.words.type & 255;
            if (source_bytes < target_bytes) {
                result = new_value_register(type);
                destination = operand_for_register(result);
                if (source_bytes == 1) operation = (node->attributes.words.type & 0x200) ? MC_MOVZX : MC_MOVSX;
                else operation = (node->attributes.words.type & 0x200) ? MC_MOVZXW : MC_MOVSXW;
                emit_machine_operation(operation, type, destination, source);
                result = destination->value.reg.words.number;
            }
        } else if ((type & 0xf00) && (source_type & 0xff00) == 0x1000) {
            temporary = require_external_symbol("__ftol");
            right = new_operand(OPERAND_SYMBOL, 0, 0, temporary);
            emit_call(code_pointer_bytes, right, 0, 0x4000, 1);
            result = new_value_register(type);
            destination = operand_for_register(result);
            emit_machine_operation(MC_MOV, type, destination, &operand_eax);
            result = destination->value.reg.words.number;
            emit_float_return();
        } else {
            source_type = left_node->attributes.words.type;
            if ((node->attributes.words.type & 0xff00) == 0x1000 && (source_type & 0xf00)) {
                if (!(source_type & 0x100)) {
                    temporary = (SymbolHeader *)allocate_stack_temporary((source_type & 255) * 2);
                    destination = frame_temporary_operand(temporary, 0);
                    emit_machine_operation(MC_MOV, 4, destination, source);
                    right = immediate_operand(0);
                    destination = frame_temporary_operand(temporary, left_node->attributes.words.type & 255);
                    emit_machine_operation(MC_MOV, 4, destination, right);
                    source = frame_temporary_operand(temporary, 0);
                    emit_machine_operation(MC_FILD, (left_node->attributes.words.type & 255) * 2, &operand_st0, source);
                } else {
                    temporary = (SymbolHeader *)allocate_stack_temporary(source_type & 255);
                    destination = frame_temporary_operand(temporary, 0);
                    instruction = emit_machine_operation(MC_MOV, 4, destination, source);
                    instruction->opcode |= 0x200000;
                    source = frame_temporary_operand(temporary, 0);
                    emit_machine_operation(MC_FILD, left_node->attributes.words.type & 255, &operand_st0, source);
                }
                result = REG_ST0;
            } else if (optimize_precision &&
                (node->attributes.words.type & 255) < (left_node->attributes.words.type & 255)) {
                temporary = (SymbolHeader *)allocate_stack_temporary(node->attributes.words.type & 255);
                destination = frame_temporary_operand(temporary, 0);
                emit_machine_operation(MC_FSTP, type, destination, &operand_st0);
                source = frame_temporary_operand(temporary, 0);
                emit_machine_operation(MC_FLD, type, &operand_st0, source);
            }
        }
        break;
    case 0x42:
        result = select_conditional_value(node);
        break;
    case 0x44:
        select_value_register(node->left.node);
        if ((node->left.node->attributes.words.type & 0xff00) == 0x1000)
            emit_machine_operation(MC_FSTP, 0x100a, &operand_st0, &operand_st0);
        result = select_value_register(node->second.node);
        break;
    case 0x59:
        result = select_value_register(node->left.node);
        break;
    case 0x40: case 0x7a:
        result = select_intrinsic(node);
        break;
    case 0x74: case 0x75: case 0x76: case 0x79:
    case 0x7b: case 0x7c: case 0x81: case 0x82: case 0x83: case 0xa6:
        compiler_internal_error("@(#)codegen.c:1.167", 0x4ec);
        break;
    case 0xa2:
        compiler_internal_error("@(#)codegen.c:1.167", 0x2bc);
        break;
    case 0x84:
        reg = select_value_register(node->left.node);
        left = operand_for_register(reg);
        if (type & 0xf00) {
            emit_machine_operation(MC_MOV, type, &operand_eax, left);
            bytes = type & 255;
            if (bytes == 1) {
                emit_machine_operation(MC_CBW, 0);
                instruction = emit_machine_operation(MC_XOR, type, &operand_eax, &operand_eax);
                instruction->opcode |= 0x100000;
                instruction = emit_machine_operation(MC_SUB, type, &operand_eax, &operand_eax);
                instruction->opcode |= 0x100000;
            } else if (bytes == 2 || bytes == 4) {
                emit_machine_operation(bytes == 2 ? MC_CWD : MC_CDQ, 0);
                emit_machine_operation(MC_XOR, type, &operand_eax, &operand_edx);
                emit_machine_operation(MC_SUB, type, &operand_eax, &operand_edx);
            } else compiler_internal_error("@(#)codegen.c:1.167", 0x497);
            emit_machine_operation(MC_MOV, type, left, &operand_eax);
        } else emit_machine_operation(MC_FABS, type);
        result = left->value.reg.words.number;
        break;
    case 0xaa:
        if (!(type & 0x1000)) compiler_internal_error("@(#)codegen.c:1.167", 0x370);
        reg = select_value_register(node->left.node);
        left = operand_for_register(reg);
        argument = intern_leaf(0x33, 0, type, (unsigned)&extended_one);
        select_constant(argument, 0);
        emit_machine_operation(MC_FDIVRP, type, &operand_st1, &operand_st0);
        result = left->value.reg.words.number;
        break;
    case 0x3e:
        argument_bytes = 0;
        for (argument = node->second.node; argument->opcode != 0x4c; argument = argument->second.node) {
            if (argument->attributes.words.flags & 0x20) continue;
            source_type = argument->attributes.words.type;
            if (!(source_type & 0xf00)) {
                if (!(source_type & 0x1000)) {
                    select_value_register(argument->left.node);
                    argument_bytes += aligned_value_size(argument);
                } else {
                    bytes = ((source_type & 255) + 3) & ~3U;
                    select_value_register(argument->left.node);
                    right = new_operand(OPERAND_IMMEDIATE, 0, bytes, 0);
                    emit_machine_operation(MC_SUB, 0x104, &operand_esp, right);
                    destination = new_operand(OPERAND_BASED, REG_ESP, 0, 0);
                    emit_machine_operation(MC_FSTP, argument->attributes.words.type, destination, &operand_st0);
                    argument_bytes += bytes;
                }
            } else {
                reg = select_value_register(argument->left.node);
                if ((argument->attributes.words.type & 255) == 8) {
                    initialize_operand(&argument_operand, OPERAND_REGISTER, (unsigned short)(reg + 1), 0, 0);
                    emit_machine_operation(MC_PUSH, 4, &argument_operand);
                    argument_bytes += 4;
                }
                initialize_operand(&argument_operand, OPERAND_REGISTER, reg, 0, 0);
                emit_machine_operation(MC_PUSH, 4, &argument_operand);
                argument_bytes += 4;
            }
        }
        argument_count = 0;
        register_mask = 0;
        for (argument = node->second.node; argument->opcode != 0x4c; argument = argument->second.node) {
            if (argument->attributes.words.flags & 0x20) {
                reg = (unsigned short)argument->info.words.identifier;
                register_mask |= 1U << (reg & 31);
                argument_registers[argument_count] = reg;
                value_reg = select_value_register(argument->left.node);
                argument_values[argument_count] = operand_for_register(value_reg);
                ++argument_count;
            }
        }
        left_node = node->left.node;
        if (left_node->opcode == 0x26) right = new_operand(OPERAND_SYMBOL, 0, 0, left_node->right.symbol);
        else {
            reg = select_value_register(left_node);
            right = operand_for_register(reg);
        }
        for (index = 0; index < argument_count; ++index) {
            destination = operand_for_register(argument_registers[index]);
            emit_machine_operation(MC_MOV, 0x104, destination, argument_values[index]);
        }
        if (type & 0x1000) result_flags = 0x4000;
        else result_flags = (type & 255) == 8 ? 0x11 : 1;
        if (!node->detail.control.code) {
            emit_call(left_node->attributes.words.type & 255, right, 0, register_mask, result_flags);
            right = new_operand(OPERAND_IMMEDIATE, 0, argument_bytes, 0);
            emit_machine_operation(MC_ADD, 0x104, &operand_esp, right);
        } else emit_call(left_node->attributes.words.type & 255, right, argument_bytes, register_mask, result_flags);
        result = new_value_register(type);
        destination = operand_for_register(result);
        if (type & 0x1000) mark_x87_call_result();
        else if ((type & 255) == 8) {
            high_left = high_dword_operand(duplicate_operand(destination));
            emit_machine_operation(MC_MOV, 4, destination, &operand_eax);
            emit_machine_operation(MC_MOV, 4, high_left, &operand_edx);
        } else emit_machine_operation(MC_MOV, type, destination, &operand_eax);
        break;
    case 0xae: case 0xaf:
        reg = select_value_register(node->left.node);
        left = operand_for_register(reg);
        reg = select_value_register(node->second.node);
        right = operand_for_register(reg);
        if (((type & 0x200) || (type & 0xc00)) && cpu_model <= 3) {
            reg = new_value_register(type);
            destination = operand_for_register(reg);
            emit_machine_operation(MC_SUB, type, left, right);
            emit_machine_operation(MC_SBB, type, destination, destination);
            if (opcode == 0xae) emit_machine_operation(MC_NOT, type, destination);
            emit_machine_operation(MC_AND, type, left, destination);
            emit_machine_operation(MC_ADD, type, left, right);
        } else {
            label = new_internal_label();
            destination = new_operand(OPERAND_SYMBOL, 0, 0, &label->symbol);
            if ((type & 0x100) || (type & 0x200) || (type & 0xc00)) {
                map = (type & 0x100) ? signed_false_conditions : unsigned_false_conditions;
                condition = lookup_opcode(map, opcode == 0xae ? 0x21 : 0x23);
                emit_machine_operation(MC_CMP, type, left, right);
                emit_machine_operation(MC_JCC, 0x104, destination, condition);
                emit_register_barrier(left, type);
                emit_machine_operation(MC_MOV, type, left, right);
                emit_register_barrier(left, type);
                emit_label(destination->value.symbol);
            } else {
                if (!(type & 0x1000)) compiler_internal_error("@(#)codegen.c:1.167", 0x46f);
                arrange_x87_left(node->left.node);
                emit_machine_operation(MC_FCOM, type, &operand_st0, &operand_st1);
                if (optimize_precision) emit_machine_operation(MC_FWAIT, 0);
                emit_machine_operation(MC_FNSTSW, 0x104, &operand_eax);
                emit_machine_operation(MC_SAHF, 0);
                emit_machine_operation(MC_JCC, 0x104, destination, opcode == 0xaf ? 7 : 2);
                emit_machine_operation(MC_FXCH, type, &operand_st1);
                emit_label(destination->value.symbol);
                emit_machine_operation(MC_FSTP, type, left, &operand_st0);
            }
        }
        result = left->value.reg.words.number;
        break;
    }
    temporary_operand_count = saved_operands;
    if (node->attributes.words.type & 0x3000) record_x87_node(node);
    return result;
}

/* Original [00019830,000198e0). Select the left operand first so a
   zero-comparison specialization can use its flags. Otherwise emit the
   integer compare or the native x87 compare/status-to-flags sequence. */
void __fastcall select_condition(Node *node)
{
    MachineOperand *left, *right;
    left = operand_for_register(select_value_register(node->left.node));
    if (comparison_uses_intrinsic_flags(node)) return;
    right = operand_for_register(select_value_register(node->second.node));
    if (node->attributes.words.type & 0xf00) {
        emit_machine_operation(MC_CMP, node->attributes.words.type, left, right);
        return;
    }
    arrange_x87_left(node->left.node);
    emit_machine_operation(MC_FCOMPP, node->attributes.words.type, left, right);
    if (optimize_precision) emit_machine_operation(MC_FWAIT, 0);
    emit_machine_operation(MC_FNSTSW, 0x104, &operand_eax);
    emit_machine_operation(MC_SAHF, 0);
}

/* Original [0001dc10,0001dc70). The bounded table at00080278..00080290
   has two semantic entries: intrinsic171 compared with zero for equality
   or inequality. Express that predicate directly; the intrinsic's broader
   identity will be named when its selector is reconstructed. */
int __fastcall comparison_uses_intrinsic_flags(Node *node)
{
    Node *intrinsic = node->left.node;
    if (intrinsic->opcode != 0x40 || intrinsic->left.node->opcode != 0x33 ||
        node->second.node->opcode != 0x33 || node->second.node->left.value != 0) return 0;
    return intrinsic->left.node->left.value == 171 &&
        (node->opcode == 0x1f || node->opcode == 0x20);
}
