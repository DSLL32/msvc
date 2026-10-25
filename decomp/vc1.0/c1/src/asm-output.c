/* Compact frontend IR encoding of normalized assembler operands. */
#include "asm.h"
#include "global-ir.h"
#include "control-ir.h"
#include <recon.h>

/* Count, opcode and size/condition precede at most three operands. The
   original workspace ends at the independent target_cpu_mask at0x5b124.
   Longest format needs13 bytes plus two six-byte symbolic-register prefixes:
   3 + 3*(13+12) =78, within this128-byte allocation and signed count. */
extern FrontendSymbol *translation_sentinel, *translation_placeholder;
char asm_instruction_record[128]; /* original0x5b0a4..0x5b124 */
char *asm_operand_formats[21] = {
    "", "mr", "mv", "msog", "mdbg", "msobg", "mdibxg", "msoibxg",
    "mso", "mso", "ms", "ms", "md", "m#", "ms", "m", "mdg", "m", "msr", "mdbg", "mr"
};

/* Original0x4f0cd..0x4f12f. This interface consumes parser register IDs,
   before normalize_asm_addresses replaces them with encoded segment values. */
void write_asm_segment_prefix(void)
{
    unsigned char segment;
    segment = asm_operands[0].address->flags & 63;
    if (asm_operand_count > 1 && !segment)
        segment = asm_operands[1].address->flags & 63;
    if (segment) {
        asm_instruction_record[0] = 5;
        asm_instruction_record[1] = (char)0xaf;
        asm_instruction_record[2] = 0;
        asm_instruction_record[3] = 1;
        asm_instruction_record[4] = (unsigned char)asm_register_properties[segment] & 63;
        asm_instruction_record[5] = 0;
        record_line_information(22);
    }
}

/* Original0x4f1ee..0x4f400; dispatch table0x4f401..0x4f474. */
void serialize_asm_instruction(void)
{
    int index;
    unsigned char *cursor = (unsigned char *)asm_instruction_record + 3;
    unsigned char mode, byte;
    unsigned symbol_index;
    AsmAddress *address;
    AsmOperand *operand;
    char *format;
    asm_instruction_record[1] = asm_encoded_opcode;
    asm_instruction_record[2] = asm_instruction_modifier;
    if (asm_operation != 15 && asm_operation != 189) {
        switch (asm_instruction_size & 15) {
        case 0: asm_instruction_record[2] = 1; break;
        case 1: asm_instruction_record[2] = 2; break;
        case 2: asm_instruction_record[2] = 4; break;
        case 3: asm_instruction_record[2] = 8; break;
        case 4: asm_instruction_record[2] = 10; break;
        case 5: asm_instruction_record[2] = 6; break;
        default: asm_instruction_record[2] = 0; break;
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
    asm_instruction_record[0] = (char)(cursor - (unsigned char *)asm_instruction_record);
}

/* Original0x4f12f..0x4f1ed. Select a compact operand format after register
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
            if (options.option_qc) return 8;
            if (asm_operand_class == 16 || asm_operand_class == 15 ||
                asm_operand_class == 27 || asm_operand_class == 14) mode = 12;
        }
        if (address->symbol == translation_placeholder && !options.option_qc &&
            (asm_operand_class == 6 || asm_operand_class == 13)) mode = 11;
        break;
    }
    return mode;
}
