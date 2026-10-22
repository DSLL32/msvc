/* Expression meanings of the frontend's shared token vocabulary. Declaration
   keywords retain their meanings in declaration_codes, not a duplicate table. */
#include "expression-records.h"
#include <recon.h>
ExpressionRecordRule expression_record_rules[] = {
    { TOK_NONE, 0 }, { TOK_UNALIGNED, 0 },
    { TOK_IDENTIFIER, 0x26 },
    { TOK_STRING, 0x33 }, { TOK_FLOAT_CONSTANT, 0x33 },
    { TOK_DOUBLE_CONSTANT, 0x33 }, { TOK_LONG_DOUBLE_CONSTANT, 0x33 },
    { TOK_INT_CONSTANT, 0x33 }, { TOK_LONG_CONSTANT, 0x33 },
    { TOK_UINT_CONSTANT, 0x33 }, { TOK_ULONG_CONSTANT, 0x33 },
    { TOK_BIT_AND, 0x0b }, { TOK_LOGICAL_AND, 0x1c }, { TOK_AND_ASSIGN, 0x17 },
    { TOK_ASSIGN, 0x32 }, { TOK_BIT_FIELD, 0x37 }, { TOK_COLON, 0x43 },
    { TOK_COMMA, 0x44 }, { TOK_PREDECREMENT, 0x36 },
    { TOK_DIVIDE, 0x05 }, { TOK_DIVIDE_ASSIGN, 0x12 },
    { TOK_EQUAL, 0x1f }, { TOK_LOGICAL_NOT, 0x1a }, { TOK_DEREFERENCE, 0x30 },
    { TOK_GREATER, 0x24 }, { TOK_GREATER_EQUAL, 0x23 }, { TOK_PREINCREMENT, 0x35 },
    { TOK_LEFT_BRACKET, 0x28 }, { TOK_LEFT_SHIFT_ASSIGN, 0x15 }, { TOK_LEFT_SHIFT, 0x09 },
    { TOK_LESS, 0x22 }, { TOK_LESS_EQUAL, 0x21 },
    { TOK_MINUS, 0x03 }, { TOK_MINUS_ASSIGN, 0x10 },
    { TOK_REMAINDER, 0x06 }, { TOK_REMAINDER_ASSIGN, 0x13 },
    { TOK_MULTIPLY, 0x04 }, { TOK_MULTIPLY_ASSIGN, 0x11 }, { TOK_NOT_EQUAL, 0x20 },
    { TOK_BIT_OR, 0x0c }, { TOK_OR_ASSIGN, 0x19 }, { TOK_LOGICAL_OR, 0x1b },
    { TOK_MEMBER, 0x27 }, { TOK_ARROW, 0x27 },
    { TOK_PLUS, 0x02 }, { TOK_PLUS_ASSIGN, 0x0f },
    { TOK_POSTDECREMENT, 0x10 }, { TOK_POSTINCREMENT, 0x0f },
    { TOK_QUESTION, 0x42 }, { TOK_RIGHT_SHIFT_ASSIGN, 0x16 }, { TOK_RIGHT_SHIFT, 0x0a },
    { TOK_BIT_NOT, 0x0e }, { TOK_UNARY_MINUS, 0x08 },
    { TOK_BIT_XOR, 0x0d }, { TOK_XOR_ASSIGN, 0x18 },
    { TOK_BASED_REFERENCE, 0x68 }, { TOK_SEGMENT_OFFSET, 0x8a }, { TOK_DESEG, 0x8b },
    { TOK_ARGUMENT, 0x55 }, { TOK_CASE, 0x3d }, { TOK_CAST, 0x2c },
    { TOK_ENTRY, 0x46 }, { TOK_EXCEPT, 0x8d }, { TOK_FINALLY, 0x8e },
    { TOK_ARGUMENT_END, 0x3e }, { TOK_INITIALIZING, 0x32 },
    { TOK_LEFT_PAREN, 0x59 }, { TOK_INTRINSIC, 0x40 }, { TOK_SELF, 0x85 },
    { TOK_SWITCH, 0x3c }, { TOK_RESERVED_136, 0x3b }, { TOK_TRY, 0x8f },
    { TOK_UNARY_PLUS, 0x59 }
};
unsigned expression_record_rule_count = sizeof(expression_record_rules) / sizeof(expression_record_rules[0]);
