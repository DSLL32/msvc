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
typedef struct AsmOpcodeAlias { unsigned char opcode, modifier; } AsmOpcodeAlias;
extern AsmOpcodeAlias asm_opcode_aliases[98];
unsigned char classify_asm_identifier(void);
short __stdcall find_asm_keyword(char *name);
short __stdcall find_asm_mnemonic(char *name);
void copy_asm_comment(void);
extern unsigned char asm_current_token, asm_token_classes[213];
void __stdcall emit_asm_label(char *name);
void __stdcall set_asm_directive(short directive, unsigned value);
typedef struct AsmInstructionProperties {
    int operand_class;
    unsigned char processors;
} AsmInstructionProperties;
extern AsmInstructionProperties asm_instruction_properties[230];
extern int asm_operand_class;
void __stdcall select_asm_opcode(short opcode);
void emit_asm_instruction(void);
extern unsigned short asm_operand_class_rules[60];
enum AsmOperandCategory {
    ASM_MEMORY, ASM_REGISTER, ASM_IMMEDIATE, ASM_INVALID
};
typedef struct AsmAddressRule { unsigned char mode, category; } AsmAddressRule;
extern AsmAddressRule asm_address_rules[32];
#define ASM_OPERAND_CATEGORY(address_) (asm_address_rules[(address_)->address_flags & 31].category)
int validate_asm_operand_count(void);
int validate_asm_operands(void);
int __stdcall fits_asm_integer(unsigned char bits, unsigned value);
extern unsigned char asm_encoded_opcode, asm_instruction_size, asm_instruction_modifier;
extern AsmAddress *asm_output_addresses[3];
extern AsmOperand *asm_output_operands[3];
void prepare_asm_instruction(void);
unsigned char choose_asm_instruction_size(void);
unsigned char resolve_asm_operand_size(void);
int __stdcall asm_operand_accepts_size(AsmOperand *operand, unsigned char size);
int __stdcall asm_operand_has_type_flags(AsmOperand *operand, unsigned short flags);
unsigned normalize_asm_x87_operands(void);
void normalize_asm_addresses(void);
void __stdcall report_asm_operand_error(unsigned code, int operand);
void __stdcall swap_asm_pointer_slots(void **first, void **second);
void serialize_asm_instruction(void);
unsigned char __stdcall asm_address_mode_code(AsmAddress *address);
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
AsmExpression *__stdcall make_asm_address_register(short reg);
extern unsigned short asm_register_properties[46];
AsmExpression *__stdcall make_asm_stack_register(short reg, int index);
void __stdcall set_asm_operand_size(unsigned char size);
unsigned char __stdcall asm_size_encoding(unsigned bytes);
void write_asm_segment_prefix(void);
#endif
