/* Token repair diagnostics and syntax-recovery semantic values. */
#include <stdarg.h>
#include "parser.h"
#include "inline-capture.h"
#include "type-diagnostics-internal.h"
#include "lexer.h"
#include "source-input.h"
#include <recon.h>

void discard_trailing_expression_records(void);

/* Original0x26190..0x2636c, stdcall8. Diagnose the actual token and
   inserted delimiter, formatting type names within their proper lifetime. */
void __stdcall report_inserted_token(int actual, int expected)
{
    short text;
    char *description;
    int temporary_text;
    QualifiedDeclarator *qualified;
    ++syntax_diagnostic_count;
    if (current_class_context >= 0) ++captured_class_error_count;
    switch (actual) {
    case 0:
        report_syntax_error(60);
        return;
    case 2:
        if (current_token_value.name)
            report_syntax_error(146, token_spellings[expected], current_token_value.name);
        else report_syntax_error(145, token_spellings[expected]);
        return;
    case 34:
    case 35:
    case 40:
    case 163:
        temporary_text = 0;
        if (type_specifier_symbol) description = type_specifier_symbol->name;
        else {
            begin_type_diagnostic();
            text = reserve_type_diagnostic();
            temporary_text = 1;
            format_declaration_base_type(text, actual == 163 ?
                current_token_value.expression->type : current_token_value.type_name.type);
            description = type_diagnostic_text(text);
        }
        if (expected == 138 && actual == 163 &&
            (!(token_allocation_flags & 0x80000) || (token_allocation_flags & 0x40)) &&
            current_class_context < 0)
            report_syntax_error(199, description);
        else report_syntax_error(144, token_spellings[expected], description);
        if (temporary_text) end_type_diagnostic();
        discard_trailing_expression_records();
        return;
    case 170:
        if (expected == 138) {
            qualified = (QualifiedDeclarator *)current_token_value.symbol;
            description = qualified->resolution->binding->target->name;
            if (description) {
                report_syntax_error(657, description);
                return;
            }
        }
        break;
    }
    report_syntax_error(143, token_spellings[expected], token_spellings[actual]);
}

/* Original0x26370..0x263ba, stdcall4 with EDX:EAX return. The original
   leaves unused semantic-value bytes unwritten. Zero those output bytes;
   only the token-specific payload belongs to the repaired grammar symbol. */
RECON_RETURN8_TYPE(ParserValue) __stdcall recovery_token_value(int token)
{
    ParserValue value;
    value.words.low = value.words.high = 0;
    switch (token) {
    case 35:
        value.type_name.type = type_0000;
        break;
    case 111:
        value.words.low = 0;
        break;
    default:
        value.bytes[0] = (unsigned char)token;
        break;
    }
    RECON_RETURN8(value);
}

/* Original0x263c0..0x26508, stdcall4. Report raw-token syntax errors,
   preserving type/qualified-name diagnostics and class capture counters. */
void __stdcall report_parser_error(char *message)
{
    unsigned category;
    ++syntax_diagnostic_count;
    if (current_class_context >= 0) ++captured_class_error_count;
    if (*message == 'y') fatal_error(26);
    category = token_categories[current_token_code];
    switch (category) {
    case 0:
        report_syntax_error(60);
        return;
    case 2:
        if (current_token_value.name) report_syntax_error(61, current_token_value.name);
        return;
    case 34:
    case 35:
    case 40:
        if (type_specifier_symbol) report_syntax_error(226, type_specifier_symbol->name);
        else if (!(current_token_value.declaration.storage[0] & 0x10))
            report_type_error(62, current_token_value.type_name.type);
        else report_missing_declarator(59, &current_token_value.declaration);
        return;
    case 161:
        unwind_qualified_scopes(1);
        break;
    case 163:
        if (current_token_value.expression && current_token_value.expression->type) {
            report_type_error(629, current_token_value.expression->type);
            return;
        }
        break;
    }
    report_syntax_error(59, token_spellings[current_token_code]);
}

/* Original0x26580..0x26650, cdecl varargs. Stop tokens apply only
   outside nested parentheses/braces; the final stop-list entry is token1. */
unsigned char synchronize_tokens(unsigned first, ...)
{
    int braces = 0, parentheses = 0;
    unsigned stop;
    va_list stops;
    parser_lookahead = first;
    for (;;) {
        if (!braces && !parentheses) {
            va_start(stops, first);
            do {
                stop = va_arg(stops, unsigned);
                if ((unsigned char)parser_lookahead == (unsigned char)stop) {
                    va_end(stops);
                    return (unsigned char)parser_lookahead;
                }
            } while ((unsigned char)stop != 1);
            va_end(stops);
        }
        switch (parser_lookahead) {
        case 0: return 1;
        case 3:
            text_work_cursor = expression_storage;
            break;
        case 119:
            fatal_error(4);
            return 1;
        case 127: ++braces; break;
        case 128: ++parentheses; break;
        case 133: --braces; break;
        case 136: --parentheses; break;
        case 138:
            release_arena(2);
            text_work_cursor = expression_storage;
            break;
        case 161:
            unwind_qualified_scopes(1);
            break;
        }
        parser_lookahead = scan_token();
    }
}

/* Original0x26510..0x2657d, cdecl0. Skip the rejected definition,
   preserving nested braces and putting the unmatched closing brace back. */
void skip_balanced_brace_body(void)
{
    int braces = 0;
    for (;;) {
        scan_token();
        switch (current_token_code) {
        case 0: return;
        case 0x77:
            fatal_error(4);
            return;
        case 0x7f:
            ++braces;
            break;
        case 0x85:
            if (!braces) {
                --source_text_cursor;
                return;
            }
            --braces;
            break;
        case 0x8a:
            release_arena(2);
            text_work_cursor = expression_storage;
            break;
        }
        text_work_cursor = expression_storage;
    }
}
