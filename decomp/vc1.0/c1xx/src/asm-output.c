/* Compact frontend IR encoding of normalized assembler operands. */
#include "asm.h"
#include "ir.h"
#include "control-ir.h"
#include <recon.h>

extern CppSymbol *translation_sentinel, *translation_placeholder;

/* Encountered format prefix0xaa360..0xaa3b4; following gap is unowned. */
char *asm_operand_formats[21] = {
    "", "mr", "mv", "msog", "mdbg", "msobg", "mdibxg", "msoibxg",
    "mso", "mso", "ms", "ms", "md", "m#", "ms", "m", "mdg", "m", "msr", "mdbg", "mr"
};

/* Original0x7ed1f..0x7ef32; owned dispatch tables0x7ef32..0x7efa6. */
void serialize_asm_instruction(void)
{
    int index;
    unsigned char *cursor = (unsigned char *)control_counted_data + 3;
    unsigned char mode, byte;
    unsigned symbol_index;
    AsmAddress *address;
    AsmOperand *operand;
    char *format;
    control_counted_data[1] = asm_encoded_opcode;
    if (asm_operation == 15 || asm_operation == 189) {
        control_counted_data[2] = asm_instruction_modifier;
    } else {
        switch (asm_instruction_size & 15) {
        case 0: control_counted_data[2] = 1; break;
        case 1: control_counted_data[2] = 2; break;
        case 2: control_counted_data[2] = 4; break;
        case 3: control_counted_data[2] = 8; break;
        case 4: control_counted_data[2] = 10; break;
        case 5: control_counted_data[2] = 6; break;
        default: control_counted_data[2] = 0; break;
        }
    }
    for (index = 0; index < asm_operand_count; ++index) {
        address = asm_output_addresses[index];
        operand = asm_output_operands[index];
        if (operand->register_symbol) {
            *cursor++ = 0x12;
            symbol_index = get_ir_symbol_index(operand->register_symbol);
            if (symbol_index < 0x8000) {
                *(unsigned short *)cursor = (unsigned short)symbol_index;
                cursor += 2;
            } else {
                *(unsigned short *)cursor = (unsigned short)symbol_index | 0x8000;
                *(unsigned short *)(cursor + 2) = (unsigned short)(symbol_index >> 15);
                cursor += 4;
            }
            *cursor++ = (unsigned char)asm_register_properties[8] & 63;
        }
        if (operand->frame_symbol) {
            *cursor++ = 0x12;
            symbol_index = get_ir_symbol_index(operand->frame_symbol);
            if (symbol_index < 0x8000) {
                *(unsigned short *)cursor = (unsigned short)symbol_index;
                cursor += 2;
            } else {
                *(unsigned short *)cursor = (unsigned short)symbol_index | 0x8000;
                *(unsigned short *)(cursor + 2) = (unsigned short)(symbol_index >> 15);
                cursor += 4;
            }
            *cursor++ = (unsigned char)asm_register_properties[6] & 63;
        }
        mode = asm_address_mode_code(address);
        for (format = asm_operand_formats[mode]; *format; ++format) {
            switch (*format) {
            case 'b': case 'r':
                byte = address->first_register == 63 ? 255 : address->first_register & 63;
                break;
            case 'd': case 'o': case 'v':
                *(unsigned *)cursor = address->displacement;
                cursor += 4;
                continue;
            case 'g':
                byte = address->flags & 63;
                if (byte == 63) byte = 0;
                break;
            case 'i':
                byte = address->second_register == 63 ? 255 : address->second_register & 63;
                break;
            case 'm':
                byte = mode;
                break;
            case 's':
                if (!address->symbol) {
                    *(unsigned short *)cursor = 0;
                    cursor += 2;
                } else {
                    symbol_index = get_ir_symbol_index(address->symbol);
                    if (symbol_index < 0x8000) {
                        *(unsigned short *)cursor = (unsigned short)symbol_index;
                        cursor += 2;
                    } else {
                        *(unsigned short *)cursor = (unsigned short)symbol_index | 0x8000;
                        *(unsigned short *)(cursor + 2) = (unsigned short)(symbol_index >> 15);
                        cursor += 4;
                    }
                }
                continue;
            case 'x':
                byte = (unsigned char)(1 << (address->flags >> 6));
                break;
            default:
                continue;
            }
            *cursor++ = byte;
        }
    }
    control_counted_data[0] = (char)(cursor - (unsigned char *)control_counted_data);
}

/* Original0x7ec60..0x7ed1f, stdcall4. Select a compact operand format after register
   conversion, refining symbol and indexed-address forms as required. */
unsigned char __stdcall asm_address_mode_code(AsmAddress *address)
{
    unsigned char mode = asm_address_rules[address->address_flags & 31].mode;
    switch (mode) {
    case 1:
        if ((address->first_register & 0xc0) == 0xc0) mode = 20;
        break;
    case 3:
        if (!address->symbol) mode = 16;
        break;
    case 4:
        if (address->second_register != 63) mode = 6;
        break;
    case 5:
        if (address->second_register != 63) mode = 7;
        break;
    case 8:
        if (address->symbol == translation_sentinel) {
            if (options.configuration.option_qc) return 8;
            if (asm_operand_class == 16 || asm_operand_class == 15 ||
                asm_operand_class == 27 || asm_operand_class == 14) mode = 12;
        }
        if (address->symbol == translation_placeholder && !options.configuration.option_qc &&
            (asm_operand_class == 6 || asm_operand_class == 13)) mode = 11;
        break;
    }
    return mode;
}
