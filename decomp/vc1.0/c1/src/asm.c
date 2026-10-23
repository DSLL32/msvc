/* Inline assembler entry, operands and instruction state. */
#include "asm.h"
#include "ir.h"
#include <recon.h>

int asm_skip_initial_newlines; /* original 0x5b144 */

/* Original 0x4e40c..0x4e448. Assembly uses a separate yacc parser,
   while sharing the C scanner, semantic value and source-record writer. */
void parse_asm_statement(void)
{
    scanner_flags |= 0x00400000UL;
    function_symbol->attributes |= 0x1000;
    record_line_information(0x19);
    initialize_inline_assembler();
    asm_skip_initial_newlines = 1;
    parse_inline_assembler();
    scanner_flags &= 0xffbfffffUL;
    record_line_information(0x1a);
}

AsmOperand *asm_operand; /* original 0x5b000 */
int asm_brackets_used; /* original 0x5b004 */
short asm_operation, asm_opcode; /* original 0x5b018 / 0x5b058 */
int asm_member_mode, asm_operand_count; /* original 0x5b01c / 0x5b024 */
unsigned short asm_code_alignment; /* original 0x5b020 */
unsigned asm_instruction_flags; /* original 0x5b128 */
int asm_operand_mode; /* original 0x5b12c */

/* Original 0x4dd5f..0x4ddaf. Instruction-local parser state is reset
   after each statement; operand zero becomes the active result descriptor. */
void initialize_inline_assembler(void)
{
    asm_operation = 0;
    asm_opcode = 0;
    asm_operand_count = 0;
    asm_operand_mode = 0;
    asm_member_mode = 0;
    asm_brackets_used = 0;
    asm_instruction_flags = 0;
    asm_operand = reset_asm_operand(0);
}

AsmOperand asm_operands[3]; /* original 0x5b028..0x5b058 */
AsmAddress asm_addresses[3]; /* original 0x5b074..0x5b098 */
FrontendSymbol *asm_member_context; /* original 0x5b130 */
typedef char AsmOperand_size[(sizeof(AsmOperand) == 16) ? 1 : -1];
typedef char AsmAddress_size[(sizeof(AsmAddress) == 12) ? 1 : -1];

/* Original 0x4ddb0..0x4de18. Preserve the untouched reserved halfword;
   each operand owns its fixed companion descriptor instead of an allocation. */
AsmOperand *__stdcall reset_asm_operand(int index)
{
    AsmOperand *operand = &asm_operands[index];
    AsmAddress *address = &asm_addresses[index];
    asm_member_context = 0;
    operand->size = 15;
    operand->flags = 0;
    operand->address = address;
    address->symbol = 0;
    address->displacement = 0;
    address->address_flags = 0;
    address->flags &= 0xc0;
    address->first_register = 0;
    address->second_register = 0;
    address->flags &= 0x3f;
    operand->register_symbol = 0;
    operand->frame_symbol = 0;
    return operand;
}

int asm_open_braces; /* original 0x5b1e0 */
int asm_address_mode; /* original 0x5b010 */
int asm_explicit_size; /* original 0x5b134 */
/* Original0x72e70..0x72e79. Operand-size encodings for the data and
   code-type keywords WORD..FAR, including the distinct six-byte FWORD code. */
unsigned char asm_operand_size_codes[9] = {
    1, /* WORD, ID46 */
    2, /* DWORD, ID47 */
    5, /* FWORD, ID48 */
    3, /* QWORD, ID49 */
    4, /* TBYTE, ID50 */
    0, /* BYTE, ID51 */
    0, /* SHORT, ID52 */
    2, /* NEAR, ID53 */
    5  /* FAR, ID54 */
};

/* Original 0x4dfd8..0x4e01e; dispatch table 0x4e01f..0x4e046.
   Values 0xffff/0xfffe denote near/far addresses rather than storage bytes. */
unsigned char __stdcall asm_size_encoding(unsigned bytes)
{
    if ((int)bytes < 11) {
        switch (bytes) {
        case 1: return 0;
        case 2: return 1;
        case 4: return 2;
        case 6: return 5;
        case 8: return 3;
        case 10: return 4;
        default: return 15;
        }
    }
    if (bytes == 65535) return 2;
    if (bytes == 65534) return 5;
    return 15;
}

/* Original 0x4e274..0x4e28f. */
void __stdcall add_asm_displacement(unsigned value)
{
    asm_operand->address->displacement += value;
    asm_operand->address->address_flags |= 2;
}

extern FrontendSymbol *translation_sentinel;

/* Original0x4de77..0x4defb. Explicit PTR size supersedes a symbol's size,
   but duplicate/invalid specifications retain the original diagnostics. */
void __stdcall set_asm_operand_size(unsigned char size)
{
    AsmAddress *address;
    char *context;
    if ((asm_operand->flags & 1) || (asm_operand->size = size) == 15) {
        context = asm_error_context();
        report_syntax_error(408,context);
    }
    if (!asm_address_mode) {
        address = asm_operand->address;
        if (address->symbol) {
            if (address->symbol == translation_sentinel && !(address->flags & 0x3f))
                address->flags = (address->flags & 0xda) | 0x1a;
            asm_operand->address->address_flags &= 0xef;
            asm_operand->address->address_flags |= 0x10;
        }
    }
    asm_operand->flags |= 1;
}

/* Original0x4e324..0x4e3f2. Diagnostics own a temporary aligned arena
   string, optionally followed by the one-based operand number. */
char *asm_error_context(void)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    int operand = asm_operand_mode != 0;
    char *text = message_text(operand ? 322 : 323);
    char *scan = text, *result, *end;
    unsigned bytes = 1;
    while (*scan++) ++bytes;
    bytes = (bytes + (operand ? 5 : 3)) & ~3U;
    if (pool->remaining < (int)bytes) refill_arena(&compiler_arena,pool);
    result = pool->next;
    pool->remaining -= bytes;
    pool->next += bytes;
    end = copy_asm_context_text(result,text);
    if (operand) {
        *end++ = ' ';
        *end++ = (char)(asm_operand_count + '1');
    }
    *end = 0;
    return result;
}

/* Original0x4e3f2..0x4e40c, RET8. Return the copied terminator so
   the diagnostic caller can append its operand suffix without rescanning. */
char *__stdcall copy_asm_context_text(char *destination, char *source)
{
    char *cursor = destination;
    do { *cursor++ = *source++; } while (cursor[-1]);
    return cursor - 1;
}
