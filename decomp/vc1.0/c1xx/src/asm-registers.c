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
