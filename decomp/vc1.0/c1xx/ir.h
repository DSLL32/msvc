#ifndef IR_H
#define IR_H
#include "types.h"
#include "global-ir.h"

enum IrField {
    IR_FIELD_END = 0,
    IR_FIELD_CALL_KIND = 1,
    IR_FIELD_INTEGER = 2,
    IR_FIELD_STORAGE = 3,
    IR_FIELD_RESERVED_4 = 4,
    IR_FIELD_TYPE_NUMBER = 5,
    IR_FIELD_NODE = 6,
    IR_FIELD_ZERO = 7,
    IR_FIELD_SYMBOL_INDEX = 8,
    IR_FIELD_RESERVED_9 = 9,
    IR_FIELD_RESERVED_10 = 10,
    IR_FIELD_CONSTANT = 11,
    IR_FIELD_RESERVED_12 = 12,
    IR_FIELD_ZERO_13 = 13,
    IR_FIELD_ZERO_14 = 14,
    IR_FIELD_RESERVED_15 = 15,
    IR_FIELD_RESERVED_16 = 16,
    IR_FIELD_PACKED_VALUE = 17,
    IR_FIELD_RESERVED_18 = 18,
    IR_FIELD_RESERVED_19 = 19,
    IR_FIELD_RESERVED_112 = 112
};

extern unsigned char *ir_record_formats[191];
#define expression_record_count (function_ir.complexity)
extern CppSymbol *function_symbol;
extern CppSymbol *label_symbols;
extern CppSymbol *current_label_symbol;
CType *builtin_integer_type(void);
void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode);
void __stdcall reference_ir_symbol(CppSymbol *symbol);
unsigned get_ir_symbol_index(CppSymbol *symbol);
#endif
