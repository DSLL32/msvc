/* Machine selection of serialized intrinsic operators. */
#include "intrinsic-selection.h"
#include "frame.h"
#include <recon.h>
extern int default_segment_alignment, code_pointer_bytes;
SymbolHeader *__fastcall require_external_symbol(char *name);

/* Original [0001b270,0001bbf4), including95 dispatch entries at1ba78.
   Emission is variadic: no-operand and single-operand instructions pass only
   their original arguments, avoiding the decompiler's spurious spare GPRs. */
unsigned short __fastcall select_intrinsic(Node *node)
{
    unsigned short result = 0xffff, type = node->attributes.words.type, address_register;
    unsigned intrinsic = node->left.node->left.value, bytes, count, part, width;
    int floats;
    Node *arguments, *value;
    MachineOperand *left, *right, *memory, *temporary;
    FunctionSymbol *symbol;
    switch (intrinsic) {
    case 0x96:
        result = select_intrinsic_block(node, (unsigned short)node->detail.control.code, 0);
        break;
    case 0x97:
        result = select_intrinsic_block(node, (unsigned short)node->detail.control.code, 1);
        break;
    case 0xa3: result = select_intrinsic_a3_a4(node, 1); break;
    case 0xa4: result = select_intrinsic_a3_a4(node, 0); break;
    case 0xa5: result = select_intrinsic_a5(node); break;
    case 0xa6: result = select_intrinsic_a6(node); break;
    case 0xa7:
        result = select_intrinsic_string_length(node->second.node->left.node, 0, type);
        break;
    case 0xa8: case 0xaf: case 0xe0:
        left = operand_for_register(select_value_register(node->second.node->left.node));
        if (intrinsic == 0xa8) type = 0x101;
        else if (intrinsic == 0xaf) type = 0x102;
        else type = 0x104;
        emit_machine_operation(MC_MOV, 0x102, &operand_edx, left);
        emit_machine_operation(MC_IN, type, &operand_eax, &operand_edx);
        left = operand_for_register(new_value_register(type));
        emit_machine_operation(MC_MOV, type, left, &operand_eax);
        result = left->value.reg.words.number;
        break;
    case 0xa9: case 0xb0: case 0xe1:
        left = operand_for_register(select_value_register(node->second.node->left.node));
        right = operand_for_register(select_value_register(node->second.node->second.node->left.node));
        if (intrinsic == 0xa9) type = 0x101;
        else if (intrinsic == 0xb0) type = 0x102;
        else type = 0x104;
        emit_machine_operation(MC_MOV, 0x102, &operand_edx, left);
        emit_machine_operation(MC_MOV, type, &operand_eax, right);
        emit_machine_operation(MC_OUT, type, &operand_edx, &operand_eax);
        result = right->value.reg.words.number;
        break;
    case 0xaa: result = select_intrinsic_aa_ab(node, 0); break;
    case 0xab: result = select_intrinsic_aa_ab(node, 1); break;
    case 0xac: result = select_intrinsic_block(node, 0x101, 0); break;
    case 0xad: result = select_intrinsic_block(node, 0x101, 1); break;
    case 0xb1:
        emit_machine_operation(MC_STI, 0);
        result = 0;
        break;
    case 0xb2:
        emit_machine_operation(MC_CLI, 0);
        result = 0;
        break;
    case 0xb3:
        arguments = node->second.node;
        bytes = aligned_value_size(node);
        if ((int)bytes <= 20) {
            address_register = 0;
            if (arguments->left.node->opcode != 0x26)
                address_register = select_value_register(arguments->left.node);
            count = arguments->second.node->left.node->left.value;
            while ((int)count > 0) {
                left = operand_for_register(new_value_register(0x100));
                if (address_register) memory = new_operand(OPERAND_BASED, address_register, 0, 0);
                else memory = symbol_operand(arguments->left.node, 1);
                part = count & 3;
                if (!part) part = 4;
                if (part == 3) {
                    memory->value.address->first.displacement += count - 1;
                    emit_machine_operation(MC_MOV, 0x101, left, memory);
                    emit_machine_operation(MC_SHL, 0x104, left,
                        new_operand(OPERAND_IMMEDIATE, 0, 16, 0));
                    memory = duplicate_operand(memory);
                    memory->value.address->first.displacement -= 2;
                    width = 0x102;
                } else {
                    memory->value.address->first.displacement += count - part;
                    width = part | 0x100;
                }
                emit_machine_operation(MC_MOV, width, left, memory);
                emit_machine_operation(MC_PUSH, 0x104, left);
                count -= part;
            }
        } else {
            emit_machine_operation(MC_SUB, 0x104, &operand_esp,
                new_operand(OPERAND_IMMEDIATE, 0, bytes, 0));
            left = operand_for_register(select_value_register(arguments->left.node));
            emit_machine_operation(MC_MOV, 0x104, &operand_esi, left);
            emit_machine_operation(MC_MOV, 0x104, &operand_edi, &operand_esp);
            count = arguments->second.node->left.node->left.value;
            emit_machine_operation(MC_MOV, 0x104, &operand_ecx,
                new_operand(OPERAND_IMMEDIATE, 0, (unsigned)((int)count / 4), 0));
            emit_machine_operation(MC_REP, 0);
            emit_machine_operation(MC_MOVS, 0x104);
            if (count & 2) emit_machine_operation(MC_MOVS, 0x102);
            if (count & 1) emit_machine_operation(MC_MOVS, 0x101);
        }
        result = 0;
        break;
    case 0xd7: case 0xd9: case 0xda: case 0xf1: case 0xf2: case 0xf4:
        result = select_assembly_record(node);
        break;
    case 0xd8:
        stack_frame_extent = 1;
        dynamic_stack_allocation = 1;
        value = node->second.node->left.node;
        if (value->opcode == 0x33)
            emit_machine_operation(MC_MOV, 0x104, &operand_eax,
                immediate_operand((value->left.value + default_segment_alignment - 1) &
                    ~(default_segment_alignment - 1U)));
        else {
            left = operand_for_register(select_value_register(value));
            emit_machine_operation(MC_MOV, 0x104, &operand_eax, left);
            emit_machine_operation(MC_ADD, 0x104, &operand_eax,
                immediate_operand(default_segment_alignment - 1));
            emit_machine_operation(MC_AND, 0x104, &operand_eax,
                immediate_operand(~(default_segment_alignment - 1U)));
        }
        emit_register_barrier(&operand_eax, 0x104);
        left = new_operand(OPERAND_SYMBOL, 0, 0, require_external_symbol("__alloca_probe"));
        emit_call(code_pointer_bytes, left, 0, 0, 0);
        emit_register_barrier(&operand_esp, 0x104);
        if (current_function->symbol->field0e & 0x1000) select_assembly_record(node);
        result = REG_ESP;
        break;
    case 0xdb:
        stack_frame_extent = 1;
        if (current_function->symbol->field0e & 0x1000) select_assembly_record(node);
        left = operand_for_register(select_value_register(node->second.node->left.node));
        emit_machine_operation(MC_PUSH, 0x104, left);
        symbol = (FunctionSymbol *)require_external_symbol("__setjmp");
        symbol->field24 &= ~0x700U;
        left = new_operand(OPERAND_SYMBOL, 0, 0, &symbol->symbol);
        emit_call(code_pointer_bytes, left, 0, 0, 0);
        emit_machine_operation(MC_ADD, 4, &operand_esp, immediate_operand(4));
        result = REG_EAX;
        break;
    case 0xde:
        emit_machine_operation(MC_SUB, 0x104, &operand_esp, immediate_operand(aligned_value_size(node)));
        left = select_lvalue_operand(node->second.node->left.node);
        emit_machine_operation(MC_MOV, 0x104, left, &operand_esp);
        result = 0x1a;
        break;
    case 0xdf:
        select_value_register(node->second.node->second.node->left.node);
        result = select_value_register(node->second.node->second.node->second.node->left.node);
        break;
    case 0xf3:
        left = operand_for_register(new_value_register(0x104));
        right = operand_for_register(new_value_register(0x104));
        memory = new_operand(OPERAND_ABSOLUTE, 0, 0, lookup_external_name("__tls_array"));
        memory->flags = (memory->flags & ~0x30U) | 0xc0;
        temporary = new_operand(OPERAND_ABSOLUTE, 0, 0, lookup_external_name("__tls_index"));
        emit_machine_operation(MC_MOV, 0x104, left, memory);
        emit_machine_operation(MC_MOV, 0x104, right, temporary);
        emit_machine_operation(MC_SHL, 0x104, right, immediate_operand(2));
        emit_machine_operation(MC_ADD, 0x104, left, right);
        memory = new_operand(OPERAND_BASED, left->value.reg.words.number, 0, 0);
        emit_machine_operation(MC_MOV, 0x104, left, memory);
        result = left->value.reg.words.number;
        break;
    default:
        if (node->opcode == 0x7a) result = select_runtime_intrinsic(node);
        else compiler_internal_error("@(#)cgintrin.c:1.57", 0x244);
        break;
    }
    if (node->left.node->opcode == 0x26 && node->left.node->right.symbol->kind == 4) {
        floats = intrinsic_float_cleanup_count((FunctionSymbol *)node->left.node->right.symbol);
        while (floats-- > 0) emit_float_return();
    }
    return result;
}

/* Original [0001c4a0,0001c676). Choose count/fill operands and dword
   transfer eligibility, preserving signed constant shifts and the original
   multiplication/shift proofs for a remainder-free byte or word count. */
unsigned short __fastcall select_intrinsic_block(Node *node, unsigned short type, int fill)
{
    Node *destination_tree, *source_tree, *count_tree, *arguments;
    MachineOperand *count_operand, *source, *destination;
    unsigned constant_count = 0, bytes = type & 255;
    int wide, remainder, direction;
    arguments = node->second.node;
    destination_tree = arguments->left.node;
    arguments = arguments->second.node;
    source_tree = arguments->left.node;
    count_tree = arguments->second.node->left.node;
    if (count_tree->opcode == 0x33) constant_count = count_tree->left.value;
    if (!constant_count && count_tree->opcode == 0x33)
        return operand_for_register(select_value_register(destination_tree))->value.reg.words.number;
    if (bytes != 4 && !(node->attributes.words.flags & 0x80)) {
        if (count_tree->opcode == 0x33)
            count_operand = new_operand(OPERAND_IMMEDIATE, 0,
                (unsigned)((int)count_tree->left.value >> (bytes == 1 ? 2 : 1)), 0);
        else count_operand = operand_for_register(select_value_register(count_tree));
        wide = 1;
    } else {
        count_operand = operand_for_register(select_value_register(count_tree));
        wide = 0;
    }
    if (fill && wide) source = select_replicated_fill_operand(source_tree, type, constant_count);
    else source = operand_for_register(select_value_register(source_tree));
    destination = operand_for_register(select_value_register(destination_tree));
    remainder = 0;
    if (wide) {
        remainder = 1;
        if (bytes == 1 &&
            ((count_tree->opcode == 4 && count_tree->second.node->opcode == 0x33 &&
              !(count_tree->second.node->left.value & 3)) ||
             (count_tree->opcode == 9 && count_tree->second.node->opcode == 0x33 &&
              (int)count_tree->second.node->left.value >= 2))) remainder = 0;
        else if (bytes == 2 &&
            ((count_tree->opcode == 4 && count_tree->second.node->opcode == 0x33 &&
              !(count_tree->second.node->left.value & 1)) ||
             (count_tree->opcode == 9 && count_tree->second.node->opcode == 0x33 &&
              (int)count_tree->second.node->left.value >= 1))) remainder = 0;
    }
    direction = (node->attributes.words.flags & 0x80) ? -1 : 1;
    select_block_operation(constant_count, count_operand, source, destination,
        wide, type, fill, remainder, direction);
    return destination->value.reg.words.number;
}

/* Original [0001d4e0,0001d66a). Form a dword whose byte/word lanes carry
   the fill value, skipping unused upper lanes for a known short transfer. */
MachineOperand *__fastcall select_replicated_fill_operand(Node *value, unsigned short type, unsigned count)
{
    MachineOperand *result, *source, *pair;
    unsigned bits;
    result = operand_for_register(new_value_register(0x100));
    if (value->opcode == 0x33) {
        bits = value->left.value;
        if ((type & 255) == 1) bits = (bits & 255) | (bits << 8);
        bits = (bits & 0xffff) | (bits << 16);
        emit_machine_operation(MC_MOV, 0x204, result,
            new_operand(OPERAND_IMMEDIATE, 0, bits, 0));
        return result;
    }
    source = operand_for_register(select_value_register(value));
    if ((type & 255) == 1) {
        pair = operand_for_register(new_value_register(0x100));
        emit_machine_operation(MC_MOV, 0x201, pair, source);
        emit_machine_operation(MC_SHL, 0x202, pair, new_operand(OPERAND_IMMEDIATE, 0, 8, 0));
        emit_machine_operation(MC_MOV, 0x201, pair, source);
        emit_machine_operation(MC_MOV, 0x204, result, pair);
        if (!count || count > 3) {
            emit_machine_operation(MC_SHL, 0x204, result, new_operand(OPERAND_IMMEDIATE, 0, 16, 0));
            emit_machine_operation(MC_MOV, 0x202, result, pair);
        }
    } else {
        emit_machine_operation(MC_MOV, 0x204, result, source);
        if (!count || count > 1) {
            emit_machine_operation(MC_SHL, 0x204, result, new_operand(OPERAND_IMMEDIATE, 0, 16, 0));
            emit_machine_operation(MC_MOV, 0x202, result, source);
        }
    }
    return result;
}
