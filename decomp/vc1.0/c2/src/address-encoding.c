/* x86 register and memory-address encodings. */
#include "instruction-encoding.h"
#include "instruction-selection.h"
/* Original [00026540,00026700). Address forms choose ModR/M, optional SIB,
   and displacement bytes. Only the low words hold physical register IDs. */
void __fastcall encode_modrm(MachineInstruction *instruction, MachineOperand *operand, unsigned reg)
{
    unsigned group = (reg & 7) << 3;
    unsigned kind = operand->flags & 15, bytes = 0, mode;
    MachineAddress *address;
    unsigned short base;
    if (kind == OPERAND_REGISTER) {
        base = operand->value.reg.words.number;
        if (base >= REG_ST0) base = (unsigned short)(base - REG_ST0);
        *encoding_cursor++ = (unsigned char)(base | group | 0xc0);
        return;
    }
    if (kind == OPERAND_ABSOLUTE || kind == 12) {
        *encoding_cursor++ = (unsigned char)(group | 5);
        bytes = (instruction->opcode >> 4) & 15;
    } else if (kind == OPERAND_BASED) {
        address = operand->value.address;
        base = address->base.words.number;
        if (base == REG_ESP) {
            address->index.words.number = REG_ESP;
            operand->flags &= ~0x300U;
            goto indexed;
        }
        mode = displacement_mode(instruction, operand, &bytes);
        *encoding_cursor++ = (unsigned char)(mode | group | base);
    } else if (kind == OPERAND_INDEXED) {
        address = operand->value.address;
        base = address->base.words.number;
        if (base == 0x1b) {
            *encoding_cursor++ = (unsigned char)(group | 4);
            *encoding_cursor++ = (unsigned char)(((operand->flags >> 8) << 6) |
                ((address->index.words.number & 7) << 3) | 5);
            bytes = 4;
        } else {
            if (address->index.words.number == REG_ESP) {
                if (operand->flags & 0x300) compiler_internal_error("@(#)code.c:1.60", 0x268);
                address->index.words.number = base;
                address->base.words.number = REG_ESP;
            }
indexed:
            mode = displacement_mode(instruction, operand, &bytes);
            *encoding_cursor++ = (unsigned char)(mode | group | 4);
            address = operand->value.address;
            *encoding_cursor++ = (unsigned char)(((operand->flags >> 8) << 6) |
                ((address->index.words.number & 7) << 3) | (address->base.words.number & 7));
        }
    }
    encode_operand(operand, bytes, 0);
}
