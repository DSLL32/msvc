/* x86 register and memory-address encodings. */
#include "instruction-encoding.h"
#include "instruction-selection.h"
#include "function-input.h"
#include <recon.h>
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
/* Original [00026970,00026a0c). Choose the address displacement width,
   including a resolved frame offset and any explicit padding enlargement. */
unsigned __fastcall displacement_mode(MachineInstruction *instruction,
    MachineOperand *operand, unsigned *bytes)
{
    /* Original805c0..805d4: five width-indexed ModR/M mode fields. Widths
       two/three are invalid (-1); the adjacent805d4 object is a source string. */
    static const unsigned displacement_modes[5] = { 0, 0x40, ~0U, ~0U, 0x80 };
    MachineAddress *address = operand->value.address;
    SymbolHeader *symbol = address->symbol;
    unsigned storage;
    if (!symbol) {
        if (!address->first.displacement && address->base.words.number != REG_EBP) *bytes = 0;
        else *bytes = fits_signed_byte(address->first.displacement) ? 1 : 4;
    } else {
        storage = symbol->kind == 1 ? ((TemporarySymbol *)symbol)->attributes & 0x70 : 0;
        if (((symbol->kind == 1 && (storage == 0x10 || storage == 0x30)) || symbol->kind == 19) &&
            fits_signed_byte(symbol->field04 + address->first.displacement)) *bytes = 1;
        else *bytes = 4;
    }
    *bytes += (instruction->record.encoding_length >> 13) & 7;
    return displacement_modes[*bytes];
}
