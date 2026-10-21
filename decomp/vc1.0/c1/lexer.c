/* Lexical token pushback and source dispatch. */
#include "lexer.h"
#include "source-input.h"
#include "characters.h"

unsigned token_line; /* original 0x604b8 */
unsigned scanner_flags; /* original 0x6bb44 */
unsigned char current_token_code; /* original 0x6c008 */
TokenValue current_token_value; /* original 0x5b150..0x5b158 */
LexicalToken initial_token_stack[3]; /* original 0x64d48..0x64d78 */
LexicalToken *token_stack_top = initial_token_stack; /* original 0x6a7b8 */
LexicalToken *token_stack_read = initial_token_stack; /* original 0x6a7bc */
int token_stack_count; /* original 0x6a7c0 */

/* Original 0x199a0..0x19a3e. */
unsigned char scan_token(void)
{
    LexicalToken *token;
    if (token_stack_count <= 0 && !(scanner_flags & SCANNER_SUPPRESS_POP))
        return scan_source_token();
    if (!(scanner_flags & SCANNER_SUPPRESS_POP)) {
        token = --token_stack_top;
        if (!--token_stack_count) {
            token_stack_read = initial_token_stack;
            token_stack_top = initial_token_stack;
            token_stack_count = 0;
        }
    }
    /* The original suppress-pop branch reloads an uninitialized automatic
       pointer. Its callers must leave that mode clear on entry here. */
    current_token_code = token->code;
    current_token_value = token->value;
    return token->category;
}

unsigned parser_flags; /* original 0x6bb48 */
unsigned char pch_output_written; /* original 0x6be74 */
char token_text[128]; /* original 0x5b158..0x5b1d8 */

#define NEXT_CHARACTER_CLASS() \
    character_class[(unsigned char)*source_text_cursor++]
#define EMIT_LEXICAL_TOKEN(code_) do { \
    current_token_code = (unsigned char)(code_); \
    current_token_value.bytes[0] = current_token_code; \
    return token_categories[current_token_code]; \
} while (0)

/* Original 0x19a40..0x1a8b8; class-dispatch table 0x1a8b8..0x1a970. */
unsigned char scan_source_token(void)
{
    unsigned char class, lookahead, code;
    unsigned flags, bytes;
    int wide;
    MacroDefinition *macro;
    char *text;
    if (!source_text_cursor)
        fatal_error(4);
next_character:
    class = NEXT_CHARACTER_CLASS();
dispatch:
    switch (class) {
    case CC_CARRIAGE_RETURN:
        goto next_character;
    case CC_SPACE:
        if (options.option_E) {
            do {
                fputc((unsigned char)source_text_cursor[-1], preprocessor_stream);
            } while (character_flags[(unsigned char)*source_text_cursor++] & CF_SPACE);
            --source_text_cursor;
        } else {
            while (character_flags[(unsigned char)*source_text_cursor++] & CF_SPACE)
                ;
            --source_text_cursor;
        }
        goto next_character;
    case CC_SLASH:
        if (skip_source_comment()) goto next_character;
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_DIVIDE_ASSIGN);
        code = TOK_DIVIDE;
        goto finish_operator;
    case CC_STAR:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_MULTIPLY_ASSIGN);
        if (lookahead == CC_SLASH && !options.option_E)
            compiler_warning(1, 138);
        code = TOK_MULTIPLY;
        goto finish_operator;
    case CC_BANG:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_NOT_EQUAL);
        code = TOK_LOGICAL_NOT;
        goto finish_operator;
    case CC_PERCENT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_REMAINDER_ASSIGN);
        code = TOK_REMAINDER;
        goto finish_operator;
    case CC_EQUAL:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_EQUAL);
        code = TOK_ASSIGN;
        goto finish_operator;
    case CC_CARET:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_XOR_ASSIGN);
        code = TOK_BIT_XOR;
        goto finish_operator;
    case CC_BAR:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_OR_ASSIGN);
        if (lookahead == CC_BAR) EMIT_LEXICAL_TOKEN(TOK_LOGICAL_OR);
        code = TOK_BIT_OR;
        goto finish_operator;
    case CC_AMPERSAND:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_AND_ASSIGN);
        if (lookahead == CC_AMPERSAND) EMIT_LEXICAL_TOKEN(TOK_LOGICAL_AND);
        code = TOK_BIT_AND;
        goto finish_operator;
    case CC_PLUS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_PLUS_ASSIGN);
        if (lookahead == CC_PLUS) EMIT_LEXICAL_TOKEN(TOK_PREINCREMENT);
        code = TOK_PLUS;
        goto finish_operator;
    case CC_MINUS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_MINUS_ASSIGN);
        if (lookahead == CC_MINUS) EMIT_LEXICAL_TOKEN(TOK_PREDECREMENT);
        if (lookahead == CC_GREATER) EMIT_LEXICAL_TOKEN(TOK_ARROW);
        code = TOK_MINUS;
        goto finish_operator;
    case CC_LESS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_LESS_EQUAL);
        if (lookahead == CC_LESS) {
            lookahead = NEXT_CHARACTER_CLASS();
            if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_LEFT_SHIFT_ASSIGN);
            class = CC_LEFT_SHIFT;
            code = TOK_LEFT_SHIFT;
        } else code = TOK_LESS;
        goto finish_operator;
    case CC_GREATER:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_GREATER_EQUAL);
        if (lookahead == CC_GREATER) {
            lookahead = NEXT_CHARACTER_CLASS();
            if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_RIGHT_SHIFT_ASSIGN);
            class = CC_RIGHT_SHIFT;
            code = TOK_RIGHT_SHIFT;
        } else code = TOK_GREATER;
        goto finish_operator;
    case CC_LEFT_SHIFT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_LEFT_SHIFT_ASSIGN);
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(TOK_LEFT_SHIFT);
    case CC_RIGHT_SHIFT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_RIGHT_SHIFT_ASSIGN);
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(TOK_RIGHT_SHIFT);
    case CC_COLON:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_GREATER) EMIT_LEXICAL_TOKEN(TOK_SEGMENT_OFFSET);
        code = TOK_COLON;
        goto finish_operator;
    case CC_RIGHT_BRACE: EMIT_LEXICAL_TOKEN(TOK_RIGHT_BRACE);
    case CC_RIGHT_BRACKET: EMIT_LEXICAL_TOKEN(TOK_RIGHT_BRACKET);
    case CC_COMMA: EMIT_LEXICAL_TOKEN(TOK_COMMA);
    case CC_RIGHT_PAREN: EMIT_LEXICAL_TOKEN(TOK_RIGHT_PAREN);
    case CC_LEFT_BRACE: EMIT_LEXICAL_TOKEN(TOK_LEFT_BRACE);
    case CC_LEFT_BRACKET: EMIT_LEXICAL_TOKEN(TOK_LEFT_BRACKET);
    case CC_LEFT_PAREN: EMIT_LEXICAL_TOKEN(TOK_LEFT_PAREN);
    case CC_SEMICOLON: EMIT_LEXICAL_TOKEN(TOK_SEMICOLON);
    case CC_TILDE: EMIT_LEXICAL_TOKEN(TOK_BIT_NOT);
    case CC_ESCAPE_OR_END:
        switch ((unsigned char)source_text_cursor[-1]) {
        case '\\':
            if (!options.option_E) {
                if (!consume_line_escape()) report_syntax_error(17);
            } else {
                fputc('\\', preprocessor_stream);
                fputc(next_source_character(), preprocessor_stream);
            }
            goto next_character;
        case '?':
            if (!(recover_trigraph() & 0x40)) goto next_character;
            EMIT_LEXICAL_TOKEN(TOK_QUESTION);
        }
        if (macro_expansion_active) {
            recover_special_character();
            goto next_character;
        }
        if (!fill_source_buffer()) goto next_character;
        if (close_source_file()) goto next_character;
        if (options.pch_create && !pch_output_written) {
            prepare_pch_output(options.source_file);
            finish_pch_output();
        }
        if (options.pch_create_file && !pch_output_written)
            report_syntax_error(857, options.pch_create_file);
        parser_flags |= 0x200;
        EMIT_LEXICAL_TOKEN(TOK_END_OF_FILE);
    case CC_NEWLINE:
        if (scanner_flags & 0x10000) {
            --source_text_cursor;
            current_token_code = TOK_NEWLINE;
            return TOK_NEWLINE;
        }
        if (scanner_flags & 0x400000) {
            current_token_code = TOK_NEWLINE;
            return TOK_NEWLINE;
        }
        if (options.option_E) {
            fputc('\n', preprocessor_stream);
            if (scanner_flags & 0x800000) {
                scanner_flags &= ~0x800000;
                if (!options.option_EP) {
                    emit_line_directive();
                    fputc('\n', preprocessor_stream);
                }
            }
        }
        begin_source_line();
        goto next_character;
    case CC_HASH:
        if (!options.option_E) {
            report_syntax_error(14);
            skip_source_line();
        } else
            fwrite("#", 1, 1, preprocessor_stream);
        goto next_character;
    case CC_MACRO_END:
        ++source_text_cursor;
        goto next_character;
    case CC_APOSTROPHE:
        wide = 0;
        goto character_literal;
    case CC_QUOTE:
        wide = 0;
        goto string_literal;
    case CC_WIDE_PREFIX:
    wide_prefix:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_ESCAPE_OR_END) {
            flags = recover_special_character();
            if (flags & ~0x70U) goto wide_prefix;
            if (flags & 0x20) ++source_text_cursor;
        } else if (lookahead == CC_APOSTROPHE) {
            wide = 1;
            goto character_literal;
        } else if (lookahead == CC_QUOTE) {
            wide = 1;
            goto string_literal;
        }
        --source_text_cursor;
        source_text_cursor[-1] = 'L';
        /* A non-literal L remains an identifier's first character. */
    case CC_IDENTIFIER:
        if (macro_expansion_active > 0) {
            if (!scan_macro_identifier()) goto identifier_ready;
        } else
            scan_source_identifier();
        macro = lookup_macro();
        if (macro && !macro->flags && expand_macro(macro))
            goto next_character;
    identifier_ready:
        if (options.option_E) {
            for (text = interned_name; *text; ++text)
                fputc((signed char)*text, preprocessor_stream);
            current_token_code = TOK_PP_OUTPUT;
            return TOK_PP_OUTPUT;
        }
        code = (SYMBOL_FROM_NAME(interned_name)->flags & 1) ?
            KEYWORD_FROM_NAME(interned_name)->token : TOK_IDENTIFIER;
        if (code == TOK_IDENTIFIER || (parser_flags & 0x10000000)) {
            current_token_value.identifier = interned_name;
            current_token_code = TOK_IDENTIFIER;
            return TOK_IDENTIFIER;
        }
        current_token_code = code;
        current_token_value.bytes[0] = code;
        code = token_categories[code];
        if (code == TOK_NEWLINE) {
            compiler_warning(1, 226, interned_name);
            goto next_character;
        }
        return code;
    case CC_DOT:
    dot_lookahead:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_ESCAPE_OR_END) {
            flags = recover_special_character();
            if (flags & ~0x70U) goto dot_lookahead;
            if (flags & 0x20) ++source_text_cursor;
        } else if (lookahead == CC_DOT) {
            if (match_source_character('.')) EMIT_LEXICAL_TOKEN(TOK_ELLIPSIS);
            source_text_cursor[-1] = '.';
        } else if (lookahead == CC_DIGIT) {
            token_text[0] = '.';
            token_text[1] = source_text_cursor[-1];
            if (!(scanner_flags & 0x8000) && (options.option_E || (scanner_flags & 0x1000000))) {
                bytes = read_number_text(token_text + 2);
                if (options.option_E) fwrite(token_text, bytes, 1, preprocessor_stream);
                current_token_code = TOK_PP_OUTPUT;
                return token_categories[TOK_PP_OUTPUT];
            }
            current_token_code = scan_float_tail(token_text + 2);
            return token_categories[current_token_code];
        }
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(TOK_MEMBER);
    case CC_DIGIT:
        if (!(scanner_flags & 0x8000) && (options.option_E || (scanner_flags & 0x1000000))) {
            token_text[0] = source_text_cursor[-1];
            bytes = read_number_text(token_text + 1);
            if (options.option_E) fwrite(token_text, bytes, 1, preprocessor_stream);
            current_token_code = TOK_PP_OUTPUT;
            return token_categories[TOK_PP_OUTPUT];
        }
        if ((scanner_flags & 0x400000) && !(scanner_flags & 0x10000))
            current_token_code = scan_assembler_number((unsigned char)source_text_cursor[-1]);
        else
            current_token_code = scan_number((unsigned char)source_text_cursor[-1]);
        return token_categories[current_token_code];
    default:
        if (!options.option_E)
            report_syntax_error(18, (unsigned char)source_text_cursor[-1]);
        else
            fputc((unsigned char)source_text_cursor[-1], preprocessor_stream);
        goto next_character;
    }
finish_operator:
    if (lookahead == CC_ESCAPE_OR_END) {
        flags = recover_special_character();
        if (flags & ~0x70U) goto dispatch;
        if (flags & 0x20) ++source_text_cursor;
    }
    --source_text_cursor;
    EMIT_LEXICAL_TOKEN(code);
character_literal:
    if (options.option_E) {
        if (wide) fputc('L', preprocessor_stream);
        copy_quoted_literal('\'');
        goto next_character;
    }
    current_token_code = scan_character_literal(wide);
    return token_categories[current_token_code];
string_literal:
    if (options.option_E) {
        if (wide) fputc('L', preprocessor_stream);
        copy_quoted_literal('"');
        goto next_character;
    }
    scan_string_literal(wide, '"');
    current_token_code = TOK_STRING;
    return TOK_STRING;
}

/* Original 0x21bb0..0x21d18; class-dispatch table 0x21d18..0x21d48. */
void begin_source_line(void)
{
    unsigned char class, character;
    ++source_line_number;
    for (;;) {
        class = NEXT_CHARACTER_CLASS();
        switch (class) {
        case CC_CARRIAGE_RETURN:
            continue;
        case CC_SLASH:
            if (skip_source_comment()) continue;
            goto line_ready;
        case CC_ESCAPE_OR_END:
            character = (unsigned char)source_text_cursor[-1];
            if (!character || character == 0x1a) {
                if (!fill_source_buffer()) continue;
                if (pch_mode && !include_depth) finish_pch_capture();
                return;
            }
            if (character == '?') {
                if (!(recover_trigraph() & ~0x40U)) goto line_ready;
                continue;
            }
            if (consume_line_escape()) continue;
            goto line_ready;
        case CC_NEWLINE:
            ++source_line_number;
            /* Blank lines are copied by /E along with other leading space. */
        case CC_SPACE:
            if (options.option_E)
                fputc((unsigned char)source_text_cursor[-1], preprocessor_stream);
            else {
                while (character_flags[(unsigned char)*source_text_cursor++] & CF_SPACE)
                    ;
                --source_text_cursor;
            }
            continue;
        case CC_HASH:
            browser_file_cookie = 1;
            preprocess_directive();
            continue;
        default:
            goto line_ready;
        }
    }
line_ready:
    if (options.option_ide) report_ide_line();
    browser_file_cookie = 1;
    --source_text_cursor;
    if ((scanner_flags & 0x80000) && !(scanner_flags & 0x40))
        record_line_information(1);
    if (pch_mode && !include_depth) finish_pch_capture();
}

/* Original 0x18bd0..0x18c36. The caller must provide stack capacity. */
void __stdcall unget_token(unsigned char category)
{
    ++token_stack_count;
    token_stack_top->category = category;
    token_stack_top->code = current_token_code;
    token_stack_top->value = current_token_value;
    token_stack_top->line = token_line;
    ++token_stack_top;
    if (token_stack_top >= initial_token_stack + 3)
        scanner_flags |= SCANNER_LOOKAHEAD_FULL;
}
