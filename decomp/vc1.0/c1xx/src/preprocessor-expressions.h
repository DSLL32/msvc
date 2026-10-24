#ifndef PREPROCESSOR_EXPRESSIONS_H
#define PREPROCESSOR_EXPRESSIONS_H
#include <stddef.h>
#include <recon.h>
/* Integer-expression prefix: type WORD, alignment padding, value DWORD.
   The padding has no initialization or comparison credit. */
typedef struct PreprocessorValue {
    unsigned short type;
    long value;
} PreprocessorValue;
typedef char PreprocessorValueLayoutMustMatch[
    (sizeof(PreprocessorValue) == 8 && offsetof(PreprocessorValue, value) == 4) ? 1 : -1];
extern unsigned char preprocessor_expression_token;
extern int preprocessor_expression_parentheses;
extern int preprocessor_skip_evaluation;
PreprocessorValue *parse_preprocessor_conditional(void);
PreprocessorValue *parse_preprocessor_logical_or(void);
PreprocessorValue *parse_preprocessor_logical_and(void);
PreprocessorValue *__stdcall evaluate_preprocessor_binary(unsigned char operation, PreprocessorValue *left, PreprocessorValue *right);
int __stdcall accept_preprocessor_token(unsigned char token);
#endif
