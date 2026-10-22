/* Machine instruction sizing and emission share the same encoder. */
#include "instruction-encoding.h"
#include "machine-layout.h"
/* Original [00025520,00025540). Sizing uses scratch output without relocation
   callbacks; the encoder's returned byte count is retained. */
unsigned __fastcall machine_instruction_size(MachineInstruction *instruction)
{
    unsigned char buffer[20];
    return encode_machine_instruction(instruction, buffer, 0, 0);
}

/* Original [00025540,00026540), including its 20-, 8-, and 16-entry
   dispatch tables. Encoding patterns retain original candidate order. */
#include "instruction-encoding.h"
#include "encoding-recipes.h"
#include "instruction-selection.h"
#include "output.h"
#include <recon.h>
extern int coff_object, byte_block_first;
unsigned encoding_location; /* 0007cab0 */
int encoding_emit_relocations; /* 0007cab4 */
unsigned char *encoding_buffer, *encoding_cursor; /* 0007cab8,0007cabc */
/* Padding templates [00080574,0008059b), pointer choices [0008059c,000805bc).
   Five bytes use ADD EAX,0; the other multi-byte forms preserve flags. */
static const unsigned char padding_one[] = {0x90};
static const unsigned char padding_two[] = {0x8b,0xff};
static const unsigned char padding_three[] = {0x8d,0x49,0};
static const unsigned char padding_four[] = {0x8d,0x64,0x24,0};
static const unsigned char padding_five[] = {0x05,0,0,0,0};
static const unsigned char padding_six[] = {0x8d,0x9b,0,0,0,0};
static const unsigned char padding_seven[] = {0x8d,0xa4,0x24,0,0,0,0};
static const unsigned char *const padding_instructions[] = {
    0, padding_one, padding_two, padding_three, padding_four,
    padding_five, padding_six, padding_seven
};
#define KIND(o) ((o)->flags & 15)
#define FLOATING_OPERATION(op) (((op) >= MC_FADD && (op) <= MC_FLD1) || ((op) >= MC_FUCOM && (op) <= MC_FCOS))
unsigned __fastcall encode_machine_instruction(MachineInstruction *instruction,
    unsigned char *buffer, unsigned location, int emit)
{
    const InstructionEncoding *recipe;
    const EncodingPattern *pattern;
    const unsigned char *padding;
    MachineOperand *left = &instruction->operands[0], *right = &instruction->operands[1], *operand;
    MachineOperand adjusted;
    MachineAddress *address;
    unsigned operation = (instruction->opcode >> 8) & 255;
    unsigned width = (instruction->opcode >> 4) & 15;
    unsigned properties = machine_selection_properties[operation];
    unsigned segment, kind, group, bits, index, special_register;
    int count;
    unsigned char *prefix_end = buffer, *opcode, first;
    unsigned short reg;
    encoding_emit_relocations = emit;
    encoding_location = location;
    encoding_buffer = buffer;
    count = machine_operand_counts[operation];
    if (properties & MC_WAIT_PREFIX) *prefix_end++ = 0x9b;
    if (width == 2 && !FLOATING_OPERATION(operation) && !(properties & MC_NO_OPERAND_PREFIX))
        *prefix_end++ = 0x66;
    segment = count > 0 ? (left->flags >> 4) & 15 : 0;
    if (!segment && count > 1) segment = (right->flags >> 4) & 15;
    if (segment) {
        if (segment < 8 || segment > 13) compiler_internal_error("@(#)code.c:1.60", 0x87);
        *prefix_end++ = segment < 12 ? (unsigned char)(((segment & 3) << 3) | 0x26) :
            (unsigned char)(segment == 12 ? 0x64 : 0x65);
    }
    recipe = &instruction_encodings[operation];
    pattern = recipe->patterns;
    for (;;) {
        encoding_cursor = prefix_end;
        if (properties & MC_ESCAPE_OPCODE) *encoding_cursor++ = 0x0f;
        opcode = encoding_cursor++;
        *opcode = pattern->opcode;
        switch (pattern->form) {
        case ENC_MODRM: case ENC_MODRM_ALTERNATE:
modrm_form:
            if (count == 1) {
                group = (unsigned)((signed char)recipe->extension >> 3);
                if (width > 4 && (operation == MC_JMP || operation == MC_CALL)) group |= 1;
                operand = left;
                goto modrm;
            }
            if (KIND(left) == OPERAND_REGISTER) {
                encode_modrm(instruction, right, left->value.reg.words.number);
                if (pattern->modifiers & ENCODE_DIRECTION) *opcode |= 2;
                goto finished;
            }
            if (KIND(right) != OPERAND_REGISTER) compiler_internal_error("@(#)code.c:1.60", 0xb8);
modrm_right_register:
            group = right->value.reg.words.number;
            operand = left;
            goto modrm;
        case ENC_RM_IMMEDIATE:
            kind = KIND(right);
            if (kind != OPERAND_IMMEDIATE && kind != OPERAND_LABEL_ADDRESS && kind != 7) break;
            encode_modrm(instruction, left, (unsigned)((signed char)recipe->extension >> 3));
            bits = encode_operand(right, width, (pattern->modifiers & ENCODE_SIGN_EXTEND) ? 0x20 : 0);
            *opcode |= (unsigned char)bits;
            goto finished;
        case ENC_ACCUMULATOR_ADDRESS:
            if (KIND(left) == OPERAND_REGISTER && left->value.reg.words.number == REG_EAX &&
                (KIND(right) == OPERAND_ABSOLUTE || KIND(right) == 12)) {
                encode_operand(right, 4, 0);
                goto finished;
            }
            if (KIND(right) == OPERAND_REGISTER && right->value.reg.words.number == REG_EAX &&
                (KIND(left) == OPERAND_ABSOLUTE || KIND(left) == 12)) {
                encode_operand(left, 4, 0);
                *opcode |= 2;
                goto finished;
            }
            break;
        case ENC_PUSH_IMMEDIATE:
            kind = KIND(left);
            if (kind != OPERAND_IMMEDIATE && kind != OPERAND_LABEL_ADDRESS && kind != 7) break;
            bits = encode_operand(left, width, 1);
            *opcode |= (unsigned char)bits;
            goto finished;
        case ENC_REGISTER_IMMEDIATE:
            kind = KIND(right);
            if (KIND(left) != OPERAND_REGISTER ||
                (kind != OPERAND_IMMEDIATE && kind != OPERAND_LABEL_ADDRESS && kind != 7)) break;
            *opcode |= (unsigned char)left->value.reg.words.number;
            if (width > 1) *opcode |= 8;
            encode_operand(right, width, 0);
            goto finished;
        case ENC_REGISTER_OPCODE:
            if (KIND(left) != OPERAND_REGISTER || width == 1) break;
            *opcode |= (unsigned char)left->value.reg.words.number;
            goto finished;
        case ENC_FIXED:
            if (recipe->extension) *encoding_cursor++ = recipe->extension;
            goto finished;
        case ENC_RELATIVE_BYTE: case ENC_RELATIVE_NEAR: case ENC_FAR_ADDRESS:
            bits = pattern->form == ENC_RELATIVE_BYTE ? 1 : pattern->form == ENC_RELATIVE_NEAR ? 4 : 6;
            if (pattern->form == ENC_RELATIVE_BYTE && width != 1 && operation != MC_JCXZ) break;
            if (pattern->form == ENC_RELATIVE_NEAR && width >= 5) break;
            if (pattern->form == ENC_FAR_ADDRESS && width != 6) break;
            kind = KIND(left);
            if (kind == OPERAND_SYMBOL || (kind == 10 && pattern->form != ENC_FAR_ADDRESS)) {
                encode_operand(left, bits, 0);
                goto finished;
            }
            if (kind == OPERAND_LABEL_ADDRESS) {
                initialize_operand(&adjusted, OPERAND_SYMBOL, 0x1a, 0, left->value.address->symbol);
                adjusted.value.symbol->field04 += (unsigned)left->value.address->first.displacement;
                encode_operand(&adjusted, bits, 0);
                adjusted.value.symbol->field04 -= (unsigned)left->value.address->first.displacement;
                goto finished;
            }
            break;
        case ENC_SEGMENT_MOVE:
            reg = left->value.reg.words.number;
            if (KIND(left) == OPERAND_REGISTER && reg >= 8 && reg <= 13) {
                encode_modrm(instruction, right, reg & 7);
                *opcode |= 2;
                goto finished;
            }
            reg = right->value.reg.words.number;
            if (KIND(right) != OPERAND_REGISTER || reg < 8 || reg > 13) break;
            group = reg & 7;
            operand = left;
            goto modrm;
        case ENC_SEGMENT_STACK:
            reg = left->value.reg.words.number;
            if (KIND(left) != OPERAND_REGISTER || reg < 8 || reg > 13) break;
            *buffer = pattern->opcode;
            encoding_cursor = buffer + 1;
            opcode = buffer;
            if (reg < 12) *buffer |= (unsigned char)((reg & 3) << 3);
            else {
                *buffer = 0x0f;
                *encoding_cursor++ = (unsigned char)((0x81 - (operation == MC_PUSH)) | ((reg & 7) << 3));
            }
            goto finished;
        case ENC_SPECIAL:
            switch (operation) {
            case MC_JCC:
                if (width == 4) {
                    *opcode = 0x0f;
                    *encoding_cursor++ = (unsigned char)((right->flags & 15) | pattern[1].opcode);
                } else *opcode |= (unsigned char)(right->flags & 15);
                encode_operand(left, width, 0);
                goto finished;
            case MC_ENTER:
                encode_integer(left->value.constant, 2);
                encode_integer(right->value.constant, 1);
                goto finished;
            case MC_INT:
                if (left->value.constant != 3) {
                    *opcode |= 1;
                    *encoding_cursor++ = (unsigned char)left->value.constant;
                }
                goto finished;
            case MC_RET:
                if (width != 4) *opcode |= 8;
                if (!left->value.constant) *opcode |= 1;
                else encode_integer(left->value.constant, 2);
                goto finished;
            case MC_XCHG:
                if (width != 1) {
                    if (KIND(left) == OPERAND_REGISTER && left->value.reg.words.number == REG_EAX && KIND(right) == OPERAND_REGISTER) {
                        *opcode |= (unsigned char)right->value.reg.words.number;
                        goto finished;
                    }
                    if (KIND(right) == OPERAND_REGISTER && right->value.reg.words.number == REG_EAX && KIND(left) == OPERAND_REGISTER) {
                        *opcode = (unsigned char)left->value.reg.words.number | pattern->opcode;
                        goto finished;
                    }
                }
                break;
            case MC_IN: case MC_OUT:
                if (KIND(left) == OPERAND_IMMEDIATE) {
                    *opcode = pattern[1].opcode;
                    *encoding_cursor++ = (unsigned char)left->value.constant;
                }
                if (!FLOATING_OPERATION(operation) && width != 1) *opcode |= 1;
                goto finished;
            case MC_FST:
                if (width == 10) compiler_internal_error("@(#)code.c:1.60", 0x18f);
                /* Fall through to the register/extended-store alternatives. */
            case MC_FSTP:
                if (KIND(left) == OPERAND_REGISTER) {
                    *opcode |= 4;
                    group = (unsigned)((signed char)recipe->extension >> 3);
                    operand = left;
                    goto modrm;
                }
                /* Fall through to the 80-bit memory alternative. */
            case MC_FLD:
                if (width != 10 || (operation == MC_FLD && KIND(right) == OPERAND_REGISTER)) break;
                *opcode |= 2;
                operand = operation == MC_FLD ? right : left;
                group = (unsigned)((signed char)(recipe->extension | 0x28) >> 3);
                goto modrm;
            case MC_FILD: case MC_FISTP:
                if (width != 8) break;
                *opcode |= 4;
                operand = operation == MC_FILD ? right : left;
                group = (unsigned)((signed char)(recipe->extension | 0x28) >> 3);
                goto modrm;
            case MC_FSTSW: case MC_FNSTSW:
                if (KIND(left) != OPERAND_REGISTER) break;
                operand = left;
                group = 4;
                goto modrm;
            case MC_IMUL3:
                if (KIND(right) != OPERAND_BASED || width == 1) compiler_internal_error("@(#)code.c:1.60", 0x16f);
                address = right->value.address;
                encode_modrm(instruction, left, address->base.words.number);
                if (!address->symbol) {
                    initialize_operand(&adjusted, OPERAND_IMMEDIATE, 0x1a, (unsigned)address->first.displacement, 0);
                    bits = encode_operand(&adjusted, width, 1);
                } else bits = encode_operand(right, width, 1);
                *opcode |= (unsigned char)bits;
                goto finished;
            case MC_NOP:
                padding = padding_instructions[width];
                for (index = 0; index < width / 4; ++index)
                    ((unsigned *)opcode)[index] = ((const unsigned *)padding)[index];
                for (index = width & ~3U; index < width; ++index) opcode[index] = padding[index];
                encoding_cursor += width - 1;
                goto finished;
            case MC_DATA:
                encoding_cursor = prefix_end;
                if (KIND(left) == OPERAND_IMMEDIATE) encode_integer(left->value.constant, width);
                else if (KIND(left) == OPERAND_LABEL_ADDRESS) {
                    bits = coff_object && !byte_block_first ? left->value.address->symbol->field04 : 0;
                    encode_integer(bits, width);
                    if (encoding_emit_relocations)
                        output_callbacks.absolute_reference(0, left->value.address->symbol);
                }
                goto finished;
            case MC_SCC:
                *opcode |= (unsigned char)(right->flags & 15);
                goto modrm_form;
            case MC_IMUL2:
                kind = KIND(right);
                if (kind != OPERAND_IMMEDIATE && kind != OPERAND_LABEL_ADDRESS && kind != 7) break;
                first = *opcode;
                *prefix_end = first;
                encoding_cursor = prefix_end + 1;
                encode_modrm(instruction, left, left->value.reg.words.number);
                bits = encode_operand(right, width, 1);
                *prefix_end = first | (unsigned char)bits;
                opcode = prefix_end;
                goto finished;
            case MC_SHLD: case MC_SHRD:
                if (KIND(right) == OPERAND_REGISTER) {
                    encode_modrm(instruction, left, right->value.reg.words.number);
                    *opcode |= 1;
                } else {
                    if (KIND(right) != OPERAND_BASED) compiler_internal_error("@(#)code.c:1.60", 0x15f);
                    address = right->value.address;
                    encode_modrm(instruction, left, address->base.words.number);
                    *encoding_cursor++ = (unsigned char)address->first.displacement;
                }
                goto finished;
            case MC_BT: case MC_BTS: case MC_BTR: case MC_BTC:
                kind = KIND(right);
                if (kind != OPERAND_IMMEDIATE) {
                    if (kind != OPERAND_LABEL_ADDRESS && kind != 7) {
                        if (kind != OPERAND_REGISTER) compiler_internal_error("@(#)code.c:1.60", 0x1af);
                        ++pattern;
                        encoding_cursor[-1] = pattern->opcode;
                        goto modrm_right_register;
                    }
                    compiler_internal_error("@(#)code.c:1.60", 0x1ab);
                }
                encode_modrm(instruction, left, (unsigned)((signed char)recipe->extension >> 3));
                encode_integer(right->value.constant, 1);
                goto finished;
            case MC_MOVSR:
                if (KIND(left) == 11) {
                    if (KIND(right) != OPERAND_REGISTER) compiler_internal_error("@(#)code.c:1.60", 0x1b9);
                    *opcode |= 2;
                    special_register = left->value.reg.words.number;
                    operand = right;
                } else {
                    if (KIND(left) != OPERAND_REGISTER || KIND(right) != 11)
                        compiler_internal_error("@(#)code.c:1.60", 0x1bf);
                    special_register = right->value.reg.words.number;
                    operand = left;
                }
                if (special_register > 7) {
                    if (special_register < 16) { *opcode |= 1; special_register -= 8; }
                    else { *opcode |= 4; special_register -= 16; }
                }
                *encoding_cursor++ = (unsigned char)(((special_register | 0xf8U) << 3) | operand->value.constant);
                goto finished;
            default:
                compiler_internal_error("@(#)code.c:1.60", 0x201);
                goto finished;
            }
            break;
        case ENC_X87_ARITHMETIC:
            if (KIND(left) != OPERAND_REGISTER) compiler_internal_error("@(#)code.c:1.60", 0x217);
            operand = left;
            if (count == 1) group = (unsigned)((signed char)recipe->extension >> 3);
            else if (left->value.reg.words.number == REG_ST0 &&
                     (KIND(right) != OPERAND_REGISTER || right->value.reg.words.number != REG_ST0)) {
                if (pattern->modifiers & ENCODE_SIGN_EXTEND) compiler_internal_error("@(#)code.c:1.60", 0x222);
                if (KIND(right) != OPERAND_REGISTER && width == ((pattern->modifiers & ENCODE_DIRECTION) ? 2U : 8U))
                    *opcode |= 4;
                group = (unsigned)((signed char)recipe->extension >> 3);
                operand = right;
            } else {
                *opcode |= 4;
                if (KIND(right) != OPERAND_REGISTER) compiler_internal_error("@(#)code.c:1.60", 0x21e);
                group = pattern[1].opcode;
            }
            goto modrm;
        case ENC_X87_MEMORY:
            operand = KIND(left) == OPERAND_REGISTER ? right : left;
            if (KIND(operand) != OPERAND_REGISTER && width == ((pattern->modifiers & ENCODE_DIRECTION) ? 2U : 8U))
                *opcode |= 4;
            group = (unsigned)((signed char)recipe->extension >> 3);
            goto modrm;
        case ENC_MEMORY_GROUP:
            operand = left;
            group = (unsigned)((signed char)recipe->extension >> 3);
            goto modrm;
        case ENC_FIXED_SECOND:
            *encoding_cursor++ = recipe->extension;
            goto finished;
        case ENC_SHIFT:
            encode_modrm(instruction, left, (unsigned)((signed char)recipe->extension >> 3));
            if (KIND(right) == OPERAND_REGISTER) {
                if (right->value.reg.words.number != REG_ECX) compiler_internal_error("@(#)code.c:1.60", 0x20a);
                *opcode |= pattern[1].opcode;
            } else {
                if (KIND(right) != OPERAND_IMMEDIATE) compiler_internal_error("@(#)code.c:1.60", 0x20d);
                if (right->value.constant == 1) *opcode |= 0x10;
                else encode_integer(right->value.constant, 1);
            }
            goto finished;
        case ENC_ACCUMULATOR_IMMEDIATE:
            kind = KIND(right);
            if (KIND(left) == OPERAND_REGISTER && left->value.reg.words.number == REG_EAX &&
                (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS || kind == 7) &&
                (width != 4 || !(pattern->modifiers & ENCODE_SIGN_EXTEND) || kind != OPERAND_IMMEDIATE ||
                 !fits_signed_byte((int)right->value.constant))) {
                bits = encode_operand(right, width, 0);
                *opcode |= (unsigned char)bits;
                goto finished;
            }
            break;
        default:
            compiler_internal_error("@(#)code.c:1.60", 0x247);
            goto finished;
        }
        ++pattern;
    }
modrm:
    encode_modrm(instruction, operand, group);
finished:
    if (!FLOATING_OPERATION((instruction->opcode >> 8) & 255) &&
        (pattern->modifiers & ENCODE_WIDTH) && width != 1) *opcode |= 1;
    return (unsigned)(encoding_cursor - buffer);
}
