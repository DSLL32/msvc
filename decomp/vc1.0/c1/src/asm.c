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
void *asm_member_context; /* original 0x5b130 */
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
/* Operand-size encodings indexed by the assembler's word..byte type IDs.
   They are encodings, including the distinct six-byte FWORD code. */
unsigned char asm_data_size_codes[6] = {
    1, /* WORD, ID46 */
    2, /* DWORD, ID47 */
    5, /* FWORD, ID48 */
    3, /* QWORD, ID49 */
    4, /* TBYTE, ID50 */
    0  /* BYTE, ID51 */
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
