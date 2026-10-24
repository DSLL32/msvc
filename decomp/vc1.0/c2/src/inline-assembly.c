/* Decode the frontend's serialized inline-assembly instructions. */
#include "function-input.h"
#include "machine.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include "tree-rewrite.h"
#include "flow-edges.h"
#include "expression-fields.h"
#include <recon.h>
MachineInstruction *__fastcall decode_assembly_instruction(unsigned char *bytes, ScopeHash *scope);
/* Original1a970..1a980. The node's serialized payload becomes machine IR. */
void __fastcall decode_inline_assembly(Node *node, ScopeHash *scope)
{
    node->left.pointer = decode_assembly_instruction(node->left.pointer, scope);
}

unsigned char *__fastcall decode_assembly_operand(MachineOperand *operand,
    unsigned char *bytes, ScopeHash *scope);
/* The frontend numbers its x87 registers40..47; machine IR uses14..21. */
enum AssemblyRegister { ASM_ST0 = 40, ASM_X87_REGISTER_BIAS = 26,
    ASM_HIGH_BYTE_REGISTER = 4 };
/* Original1a980..1ae11. Decode complete operands before building the
   machine record; special frontend forms normalize to backend operation IDs. */
MachineInstruction *__fastcall decode_assembly_instruction(unsigned char *bytes, ScopeHash *scope)
{
    unsigned length = bytes[0], operation = bytes[1], size = bytes[2];
    unsigned char *cursor = bytes + 3;
    MachineOperand operands[3], *operand;
    MachineInstruction *instruction;
    MachineAddress *address;
    SymbolHeader *symbol;
    unsigned displacement, encoded_operation;
    int count, index;
    if (operation >= MC_LAST)
        compiler_internal_error("@(#)codegen.c:1.167", 0x98d);
    switch (operation) {
    case MC_FILD: case MC_FLD:
        initialize_operand(&operands[0], OPERAND_REGISTER, ASM_ST0, 0, NULL);
        cursor = decode_assembly_operand(&operands[1], cursor, scope);
        break;
    case MC_FIST: case MC_FISTP: case MC_FST: case MC_FSTP:
        cursor = decode_assembly_operand(&operands[0], cursor, scope);
        initialize_operand(&operands[1], OPERAND_REGISTER, ASM_ST0, 0, NULL);
        break;
    default:
        if (operation == MC_DEC || operation == MC_INC) count = 2;
        else if (operation == MC_IMUL3) count = 3;
        else count = machine_operand_counts[operation];
        for (index = 0; index < count; ++index)
            cursor = decode_assembly_operand(&operands[index], cursor, scope);
        break;
    }
    if (cursor - length != bytes)
        compiler_internal_error("@(#)codegen.c:1.167", 0x9ab);
    switch (operation) {
    case MC_CALL:
        if (machine_operand_counts[operation] != 1)
            compiler_internal_error("@(#)codegen.c:1.167", 0x9d4);
        instruction = new_machine_record(0, operation, size, &operands[0], 0, 0);
        break;
    case MC_JCC:
        if (machine_operand_counts[operation] != 1)
            compiler_internal_error("@(#)codegen.c:1.167", 0x9cb);
        instruction = new_machine_record(0, MC_JCC, 4, &operands[0], size);
        break;
    case MC_CBW:
        if (size == 1) instruction = new_machine_record(0, MC_CBW, 0);
        else if (size == 2) instruction = new_machine_record(0, MC_CWDE, 0);
        else {
            instruction = NULL;
            compiler_internal_error("@(#)codegen.c:1.167", 0x9bf);
        }
        break;
    case MC_CWD:
        if (size == 2) instruction = new_machine_record(0, MC_CWD, 0);
        else if (size == 4) instruction = new_machine_record(0, MC_CDQ, 0);
        else {
            instruction = NULL;
            compiler_internal_error("@(#)codegen.c:1.167", 0x9c6);
        }
        break;
    case MC_IMUL3:
        if ((operands[2].flags & 15) == OPERAND_IMMEDIATE) {
            displacement = operands[2].value.constant;
            symbol = NULL;
        } else {
            displacement = operands[2].value.address->first.displacement;
            symbol = operands[2].value.address->symbol;
        }
        initialize_operand(&operands[0], OPERAND_BASED,
            operands[0].value.reg.words.number, displacement, symbol);
        instruction = new_machine_record(0, operation, size, &operands[1], &operands[0]);
        break;
    case MC_SCC:
        if (machine_operand_counts[operation] != 1)
            compiler_internal_error("@(#)codegen.c:1.167", 0x9d0);
        instruction = new_machine_record(0, MC_SCC, 1, &operands[0], size);
        break;
    default:
        if (operation == MC_EMIT) operation = MC_DATA;
        instruction = new_machine_record(0, operation, size, &operands[0], &operands[1]);
        break;
    }
    encoded_operation = (instruction->opcode >> 8) & 255;
    count = machine_operand_counts[encoded_operation];
    if ((encoded_operation >= MC_FADD && encoded_operation <= MC_FLD1) ||
        (encoded_operation >= MC_FUCOM && encoded_operation <= MC_FCOS)) {
        if (encoded_operation != MC_FSTSW && encoded_operation != MC_FNSTSW) {
            for (index = count; index > 0; --index) {
                operand = &instruction->operands[index == 2 ? 1 : 0];
                if ((operand->flags & 15) == OPERAND_REGISTER) {
                    if (operand->value.reg.words.number < ASM_ST0)
                        compiler_internal_error("@(#)codegen.c:1.167", 0x9ec);
                    operand->value.reg.words.number -= ASM_X87_REGISTER_BIAS;
                }
            }
        }
    } else if ((instruction->opcode & 0xf0) == 0x10) {
        if (count == 1 || count == 2) {
            if (count == 2 &&
                (instruction->operands[1].flags & 15) == OPERAND_REGISTER &&
                (instruction->operands[1].value.reg.words.number & ASM_HIGH_BYTE_REGISTER)) {
                instruction->opcode |= 0x100000;
                instruction->operands[1].value.reg.words.number &= ~ASM_HIGH_BYTE_REGISTER;
            }
            if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                (instruction->operands[0].value.reg.words.number & ASM_HIGH_BYTE_REGISTER)) {
                instruction->opcode |= 0x80000;
                instruction->operands[0].value.reg.words.number &= ~ASM_HIGH_BYTE_REGISTER;
            }
        }
    }
    switch (encoded_operation) {
    case MC_JCC: case MC_JMP: case MC_CALL: case MC_JCXZ:
    case MC_LOOP: case MC_LOOPNZ: case MC_LOOPZ:
        operand = &instruction->operands[0];
        if ((operand->flags & 15) == OPERAND_LABEL_ADDRESS) {
            address = operand->value.address;
            if (!address->symbol && address->first.displacement) {
                operand->flags = (operand->flags & ~5U) | 10;
                return instruction;
            }
            if (address->symbol && !address->first.displacement) {
                operand->flags = (operand->flags & ~7U) | OPERAND_SYMBOL;
                symbol = address->symbol;
                release_machine_address(address);
                operand->value.symbol = symbol;
            }
        }
        break;
    }
    return instruction;
}


/* Original1a950..1a964. Function selection already owns a nonempty stream;
   splice the node's decoded record after its current tail. */
void __fastcall append_embedded_record(Node *node)
{
    MachineInstruction *instruction = node->left.pointer;
    instruction->previous = machine_tail;
    machine_tail->next = instruction;
    machine_tail = instruction;
}

/* Original [00027850,00027912). The visitor builds a temporary list of
   assembly references. Append the shared sentinel, wrap it in the typed
   marker tree, then mark every referenced symbol before returning the wrapper.
   The visitor's local head is output storage, not an incoming hidden argument. */
Node *__fastcall extract_assembly_labels(Node *node)
{
    Node *head = NULL, *tail, *result;
    SymbolHeader *symbol;
    visit_machine_symbols(node->left.pointer, collect_assembly_reference, &head);
    if (!head) return NULL;
    tail = head;
    while (tail->second.node) tail = tail->second.node;
    tail->second.node = intern_leaf(0x4c, 0);
    result = build_tree("NPNt", 0x4c, integer_node(0xf0, 0x102), 0x40, 0x102);
    result->right.node = head;
    while (head->opcode != 0x4c) {
        symbol = head->left.node->right.symbol;
        ((FunctionSymbol *)symbol)->field24 |= 0x2000;
        symbol->field0e |= 0x2000;
        head = head->second.node;
    }
    return result;
}
