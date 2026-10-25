#ifndef INLINE_ASSEMBLER_H
#define INLINE_ASSEMBLER_H
#include "parser.h"
#include "asm-grammar.h"
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
/* Assembler expression nodes preserve their twelve-byte common prefix. */
typedef struct AsmExpression {
    short kind;
    unsigned short flags;
    union { unsigned integer; CppSymbol *symbol; char *text; } value;
    unsigned field08;
} AsmExpression;
extern int asm_open_braces, asm_address_mode, asm_explicit_size;
extern unsigned char asm_operand_size_codes[9];
extern char *asm_token_names[50];
void __stdcall report_asm_expected_token(int actual, int expected);
RECON_RETURN8_TYPE(ParserValue) __stdcall asm_recovery_value(int token);
void __stdcall report_asm_parser_error(char *text);
int finish_asm_line(void);
unsigned char read_asm_token(void);
void __stdcall emit_asm_label(char *name);
void __stdcall set_asm_directive(short directive, unsigned value);
void __stdcall select_asm_opcode(short opcode);
void emit_asm_instruction(void);
void __stdcall add_asm_displacement(unsigned value);
AsmExpression *__stdcall multiply_asm_expression(unsigned short operation, AsmExpression *left, AsmExpression *right);
AsmExpression *__stdcall evaluate_asm_binary(unsigned short operation, AsmExpression *left, AsmExpression *right);
AsmExpression *__stdcall combine_asm_addresses(unsigned short operation, AsmExpression *left, AsmExpression *right);
void __stdcall set_asm_address_operator(short operation);
char *asm_error_context(void);
CppSymbol *__stdcall lookup_asm_symbol(char *name, int allow_label);
AsmExpression *__stdcall evaluate_asm_unary(short operation, AsmExpression *value);
AsmExpression *__stdcall apply_asm_segment(short segment, AsmExpression *value);
unsigned __stdcall asm_symbol_type_size(CppSymbol *symbol);
unsigned __stdcall asm_symbol_length(CppSymbol *symbol);
unsigned __stdcall asm_member_offset(char *name);
AsmExpression *__stdcall make_asm_constant(unsigned value);
AsmExpression *__stdcall make_asm_symbol(char *name);
AsmExpression *__stdcall make_asm_segment_value(AsmExpression *value);
AsmExpression *__stdcall make_asm_offset(AsmExpression *value);
AsmExpression *__stdcall make_asm_register(short reg);
AsmExpression *__stdcall make_asm_stack_register(short reg, int index);
void __stdcall set_asm_operand_size(unsigned char size);
unsigned char __stdcall asm_size_encoding(unsigned bytes);
#endif
