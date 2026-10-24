/* Expand zero-extending moves into ordinary moves and masking operations. */
#include "instruction-rewrite.h"
#include "register-use.h"
#include "function-input.h"
#include <recon.h>
#define OPERATION(i) (((i)->opcode >> 8) & 255)
#define WIDTH(i) (((i)->opcode >> 4) & 15)
#define KIND(o) ((o)->flags & 15)
#define SET_OPERATION(i,n) ((i)->opcode = ((i)->opcode & ~0xff00U) | ((unsigned)(n) << 8))
#define SET_WIDTH(i,n) ((i)->opcode = ((i)->opcode & ~0xf0U) | ((unsigned)(n) << 4))
#define HIGH_SECOND 0x100000U

/* Original [00024740,00024bfd). Keep the high-byte address adjustment even
   when no expansion is selected. A following AND of the same destination
   can be folded into the new mask and then removed through its own entry. */
int __fastcall expand_zero_extension(MachineInstruction *instruction)
{
    MachineInstruction *following, *inserted;
    MachineOperand *source;
    MachineAddress *address;
    SymbolHeader *symbol;
    unsigned first_operation = MC_NOP, second_operation = MC_NOP;
    unsigned first_width = 0, second_width = 0, mask = 0;
    unsigned short destination, source_register;
    unsigned operation, high, width, role;
    int byte_destination, following_mask = 0, same_register = 0;
    int aligned = 0, expand = 0;

    destination = instruction->operands[0].value.reg.words.number & 0x3fff;
    byte_destination = destination <= 3;
    following = instruction->next;
    if (!(following->opcode & 15) && OPERATION(following) == MC_AND &&
        operands_equal(&instruction->operands[0], &following->operands[0]) &&
        KIND(&instruction->next->operands[1]) == OPERAND_IMMEDIATE)
        following_mask = 1;
    source = &instruction->operands[1];
    if (KIND(source) == OPERAND_REGISTER) {
        source_register = source->value.reg.words.number & 0x3fff;
        high = (instruction->opcode & HIGH_SECOND) != 0;
        if (source_register >= 4) source_register -= 4;
        operation = OPERATION(instruction);
        if (operation == MC_MOVZX) {
            if (WIDTH(instruction) == 2) {
                first_operation = MC_MOV; second_operation = MC_AND;
                first_width = byte_destination ? 1 : 2;
                if (!(high && first_width == 2)) {
                    second_width = 4; mask = 0xffff00ffU; expand = 1;
                }
            } else {
                expand = 1;
                if (byte_destination && source_register != destination && !following_mask) {
                    first_operation = MC_XOR; second_operation = MC_MOV;
                    first_width = 4; second_width = 1;
                } else {
                    first_operation = MC_MOV; second_operation = MC_AND;
                    if (high) {
                        if (!byte_destination) expand = 0;
                        else first_width = 1;
                    } else first_width = 4;
                    second_width = 4; mask = 0xff;
                }
            }
        } else {
            expand = 1;
            first_operation = MC_MOV; second_operation = MC_AND;
            first_width = second_width = 4; mask = 0xffff;
        }
    } else {
        address = source->value.address;
        if (KIND(source) == OPERAND_BASED || KIND(source) == OPERAND_INDEXED) {
            same_register = address->base.words.number ==
                instruction->operands[0].value.reg.words.number;
            if (!same_register && KIND(source) == OPERAND_INDEXED)
                same_register = address->index.words.number ==
                    instruction->operands[0].value.reg.words.number;
        }
        if (instruction->opcode & HIGH_SECOND) {
            instruction->opcode &= ~HIGH_SECOND;
            address->first.displacement = (long)((unsigned)address->first.displacement + 1);
        }
        symbol = address->symbol;
        if (symbol) {
            role = symbol->kind == 1 ? ((BackendSymbol *)symbol)->attributes & 0x70 : 0;
            if ((symbol->kind == 1 && (role == 0x10 || role == 0x30)) || symbol->kind == 0x13) {
                width = WIDTH(instruction);
                aligned = !((symbol->field04 + (unsigned)address->first.displacement) & (width - 1));
            }
        }
        operation = OPERATION(instruction);
        if (operation == MC_MOVZX) {
            if (WIDTH(instruction) == 2) {
                if (byte_destination || aligned) {
                    expand = 1;
                    first_operation = MC_MOV; second_operation = MC_AND;
                    first_width = byte_destination ? 1 : 2;
                    second_width = 4; mask = 0xffff00ffU;
                }
            } else if (byte_destination) {
                expand = 1;
                if (!same_register && !following_mask) {
                    first_operation = MC_XOR; second_operation = MC_MOV;
                    first_width = 4; second_width = 1;
                } else {
                    first_operation = MC_MOV; second_operation = MC_AND;
                    first_width = 1; second_width = 4; mask = 0xff;
                }
            } else if (aligned) {
                expand = 1;
                first_operation = MC_MOV; second_operation = MC_AND;
                first_width = second_width = 4; mask = 0xff;
            }
        } else {
            expand = 1;
            if (!same_register && !following_mask && !aligned) {
                first_operation = MC_XOR; second_operation = MC_MOV;
                first_width = 4; second_width = 2;
            } else {
                first_operation = MC_MOV; second_operation = MC_AND;
                first_width = aligned ? 4 : 2;
                second_width = 4; mask = 0xffff;
            }
        }
    }
    if (expand) {
        inserted = new_machine_instruction(0);
        SET_OPERATION(instruction, first_operation);
        SET_WIDTH(instruction, first_width);
        SET_OPERATION(inserted, second_operation);
        SET_WIDTH(inserted, second_width);
        initialize_operand(&inserted->operands[0], OPERAND_REGISTER,
            instruction->operands[0].value.reg.words.number, 0, NULL);
        if (first_operation == MC_XOR) {
            inserted->operands[1] = instruction->operands[1];
            inserted->opcode = (inserted->opcode & ~HIGH_SECOND) |
                (instruction->opcode & HIGH_SECOND);
            instruction->opcode &= ~HIGH_SECOND;
            initialize_operand(&instruction->operands[1], OPERAND_REGISTER,
                instruction->operands[0].value.reg.words.number, 0, NULL);
        } else initialize_operand(&inserted->operands[1], OPERAND_IMMEDIATE, 0, mask, NULL);
        following = instruction->next;
        inserted->next = following;
        following->previous = inserted;
        inserted->previous = instruction;
        instruction->next = inserted;
        if (following_mask) {
            inserted->operands[1].value.constant = following->operands[1].value.constant & mask;
            remove_machine_instruction(following);
        }
    }
    return expand;
}
