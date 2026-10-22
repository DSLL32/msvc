#ifndef EXPRESSION_RECORDS_H
#define EXPRESSION_RECORDS_H
#include "function-definitions.h"
#include "operators.h"
/* Expression meanings override the shared declaration vocabulary. Unknown
   and declaration-only tokens retain the original invalid record, 0x9e. */
typedef struct ExpressionRecordRule {
    unsigned operation, record;
} ExpressionRecordRule;
extern ExpressionRecordRule expression_record_rules[];
extern unsigned expression_record_rule_count;
int __stdcall emit_string_intrinsic(IrToken *expression);
#define WRITE_EXPRESSION_RECORD(expression_) do { \
    record = 0x9e; \
    for (rule = 0; rule < declaration_code_count; ++rule) \
        if (declaration_codes[rule].token == (expression_)->kind) { \
            record = declaration_codes[rule].value; break; \
        } \
    for (rule = 0; rule < expression_record_rule_count; ++rule) \
        if (expression_record_rules[rule].operation == (expression_)->kind) { \
            record = expression_record_rules[rule].record; break; \
        } \
    write_ir_record(record, (expression_)); \
} while (0)
enum SerializedTypeKind {
    IR_TYPE_UNKNOWN = 0, IR_TYPE_SIGNED = 1, IR_TYPE_UNSIGNED = 2,
    IR_TYPE_POINTER = 3, IR_TYPE_FLOAT = 5, IR_TYPE_AGGREGATE = 6,
    IR_TYPE_VOID = 7, IR_TYPE_EXTENDED = 10, IR_TYPE_FAR16_POINTER = 11
};
unsigned short __stdcall expression_type_alignment(CType *type);
unsigned short __stdcall node_alignment(IrToken *expression);
unsigned __stdcall complete_type_size(CType *type);
unsigned __stdcall member_pointer_inheritance_model(CType *type);
#endif
