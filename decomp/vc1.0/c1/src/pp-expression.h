#ifndef PP_EXPRESSION_H
#define PP_EXPRESSION_H
#include "preprocessor.h"
#include <recon.h>

/* Compact constant descriptor used by the preprocessor evaluator. */
typedef struct PreprocessorValue {
    unsigned short type_flags, reserved;
    unsigned value;
} PreprocessorValue;
extern unsigned char preprocessor_expression_token;
extern int preprocessor_parentheses;
PreprocessorValue *parse_pp_conditional(void);
PreprocessorValue *parse_pp_logical_or(void);
int __stdcall accept_pp_token(unsigned char expected);

extern int pp_unevaluated_depth;
PreprocessorValue *parse_pp_logical_and(void);
PreprocessorValue *__stdcall apply_pp_operator(unsigned char operation, PreprocessorValue *left, PreprocessorValue *right);

PreprocessorValue *parse_pp_bitwise_or(void);

PreprocessorValue *parse_pp_bitwise_xor(void);

PreprocessorValue *parse_pp_bitwise_and(void);

PreprocessorValue *parse_pp_equality(void);

PreprocessorValue *parse_pp_relational(void);

PreprocessorValue *parse_pp_shift(void);

PreprocessorValue *parse_pp_additive(void);

PreprocessorValue *parse_pp_multiplicative(void);

PreprocessorValue *parse_pp_unary(void);

PreprocessorValue *parse_pp_primary(void);

extern PreprocessorValue preprocessor_literal;
int __stdcall evaluate_defined_operator(char *name);

#endif
