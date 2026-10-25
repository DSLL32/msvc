/* Operand-class grammar: count, permitted categories, register families and
   size encodings. The original encodes these fields in one sixteen-bit rule. */
#include "asm.h"
#include <recon.h>

#define ONE_OPERAND   0x4000
#define TWO_OPERANDS  0x8000
#define THREE_OPERANDS 0xc000
#define SIZE_BYTE   0x0100
#define SIZE_WORD   0x0200
#define SIZE_DWORD  0x0400
#define SIZE_QWORD  0x0800
#define SIZE_TBYTE  0x1000
#define SIZE_FWORD  0x2000
#define SEGMENT_ALLOWED 0x40
#define X87_OPERANDS 0x80
/* Bits0..5 select mem/mem,mem/reg,mem/imm,reg/mem,reg/reg,reg/imm. */
unsigned short asm_operand_class_rules[60] = {
    SIZE_BYTE | SIZE_WORD | SIZE_DWORD | SIZE_QWORD | SIZE_TBYTE | SIZE_FWORD, /* operand class 0 */
    SIZE_BYTE | SIZE_WORD | SIZE_DWORD | SIZE_QWORD | SIZE_TBYTE | SIZE_FWORD, /* operand class 1 */
    SIZE_BYTE | 0x01, /* operand class 2 */
    SIZE_BYTE | SIZE_WORD | SIZE_DWORD | SIZE_QWORD | SIZE_TBYTE | SIZE_FWORD, /* operand class 3 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x1a, /* operand class 4 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x3e, /* operand class 5 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x3e, /* operand class 6 */
    TWO_OPERANDS | SIZE_WORD | SIZE_DWORD | 0x08, /* operand class 7 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x36, /* operand class 8 */
    ONE_OPERAND | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x03, /* operand class 9 */
    SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x0b, /* operand class 10 */
    ONE_OPERAND | SIZE_WORD | SIZE_DWORD | SEGMENT_ALLOWED | 0x07, /* operand class 11 */
    ONE_OPERAND | SIZE_WORD | SIZE_DWORD | SEGMENT_ALLOWED | 0x03, /* operand class 12 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | SEGMENT_ALLOWED | 0x3e, /* operand class 13 */
    ONE_OPERAND | SIZE_DWORD | SIZE_FWORD | 0x07, /* operand class 14 */
    ONE_OPERAND | SIZE_BYTE | SIZE_DWORD | SIZE_FWORD | 0x07, /* operand class 15 */
    ONE_OPERAND | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x04, /* operand class 16 */
    SIZE_DWORD | SIZE_FWORD | 0x04, /* operand class 17 */
    ONE_OPERAND | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x03, /* operand class 18 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD, /* operand class 19 */
    THREE_OPERANDS | SIZE_WORD | SIZE_DWORD | 0x38, /* operand class 20 */
    ONE_OPERAND | SIZE_WORD | SIZE_FWORD | 0x03, /* operand class 21 */
    ONE_OPERAND | SIZE_WORD | 0x03, /* operand class 22 */
    TWO_OPERANDS | SIZE_WORD | SIZE_DWORD | 0x18, /* operand class 23 */
    SIZE_BYTE | SIZE_WORD | SIZE_DWORD | SIZE_QWORD | SIZE_TBYTE | SIZE_FWORD, /* operand class 24 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD | SIZE_DWORD, /* operand class 25 */
    ONE_OPERAND | SIZE_BYTE | 0x04, /* operand class 26 */
    ONE_OPERAND | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x04, /* operand class 27 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x30, /* operand class 28 */
    TWO_OPERANDS | SIZE_WORD | 0x12, /* operand class 29 */
    ONE_OPERAND | SIZE_BYTE | 0x03, /* operand class 30 */
    SIZE_TBYTE, /* operand class 31 */
    SIZE_TBYTE, /* operand class 32 */
    SIZE_TBYTE, /* operand class 33 */
    SIZE_TBYTE, /* operand class 34 */
    SIZE_TBYTE, /* operand class 35 */
    SIZE_WORD, /* operand class 36 */
    SIZE_WORD, /* operand class 37 */
    ONE_OPERAND | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | SIZE_QWORD | SIZE_TBYTE | SIZE_FWORD | 0x03, /* operand class 38 */
    ONE_OPERAND | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | SIZE_QWORD | SIZE_TBYTE | SIZE_FWORD | 0x03, /* operand class 39 */
    ONE_OPERAND | SIZE_QWORD | X87_OPERANDS | 0x02, /* operand class 40 */
    ONE_OPERAND | SIZE_DWORD | SIZE_QWORD | SIZE_TBYTE | X87_OPERANDS | 0x03, /* operand class 41 */
    ONE_OPERAND | SIZE_QWORD | 0x01, /* operand class 42 */
    SIZE_DWORD | SIZE_QWORD | X87_OPERANDS | 0x13, /* operand class 43 */
    TWO_OPERANDS | SIZE_QWORD | X87_OPERANDS | 0x10, /* operand class 44 */
    SIZE_TBYTE, /* operand class 45 */
    0, /* operand class 46 */
    0, /* operand class 47 */
    0, /* operand class 48 */
    TWO_OPERANDS | SIZE_WORD | SIZE_DWORD | 0x08, /* operand class 49 */
    THREE_OPERANDS | SIZE_WORD | SIZE_DWORD | 0x12, /* operand class 50 */
    TWO_OPERANDS | SIZE_WORD | SIZE_DWORD | 0x18, /* operand class 51 */
    TWO_OPERANDS | SIZE_WORD | SIZE_DWORD | 0x36, /* operand class 52 */
    TWO_OPERANDS | SIZE_DWORD | 0x10, /* class53 */
    ONE_OPERAND | SIZE_DWORD | 0x02, /* class54 */
    TWO_OPERANDS | SIZE_BYTE | SIZE_WORD | SIZE_DWORD | 0x12, /* class55 */
    ONE_OPERAND | SIZE_DWORD | 0x01, /* class56 */
    ONE_OPERAND | SIZE_WORD | SIZE_DWORD | X87_OPERANDS | 0x01, /* class57 */
    ONE_OPERAND | SIZE_WORD | SIZE_DWORD | SIZE_QWORD | X87_OPERANDS | 0x01, /* class58 */
    ONE_OPERAND | SIZE_TBYTE | 0x01, /* class59 */
};

/* Modes here describe the flags' common operand shape. Address preparation
   may further refine that shape for index/scale and displacement widths. */
AsmAddressRule asm_address_rules[32] = {
    { 0, ASM_INVALID }, /* address flags 0x00 */
    { 8, ASM_IMMEDIATE }, /* address flags 0x01 */
    { 2, ASM_IMMEDIATE }, /* address flags 0x02 */
    { 8, ASM_IMMEDIATE }, /* address flags 0x03 */
    { 1, ASM_REGISTER }, /* address flags 0x04 */
    { 0, ASM_INVALID }, /* address flags 0x05 */
    { 4, ASM_MEMORY }, /* address flags 0x06 */
    { 0, ASM_INVALID }, /* address flags 0x07 */
    { 0, ASM_INVALID }, /* address flags 0x08 */
    { 9, ASM_IMMEDIATE }, /* address flags 0x09 */
    { 0, ASM_INVALID }, /* address flags 0x0a */
    { 9, ASM_IMMEDIATE }, /* address flags 0x0b */
    { 0, ASM_INVALID }, /* address flags 0x0c */
    { 0, ASM_INVALID }, /* address flags 0x0d */
    { 0, ASM_INVALID }, /* address flags 0x0e */
    { 0, ASM_INVALID }, /* address flags 0x0f */
    { 0, ASM_INVALID }, /* address flags 0x10 */
    { 0, ASM_INVALID }, /* address flags 0x11 */
    { 3, ASM_MEMORY }, /* address flags 0x12 */
    { 3, ASM_MEMORY }, /* address flags 0x13 */
    { 1, ASM_REGISTER }, /* address flags 0x14 */
    { 5, ASM_MEMORY }, /* address flags 0x15 */
    { 4, ASM_MEMORY }, /* address flags 0x16 */
    { 5, ASM_MEMORY }, /* address flags 0x17 */
    { 0, ASM_INVALID }, /* address flags 0x18 */
    { 9, ASM_IMMEDIATE }, /* address flags 0x19 */
    { 0, ASM_INVALID }, /* address flags 0x1a */
    { 9, ASM_IMMEDIATE }, /* address flags 0x1b */
    { 0, ASM_INVALID }, /* address flags 0x1c */
    { 0, ASM_INVALID }, /* address flags 0x1d */
    { 0, ASM_INVALID }, /* address flags 0x1e */
    { 0, ASM_INVALID }, /* address flags 0x1f */
};

/* Original 0x7efa6..0x7f15b. Most classes specify an exact count; string,
   multiply and x87 families admit instruction-specific alternate forms. */
int validate_asm_operand_count(void)
{
    if ((asm_operand_class_rules[asm_operand_class] >> 14) == (unsigned short)asm_operand_count)
        return 1;
    if (asm_operand_class > 17) {
        if (asm_operand_class == 40) {
            if (!asm_operand_count && asm_operation == 95) return 1;
        } else if (asm_operand_class == 43) goto variable_count;
        return 0;
    }
    if (asm_operand_class == 17) return asm_operand_count == 1;
    if (asm_operand_class == 2) return asm_operation == 60 && asm_operand_count == 1;
    if (asm_operand_class != 9) {
        if (asm_operand_class != 10 || asm_opcode != asm_operation) return 0;
        if (asm_operation == 57 || asm_operation == 53) return asm_operand_count == 2;
    variable_count:
        if (asm_operation != 55 && asm_operation != 58 && asm_operation != 59 &&
            asm_operation != 92 && asm_operation != 82 && asm_operation != 93)
            return asm_operand_count < 3;
        return asm_operand_count == 1;
    }
    if (asm_operation != 6 || !asm_operand_count) return 0;
    if (asm_operand_count == 2) {
        if (ASM_OPERAND_CATEGORY(asm_operands[1].address) != ASM_IMMEDIATE) {
            asm_operation = 193;
            goto selected_multiply;
        }
    } else if (asm_operand_count != 3) goto selected_multiply;
    asm_operation = 163;
selected_multiply:
    asm_operand_class = asm_instruction_properties[asm_operation].operand_class;
    return 1;
}

/* Original 0x7f1dc..0x7f82d; class dispatch table 0x7f82e..0x7f8e1.
   Category-pair bits use radix three: memory=0, register=1, immediate=2. */
int validate_asm_operands(void)
{
    AsmAddress *first = asm_operands[0].address;
    AsmAddress *second = asm_operands[1].address;
    AsmAddress *third = asm_operands[2].address;
    AsmAddress *source, *destination;
    unsigned char first_flags = first->address_flags, segment_count, flags;
    unsigned pair = ASM_OPERAND_CATEGORY(first), mask, register_pairs;
    unsigned short family;
    if (asm_operand_count > 1) pair = pair * 3 + ASM_OPERAND_CATEGORY(second);
    mask = 1U << (pair & 31);
    if (pair < 6) {
        if (!(mask & asm_operand_class_rules[asm_operand_class])) return 0;
    } else if (asm_operation != 11 && asm_operation != 5 && asm_operation != 4) return 0;
    register_pairs = mask & 0x3a;
    if (register_pairs) {
        segment_count = asm_operand_count > 1 &&
            (asm_register_properties[second->first_register] & 0xc0) == 0x40;
        segment_count += (asm_register_properties[first->first_register] & 0xc0) == 0x40;
        if (segment_count && (segment_count > 1 ||
            (asm_operand_class_rules[asm_operand_class] & 0xc0) != 0x40)) return 0;
    }
    if ((asm_operand_class_rules[asm_operand_class] & 0xc0) == 0x80) {
        if (register_pairs && !(first_flags & 0x34)) return 0;
        if ((mask & 0x10) && !(second->address_flags & 0x34)) return 0;
    }
    switch (asm_operand_class) {
    case 8:
        if (!(mask & 0x24)) {
            if (second->first_register != 18) return 0;
        } else {
            if (target_cpu_mask == 1 && second->displacement != 1) return 0;
            if ((second->address_flags & 1) || !fits_asm_integer(8,second->displacement)) return 0;
        }
        break;
    case 10:
        if (asm_operation == 57 || asm_operation == 53) {
            source = asm_operands[1].address;
            destination = asm_operands[0].address;
        } else {
            source = asm_operands[0].address;
            destination = asm_operands[1].address;
        }
        if (asm_operation != 57 && asm_operation != 55) {
            flags = source->flags & 63;
            if (flags) {
                if (flags != 25) return 0;
                source->flags &= 0xc0;
            }
        }
        if ((source->flags & 63) == 28) source->flags &= 0xc0;
        if (asm_operand_count == 2) {
            if ((destination->flags & 63) == 28) destination->flags &= 0xc0;
            if ((asm_operation == 57 || asm_operation == 54) && destination->first_register != 11) return 0;
        }
        break;
    case 11:
        if (target_cpu_mask == 1 && pair == 2) return 0;
        break;
    case 13:
        family = asm_register_properties[first->first_register] & 0xc0;
        if (family == 0xc0 || (asm_register_properties[second->first_register] & 0xc0) == 0xc0) {
            if (pair == 4 && (family != 0xc0 ||
                (asm_register_properties[second->first_register] & 0xc0) != 0xc0)) {
                asm_operand_class = asm_instruction_properties[208].operand_class;
                asm_operation = 208;
                return 1;
            }
            return 0;
        }
        if (pair == 5 && family == 0x40) return 0;
        /* MOV also observes the segment destination restriction. */
    case 12:
        if (register_pairs && first->first_register == 26) return 0;
        break;
    case 14: case 15:
        if (pair == 2 && !(first_flags & 1)) return 0;
        break;
    case 19:
        if (pair == 8) return fits_asm_integer(8,second->displacement);
        return 0;
    case 20:
        if (target_cpu_mask == 1 || !register_pairs) return 0;
        if (target_cpu_mask != 8 && ((asm_operand_count == 2) != (pair == 5))) return 0;
        break;
    case 25:
        if (ASM_OPERAND_CATEGORY(first) != ASM_IMMEDIATE ||
            !fits_asm_integer(6,first->displacement)) return 0;
        if (pair == 8 || (pair == 7 &&
            (asm_register_properties[second->first_register] & 0xc0) == 0x40)) return 0;
        break;
    case 26:
        return fits_asm_integer(8,first->displacement);
    case 27:
        if (!first->displacement) return (first_flags & 1) != 0;
        return 0;
    case 28:
        if (asm_operation == 11) {
            source = asm_operands[1].address;
            destination = asm_operands[0].address;
        } else {
            source = asm_operands[0].address;
            destination = asm_operands[1].address;
        }
        if (ASM_OPERAND_CATEGORY(source) != ASM_REGISTER ||
            !(asm_register_properties[source->first_register] & 0x1000)) return 0;
        if ((ASM_OPERAND_CATEGORY(destination) != ASM_IMMEDIATE ||
             !fits_asm_integer(8,destination->displacement)) &&
            (ASM_OPERAND_CATEGORY(destination) != ASM_REGISTER || destination->first_register != 11)) return 0;
        break;
    case 38: case 39:
        if ((asm_operation != 110 && asm_operation != 108) || first->first_register != 9)
            return pair == 0;
        break;
    case 43: case 44:
        if (asm_operation == 92 || asm_operation == 93 || asm_operation == 209 || asm_operation == 210) {
            if (pair == 4) return 0;
        } else if (asm_operand_count == 2) {
            if (pair != 4) return 0;
            if (second->first_register && (asm_operand_class == 44 || first->first_register)) return 0;
        } else if (pair) return 0;
        break;
    case 50:
        if ((ASM_OPERAND_CATEGORY(third) != ASM_IMMEDIATE ||
             !fits_asm_integer(8,third->displacement)) &&
            (ASM_OPERAND_CATEGORY(third) != ASM_REGISTER || third->first_register != 18)) return 0;
        break;
    case 52:
        if (ASM_OPERAND_CATEGORY(second) == ASM_IMMEDIATE &&
            ((second->address_flags & 1) || !fits_asm_integer(8,second->displacement))) return 0;
        break;
    }
    return 1;
}

/* Original0x7f15c..0x7f17f, stdcall8. The x86 shift masks its count;
   the signed comparison deliberately retains wrapped ABS(INT_MIN). */
int __stdcall fits_asm_integer(unsigned char bits, unsigned value)
{
    unsigned magnitude = (int)value < 0 ? 0U - value : value;
    return (int)(1U << (bits & 31)) > (int)magnitude;
}
