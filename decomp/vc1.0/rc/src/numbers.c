/* RC's numeric syntax: decimal, lowercase 0x hexadecimal, explicit 0o octal,
 * unary minus/complement and the original left-to-right expression evaluator. */
#include "tokens.h"
#include <ctype.h>
#include <string.h>
#include <recon.h>
static char numeric_character;
static char *numeric_cursor;
static int expression_has_long, parenthesis_depth;

/* Original 0x010083a0..0x010083b3. Only AL is the return value. */
char __cdecl next_numeric_character(void)
{
    numeric_character = *numeric_cursor++;
    return numeric_character;
}
/* Original 0x01008ac0..0x01008b48. */
unsigned long __cdecl scan_hex_number(void)
{
    unsigned long value = 0;
    while (isxdigit(next_numeric_character())) {
        numeric_character = (char)rc_toupper(numeric_character);
        value = value * 16 + (numeric_character >= 'A' ?
                             numeric_character - 'A' + 10 : numeric_character - '0');
    }
    return value;
}
/* Original 0x01008b50..0x01008b7e. */
unsigned long __cdecl scan_octal_number(void)
{
    unsigned long value = 0;
    next_numeric_character();
    while (numeric_character >= '0' && numeric_character <= '7') {
        value = value * 8 + numeric_character - '0';
        next_numeric_character();
    }
    return value;
}
/* Original 0x01008b80..0x01008be2. */
unsigned long __cdecl scan_decimal_number(void)
{
    unsigned long value = 0;
    while (isdigit(numeric_character)) {
        value = value * 10 + numeric_character - '0';
        next_numeric_character();
    }
    return value;
}
/* Original 0x01008bf0..0x01008d1d. */
void __cdecl scan_number_literal(void)
{
    char *next;
    unsigned long value;
    int negative = 0, complement;
    numeric_character = source_character;
    current_token.spelling[0] = source_character;
    next = current_token.spelling + 1;
    while (!token_delimiters[advance_character()])
        *next++ = source_character;
    *next = 0;
    numeric_cursor = current_token.spelling + 1;
    current_token.kind = TOKEN_NUMBER;
    if (numeric_character == '-') {
        negative = 1;
        next_numeric_character();
    }
    complement = numeric_character == '~';
    if (complement)
        next_numeric_character();
    if (numeric_character == '0') {
        next_numeric_character();
        if (numeric_character == 'x')
            value = scan_hex_number();
        else if (numeric_character == 'o')
            value = scan_octal_number();
        else
            value = scan_decimal_number();
    } else
        value = scan_decimal_number();
    current_token.is_long = numeric_character == 'L' || numeric_character == 'l';
    if (current_token.is_long)
        next_numeric_character();
    if (negative)
        value = -value;
    if (complement)
        value = ~value;
    current_token.value = value;
    current_token.length = (WORD)value;
}
/* Original 0x01008d80..0x01008d85, a separate tail-call vertex. */
void __cdecl scan_number_token(void)
{
    scan_number_literal();
}
/* Original 0x01008d20..0x01008d7d. */
void __cdecl scan_expression(void)
{
    unsigned long value;
    expression_has_long = parenthesis_depth = 0;
    value = expression_value();
    if (parenthesis_depth)
        parser_error("Mismatched parentheses", "");
    if (expression_has_long)
        current_token.is_long = 1;
    current_token.kind = TOKEN_NUMBER;
    current_token.value = value;
    current_token.length = (WORD)value;
}
/* Original 0x01008d90..0x01008de6. */
unsigned long __cdecl expression_value(void)
{
    unsigned long value = expression_factor(), right;
    char operator;
    BOOL present;
    for (;;) {
        present = expression_operator(&operator);
        if (operator == ')') {
            --parenthesis_depth;
            advance_character();
            skip_expression_space();
        }
        if (!present)
            break;
        right = expression_factor();
        value = apply_operator(value, operator, right);
    }
    return value;
}
/* Original 0x01008df0..0x01008e4b. */
unsigned long __stdcall apply_operator(unsigned long left, char operator, unsigned long right)
{
    switch (operator) {
    case '&': return left & right;
    case '+': return left + right;
    case '-': return left - right;
    case '|': return left | right;
    default: return left;
    }
}
/* Original 0x01008e50..0x01008eea. */
unsigned long __cdecl expression_factor(void)
{
    if (source_character == '(') {
        ++parenthesis_depth;
        advance_character();
        skip_expression_space();
        return expression_value();
    }
    if (source_character != '-' && source_character != '~' && !isdigit(source_character))
        parser_error("Numeric value expected at '%c'", &source_character);
    scan_number_literal();
    if (current_token.is_long)
        expression_has_long = 1;
    skip_expression_space();
    return current_token.value;
}
/* Original 0x01008ef0..0x01008f32. */
BOOL __stdcall expression_operator(char *operator)
{
    char *found = strchr("+-|&", source_character);
    *operator = source_character;
    if (!found)
        return FALSE;
    advance_character();
    skip_expression_space();
    return TRUE;
}
/* Original 0x01008f40..0x01008f5f. */
void __cdecl skip_expression_space(void)
{
    while ((unsigned char)source_character <= ' ')
        advance_character();
}
