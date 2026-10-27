/* The preprocessor expression parser has its own one-token lookahead and
 * sixteen-byte integer records. It does not use the language LR parser. */
#include "frontend.h"
#include "parser-actions.h"
#include "parser-grammar.h"
#include <string.h>
#include <stdio.h>
#include <recon.h>
#pragma intrinsic(memcpy)
#pragma intrinsic(strcmp)

/* Only the QWORD is consumed by the precedence routines. Record producers
 * establish the leading type and representation bytes separately. */
typedef CompilerConditionalValue ConditionalValue;

enum ConditionalOperator {
    COND_END = 0x2d, COND_BIT_AND = 0x31, COND_LOGICAL_AND = 0x32,
    COND_COLON = 0x36, COND_DIVIDE = 0x39, COND_EQUAL = 0x3b,
    COND_NOT = 0x3c, COND_GREATER = 0x3e, COND_GREATER_EQUAL = 0x3f,
    COND_LEFT_SHIFT = 0x43, COND_LESS = 0x44, COND_LESS_EQUAL = 0x45,
    COND_MINUS = 0x46, COND_REMAINDER = 0x48, COND_MULTIPLY = 0x4a,
    COND_NOT_EQUAL = 0x4c, COND_BIT_OR = 0x4d, COND_LOGICAL_OR = 0x4f,
    COND_PLUS = 0x51, COND_QUESTION = 0x56, COND_RIGHT_SHIFT = 0x58,
    COND_COMPLEMENT = 0x59, COND_NEGATE = 0x5a, COND_BIT_XOR = 0x5b,
    COND_VALUE = 0x65, COND_LEFT_PAREN = 0x76, COND_RIGHT_PAREN = 0x7e,
    COND_POSITIVE = 0x86, COND_EMPTY = 0x88
};

extern "C" {
ConditionalValue *__cdecl parse_conditional_choice(void); /*41632*/
ConditionalValue *__cdecl parse_conditional_logical_or(void); /*41652*/
ConditionalValue *__cdecl parse_conditional_logical_and(void); /*416ad*/
ConditionalValue *__cdecl parse_conditional_bit_or(void); /*4170a*/
ConditionalValue *__cdecl parse_conditional_bit_xor(void); /*41728*/
ConditionalValue *__cdecl parse_conditional_bit_and(void); /*41746*/
ConditionalValue *__cdecl parse_conditional_equality(void); /*41764*/
ConditionalValue *__cdecl parse_conditional_relation(void); /*417a3*/
ConditionalValue *__cdecl parse_conditional_shift(void); /*41833*/
ConditionalValue *__cdecl parse_conditional_sum(void); /*41863*/
ConditionalValue *__cdecl parse_conditional_product(void); /*418a5*/
ConditionalValue *__cdecl parse_conditional_unary(void); /*418e7*/
ConditionalValue *__cdecl parse_conditional_primary(void); /*41a15*/
int __cdecl match_conditional_token(unsigned); /*41943, only low BYTE is read*/
ConditionalValue *__cdecl apply_conditional_operator(unsigned, ConditionalValue *, ConditionalValue *); /*41ab0*/
int __cdecl evaluate_conditional_identifier(CompilerNameEntry *); /*41d87*/
int __cdecl skip_conditional_whitespace(void); /*f154*/
int __cdecl scan_identifier(unsigned); /*e339, low BYTE*/
int __cdecl scan_macro_identifier(unsigned); /*ed78, low BYTE*/
unsigned __cdecl process_special_character(void); /*ef91*/
void __cdecl grow_work_buffer(BYTE *, BYTE *, BYTE **); /*80a31*/
}

extern "C" __int64 __cdecl evaluate_preprocessor_condition(void)
{
    ConditionalValue *value;
    __int64 result;
    frontend_data.conditional_parentheses = 0;
    frontend_data.conditional_token = COND_EMPTY;
    value = parse_conditional_choice();
    if (frontend_data.conditional_token == COND_RIGHT_PAREN) {
        if (frontend_data.conditional_parentheses-- == 0)
            report_compiler_diagnostic(12, 40);
    } else if (frontend_data.conditional_token != COND_END) {
        report_language_diagnostic(1, 67, "#if/#elif");
    }
    if (frontend_data.conditional_parentheses > 0)
        report_compiler_diagnostic(12, 41);
    result = value->integer;
    frontend_bss.arenas[7].ResetStorage(); /* actual receiver c65e0 */
    return result;
}

extern "C" ConditionalValue *__cdecl parse_conditional_choice(void)
{
    ConditionalValue *condition = parse_conditional_logical_or();
    if (match_conditional_token(COND_QUESTION)) {
        ConditionalValue *selected = parse_conditional_logical_or();
        ConditionalValue *otherwise;
        if (!match_conditional_token(COND_COLON)) {
            /* The original missing-colon branch reads an unwritten stack
             * slot. It has no supported value representation. */
            __asm { int 3 }
        }
        otherwise = parse_conditional_logical_or();
        return condition->integer ? selected : otherwise;
    }
    return condition;
}

extern "C" ConditionalValue *__cdecl parse_conditional_logical_or(void)
{
    int saved_suppression = frontend_bss.conditional_suppression;
    ConditionalValue *value = parse_conditional_logical_and();
    while (match_conditional_token(COND_LOGICAL_OR)) {
        if (value->integer) ++frontend_bss.conditional_suppression;
        ConditionalValue *right = parse_conditional_logical_and();
        value = apply_conditional_operator(COND_LOGICAL_OR, value, right);
    }
    frontend_bss.conditional_suppression = saved_suppression;
    return value;
}

extern "C" ConditionalValue *__cdecl parse_conditional_logical_and(void)
{
    int saved_suppression = frontend_bss.conditional_suppression;
    ConditionalValue *value = parse_conditional_bit_or();
    while (match_conditional_token(COND_LOGICAL_AND)) {
        if (!value->integer) ++frontend_bss.conditional_suppression;
        ConditionalValue *right = parse_conditional_bit_or();
        value = apply_conditional_operator(COND_LOGICAL_AND, value, right);
    }
    frontend_bss.conditional_suppression = saved_suppression;
    return value;
}

extern "C" ConditionalValue *__cdecl parse_conditional_bit_or(void)
{
    ConditionalValue *value = parse_conditional_bit_xor();
    while (match_conditional_token(COND_BIT_OR)) {
        ConditionalValue *right = parse_conditional_bit_xor();
        value = apply_conditional_operator(COND_BIT_OR, value, right);
    }
    return value;
}

extern "C" ConditionalValue *__cdecl parse_conditional_bit_xor(void)
{
    ConditionalValue *value = parse_conditional_bit_and();
    while (match_conditional_token(COND_BIT_XOR)) {
        ConditionalValue *right = parse_conditional_bit_and();
        value = apply_conditional_operator(COND_BIT_XOR, value, right);
    }
    return value;
}

extern "C" ConditionalValue *__cdecl parse_conditional_bit_and(void)
{
    ConditionalValue *value = parse_conditional_equality();
    while (match_conditional_token(COND_BIT_AND)) {
        ConditionalValue *right = parse_conditional_equality();
        value = apply_conditional_operator(COND_BIT_AND, value, right);
    }
    return value;
}

extern "C" ConditionalValue *__cdecl parse_conditional_equality(void)
{
    ConditionalValue *value = parse_conditional_relation();
    if (match_conditional_token(COND_EQUAL)) {
        ConditionalValue *right = parse_conditional_relation();
        return apply_conditional_operator(COND_EQUAL, value, right);
    }
    if (match_conditional_token(COND_NOT_EQUAL)) {
        ConditionalValue *right = parse_conditional_relation();
        return apply_conditional_operator(COND_NOT_EQUAL, value, right);
    }
    return value;
}

extern "C" ConditionalValue *__cdecl parse_conditional_relation(void)
{
    ConditionalValue *value = parse_conditional_shift();
    if (match_conditional_token(COND_LESS)) {
        ConditionalValue *right = parse_conditional_shift();
        return apply_conditional_operator(COND_LESS, value, right);
    }
    if (match_conditional_token(COND_GREATER)) {
        ConditionalValue *right = parse_conditional_shift();
        return apply_conditional_operator(COND_GREATER, value, right);
    }
    if (match_conditional_token(COND_LESS_EQUAL)) {
        ConditionalValue *right = parse_conditional_shift();
        return apply_conditional_operator(COND_LESS_EQUAL, value, right);
    }
    if (match_conditional_token(COND_GREATER_EQUAL)) {
        ConditionalValue *right = parse_conditional_shift();
        return apply_conditional_operator(COND_GREATER_EQUAL, value, right);
    }
    return value;
}

extern "C" ConditionalValue *__cdecl parse_conditional_shift(void)
{
    ConditionalValue *value = parse_conditional_sum();
    if (match_conditional_token(COND_RIGHT_SHIFT)) {
        ConditionalValue *right = parse_conditional_sum();
        return apply_conditional_operator(COND_RIGHT_SHIFT, value, right);
    }
    if (match_conditional_token(COND_LEFT_SHIFT)) {
        ConditionalValue *right = parse_conditional_sum();
        return apply_conditional_operator(COND_LEFT_SHIFT, value, right);
    }
    return value;
}

extern "C" ConditionalValue *__cdecl parse_conditional_sum(void)
{
    ConditionalValue *value = parse_conditional_product();
    for (;;) {
        if (match_conditional_token(COND_PLUS)) {
            ConditionalValue *right = parse_conditional_product();
            value = apply_conditional_operator(COND_PLUS, value, right);
        } else if (match_conditional_token(COND_MINUS)) {
            ConditionalValue *right = parse_conditional_product();
            value = apply_conditional_operator(COND_MINUS, value, right);
        } else if (match_conditional_token(COND_PLUS)) {
            ConditionalValue *right = parse_conditional_product();
            value = apply_conditional_operator(COND_PLUS, value, right);
        } else return value;
    }
}

extern "C" ConditionalValue *__cdecl parse_conditional_product(void)
{
    ConditionalValue *value = parse_conditional_unary();
    for (;;) {
        if (match_conditional_token(COND_MULTIPLY)) {
            ConditionalValue *right = parse_conditional_unary();
            value = apply_conditional_operator(COND_MULTIPLY, value, right);
        } else if (match_conditional_token(COND_DIVIDE)) {
            ConditionalValue *right = parse_conditional_unary();
            value = apply_conditional_operator(COND_DIVIDE, value, right);
        } else if (match_conditional_token(COND_REMAINDER)) {
            ConditionalValue *right = parse_conditional_unary();
            value = apply_conditional_operator(COND_REMAINDER, value, right);
        } else return value;
    }
}

extern "C" ConditionalValue *__cdecl parse_conditional_unary(void)
{
    if (match_conditional_token(COND_NOT))
        return apply_conditional_operator(COND_NOT, parse_conditional_unary(), NULL);
    if (match_conditional_token(COND_COMPLEMENT))
        return apply_conditional_operator(COND_COMPLEMENT, parse_conditional_unary(), NULL);
    if (match_conditional_token(COND_PLUS))
        return apply_conditional_operator(COND_POSITIVE, parse_conditional_unary(), NULL);
    if (match_conditional_token(COND_MINUS))
        return apply_conditional_operator(COND_NEGATE, parse_conditional_unary(), NULL);
    return parse_conditional_primary();
}

extern "C" int __cdecl match_conditional_token(unsigned requested)
{
    BYTE token = frontend_data.conditional_token;
    BYTE expected = (BYTE)requested;
    if (token == COND_END) return 0;
    if (token != COND_EMPTY) {
        if (token != expected) return 0;
        frontend_data.conditional_token = COND_EMPTY;
        return 1;
    }
    ParserTokenView *value = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
    if (value->kind >= PARSER_TOKEN_KIND_COUNT) { __asm { int 3 } }
    unsigned category = parser_token_classes[value->kind];
    if (category == 0x65) {
        frontend_bss.conditional_value.type_code = value->value.literal.type->flags;
        if (!(frontend_bss.conditional_value.type_code & 0x40)) report_compiler_error(17);
        frontend_bss.conditional_value.integer = value->value.literal.value;
        if (expected == COND_VALUE) return 1;
    } else if (category == 6) {
        frontend_bss.conditional_value.integer = evaluate_conditional_identifier((CompilerNameEntry *)value->value.pointer);
        frontend_bss.conditional_value.type_code = 0x63;
        if (expected == COND_VALUE) return 1;
    } else {
        if (value->kind == expected) return 1;
        frontend_data.conditional_token = value->kind;
        return 0;
    }
    frontend_data.conditional_token = COND_VALUE;
    return 0;
}
extern "C" ConditionalValue *__cdecl parse_conditional_primary(void)
{
    ConditionalValue *value;
    if (match_conditional_token(COND_LEFT_PAREN)) {
        ++frontend_data.conditional_parentheses;
        value = parse_conditional_choice();
        if (!match_conditional_token(COND_RIGHT_PAREN))
            report_compiler_diagnostic(12, 41);
        --frontend_data.conditional_parentheses;
        return value;
    }
    if (!match_conditional_token(COND_VALUE)) report_compiler_error(17);
    CompilerArena *arena = &frontend_bss.arenas[7];
    value = (ConditionalValue *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    memcpy(value, &frontend_bss.conditional_value, 16);
    return value;
}
/* Primitive codes carry a table-selected width; other scalar encodings put
 * their width directly in the low nibble. This expression is inlined at its
 * original consumers, rather than adding an application graph vertex. */
#define CONDITIONAL_WIDTH(code) (((code) & 0x340) || !((code) & 0xfff0) ? \
    compiler_primitive_type_bytes[(code) & 15] : ((code) & 15))
extern "C" ConditionalValue *__cdecl apply_conditional_operator(unsigned operation,
    ConditionalValue *left, ConditionalValue *right)
{
    int unsigned_result = (left->type_code & 0x4000) ||
        (right && (right->type_code & 0x4000));
    unsigned width = CONDITIONAL_WIDTH(left->type_code);
    unsigned count;
    int shift_unsigned;
    if (right && width < (unsigned)CONDITIONAL_WIDTH(right->type_code))
        width = CONDITIONAL_WIDTH(right->type_code);
    if (width == (unsigned)compiler_primitive_type_bytes[1]) unsigned_result = 0;
    switch ((BYTE)operation) {
    case COND_BIT_AND:
        left->integer &= right->integer;
        break;
    case COND_BIT_OR:
        left->integer |= right->integer;
        break;
    case COND_BIT_XOR:
        left->integer ^= right->integer;
        break;
    case COND_LOGICAL_OR:
        left->integer = left->integer || right->integer;
        unsigned_result = 0;
        break;
    case COND_LOGICAL_AND:
        left->integer = left->integer && right->integer;
        unsigned_result = 0;
        break;
    case COND_EQUAL:
        left->integer = left->integer == right->integer;
        unsigned_result = 0;
        break;
    case COND_NOT_EQUAL:
        left->integer = left->integer != right->integer;
        unsigned_result = 0;
        break;
    case COND_LESS:
        left->integer = unsigned_result ?
            (unsigned __int64)left->integer < (unsigned __int64)right->integer :
            left->integer < right->integer;
        unsigned_result = 0;
        break;
    case COND_GREATER:
        left->integer = unsigned_result ?
            (unsigned __int64)left->integer > (unsigned __int64)right->integer :
            left->integer > right->integer;
        unsigned_result = 0;
        break;
    case COND_LESS_EQUAL:
        left->integer = unsigned_result ?
            (unsigned __int64)left->integer <= (unsigned __int64)right->integer :
            left->integer <= right->integer;
        unsigned_result = 0;
        break;
    case COND_GREATER_EQUAL:
        left->integer = unsigned_result ?
            (unsigned __int64)left->integer >= (unsigned __int64)right->integer :
            left->integer >= right->integer;
        unsigned_result = 0;
        break;
    case COND_NOT:
        left->integer = !left->integer;
        unsigned_result = 0;
        break;
    case COND_RIGHT_SHIFT:
        shift_unsigned = (left->type_code & 0x4000) ||
            (frontend_data.options.unsigned_char && (left->type_code & 0x2fdf) == 0x41 &&
             !(left->type_code & 0x8000));
        count = (BYTE)right->integer;
        if (shift_unsigned)
            left->integer = count < 64 ? (unsigned __int64)left->integer >> count : 0;
        else
            left->integer = count < 64 ? left->integer >> count : (left->integer < 0 ? -1 : 0);
        unsigned_result = shift_unsigned;
        break;
    case COND_LEFT_SHIFT:
        unsigned_result = (left->type_code & 0x4000) ||
            (frontend_data.options.unsigned_char && (left->type_code & 0x2fdf) == 0x41 &&
             !(left->type_code & 0x8000));
        count = (BYTE)right->integer;
        left->integer = count < 64 ? (unsigned __int64)left->integer << count : 0;
        break;
    case COND_PLUS:
        left->integer = (unsigned __int64)left->integer + (unsigned __int64)right->integer;
        break;
    case COND_MINUS:
        left->integer = (unsigned __int64)left->integer - (unsigned __int64)right->integer;
        break;
    case COND_MULTIPLY:
        left->integer = (unsigned __int64)left->integer * (unsigned __int64)right->integer;
        break;
    case COND_DIVIDE:
        if (!right->integer) {
            if (!frontend_bss.conditional_suppression) compiler_warning(124);
        } else if (unsigned_result)
            left->integer = (unsigned __int64)left->integer / (unsigned __int64)right->integer;
        else left->integer /= right->integer;
        break;
    case COND_REMAINDER:
        if (!right->integer) {
            if (!frontend_bss.conditional_suppression) compiler_warning(124);
        } else if (unsigned_result)
            left->integer = (unsigned __int64)left->integer % (unsigned __int64)right->integer;
        else left->integer %= right->integer;
        break;
    case COND_COMPLEMENT:
        left->integer = ~left->integer;
        break;
    case COND_NEGATE:
        left->integer = (unsigned __int64)0 - (unsigned __int64)left->integer;
        break;
    }
    left->type_code = unsigned_result ? 0x4044 : 0x44;
    return left;
}
#undef CONDITIONAL_WIDTH
extern "C" int __cdecl evaluate_conditional_identifier(CompilerNameEntry *identifier)
{
    int result = 0, parentheses = 0;
    BYTE character;
    unsigned category;
    if (strcmp(identifier->name, "defined")) return 0;
    if (!skip_conditional_whitespace() && !frontend_bss.macro_input_mode) return 0;
    if (*frontend_bss.source_cursor == '(') {
        parentheses = 1;
        ++frontend_bss.source_cursor;
        if (!skip_conditional_whitespace() && !frontend_bss.macro_input_mode) return 0;
    }
    character = *frontend_bss.source_cursor;
    if (frontend_bss.macro_input_mode > 0) {
        if (frontend_data.scanner.classes[character] == 0x26 ||
            (frontend_data.scanner.flags[character] & 1)) {
            ++frontend_bss.source_cursor;
            scan_macro_identifier(character);
            result = frontend_data.current_identifier->macro != NULL;
        }
    } else if (frontend_data.scanner.flags[character] & 1) {
        ++frontend_bss.source_cursor;
        scan_identifier(character);
        result = frontend_data.current_identifier->macro != NULL;
    } else {
        compiler_warning(parentheses ? 4 : 3);
    }
    character = *frontend_bss.source_cursor;
    category = frontend_data.scanner.classes[character];
    if ((category == 1 || category == 0) && !skip_conditional_whitespace()) return result;
    if (parentheses) {
        if (*frontend_bss.source_cursor == ')') {
            ++frontend_bss.source_cursor;
            --parentheses;
        }
        if (parentheses > 0 && !frontend_bss.macro_input_mode)
            report_language_diagnostic(1, 4);
    }
    return result;
}

extern "C" int __cdecl skip_conditional_whitespace(void)
{
    int found_character = 0;
    BYTE character;
    unsigned special;
    frontend_bss.macro_whitespace_pending = 0;
    frontend_data.compiler_flags &= ~0x400000U;
    for (;;) {
        character = *frontend_bss.source_cursor++;
        switch (frontend_data.scanner.classes[character]) {
        case SC_SPECIAL:
            special = process_special_character();
            if (special & 0x0a) goto finish;
            if (special & ~0x50U) continue;
            goto unread_character;
        case SC_NEWLINE:
            if (frontend_data.compiler_flags & 0x8000) {
                --frontend_bss.source_cursor;
                goto finish;
            }
            ++frontend_bss.saved_context_stack.position;
            if (frontend_bss.options.preprocess_stdout)
                fputc(character, frontend_bss.preprocessor_output);
            frontend_data.compiler_flags |= 0x400000;
            /* Fall through: a consumed newline is also whitespace. */
        case SC_SPACE:
            frontend_bss.macro_whitespace_pending = 1;
            break;
        default:
        unread_character:
            --frontend_bss.source_cursor;
            found_character = 1;
            goto finish;
        }
    }
finish:
    if (frontend_bss.macro_whitespace_pending) {
        if (frontend_bss.macro_text_cursor >= frontend_bss.work_buffers[1].limit)
            grow_work_buffer(frontend_bss.macro_text_cursor, frontend_bss.work_buffers[1].data,
                &frontend_bss.work_buffers[1].limit);
        if (frontend_bss.macro_text_cursor > frontend_bss.work_buffers[1].data &&
            frontend_bss.macro_text_cursor[-1] != ' ')
            *frontend_bss.macro_text_cursor++ = ' ';
    }
    return found_character;
}
extern "C" int __cdecl scan_macro_identifier(unsigned) { __asm { int 3 } return 0; }
