/* Inline assembler statement boundary and parser state. */
#include "asm.h"
#include "ir.h"
#include "source-input.h"
#include <recon.h>

int asm_skip_initial_newlines; /* original 0x8c144 */

/* Original 0x7df38..0x7df74, cdecl. The assembler has its own
   parser but shares source-line control records and the C++ function symbol. */
void emit_inline_assembly(void)
{
    token_allocation_flags |= 0x00400000UL;
    function_symbol->attributes |= 0x1000;
    record_line_information(25);
    initialize_inline_assembler();
    asm_skip_initial_newlines = 1;
    parse_inline_assembler();
    token_allocation_flags &= ~0x00400000UL;
    record_line_information(26);
}

AsmOperand *asm_operand; /* original 0x8c000 */
int asm_brackets_used; /* original 0x8c004 */
short asm_operation, asm_opcode; /* original 0x8c018/0x8c058 */
int asm_member_mode; /* original 0x8c01c */
int asm_operand_count; /* original 0x8c024 */
unsigned asm_instruction_flags; /* original 0x8c128 */
int asm_operand_mode; /* original 0x8c12c */

/* Original 0x7d7fe..0x7d84e, cdecl. Start with operand zero
   and clear instruction-local state before the separate assembler parser runs. */
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

/* Three operand slots: the parser's comma action increments only below two
   (original 0x7c8fc), and instruction rewriting uses slots zero through two.
   These arrays stop before the independent opcode and following state. */
AsmOperand asm_operands[3]; /* original 0x8c028..0x8c058 */
AsmAddress asm_addresses[3]; /* original 0x8c074..0x8c098 */
CppSymbol *asm_member_context; /* original 0x8c130 */
typedef char AsmOperand_size[(sizeof(AsmOperand) == 16) ? 1 : -1];
typedef char AsmAddress_size[(sizeof(AsmAddress) == 12) ? 1 : -1];

/* Original 0x7d84f..0x7d8b7, stdcall4. The reserved operand
   halfword is untouched; each fixed operand has its own address descriptor. */
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

int asm_open_braces; /* original 0x8c1e0 */
int asm_address_mode; /* original 0x8c00c, separate from instruction class 0x8c010 */
int asm_explicit_size; /* original 0x8c134 */
/* WORD..FAR logical storage encodings, original 0xa9ef0..0xa9ef8;
   parser instructions use the biased base 0xa9ec2 plus keyword IDs46..54. */
unsigned char asm_operand_size_codes[9] = { 1, 2, 5, 3, 4, 0, 0, 2, 5 };

/* Original 0x7da77..0x7dabd, stdcall4, byte result; its ten-entry
   switch is 0x7dabe..0x7dae5. Near/far sentinels encode address sizes. */
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

/* Original 0x7dd61..0x7dd7c, stdcall4. */
void __stdcall add_asm_displacement(unsigned value)
{
    asm_operand->address->displacement += value;
    asm_operand->address->address_flags |= 2;
}
