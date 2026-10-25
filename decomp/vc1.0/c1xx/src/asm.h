#ifndef INLINE_ASSEMBLER_H
#define INLINE_ASSEMBLER_H
#include "parser.h"
#include <recon.h>
typedef struct AsmAddress {
    CppSymbol *symbol;
    unsigned displacement;
    unsigned char address_flags, flags, first_register, second_register;
} AsmAddress;
typedef struct AsmOperand {
    AsmAddress *address;
    unsigned char size, flags;
    unsigned short reserved;
    CppSymbol *register_symbol, *frame_symbol;
} AsmOperand;
extern AsmOperand asm_operands[3];
extern AsmAddress asm_addresses[3];
extern CppSymbol *asm_member_context;
extern AsmOperand *asm_operand;
extern int asm_skip_initial_newlines;
extern int asm_brackets_used, asm_member_mode, asm_operand_count, asm_operand_mode;
extern short asm_operation, asm_opcode;
extern unsigned asm_instruction_flags;
AsmOperand *__stdcall reset_asm_operand(int index);
void initialize_inline_assembler(void);
int parse_inline_assembler(void);
#endif
