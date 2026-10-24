/* Integer expression evaluation for #if and #elif. */
#include "preprocessor-expressions.h"
#include "frontend.h"
#include "operators.h"
#include "macro-expansion.h"
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

int preprocessor_skip_evaluation; /* original 0x9783c: nested short-circuit state */

/* Original 0x18730..0x18781. Keep the accumulated suppression depth
   until the entire logical-OR chain has been parsed, then restore it. */
PreprocessorValue *parse_preprocessor_logical_or(void)
{
    int saved = preprocessor_skip_evaluation;
    PreprocessorValue *left, *right;
    left = parse_preprocessor_logical_and();
    while (accept_preprocessor_token(OP_LOGICAL_OR)) {
        if (left->value) ++preprocessor_skip_evaluation;
        right = parse_preprocessor_logical_and();
        left = evaluate_preprocessor_binary(OP_LOGICAL_OR, left, right);
    }
    preprocessor_skip_evaluation = saved;
    return left;
}

PreprocessorValue *parse_preprocessor_bitwise_or(void);

/* Original 0x18790..0x187e1. A false left operand suppresses evaluation
   diagnostics while the remaining logical-AND chain is parsed. */
PreprocessorValue *parse_preprocessor_logical_and(void)
{
    int saved = preprocessor_skip_evaluation;
    PreprocessorValue *left, *right;
    left = parse_preprocessor_bitwise_or();
    while (accept_preprocessor_token(OP_LOGICAL_AND)) {
        if (!left->value) ++preprocessor_skip_evaluation;
        right = parse_preprocessor_bitwise_or();
        left = evaluate_preprocessor_binary(OP_LOGICAL_AND, left, right);
    }
    preprocessor_skip_evaluation = saved;
    return left;
}

PreprocessorValue *parse_preprocessor_bitwise_xor(void);

/* Original 0x187f0..0x18821. */
PreprocessorValue *parse_preprocessor_bitwise_or(void)
{
    PreprocessorValue *left, *right;
    left = parse_preprocessor_bitwise_xor();
    while (accept_preprocessor_token(OP_BIT_OR)) {
        right = parse_preprocessor_bitwise_xor();
        left = evaluate_preprocessor_binary(OP_BIT_OR, left, right);
    }
    return left;
}

PreprocessorValue *parse_preprocessor_bitwise_and(void);

/* Original 0x18830..0x18861. */
PreprocessorValue *parse_preprocessor_bitwise_xor(void)
{
    PreprocessorValue *left, *right;
    left = parse_preprocessor_bitwise_and();
    while (accept_preprocessor_token(OP_BIT_XOR)) {
        right = parse_preprocessor_bitwise_and();
        left = evaluate_preprocessor_binary(OP_BIT_XOR, left, right);
    }
    return left;
}

PreprocessorValue *parse_preprocessor_equality(void);

/* Original 0x18870..0x188a1. */
PreprocessorValue *parse_preprocessor_bitwise_and(void)
{
    PreprocessorValue *left, *right;
    left = parse_preprocessor_equality();
    while (accept_preprocessor_token(OP_BIT_AND)) {
        right = parse_preprocessor_equality();
        left = evaluate_preprocessor_binary(OP_BIT_AND, left, right);
    }
    return left;
}

PreprocessorValue *parse_preprocessor_relational(void);

/* Original 0x188b0..0x188f3. This parser accepts one equality
   comparison, retaining the original single-comparison grammar. */
PreprocessorValue *parse_preprocessor_equality(void)
{
    PreprocessorValue *left, *right;
    left = parse_preprocessor_relational();
    if (accept_preprocessor_token(OP_EQUAL)) {
        right = parse_preprocessor_relational();
        return evaluate_preprocessor_binary(OP_EQUAL, left, right);
    }
    if (accept_preprocessor_token(OP_NOT_EQUAL)) {
        right = parse_preprocessor_relational();
        return evaluate_preprocessor_binary(OP_NOT_EQUAL, left, right);
    }
    return left;
}

PreprocessorValue *parse_preprocessor_shift(void);

/* Original 0x18900..0x1897e. */
PreprocessorValue *parse_preprocessor_relational(void)
{
    PreprocessorValue *left, *right;
    left = parse_preprocessor_shift();
    if (accept_preprocessor_token(OP_LESS)) {
        right = parse_preprocessor_shift();
        return evaluate_preprocessor_binary(OP_LESS, left, right);
    }
    if (accept_preprocessor_token(OP_GREATER)) {
        right = parse_preprocessor_shift();
        return evaluate_preprocessor_binary(OP_GREATER, left, right);
    }
    if (accept_preprocessor_token(OP_LESS_EQUAL)) {
        right = parse_preprocessor_shift();
        return evaluate_preprocessor_binary(OP_LESS_EQUAL, left, right);
    }
    if (accept_preprocessor_token(OP_GREATER_EQUAL)) {
        right = parse_preprocessor_shift();
        return evaluate_preprocessor_binary(OP_GREATER_EQUAL, left, right);
    }
    return left;
}

PreprocessorValue *parse_preprocessor_additive(void);

/* Original 0x18980..0x189c3. The right-shift probe precedes left shift. */
PreprocessorValue *parse_preprocessor_shift(void)
{
    PreprocessorValue *left, *right;
    left = parse_preprocessor_additive();
    if (accept_preprocessor_token(OP_SHIFT_RIGHT)) {
        right = parse_preprocessor_additive();
        return evaluate_preprocessor_binary(OP_SHIFT_RIGHT, left, right);
    }
    if (accept_preprocessor_token(OP_SHIFT_LEFT)) {
        right = parse_preprocessor_additive();
        return evaluate_preprocessor_binary(OP_SHIFT_LEFT, left, right);
    }
    return left;
}

PreprocessorValue *parse_preprocessor_multiplicative(void);

/* Original 0x189d0..0x18a2e. The repeated addition probe is present
   in the original; retain its call and token-state behavior. */
PreprocessorValue *parse_preprocessor_additive(void)
{
    PreprocessorValue *left, *right;
    left = parse_preprocessor_multiplicative();
    for (;;) {
        if (accept_preprocessor_token(OP_ADD)) {
            right = parse_preprocessor_multiplicative();
            left = evaluate_preprocessor_binary(OP_ADD, left, right);
            continue;
        }
        if (accept_preprocessor_token(OP_SUBTRACT)) {
            right = parse_preprocessor_multiplicative();
            left = evaluate_preprocessor_binary(OP_SUBTRACT, left, right);
            continue;
        }
        if (accept_preprocessor_token(OP_ADD)) {
            right = parse_preprocessor_multiplicative();
            left = evaluate_preprocessor_binary(OP_ADD, left, right);
            continue;
        }
        return left;
    }
}

PreprocessorValue *parse_preprocessor_unary(void);

/* Original 0x18a30..0x18a8e. */
PreprocessorValue *parse_preprocessor_multiplicative(void)
{
    PreprocessorValue *left, *right;
    left = parse_preprocessor_unary();
    for (;;) {
        if (accept_preprocessor_token(OP_MULTIPLY)) {
            right = parse_preprocessor_unary();
            left = evaluate_preprocessor_binary(OP_MULTIPLY, left, right);
            continue;
        }
        if (accept_preprocessor_token(OP_DIVIDE)) {
            right = parse_preprocessor_unary();
            left = evaluate_preprocessor_binary(OP_DIVIDE, left, right);
            continue;
        }
        if (accept_preprocessor_token(OP_REMAINDER)) {
            right = parse_preprocessor_unary();
            left = evaluate_preprocessor_binary(OP_REMAINDER, left, right);
            continue;
        }
        return left;
    }
}

PreprocessorValue *parse_preprocessor_primary(void);

/* Original 0x18a90..0x18b0f, ending with the tail transfer to primary. */
PreprocessorValue *parse_preprocessor_unary(void)
{
    PreprocessorValue *value;
    if (accept_preprocessor_token(OP_NOT)) {
        value = parse_preprocessor_unary();
        return evaluate_preprocessor_binary(OP_NOT, value, 0);
    }
    if (accept_preprocessor_token(OP_COMPLEMENT)) {
        value = parse_preprocessor_unary();
        return evaluate_preprocessor_binary(OP_COMPLEMENT, value, 0);
    }
    if (accept_preprocessor_token(OP_ADD)) {
        value = parse_preprocessor_unary();
        return evaluate_preprocessor_binary(OP_UNARY_PLUS, value, 0);
    }
    if (accept_preprocessor_token(OP_SUBTRACT)) {
        value = parse_preprocessor_unary();
        return evaluate_preprocessor_binary(OP_NEGATE, value, 0);
    }
    return parse_preprocessor_primary();
}

int __stdcall scan_preprocessor_token(unsigned char token);

/* Original 0x18140..0x18178. The fetch sentinel tail-enters the
   scanner with the same four-byte argument and cleanup contract. */
int __stdcall accept_preprocessor_token(unsigned char token)
{
    if (preprocessor_expression_token == 1) return 0;
    if (preprocessor_expression_token == 0x93) return scan_preprocessor_token(token);
    if (token == preprocessor_expression_token) {
        preprocessor_expression_token = 0x93;
        return 1;
    }
    return 0;
}

/* Original 0x97840..0x97847: only the type WORD and value DWORD
   are written by the scanner; alignment bytes carry no data credit. */
PreprocessorValue preprocessor_literal;

/* Original 0x18180..0x18242. Identifier and integral-literal tokens
   share the cached numeric token, while punctuation retains its lexical code. */
int __stdcall scan_preprocessor_token(unsigned char token)
{
    unsigned char actual;
    IrToken *expression;
    actual = read_parser_token();
    if (actual == 2) {
        preprocessor_literal.value = check_defined_macro(current_token_value.name);
        preprocessor_literal.type = 0x64;
    } else if (actual == 0x6f) {
        expression = current_token_value.expression;
        if (!(expression->type->flags & 0x40)) fatal_error(17);
        else {
            preprocessor_literal.value = expression->value.integer;
            preprocessor_literal.type = expression->type->flags;
        }
    } else {
        if (token == current_token_code) return 1;
        preprocessor_expression_token = current_token_code;
        return 0;
    }
    if (token == 7) return 1;
    preprocessor_expression_token = 7;
    return 0;
}

/* Original 0x18b10..0x18ba2. Expression values use the dedicated
   eight-byte arena; parentheses retain the same parsed value object. */
PreprocessorValue *parse_preprocessor_primary(void)
{
    ArenaPool *pool;
    PreprocessorValue *value;
    if (accept_preprocessor_token(0x80)) {
        ++preprocessor_expression_parentheses;
        value = parse_preprocessor_conditional();
        if (!accept_preprocessor_token(0x88)) fatal_error(12, ')');
        --preprocessor_expression_parentheses;
        return value;
    }
    if (!accept_preprocessor_token(7)) fatal_error(17);
    pool = &compiler_arena.pools[4];
    if (pool->remaining < (int)sizeof(PreprocessorValue))
        refill_arena(&compiler_arena, pool);
    value = (PreprocessorValue *)pool->next;
    pool->remaining -= sizeof(PreprocessorValue);
    pool->next += sizeof(PreprocessorValue);
    *value = preprocessor_literal;
    return value;
}

/* Original 0x18250..0x185b0; its 43-way token switch occupies
   0x185b4..0x1865f. Values are machine DWORDs; unsigned arithmetic keeps
   the original wrapping operations defined in the native C build. */
PreprocessorValue *__stdcall evaluate_preprocessor_binary(unsigned char operation,
    PreprocessorValue *left, PreprocessorValue *right)
{
    unsigned short left_type = left->type, width = left_type & 15;
    int unsigned_mode = (left_type & 0x4000) || (right && (right->type & 0x4000));
    if (right && (right->type & 15) > width) width = right->type & 15;
    if (width == 1) unsigned_mode = 0;
    switch (operation) {
    case OP_BIT_AND: left->value &= right->value; break;
    case OP_LOGICAL_AND:
        left->value = left->value && right->value;
        unsigned_mode = 0;
        break;
    case OP_DIVIDE:
        if (!right->value) {
            if (!preprocessor_skip_evaluation) report_syntax_error(124);
        } else if (unsigned_mode)
            left->value = (unsigned long)left->value / (unsigned long)right->value;
        else left->value /= right->value;
        break;
    case OP_EQUAL:
        left->value = left->value == right->value;
        unsigned_mode = 0;
        break;
    case OP_NOT:
        left->value = !left->value;
        unsigned_mode = 0;
        break;
    case OP_GREATER:
        left->value = unsigned_mode ?
            (unsigned long)left->value > (unsigned long)right->value : left->value > right->value;
        unsigned_mode = 0;
        break;
    case OP_GREATER_EQUAL:
        left->value = unsigned_mode ?
            (unsigned long)left->value >= (unsigned long)right->value : left->value >= right->value;
        unsigned_mode = 0;
        break;
    case OP_SHIFT_LEFT:
        unsigned_mode = (left_type & 0x4000) ||
            (options.configuration.option_J && (left_type & 0x2fdf) == 0x41 && !(left_type & 0x8000));
        left->value = (unsigned long)left->value << (right->value & 31);
        break;
    case OP_LESS:
        left->value = unsigned_mode ?
            (unsigned long)left->value < (unsigned long)right->value : left->value < right->value;
        unsigned_mode = 0;
        break;
    case OP_LESS_EQUAL:
        left->value = unsigned_mode ?
            (unsigned long)left->value <= (unsigned long)right->value : left->value <= right->value;
        unsigned_mode = 0;
        break;
    case OP_SUBTRACT:
        left->value = (unsigned long)left->value - (unsigned long)right->value;
        break;
    case OP_REMAINDER:
        if (!right->value) {
            if (!preprocessor_skip_evaluation) report_syntax_error(124);
        } else if (unsigned_mode)
            left->value = (unsigned long)left->value % (unsigned long)right->value;
        else left->value %= right->value;
        break;
    case OP_MULTIPLY:
        left->value = (unsigned long)left->value * (unsigned long)right->value;
        break;
    case OP_NOT_EQUAL:
        left->value = left->value != right->value;
        unsigned_mode = 0;
        break;
    case OP_BIT_OR: left->value |= right->value; break;
    case OP_LOGICAL_OR:
        left->value = left->value || right->value;
        unsigned_mode = 0;
        break;
    case OP_ADD:
        left->value = (unsigned long)left->value + (unsigned long)right->value;
        break;
    case OP_SHIFT_RIGHT:
        unsigned_mode = (left_type & 0x4000) ||
            (options.configuration.option_J && (left_type & 0x2fdf) == 0x41 && !(left_type & 0x8000));
        if (unsigned_mode) left->value = (unsigned long)left->value >> (right->value & 31);
        else left->value >>= right->value & 31;
        break;
    case OP_COMPLEMENT: left->value = ~left->value; break;
    case OP_NEGATE: left->value = 0UL - (unsigned long)left->value; break;
    case OP_BIT_XOR: left->value ^= right->value; break;
    }
    left->type = unsigned_mode ? 0x4044 : 0x44;
    return left;
}
