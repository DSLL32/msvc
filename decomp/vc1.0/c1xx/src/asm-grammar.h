/* Generated from asm-grammar.y; logical symbols and table declarations. */
#ifndef ASSEMBLER_GRAMMAR_H
#define ASSEMBLER_GRAMMAR_H
#include "parser-grammar.h"
#include <recon.h>

enum AsmToken {
    AS_END = 0,
    AS_EQ = 1,
    AS_GE = 2,
    AS_GT = 3,
    AS_LE = 4,
    AS_LT = 5,
    AS_NE = 6,
    AS_AND = 7,
    AS_MOD = 8,
    AS_NOT = 9,
    AS_OR = 10,
    AS_SHL = 11,
    AS_SHR = 12,
    AS_XOR = 13,
    AS_HIGH = 14,
    AS_LENGTH = 15,
    AS_LOW = 16,
    AS_OFFSET = 17,
    AS_PTR = 18,
    AS_SEG = 19,
    AS_SIZE = 20,
    AS_TYPE = 21,
    AS_PLUS = 22,
    AS_MINUS = 23,
    AS_MULTIPLY = 24,
    AS_DIVIDE = 25,
    AS_COLON = 26,
    AS_COMMA = 27,
    AS_LEFT_BRACKET = 28,
    AS_RIGHT_BRACKET = 29,
    AS_LEFT_PAREN = 30,
    AS_RIGHT_PAREN = 31,
    AS_LEFT_BRACE = 32,
    AS_SEMICOLON = 33,
    AS_NEWLINE = 34,
    AS_DOT = 35,
    AS_END_OF_FILE = 36,
    AS_BAD = 37,
    AS_MAPPED = 38,
    AS_REGISTER = 39,
    AS_ASM_DATA_TYPE = 40,
    AS_ASM_CODE_TYPE = 41,
    AS_C_DATA_TYPE = 42,
    AS_TYPEDEF_NAME = 43,
    AS_IDENTIFIER = 44,
    AS_CONSTANT = 45,
    AS_ENUM_MEMBER = 46,
    AS_PREFIX = 47,
    AS_OPCODE = 48,
    AS_DIRECTIVE = 49,
    AS_ERROR = 256,
};
extern ParserTransition asm_shifts[], asm_reductions[], asm_goto_rows[];
extern ParserState asm_states[136];
extern ParserGoto asm_gotos[24];
extern ParserProduction asm_productions[83];
#endif
