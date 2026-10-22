/* Preprocessor constant-expression evaluation. */
#include "pp-expression.h"
#include "lexer.h"
#include "source-input.h"
#include "characters.h"
#include <recon.h>

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

/* Original 0x18810..0x18842. The commutative bitwise layers use the
   right operand as the reusable result descriptor. */
PreprocessorValue *parse_pp_bitwise_or(void)
{
    PreprocessorValue *value = parse_pp_bitwise_xor(), *right;
    while (accept_pp_token(TOK_BIT_OR)) {
        right = parse_pp_bitwise_xor();
        value = apply_pp_operator(TOK_BIT_OR, right, value);
    }
    return value;
}

/* Original 0x18850..0x18882. */
PreprocessorValue *parse_pp_bitwise_xor(void)
{
    PreprocessorValue *value = parse_pp_bitwise_and(), *right;
    while (accept_pp_token(TOK_BIT_XOR)) {
        right = parse_pp_bitwise_and();
        value = apply_pp_operator(TOK_BIT_XOR, right, value);
    }
    return value;
}

/* Original 0x18890..0x188c2. */
PreprocessorValue *parse_pp_bitwise_and(void)
{
    PreprocessorValue *value = parse_pp_equality(), *right;
    while (accept_pp_token(TOK_BIT_AND)) {
        right = parse_pp_equality();
        value = apply_pp_operator(TOK_BIT_AND, right, value);
    }
    return value;
}

/* Original 0x188d0..0x18914. This evaluator admits one equality operator
   at this level and reuses its right operand for the result. */
PreprocessorValue *parse_pp_equality(void)
{
    PreprocessorValue *left = parse_pp_relational(), *right;
    if (accept_pp_token(TOK_EQUAL)) {
        right = parse_pp_relational();
        return apply_pp_operator(TOK_EQUAL, right, left);
    }
    if (accept_pp_token(TOK_NOT_EQUAL)) {
        right = parse_pp_relational();
        return apply_pp_operator(TOK_NOT_EQUAL, right, left);
    }
    return left;
}

/* Original 0x18920..0x1899f. One relational operation is admitted here. */
PreprocessorValue *parse_pp_relational(void)
{
    PreprocessorValue *left = parse_pp_shift(), *right;
    unsigned char operation;
    if (accept_pp_token(TOK_LESS)) operation = TOK_LESS;
    else if (accept_pp_token(TOK_GREATER)) operation = TOK_GREATER;
    else if (accept_pp_token(TOK_LESS_EQUAL)) operation = TOK_LESS_EQUAL;
    else if (accept_pp_token(TOK_GREATER_EQUAL)) operation = TOK_GREATER_EQUAL;
    else return left;
    right = parse_pp_shift();
    return apply_pp_operator(operation, left, right);
}

/* Original 0x189a0..0x189e4. */
PreprocessorValue *parse_pp_shift(void)
{
    PreprocessorValue *left = parse_pp_additive(), *right;
    unsigned char operation;
    if (accept_pp_token(TOK_RIGHT_SHIFT)) operation = TOK_RIGHT_SHIFT;
    else if (accept_pp_token(TOK_LEFT_SHIFT)) operation = TOK_LEFT_SHIFT;
    else return left;
    right = parse_pp_additive();
    return apply_pp_operator(operation, left, right);
}

/* Original 0x189f0..0x18a4f. Retain the original repeated plus-token
   probe after an unsuccessful minus probe. */
PreprocessorValue *parse_pp_additive(void)
{
    PreprocessorValue *value = parse_pp_multiplicative(), *right;
    unsigned char operation;
    for (;;) {
        if (accept_pp_token(TOK_PLUS)) operation = TOK_PLUS;
        else if (accept_pp_token(TOK_MINUS)) operation = TOK_MINUS;
        else if (accept_pp_token(TOK_PLUS)) operation = TOK_PLUS;
        else return value;
        right = parse_pp_multiplicative();
        value = apply_pp_operator(operation, value, right);
    }
}

/* Original 0x18a50..0x18aaf. */
PreprocessorValue *parse_pp_multiplicative(void)
{
    PreprocessorValue *value = parse_pp_unary(), *right;
    unsigned char operation;
    for (;;) {
        if (accept_pp_token(TOK_MULTIPLY)) operation = TOK_MULTIPLY;
        else if (accept_pp_token(TOK_DIVIDE)) operation = TOK_DIVIDE;
        else if (accept_pp_token(TOK_REMAINDER)) operation = TOK_REMAINDER;
        else return value;
        right = parse_pp_unary();
        value = apply_pp_operator(operation, value, right);
    }
}

/* Original 0x18ab0..0x18b30, ending with a tail branch to the primary
   parser. Unary operators keep their operand as the reusable descriptor. */
PreprocessorValue *parse_pp_unary(void)
{
    PreprocessorValue *value;
    unsigned char operation;
    if (accept_pp_token(TOK_LOGICAL_NOT)) operation = TOK_LOGICAL_NOT;
    else if (accept_pp_token(TOK_BIT_NOT)) operation = TOK_BIT_NOT;
    else if (accept_pp_token(TOK_PLUS)) operation = TOK_UNARY_PLUS;
    else if (accept_pp_token(TOK_MINUS)) operation = TOK_UNARY_MINUS;
    else return parse_pp_primary();
    value = parse_pp_unary();
    return apply_pp_operator(operation, value, 0);
}

PreprocessorValue preprocessor_literal; /* original 0x64d7c..0x64d84 */

/* Original 0x18140..0x18243. The unread marker is retained after a
   successful fresh-token match; otherwise cache the token for later probes. */
int __stdcall accept_pp_token(unsigned char expected)
{
    unsigned char category;
    IrToken *literal;
    if (preprocessor_expression_token == TOK_NEWLINE) return 0;
    if (preprocessor_expression_token != 0x8e) {
        if (expected != preprocessor_expression_token) return 0;
        preprocessor_expression_token = 0x8e;
        return 1;
    }
    category = read_parser_token();
    if (category == TOK_IDENTIFIER) {
        preprocessor_literal.value = evaluate_defined_operator(current_token_value.identifier);
        preprocessor_literal.type_flags = 0x64;
    } else if (category == TOK_CONSTANT) {
        literal = current_token_value.literal;
        if (!(literal->type->flags & 0x40)) fatal_error(17);
        else {
            preprocessor_literal.value = literal->value.integer;
            preprocessor_literal.type_flags = literal->type->flags;
        }
    } else {
        if (expected == current_token_code) return 1;
        preprocessor_expression_token = current_token_code;
        return 0;
    }
    if (expected == TOK_INT_CONSTANT) return 1;
    preprocessor_expression_token = TOK_INT_CONSTANT;
    return 0;
}

/* Original 0x21100..0x21282. Ordinary preprocessing identifiers yield 0;
   defined optionally consumes parentheses around an unexpanded macro name. */
int __stdcall evaluate_defined_operator(char *name)
{
    char *expected = "defined", *actual = name;
    unsigned char character, class;
    int result = 0, parenthesized = 0;
    while (*actual && *actual == *expected) { ++actual; ++expected; }
    if (*actual != *expected) return 0;
    if (!skip_macro_space() && !macro_rescan_depth) return 0;
    if (*source_text_cursor == '(') {
        ++source_text_cursor;
        parenthesized = 1;
        if (!skip_macro_space() && !macro_rescan_depth) return 0;
    }
    character = (unsigned char)*source_text_cursor;
    class = character_class[character];
    if (macro_rescan_depth > 0) {
        if (class == CC_MACRO_PARAMETER || class == CC_IDENTIFIER || class == CC_WIDE_PREFIX) {
            ++source_text_cursor;
            scan_macro_argument_identifier(character);
            result = lookup_macro() != 0;
        }
    } else if (class != CC_IDENTIFIER && class != CC_WIDE_PREFIX)
        report_syntax_error(parenthesized ? 4 : 3);
    else {
        ++source_text_cursor;
        if (macro_expansion_active > 0) scan_macro_identifier();
        else scan_source_identifier();
        result = lookup_macro() != 0;
    }
    class = character_class[(unsigned char)*source_text_cursor];
    if ((class == CC_SPACE || class == CC_ESCAPE_OR_END) && !skip_macro_space())
        return result;
    if (parenthesized) {
        if (character_class[(unsigned char)*source_text_cursor] == CC_RIGHT_PAREN) {
            ++source_text_cursor;
            --parenthesized;
        }
        if (parenthesized > 0 && !macro_rescan_depth) compiler_warning(1, 4);
    }
    return result;
}

/* Original 0x18b30..0x18bc3. Copy each accepted literal into evaluator
   arena 4, or parse a parenthesized conditional expression. */
PreprocessorValue *parse_pp_primary(void)
{
    PreprocessorValue *value;
    ArenaPool *pool;
    if (accept_pp_token(TOK_LEFT_PAREN)) {
        ++preprocessor_parentheses;
        value = parse_pp_conditional();
        if (!accept_pp_token(TOK_RIGHT_PAREN)) fatal_error(12, ')');
        --preprocessor_parentheses;
        return value;
    }
    if (!accept_pp_token(TOK_INT_CONSTANT)) fatal_error(17);
    pool = &compiler_arena.pools[4];
    if (pool->remaining < sizeof(PreprocessorValue)) refill_arena(&compiler_arena, pool);
    value = (PreprocessorValue *)pool->next;
    pool->remaining -= sizeof(PreprocessorValue);
    pool->next += sizeof(PreprocessorValue);
    *value = preprocessor_literal;
    return value;
}

/* Original 0x18250..0x185cd; operator dispatch table 0x185d0..0x1867c.
   Evaluate in place and retain the original integer-promotion/shift rules. */
PreprocessorValue *__stdcall apply_pp_operator(unsigned char operation,
    PreprocessorValue *left, PreprocessorValue *right)
{
    unsigned short flags = left->type_flags;
    unsigned width = flags & 15;
    int unsigned_result = (flags & 0x4000) || (right && (right->type_flags & 0x4000));
    if (right && (right->type_flags & 15) < width) width = right->type_flags & 15;
    if (width == 1) unsigned_result = 0;
    switch (operation) {
    case TOK_BIT_AND: left->value &= right->value; break;
    case TOK_LOGICAL_AND:
        left->value = left->value && right->value;
        unsigned_result = 0;
        break;
    case TOK_DIVIDE:
        if (right->value) {
            if (unsigned_result) left->value /= right->value;
            else left->value = (int)left->value / (int)right->value;
        } else if (!pp_unevaluated_depth) report_syntax_error(124);
        break;
    case TOK_EQUAL:
        left->value = left->value == right->value;
        unsigned_result = 0;
        break;
    case TOK_LOGICAL_NOT:
        left->value = !left->value;
        unsigned_result = 0;
        break;
    case TOK_GREATER:
        left->value = unsigned_result ? left->value > right->value :
                                       (int)left->value > (int)right->value;
        unsigned_result = 0;
        break;
    case TOK_GREATER_EQUAL:
        left->value = unsigned_result ? left->value >= right->value :
                                       (int)left->value >= (int)right->value;
        unsigned_result = 0;
        break;
    case TOK_LEFT_SHIFT:
        unsigned_result = (flags & 0x4000) || (!options.extensions_enabled && options.option_J &&
                           (flags & 0x2fdf) == 0x41 && !(flags & 0x8000));
        left->value <<= right->value & 31;
        break;
    case TOK_LESS:
        left->value = unsigned_result ? left->value < right->value :
                                       (int)left->value < (int)right->value;
        unsigned_result = 0;
        break;
    case TOK_LESS_EQUAL:
        left->value = unsigned_result ? left->value <= right->value :
                                       (int)left->value <= (int)right->value;
        unsigned_result = 0;
        break;
    case TOK_MINUS: left->value -= right->value; break;
    case TOK_REMAINDER:
        if (right->value) {
            if (unsigned_result) left->value %= right->value;
            else left->value = (int)left->value % (int)right->value;
        } else if (!pp_unevaluated_depth) report_syntax_error(124);
        break;
    case TOK_MULTIPLY: left->value *= right->value; break;
    case TOK_NOT_EQUAL:
        left->value = left->value != right->value;
        unsigned_result = 0;
        break;
    case TOK_BIT_OR: left->value |= right->value; break;
    case TOK_LOGICAL_OR:
        left->value = left->value || right->value;
        unsigned_result = 0;
        break;
    case TOK_PLUS: left->value += right->value; break;
    case TOK_RIGHT_SHIFT:
        unsigned_result = (flags & 0x4000) || (!options.extensions_enabled && options.option_J &&
                           (flags & 0x2fdf) == 0x41 && !(flags & 0x8000));
        if (unsigned_result) left->value >>= right->value & 31;
        else left->value = (int)left->value >> (right->value & 31);
        break;
    case TOK_BIT_NOT: left->value = ~left->value; break;
    case TOK_UNARY_MINUS: left->value = 0U - left->value; break;
    case TOK_BIT_XOR: left->value ^= right->value; break;
    }
    left->type_flags = unsigned_result ? 0x4044 : 0x44;
    return left;
}
