/* Preprocessor constant-expression evaluation. */
#include "pp-expression.h"
#include "lexer.h"

unsigned char preprocessor_expression_token = 1; /* original 0x6a7b0 */
int preprocessor_parentheses; /* original 0x6a7b4 */

/* Original 0x18680..0x186fd. Only the integer result outlives the
   evaluator's temporary arena; trailing and unbalanced tokens are diagnosed. */
int evaluate_preprocessor_expression(void)
{
    PreprocessorValue *value;
    int result, previous;
    preprocessor_parentheses = 0;
    preprocessor_expression_token = 0x8e;
    value = parse_pp_conditional();
    if (preprocessor_expression_token == TOK_RIGHT_PAREN) {
        previous = preprocessor_parentheses--;
        if (!previous) fatal_error(12, '(');
    } else if (preprocessor_expression_token != TOK_NEWLINE)
        compiler_warning(1, 67, "#if/#elif");
    if (preprocessor_parentheses > 0) fatal_error(12, ')');
    result = value->value;
    release_arena(4);
    return result;
}

/* Original 0x18700..0x1874a. Both ternary alternatives use the logical-or
   grammar entry; preserve the original missing-colon path's unset result. */
PreprocessorValue *parse_pp_conditional(void)
{
    PreprocessorValue *condition, *positive, *negative;
    condition = parse_pp_logical_or();
    if (!accept_pp_token(TOK_QUESTION)) return condition;
    positive = parse_pp_logical_or();
    if (accept_pp_token(TOK_COLON)) negative = parse_pp_logical_or();
    return condition->value ? positive : negative;
}

int pp_unevaluated_depth; /* original 0x64d78 */

/* Original 0x18750..0x187a2. Parse skipped operands as usual while the
   evaluation-depth counter suppresses diagnostics from short-circuit work. */
PreprocessorValue *parse_pp_logical_or(void)
{
    int saved_depth = pp_unevaluated_depth;
    PreprocessorValue *value = parse_pp_logical_and(), *right;
    while (accept_pp_token(TOK_LOGICAL_OR)) {
        if (value->value) ++pp_unevaluated_depth;
        right = parse_pp_logical_and();
        value = apply_pp_operator(TOK_LOGICAL_OR, value, right);
    }
    pp_unevaluated_depth = saved_depth;
    return value;
}

/* Original 0x187b0..0x18802. A false left operand suppresses evaluation
   diagnostics while the corresponding right operand is parsed. */
PreprocessorValue *parse_pp_logical_and(void)
{
    int saved_depth = pp_unevaluated_depth;
    PreprocessorValue *value = parse_pp_bitwise_or(), *right;
    while (accept_pp_token(TOK_LOGICAL_AND)) {
        if (!value->value) ++pp_unevaluated_depth;
        right = parse_pp_bitwise_or();
        value = apply_pp_operator(TOK_LOGICAL_AND, value, right);
    }
    pp_unevaluated_depth = saved_depth;
    return value;
}
