#ifndef EXPRESSION_RECORDS_H
#define EXPRESSION_RECORDS_H
#include "lexer.h"
#include <recon.h>
typedef struct ExpressionRecordRule {
    unsigned char operation;
    unsigned short record;
} ExpressionRecordRule;
extern ExpressionRecordRule expression_record_rules[];
extern unsigned expression_record_rule_count;
/* Keywords use their declaration meaning in the original shared vocabulary.
   Expression operators override that value with a serialized record number.
   Keep lookup inside the original emitter: this is not a new call-graph node. */
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
#endif
