#ifndef IR_H
#define IR_H
#include "types.h"

enum IrBaseTypeKind {
    IR_TYPE_UNKNOWN = 0, IR_TYPE_SIGNED = 1, IR_TYPE_UNSIGNED = 2,
    IR_TYPE_POINTER = 3, IR_TYPE_FLOAT = 5, IR_TYPE_AGGREGATE = 6,
    IR_TYPE_VOID = 7, IR_TYPE_EXTENDED = 10, IR_TYPE_FAR16_POINTER = 11
};
unsigned short __stdcall expression_type_alignment(CType *type);
unsigned short __stdcall node_alignment(IrToken *node);
int __stdcall complete_type_size(CType *type);

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
    IR_FIELD_RESERVED_17 = 17,
    IR_FIELD_RESERVED_18 = 18,
    IR_FIELD_RESERVED_19 = 19,
    IR_FIELD_RESERVED_112 = 112
};

typedef struct FrontendSymbol FrontendSymbol;

extern unsigned char *ir_record_formats[191];
typedef struct FunctionRecord {
    FrontendSymbol *symbol;
    unsigned expression_offset, symbol_offset, complexity;
    unsigned short parameter_count, reserved;
    unsigned flags;
} FunctionRecord;
typedef struct FunctionRecordBlock {
    struct FunctionRecordBlock *next;
    FunctionRecord records[20];
} FunctionRecordBlock;
extern FunctionRecordBlock *function_records, *last_function_records;
extern unsigned function_record_index;
extern unsigned string_record_count;
extern int exception_scope_depth[3];
extern struct FrontendSymbol *function_symbol;
extern FrontendSymbol *current_label_symbol;
void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode);
void __stdcall reference_ir_symbol(FrontendSymbol *symbol);
unsigned get_ir_symbol_index(FrontendSymbol *symbol);
#endif
