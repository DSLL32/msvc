/* Conditional expression front end, original [0100e3e0,0100e4a9). */
#include "condition.h"
int rc_expression_depth;
long rc_expression_value;
unsigned char rc_expression_token;
int rc_evaluate_condition(void)
{
    long value;
    rc_expression_depth = 0;
    rc_expression_token = TK_NEWLINE;
    value = rc_conditional_expression();
    if (rc_expression_token == TK_RPAREN) {
        if (!rc_expression_depth--) {
            rc_message_format = rc_message(1012);
            rc_format(rc_diagnostic_buffer, rc_message_format, "(");
            rc_fatal(1012);
        }
    } else if (rc_expression_token != TK_NEWLINE) {
        rc_message_format = rc_message(4067);
        rc_format(rc_diagnostic_buffer, rc_message_format, "#if/#elif");
        rc_warning(4067);
    }
    if (rc_expression_depth > 0) {
        rc_message_format = rc_message(4012);
        rc_format(rc_diagnostic_buffer, rc_message_format, ")");
        rc_fatal(4012);
    }
    return value;
}

/* Original [0100e4b0,0100e4f8). Both arms are evaluated, as in the target. */
long rc_conditional_expression(void)
{
    long condition = rc_logical_or_expression(), yes, no;
    if (!rc_match_expression_token(TK_QUESTION)) return condition;
    yes = rc_logical_or_expression();
    if (rc_match_expression_token(TK_COLON)) no = rc_logical_or_expression();
    return condition ? yes : no;
}

long rc_logical_or_expression(void)
{
    long value = rc_logical_and_expression(), right;
    while (rc_match_expression_token(TK_LOGICAL_OR)) {
        right = rc_logical_and_expression();
        value = value || right;
    }
    return value;
}
long rc_logical_and_expression(void)
{
    long value = rc_bitwise_or_expression(), right;
    while (rc_match_expression_token(TK_LOGICAL_AND)) {
        right = rc_bitwise_or_expression();
        value = value && right;
    }
    return value;
}
long rc_bitwise_or_expression(void)
{
    long value = rc_bitwise_xor_expression();
    while (rc_match_expression_token(TK_OR)) value |= rc_bitwise_xor_expression();
    return value;
}
long rc_bitwise_xor_expression(void)
{
    long value = rc_bitwise_and_expression();
    while (rc_match_expression_token(TK_XOR)) value ^= rc_bitwise_and_expression();
    return value;
}
long rc_bitwise_and_expression(void)
{
    long value = rc_equality_expression();
    while (rc_match_expression_token(TK_AND)) value &= rc_equality_expression();
    return value;
}
/* These three levels accept one operator rather than a repeated chain. */
long rc_equality_expression(void)
{
    long value = rc_relational_expression();
    if (rc_match_expression_token(TK_EQUAL)) return value == rc_relational_expression();
    if (rc_match_expression_token(TK_NOT_EQUAL)) return value != rc_relational_expression();
    return value;
}
long rc_relational_expression(void)
{
    long value = rc_shift_expression();
    if (rc_match_expression_token(TK_LESS)) return value < rc_shift_expression();
    if (rc_match_expression_token(TK_GREATER)) return value > rc_shift_expression();
    if (rc_match_expression_token(TK_LESS_EQUAL)) return value <= rc_shift_expression();
    if (rc_match_expression_token(TK_GREATER_EQUAL)) return value >= rc_shift_expression();
    return value;
}
long rc_shift_expression(void)
{
    long value = rc_additive_expression();
    if (rc_match_expression_token(TK_RSHIFT)) return value >> (rc_additive_expression() & 31);
    if (rc_match_expression_token(TK_LSHIFT)) return value << (rc_additive_expression() & 31);
    return value;
}
long rc_additive_expression(void)
{
    long value = rc_multiplicative_expression();
    for (;;) {
        if (rc_match_expression_token(TK_PLUS)) value += rc_multiplicative_expression();
        else if (rc_match_expression_token(TK_MINUS)) value -= rc_multiplicative_expression();
        else return value;
    }
}
long rc_multiplicative_expression(void)
{
    long value = rc_unary_expression();
    for (;;) {
        if (rc_match_expression_token(TK_MULTIPLY)) value *= rc_unary_expression();
        else if (rc_match_expression_token(TK_DIVIDE)) value /= rc_unary_expression();
        else if (rc_match_expression_token(TK_MODULUS)) value %= rc_unary_expression();
        else return value;
    }
}
long rc_unary_expression(void)
{
    if (rc_match_expression_token(TK_BANG)) return !rc_primary_expression();
    if (rc_match_expression_token(TK_TILDE)) return ~rc_primary_expression();
    if (rc_match_expression_token(TK_MINUS)) return -rc_primary_expression();
    return rc_primary_expression();
}
long rc_primary_expression(void)
{
    long value;
    if (rc_match_expression_token(TK_LPAREN)) {
        ++rc_expression_depth;
        value = rc_conditional_expression();
        if (rc_match_expression_token(TK_RPAREN)) {
            --rc_expression_depth;
            return value;
        }
        rc_message_format = rc_message(1012);
        rc_format(rc_diagnostic_buffer, rc_message_format, ")");
        rc_fatal(1012);
    } else if (!rc_match_expression_token(TK_INT)) {
        rc_message_format = rc_message(1017);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1017);
    }
    return rc_expression_value;
}

/* Original [0100e1e0,0100e2ec). A mismatched token is retained for the
 * caller at the next precedence level. Identifier and constant operands are
 * converted to the same cached integer token while retaining their value. */
int __stdcall rc_match_expression_token(unsigned char wanted)
{
    unsigned category;
    RcConstant *constant;
    if (rc_expression_token != TK_NEWLINE) {
        if (rc_expression_token != wanted) return 0;
        rc_expression_token = TK_NEWLINE;
        return 1;
    }
    category = rc_next_token();
    switch (category) {
    case 0:
        return 0;
    case TK_IDENTIFIER:
        rc_expression_value = rc_identifier_value(rc_token_value);
        break;
    case TK_CONSTANT:
        constant = rc_token_value;
        if ((constant->type->flags & 15) > 5) {
            rc_message_format = rc_message(1017);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_fatal(1017);
        } else rc_expression_value = constant->value.integer;
        break;
    default:
        if (rc_token_kind == wanted) return 1;
        rc_expression_token = rc_token_kind;
        return 0;
    }
    if (wanted == TK_INT) return 1;
    rc_expression_token = TK_INT;
    return 0;
}

/* Original [0100dff0,0100e1de): unknown identifiers evaluate to zero;
 * defined's operand is looked up without expanding it. */
long __stdcall rc_identifier_value(char *name)
{
    unsigned i;
    int value = 0, parenthesized = 0;
    unsigned char ch;
    for (i = 0; i < 8; ++i) if (name[i] != "defined"[i]) return 0;
    if (!rc_skip_macro_whitespace() && !rc_collecting_macro_text) return 0;
    if (*rc_input_cursor == '(') {
        parenthesized = 1;
        *rc_expansion_output++ = *rc_input_cursor++;
        if (!rc_skip_macro_whitespace() && !rc_collecting_macro_text) return 0;
    }
    ch = *rc_input_cursor;
    if (rc_collecting_macro_text > 0) {
        if (rc_character_class[ch] == LC_INTERNAL_1 ||
            rc_character_class[ch] == LC_IDENTIFIER) {
            ++rc_input_cursor;
            rc_collect_macro_identifier(ch);
        }
    } else if (rc_character_class[ch] == LC_IDENTIFIER) {
        ++rc_input_cursor;
        if (rc_expanding_macro > 0) rc_read_expanded_identifier(ch);
        else rc_read_identifier(ch);
        value = rc_find_macro() != NULL;
    } else {
        if (!parenthesized) {
            rc_message_format = rc_message(2003);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2003);
        } else {
            rc_message_format = rc_message(2004);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2004);
        }
    }
    ch = rc_character_class[*rc_input_cursor];
    if ((ch == LC_SPACE || ch == LC_END) && !rc_skip_macro_whitespace()) return value;
    if (parenthesized && rc_character_class[*rc_input_cursor] == LC_RPAREN) {
        *rc_expansion_output++ = *rc_input_cursor++;
        --parenthesized;
    }
    if (parenthesized > 0 && !rc_collecting_macro_text) {
        rc_message_format = rc_message(4004);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_warning(4004);
    }
    return value;
}

/* Original [0100bc30,0100bce0). Collapse macro whitespace into one byte
 * in the expansion work area, retaining newline for directive callers. */
int rc_skip_macro_whitespace(void)
{
    int found = 0, space = 0;
    unsigned status;
    for (;;) {
        switch (rc_character_class[*rc_input_cursor++]) {
        case LC_SPACE: case LC_CR:
            space = 1;
            continue;
        case LC_END:
            status = rc_resume_input();
            if (status & 10) break;
            if (status != 16) continue;
            --rc_input_cursor;
            found = 1;
            break;
        case LC_NEWLINE:
            if (rc_in_directive) { --rc_input_cursor; break; }
            ++rc_line_number;
            space = 1;
            continue;
        default:
            --rc_input_cursor;
            found = 1;
            break;
        }
        if (space) {
            if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
            if (rc_expansion_output[-1] != ' ') *rc_expansion_output++ = ' ';
        }
        return found;
    }
}
