/* Register operands and their x86 addressing properties. */
#include "asm.h"
#include <recon.h>

#define REG_BYTE  0x0000
#define REG_WORD  0x0100
#define REG_DWORD 0x0200
#define REG_EXTENDED 0x0300
#define REG_ACCUMULATOR 0x1000
#define REG_BASE 0x2000
#define REG_INDEX 0x4000
#define REG_SEGMENT 0x40
#define REG_X87 0x80
#define REG_SYSTEM 0xc0
unsigned short asm_register_properties[46] = {
    0x3f, /* nil */
    REG_DWORD|REG_ACCUMULATOR|REG_BASE|REG_INDEX|0, /* eax */
    REG_DWORD|REG_BASE|REG_INDEX|1, /* ecx */
    REG_DWORD|REG_BASE|REG_INDEX|2, /* edx */
    REG_DWORD|REG_BASE|REG_INDEX|3, /* ebx */
    REG_DWORD|REG_BASE|4, /* esp */
    REG_DWORD|REG_BASE|REG_INDEX|5, /* ebp */
    REG_DWORD|REG_BASE|REG_INDEX|6, /* esi */
    REG_DWORD|REG_BASE|REG_INDEX|7, /* edi */
    REG_WORD|REG_ACCUMULATOR|0, /* ax */
    REG_WORD|1, /* cx */ REG_WORD|2, /* dx */
    REG_WORD|REG_BASE|3, /* bx */ REG_WORD|4, /* sp */
    REG_WORD|REG_BASE|5, /* bp */
    REG_WORD|REG_INDEX|6, /* si */ REG_WORD|REG_INDEX|7, /* di */
    REG_BYTE|REG_ACCUMULATOR|0, /* al */ REG_BYTE|1, /* cl */
    REG_BYTE|2, /* dl */ REG_BYTE|3, /* bl */ REG_BYTE|4, /* ah */
    REG_BYTE|5, /* ch */ REG_BYTE|6, /* dh */ REG_BYTE|7, /* bh */
    REG_WORD|REG_SEGMENT|8, /* es */ REG_WORD|REG_SEGMENT|9, /* cs */
    REG_WORD|REG_SEGMENT|10, /* ss */ REG_WORD|REG_SEGMENT|11, /* ds */
    REG_WORD|REG_SEGMENT|12, /* fs */ REG_WORD|REG_SEGMENT|13, /* gs */
    REG_EXTENDED|REG_X87, /* st */
    REG_DWORD|REG_SYSTEM|0, /* cr0 */ REG_DWORD|REG_SYSTEM|2, /* cr2 */
    REG_DWORD|REG_SYSTEM|3, /* cr3 */ REG_DWORD|REG_SYSTEM|8, /* dr0 */
    REG_DWORD|REG_SYSTEM|9, /* dr1 */ REG_DWORD|REG_SYSTEM|10, /* dr2 */
    REG_DWORD|REG_SYSTEM|11, /* dr3 */ REG_DWORD|REG_SYSTEM|14, /* dr6 */
    REG_DWORD|REG_SYSTEM|15, /* dr7 */ REG_DWORD|REG_SYSTEM|19, /* tr3 */
    REG_DWORD|REG_SYSTEM|20, /* tr4 */ REG_DWORD|REG_SYSTEM|21, /* tr5 */
    REG_DWORD|REG_SYSTEM|22, /* tr6 */ REG_DWORD|REG_SYSTEM|23 /* tr7 */
};

/* Original 0x80311..0x803f7. Bracketed registers contribute an address;
   bare registers create an already-encoded operand expression. */
AsmExpression *__stdcall make_asm_register(short reg)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *expression;
    unsigned short properties;
    char *context;
    if (asm_brackets_used) return make_asm_address_register(reg);
    properties = asm_register_properties[reg] & 0xc0;
    if (properties == REG_X87) return make_asm_stack_register(reg,0);
    if (properties == REG_SYSTEM &&
        (asm_operation != 1 || (!options.configuration.option_G4 && (reg == 41 || reg == 42 || reg == 43)))) {
        context = asm_error_context();
        report_syntax_error(404,interned_name,context);
    }
    asm_operand->address->address_flags = 4;
    asm_operand->address->first_register = (unsigned char)reg;
    asm_operand->size = (asm_register_properties[reg] >> 8) & 15;
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    expression = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    expression->kind = 3;
    expression->field08 = (unsigned)reg;
    return expression;
}

/* Original 0x801c5..0x80310. A bracketed register occupies one of the
   address's two slots; the width and base/index flags distinguish x86 modes. */
AsmExpression *__stdcall make_asm_address_register(short reg)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *expression;
    AsmAddress *address;
    unsigned short properties;
    unsigned diagnostic = 0;
    char *context;
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    expression = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    expression->kind = 0;
    expression->value.integer = 0;
    address = asm_operand->address;
    if (address->first_register && address->second_register) diagnostic = 404;
    else {
        properties = asm_register_properties[reg];
        if (!(properties & 0xc0) && ((properties >> 8) & 15) == 2) {
            if (!address->first_register) address->first_register = (unsigned char)reg;
            else address->second_register = (unsigned char)reg;
            asm_operand->address->address_flags &= 0xef;
            asm_operand->address->address_flags |= 0x16;
            expression->kind = 1;
        } else if (!(properties & REG_BASE) && !(properties & REG_INDEX)) diagnostic = 403;
        else if ((properties & REG_BASE) && address->first_register) diagnostic = 402;
        else if ((properties & REG_INDEX) && address->second_register) diagnostic = 401;
        else diagnostic = 432;
    }
    if (diagnostic) {
        context = asm_error_context();
        if (diagnostic == 432) report_syntax_error(diagnostic,context);
        else report_syntax_error(diagnostic,interned_name,context);
    }
    return expression;
}

/* Original 0x7e672..0x7e782. Replace parser register IDs with backend x86
   encodings; arrange base/index slots and preserve the index scale bits. */
void normalize_asm_addresses(void)
{
    int index;
    AsmOperand *operand;
    AsmAddress *address;
    for (index = 0; index < asm_operand_count; ++index) {
        operand = &asm_operands[index];
        if ((operand->flags & 1) && ASM_OPERAND_CATEGORY(operand->address) == ASM_REGISTER)
            report_asm_operand_error(421,index);
        address = operand->address;
        if (address->second_register == 5) {
            if (!(address->flags & 0xc0)) {
                if (address->first_register == 5) report_asm_operand_error(431,index);
                else {
                    address->second_register = address->first_register;
                    operand->address->first_register = 5;
                }
            } else report_asm_operand_error(431,index);
        }
        address = operand->address;
        address->flags = (address->flags & 0xc0) |
            (asm_register_properties[address->flags & 63] & 63);
        operand->address->second_register =
            (unsigned char)asm_register_properties[operand->address->second_register];
        address = operand->address;
        if (!(address->address_flags & 0x20)) {
            if (!address->first_register) {
                if (!(address->flags & 0xc0)) {
                    address->first_register = address->second_register;
                    operand->address->second_register = (unsigned char)asm_register_properties[0];
                } else address->first_register = (unsigned char)asm_register_properties[0];
            } else address->first_register = (unsigned char)asm_register_properties[address->first_register];
        } else if (!options.configuration.option_qc) address->first_register += 40;
    }
}

/* Original0x803f8..0x804ce, stdcall8. X87 stack-register operands encode the stack
   index in the address, while their expression retains the register class. */
AsmExpression *__stdcall make_asm_stack_register(short reg, int index)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *expression;
    char *context;
    if ((asm_operand_class_rules[asm_operand_class] & 0xc0) != 0x80 ||
        reg != 31 || index > 7 || index < 0) {
        context = asm_error_context();
        if (reg != 31) {
            report_syntax_error(407,context);
            return make_asm_register(reg);
        }
        report_syntax_error(404,"ST",context);
        reg = 0;
    }
    asm_operand->address->address_flags = 0x34;
    asm_operand->address->first_register = (unsigned char)index;
    asm_operand->size = (asm_register_properties[reg] >> 8) & 15;
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    expression = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    expression->kind = 3;
    expression->field08 = (unsigned)(int)reg;
    return expression;
}

/* Original0x80072..0x800ea, stdcall8. Store the parser's segment
   ID before normalize_asm_addresses later replaces it by its encoding. */
AsmExpression *__stdcall apply_asm_segment(short segment, AsmExpression *value)
{
    AsmAddress *address;
    char *context;
    if ((asm_register_properties[segment] & 0xc0) == REG_SEGMENT &&
        !(asm_operand->address->flags & 0x3f) && !asm_brackets_used && value->kind != 3) {
        address = asm_operand->address;
        address->flags = (unsigned char)((segment & 0x3f) | (address->flags & 0xc0));
        asm_operand->address->address_flags &= 0xef;
        asm_operand->address->address_flags |= 0x10;
    } else {
        context = asm_error_context();
        report_syntax_error(422,context);
    }
    return value;
}
