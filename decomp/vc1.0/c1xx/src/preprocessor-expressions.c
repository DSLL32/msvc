/* Integer expression evaluation for #if and #elif. */
#include "preprocessor-expressions.h"
#include "frontend.h"
#include <recon.h>
unsigned char preprocessor_expression_token = 1; /* original 0x9d7b0 */
int preprocessor_expression_parentheses; /* original 0x9d7b4 */

/* Original 0x18660..0x186dc. Keep the numeric result, rather than
   normalizing it to boolean, before releasing the expression arena. */
int evaluate_preprocessor_expression(void)
{
    PreprocessorValue *expression;
    long value;
    preprocessor_expression_parentheses = 0;
    preprocessor_expression_token = 0x93;
    expression = parse_preprocessor_conditional();
    if (preprocessor_expression_token == 0x88) {
        if (!preprocessor_expression_parentheses--) fatal_error(12, '(');
    } else if (preprocessor_expression_token != 1)
        compiler_warning(1, 67, "#if/#elif");
    if (preprocessor_expression_parentheses > 0) fatal_error(12, ')');
    value = expression->value;
    release_arena(4);
    return value;
}

/* Original 0x186e0..0x18729. The original leaves the false result
   unwritten when ':' is absent. Its true result remains usable. */
PreprocessorValue *parse_preprocessor_conditional(void)
{
    PreprocessorValue *condition, *yes, *no;
    condition = parse_preprocessor_logical_or();
    if (!accept_preprocessor_token(0x5f)) return condition;
    yes = parse_preprocessor_logical_or();
    if (accept_preprocessor_token(0x3f)) no = parse_preprocessor_logical_or();
#ifdef RECON_NATIVE_TEST
    /* entryESP-4 has no writer on the original missing-colon branch.
       Only a false condition consumes that unwritten result pointer. */
    else if (!condition->value) { __asm { int 3 } }
#endif
    return condition->value ? yes : no;
}
