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
PreprocessorValue *parse_preprocessor_conditional(void);
#endif
