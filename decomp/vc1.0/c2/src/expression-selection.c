#include "instruction-selection.h"
#include "work-prepare.h"
#include "tree-rewrite.h"
#include "algebra.h"
#include <recon.h>
extern int hook_functions, code_pointer_bytes;
SymbolHeader *__fastcall require_external_symbol(char *name);

int x87_stack_depth; /* 000801c4 */
unsigned return_address_bytes; /* 0007ce4c */
/* Shared physical operands, passed by address to instruction construction. */
MachineOperand operand_eax = { OPERAND_REGISTER, { REG_EAX } }; /* 00080010 */
MachineOperand operand_ecx = { OPERAND_REGISTER, { REG_ECX } }; /* 00080020 */
MachineOperand operand_edx = { OPERAND_REGISTER, { REG_EDX } }; /* 00080028 */
MachineOperand operand_esi = { OPERAND_REGISTER, { REG_ESI } }; /*00080030*/
MachineOperand operand_edi = { OPERAND_REGISTER, { REG_EDI } }; /*00080038*/
MachineOperand operand_esp = { OPERAND_REGISTER, { REG_ESP } }; /* 00080040 */
MachineOperand operand_ebp = { OPERAND_REGISTER, { REG_EBP } }; /* 00080048 */
MachineOperand operand_st0 = { OPERAND_REGISTER, { REG_ST0 } }; /* 00080050 */

enum BranchCondition { COND_B = 2, COND_AE, COND_E, COND_NE,
    COND_BE, COND_A, COND_L = 12, COND_GE, COND_LE, COND_G };
/* Tree operators mapped to arithmetic operations and false-branch conditions.
   The original end rows are -1, although lookup_opcode tests for 255. Keep
   that distinction; valid selection paths request a key present in the map. */
const OpcodeMap signed_arithmetic_ops[] = { /* 00080060 */
    {2, MC_ADD}, {3, MC_SUB}, {0xd, MC_XOR}, {0xb, MC_AND}, {0xc, MC_OR},
    {8, MC_NEG}, {0xe, MC_NOT}, {4, MC_IMUL2}, {5, MC_IDIV}, {6, MC_IDIV},
    {9, MC_SHL}, {0xa, MC_SAR}, {0xad, MC_ROL}, {0xac, MC_ROR}, {~0U, 0}
};
const OpcodeMap unsigned_shift_ops[] = { /* 000800d8 */
    {9, MC_SHL}, {0xa, MC_SHR}, {0xad, MC_ROL}, {0xac, MC_ROR}, {~0U, 0}
};
const OpcodeMap signed_false_conditions[] = { /* 00080100 */
    {0x1f, COND_NE}, {0x20, COND_E}, {0x22, COND_GE}, {0x24, COND_LE},
    {0x21, COND_G}, {0x23, COND_L}, {~0U, 0}
};
const OpcodeMap unsigned_false_conditions[] = { /* 00080138 */
    {0x1f, COND_NE}, {0x20, COND_E}, {0x22, COND_AE}, {0x24, COND_BE},
    {0x21, COND_A}, {0x23, COND_B}, {~0U, 0}
};

/* Original [00017340,00017ca0). Select statement roots. Value trees are
   delegated to select_value_register; operand temporaries belong to this root. */
void __fastcall select_expression(Node *node)
{
    unsigned saved_operands, opcode, operation, condition, convention, bytes;
    unsigned short type, reg;
    Node *value, *target, *range, *chain;
    MachineOperand *left, *right, *high_left, *high_right, *saved;
    MachineOperand range_target;
    LabelSymbol *label;
    MachineBlockHeader *block;
    const OpcodeMap *map;
    long index, last;
    int register_this;

    if (x87_stack_depth) compiler_internal_error("@(#)codegen.c:1.167", 0x148);
    saved_operands = temporary_operand_count;
    type = node->attributes.words.type;
    opcode = node->opcode;
    if (opcode == 1) goto done;
    if (opcode == 0xf || opcode == 0x10 ||
        (opcode >= 0x15 && opcode <= 0x19) || opcode == 0x32) {
        /* This helper owns restoration when it succeeds. */
        if ((node->attributes.words.flags & 0x20) && try_select_assignment(node)) return;
        value = node->second.node;
        reg = select_value_register(value);
        right = operand_for_register(reg);
        left = select_lvalue_operand(node->left.node);
        opcode = node->opcode;
        if (opcode == 0x15 || opcode == 0x16) {
            operation = map_assignment_opcode(opcode, 0);
            map = (type & 0x200) ? unsigned_shift_ops : signed_arithmetic_ops;
            operation = lookup_opcode(map, operation);
            emit_machine_operation(MC_MOV, value->attributes.words.type, &operand_ecx, right);
            emit_machine_operation(operation, type, left, &operand_ecx);
        } else if (opcode != 0x32) {
            if (!(type & 0xf00) || (type & 255) > 4)
                compiler_internal_error("@(#)codegen.c:1.167", 0x178);
            operation = map_assignment_opcode(opcode, 0);
            operation = lookup_opcode(signed_arithmetic_ops, operation);
            emit_machine_operation(operation, type, left, right);
        } else {
            if (node->attributes.words.flags & 0x800) left->flags |= 0x8000;
            if (type & 0x1000) emit_machine_operation(MC_FSTP, type, left, right);
            else if ((type & 255) == 8) {
                high_left = high_dword_operand(duplicate_operand(left));
                high_right = high_dword_operand(duplicate_operand(right));
                emit_machine_operation(MC_MOV, 4, left, right);
                emit_machine_operation(MC_MOV, 4, high_left, high_right);
            } else emit_machine_operation(MC_MOV, type, left, right);
        }
    } else if (opcode == 0x29) emit_label(node->right.symbol);
    else if (opcode == 0x30 || opcode == 0x3e || opcode == 0x40 || opcode == 0x7a) {
        select_value_register(node);
        if (node->attributes.words.type & 0x1000)
            emit_machine_operation(MC_FSTP, 0x100a, &operand_st0, &operand_st0);
    } else if (opcode == 0x38) {
        value = node->left.node;
        select_condition(value);
        right = new_operand(OPERAND_SYMBOL, 0, 0, node->right.symbol);
        type = value->attributes.words.type;
        operation = value->opcode;
        if (type & 0x1000) operation = map_comparison_opcode(operation, 0);
        map = (type & 0xff00) == 0x100 ? signed_false_conditions : unsigned_false_conditions;
        condition = lookup_opcode(map, operation);
        emit_machine_operation(MC_JCC, 0x104, right, condition);
    } else if (opcode == 0x3a) {
        right = new_operand(OPERAND_SYMBOL, 0, 0, node->right.symbol);
        emit_machine_operation(MC_JMP, 0x104, right);
    } else if (opcode == 0x3c) {
        label = new_internal_label();
        label->symbol.field0d = 1;
        value = node->left.node;
        reg = value->left.node->opcode == 0xa2 ?
            (unsigned short)value->left.node->left.word : select_value_register(value);
        right = new_operand(OPERAND_BASED, reg, 0, &label->symbol);
        emit_machine_operation(MC_JMP, 0x104, right);
        emit_label(&label->symbol);
        range = node->second.node;
        index = (long)range->left.value;
        right = new_operand(OPERAND_LABEL_ADDRESS, 0, 0, node->right.symbol);
        for (; range->opcode != 0x4c; range = range->second.node) {
            initialize_operand(&range_target, OPERAND_LABEL_ADDRESS, 0, 0, range->right.symbol);
            while (index < (long)range->left.value) {
                ++index;
                left = duplicate_operand(right);
                emit_machine_operation(MC_DATA, 0x104, left);
                --temporary_operand_count;
            }
            while ((long)range->detail.value >= index) {
                left = duplicate_operand(&range_target);
                emit_machine_operation(MC_DATA, 0x104, left);
                --temporary_operand_count;
                last = index;
                index = (long)((unsigned)index + 1);
                if ((long)range->detail.value == last) break;
            }
        }
    } else if (opcode == 0x41) {
        reg = select_value_register(node->left.node);
        right = operand_for_register(reg);
        if (type & 0x1000) {
            block = selected_block;
            block->definitions = bitset_include(block->definitions, REG_ST0);
            emit_float_return();
        } else {
            if ((type & 255) == 8) {
                high_right = high_dword_operand(duplicate_operand(right));
                emit_machine_operation(MC_MOV, 4, &operand_eax, right);
                emit_machine_operation(MC_MOV, 4, &operand_edx, high_right);
                block = selected_block;
                block->definitions = bitset_include(block->definitions, REG_EDX);
            } else emit_machine_operation(MC_MOV, type, &operand_eax, right);
            block = selected_block;
            block->definitions = bitset_include(block->definitions, REG_EAX);
        }
    } else if (opcode == 0x46 || opcode == 0x48) {
        machine_context = emit_function_symbol(node->left.node->right.function);
        if (!(current_symbol->symbol.field0e & 0x400)) {
            if (hook_functions) {
                right = new_operand(OPERAND_SYMBOL, 0, 0, require_external_symbol("__penter"));
                emit_call(code_pointer_bytes, right, 0, 0, 0);
            }
            return_address_bytes = node->left.node->attributes.words.type & 255;
            emit_machine_operation(MC_EPUSH, 0x104, &operand_ebp);
            emit_machine_operation(MC_MOV, 0x104, &operand_ebp, &operand_esp);
            right = new_operand(OPERAND_FRAME_SIZE, 0, 0, (SymbolHeader *)0);
            emit_machine_operation(MC_SUB, 0x104, &operand_esp, right);
            emit_saved_register_pushes();
            if (current_symbol->symbol.field0e & 0x1000) select_assembly_record(node);
        }
    } else if (opcode == 0x47 || opcode == 0x49) {
        if (!(current_symbol->symbol.field0e & 0x400)) {
            if (current_symbol->symbol.field0e & 0x1000) select_assembly_record(node);
            emit_saved_register_pops();
            emit_machine_operation(MC_LEAVE, 0);
            bytes = (current_symbol->symbol.field0e & 0x1c) ? current_symbol->parameter_bytes : 0;
            right = new_operand(OPERAND_IMMEDIATE, 0, bytes, (SymbolHeader *)0);
            emit_machine_operation(MC_RET, return_address_bytes, right);
            new_selection_block((BitSet *)0, (BitSet *)0, -1, 0);
            reg = (unsigned short)return_value_register(current_symbol);
            if (reg != 0xffff) {
                block = selected_block;
                block->uses = bitset_include(block->uses, reg);
            }
        }
        finish_global_selection();
        finish_function_selection();
    } else if (opcode == 0x4f) select_directive(node);
    else if (opcode == 0x56) emit_source_filename(node->right.pointer);
    else if (opcode >= 0x8d && opcode <= 0x90) select_assembly_record(node);
    else if (opcode == 0x9d) {
        convention = current_symbol->symbol.field0e & 0x1c;
        register_this = convention == 4 || convention == 0x14;
        if (register_this) {
            block = selected_block;
            block->uses = bitset_include(block->uses, REG_ECX);
            block = selected_block;
            block->definitions = bitset_include(block->definitions, REG_ECX);
        }
        operation = node->left.node->left.value;
        if (operation == 1) {
            left = &operand_ecx;
            if (!register_this) left = new_operand(OPERAND_BASED, REG_ESP, 4, (SymbolHeader *)0);
            reg = select_value_register(node->second.node->left.node);
            right = operand_for_register(reg);
            emit_machine_operation(MC_SUB, 0x104, left, right);
            target = node->second.node->second.node->left.node;
        } else if (operation == 3) {
            if (!register_this) {
                right = new_operand(OPERAND_BASED, REG_ESP, 4, (SymbolHeader *)0);
                emit_machine_operation(MC_MOV, 0x104, &operand_eax, right);
            }
            right = new_operand(OPERAND_BASED, (unsigned short)register_this, 0, (SymbolHeader *)0);
            emit_machine_operation(MC_MOV, 0x104, &operand_eax, right);
            right = new_operand(OPERAND_BASED, REG_EAX, node->second.node->left.node->left.value,
                (SymbolHeader *)0);
            emit_machine_operation(MC_JMP, 0x104, right);
            goto done;
        } else if (operation == 4) {
            chain = node->second.node->second.node;
            saved = (MachineOperand *)0;
            if (!register_this) {
                saved = new_operand(OPERAND_BASED, REG_ESP, 4, (SymbolHeader *)0);
                emit_machine_operation(MC_MOV, 0x104, &operand_ecx, saved);
            }
            right = new_operand(OPERAND_BASED, REG_ECX,
                chain->second.node->second.node->left.node->left.value, (SymbolHeader *)0);
            emit_machine_operation(MC_SUB, 0x104, &operand_ecx, right);
            value = chain->second.node->left.node;
            if (value->left.value) {
                reg = select_value_register(value);
                right = operand_for_register(reg);
                emit_machine_operation(MC_SUB, 0x104, &operand_ecx, right);
            }
            if (!register_this) emit_machine_operation(MC_MOV, 0x104, saved, &operand_ecx);
            target = chain->left.node;
        } else {
            compiler_internal_error("@(#)codegen.c:1.167", 0x270);
            goto done;
        }
        if (target->opcode == 0x26) right = new_operand(OPERAND_SYMBOL, 0, 0, target->right.symbol);
        else {
            reg = select_value_register(target);
            right = operand_for_register(reg);
        }
        emit_machine_operation(MC_JMP, 0x104, right);
    } else compiler_internal_error("@(#)codegen.c:1.167", 0x28b);
done:
    temporary_operand_count = saved_operands;
}
