/* Operand values and their object-format relocation references. */
#include "instruction-encoding.h"
#include "instruction-selection.h"
#include "function-input.h"
#include "output.h"
#include <recon.h>
extern int coff_object, byte_block_first;
/* Original [00026700,00026900). Resolve relative values and known frame
   symbols, notify the output format, then write the chosen immediate width. */
unsigned __fastcall encode_operand(MachineOperand *operand, unsigned bytes, int sign_extend)
{
    unsigned kind = operand->flags & 15;
    unsigned offset = (unsigned)(encoding_cursor - encoding_buffer);
    unsigned value = 0, result = 0, storage;
    SymbolHeader *symbol = 0;
    MachineAddress *address;
    int resolved_symbol;
    if (kind == OPERAND_IMMEDIATE) value = operand->value.constant;
    else if (kind == OPERAND_SYMBOL) {
        symbol = operand->value.symbol;
        if (symbol->kind == 4) value = 0;
        else {
            value = symbol->field04 - encoding_location - offset - bytes;
            symbol = 0;
        }
    } else if (kind == 10) value = operand->value.constant - offset - bytes;
    else {
        address = operand->value.address;
        if (kind == OPERAND_ABSOLUTE || kind == OPERAND_LABEL_ADDRESS) {
            if (!address->symbol) value = encoding_location - current_symbol->symbol.field04;
            else bytes = 4;
        }
        value += (unsigned)address->first.displacement;
        symbol = address->symbol;
        if (coff_object && !byte_block_first) {
            if (symbol && symbol->kind != 4) value += symbol->field04;
        } else if (symbol) {
            storage = symbol->kind == 1 ? ((TemporarySymbol *)symbol)->attributes & 0x70 : 0;
            if ((symbol->kind == 1 && (storage == 0x10 || storage == 0x30)) || symbol->kind == 19)
                value += symbol->field04;
        }
    }
    if (encoding_emit_relocations) {
        storage = symbol && symbol->kind == 1 ? ((TemporarySymbol *)symbol)->attributes & 0x70 : 0;
        resolved_symbol = symbol && ((symbol->kind == 1 && (storage == 0x10 || storage == 0x30)) || symbol->kind == 19);
        if (!symbol || resolved_symbol || (int)bytes <= 1) {
            if ((kind == OPERAND_ABSOLUTE || kind == OPERAND_LABEL_ADDRESS) && !operand->value.address->symbol)
                output_callbacks.absolute_reference(offset, &current_symbol->symbol);
        } else if (bytes == 6) {
            output_callbacks.segment_reference(offset, symbol);
            output_callbacks.absolute_reference(offset + 2, symbol);
        } else if (kind == 7) output_callbacks.segment_reference(offset, symbol);
        else if (kind == OPERAND_SYMBOL) output_callbacks.relative_reference(offset, symbol);
        else output_callbacks.absolute_reference(offset, symbol);
    }
    if (sign_extend && (int)bytes > 1 && kind == OPERAND_IMMEDIATE && fits_signed_byte((int)value)) {
        bytes = 1;
        result = 2;
    }
    if (bytes == 6) {
        encode_integer(0, 2);
        bytes = 4;
    }
    encode_integer(value, bytes);
    return result;
}

/* Original [00026900,00026950). Unsupported widths reserve space without
   writing it; callers split six-byte far operands into two and four bytes. */
void __fastcall encode_integer(unsigned value, unsigned bytes)
{
    unsigned char *cursor = encoding_cursor;
    if (!bytes) return;
    if (bytes == 1) *cursor = (unsigned char)value;
    else if (bytes == 2) *(unsigned short *)cursor = (unsigned short)value;
    else if (bytes == 4) *(unsigned *)cursor = value;
    encoding_cursor = cursor + bytes;
}

/* Original [00026950,00026970). An immediate can use the signed imm8 form. */
int __fastcall fits_signed_byte(int value)
{
    return value >= -128 && value <= 127;
}
