/* Main compiler-derived token scanner. Original [0100f970,010106dc),
 * category dispatch table [010106dc,0101078c). */
#include "directives.h"

unsigned char rc_next_token(void)
{
    unsigned char ch, primary, lookahead, token;
    unsigned status;
    RcMacro *macro;
restart:
    ch = *rc_input_cursor++;
    primary = rc_character_class[ch];
dispatch:
    switch (primary) {
    case LC_CR:
        goto restart;
    case LC_SPACE:
        if (rc_preprocessing_started) {
            ch = rc_input_cursor[-1];
            do {
                fputc(ch, rc_output);
                ch = *rc_input_cursor++;
            } while (rc_character_flags[ch] & 64);
        } else {
            do ch = *rc_input_cursor++;
            while (rc_character_flags[ch] & 64);
        }
        --rc_input_cursor;
        goto restart;
    case LC_SLASH:
    case LC_STAR:
    case LC_BANG:
    case LC_PERCENT:
    case LC_EQUAL:
    case LC_XOR:
    case LC_OR:
    case LC_AND:
    case LC_PLUS:
    case LC_MINUS:
    case LC_LESS:
    case LC_GREATER:
        lookahead = rc_character_class[*rc_input_cursor++];
        switch (primary) {
        case LC_SLASH:
            if (lookahead == LC_SLASH) { rc_skip_line_comment(); goto restart; }
            if (lookahead == LC_STAR) { rc_skip_block_comment(); goto restart; }
            if (lookahead == LC_EQUAL) { token = TK_DIVIDE_ASSIGN; goto emit_token; }
            token = TK_DIVIDE;
            break;
        case LC_STAR:
            if (lookahead == LC_SLASH) {
                if (rc_preprocessing_started) fwrite("*/", 2, 1, rc_output);
                else {
                    rc_message_format = rc_message(2138);
                    rc_format(rc_diagnostic_buffer, rc_message_format);
                    rc_error(2138);
                }
                goto restart;
            }
            if (lookahead == LC_EQUAL) { token = TK_MULTIPLY_ASSIGN; goto emit_token; }
            token = TK_MULTIPLY;
            break;
        case LC_BANG:
            if (lookahead == LC_EQUAL) { token = TK_NOT_EQUAL; goto emit_token; }
            token = TK_BANG;
            break;
        case LC_PERCENT:
            if (lookahead == LC_EQUAL) { token = TK_MODULUS_ASSIGN; goto emit_token; }
            token = TK_MODULUS;
            break;
        case LC_EQUAL:
            if (lookahead == LC_EQUAL) { token = TK_EQUAL; goto emit_token; }
            token = TK_ASSIGN;
            break;
        case LC_XOR:
            if (lookahead == LC_EQUAL) { token = TK_XOR_ASSIGN; goto emit_token; }
            token = TK_XOR;
            break;
        case LC_OR:
            if (lookahead == LC_EQUAL) { token = TK_OR_ASSIGN; goto emit_token; }
            if (lookahead == LC_OR) { token = TK_LOGICAL_OR; goto emit_token; }
            token = TK_OR;
            break;
        case LC_AND:
            if (lookahead == LC_EQUAL) { token = TK_AND_ASSIGN; goto emit_token; }
            if (lookahead == LC_AND) { token = TK_LOGICAL_AND; goto emit_token; }
            token = TK_AND;
            break;
        case LC_PLUS:
            if (lookahead == LC_EQUAL) { token = TK_PLUS_ASSIGN; goto emit_token; }
            if (lookahead == LC_PLUS) { token = TK_INCREMENT; goto emit_token; }
            token = TK_PLUS;
            break;
        case LC_MINUS:
            if (lookahead == LC_EQUAL) { token = TK_MINUS_ASSIGN; goto emit_token; }
            if (lookahead == LC_MINUS) { token = TK_DECREMENT; goto emit_token; }
            if (lookahead == LC_GREATER) { token = TK_ARROW; goto emit_token; }
            token = TK_MINUS;
            break;
        case LC_LESS:
            if (lookahead == LC_EQUAL) { token = TK_LESS_EQUAL; goto emit_token; }
            if (lookahead == LC_LESS) {
                lookahead = rc_character_class[*rc_input_cursor++];
                if (lookahead == LC_EQUAL) { token = TK_LSHIFT_ASSIGN; goto emit_token; }
                primary = 21;
                token = TK_LSHIFT;
            } else token = TK_LESS;
            break;
        case LC_GREATER:
            if (lookahead == LC_EQUAL) { token = TK_GREATER_EQUAL; goto emit_token; }
            if (lookahead == LC_GREATER) {
                lookahead = rc_character_class[*rc_input_cursor++];
                if (lookahead == LC_EQUAL) { token = TK_RSHIFT_ASSIGN; goto emit_token; }
                primary = 22;
                token = TK_RSHIFT;
            } else token = TK_GREATER;
            break;
        }
        goto operator_fallback;
    case 21: /* Continuation after two '<' bytes across an input boundary. */
        lookahead = rc_character_class[*rc_input_cursor++];
        if (lookahead == LC_EQUAL) token = TK_LSHIFT_ASSIGN;
        else { --rc_input_cursor; token = TK_LSHIFT; }
        goto emit_token;
    case 22: /* Continuation after two '>' bytes. */
        lookahead = rc_character_class[*rc_input_cursor++];
        if (lookahead == LC_EQUAL) token = TK_RSHIFT_ASSIGN;
        else { --rc_input_cursor; token = TK_RSHIFT; }
        goto emit_token;
    case LC_END:
        if (rc_input_cursor[-1] == '\\') goto escaped_character;
        if (rc_expanding_macro) { rc_resume_input(); goto restart; }
        if (!rc_refill_input()) goto restart;
        if (rc_close_include()) goto restart;
        token = TK_EOF;
        goto emit_token;
    case LC_ESCAPE:
escaped_character:
        if (rc_preprocessing_started) {
            fputc('\\', rc_output);
            fputc(rc_get_logical_character(), rc_output);
        } else if (!rc_splice_line()) {
            rc_message_format = rc_message(2017);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2017);
        }
        goto restart;
    case LC_NEWLINE:
        if (rc_in_directive) {
            --rc_input_cursor;
            rc_token_kind = TK_NEWLINE;
            return TK_NEWLINE;
        }
        if (rc_preprocessing_started) fputc('\n', rc_output);
        rc_begin_line();
        goto restart;
    case LC_CHARACTER:
        if (rc_preprocessing_started) { rc_copy_quoted_text('\''); goto restart; }
        rc_token_kind = rc_parse_character();
        return (unsigned char)rc_token_info[rc_token_kind].category;
    case LC_STRING:
        if (rc_preprocessing_started) { rc_copy_quoted_text('"'); goto restart; }
        rc_parse_string();
        rc_token_kind = TK_STRING;
        return TK_STRING;
    case LC_HASH:
        if (rc_preprocessing_started) fwrite("#", 1, 1, rc_output);
        else {
            rc_message_format = rc_message(2014);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2014);
            --rc_input_cursor;
            --rc_line_number;
            rc_begin_line();
        }
        goto restart;
    case LC_BAD:
        if (rc_preprocessing_started) fputc(rc_input_cursor[-1], rc_output);
        else {
            rc_message_format = rc_message(2018);
            rc_format(rc_diagnostic_buffer, rc_message_format, (int)rc_input_cursor[-1]);
            rc_error(2018);
        }
        goto restart;
    case LC_INTERNAL_4:
        ++rc_input_cursor;
        goto restart;
    case LC_IDENTIFIER:
        if (rc_expanding_macro > 0) {
            if (!rc_read_expanded_identifier(rc_input_cursor[-1])) goto identifier;
        } else rc_read_identifier(rc_input_cursor[-1]);
        macro = rc_find_macro();
        if (macro && !macro->flags && rc_expand_macro(macro)) goto restart;
identifier:
        if (rc_preprocessing_started) {
            fwrite(rc_macro_name, rc_macro_name_length - 1, 1, rc_output);
            rc_token_kind = TK_NEWLINE;
            return TK_NEWLINE;
        }
        rc_token_value = rc_macro_name;
        rc_token_hash = rc_macro_hash;
        rc_token_length = (unsigned char)rc_macro_name_length;
        /* The original returns identifier category without replacing kind. */
        return TK_IDENTIFIER;
    case LC_AT:
    case LC_BAD_PUNCTUATION:
        rc_message_format = rc_message(2018);
        rc_format(rc_diagnostic_buffer, rc_message_format, (int)rc_input_cursor[-1]);
        rc_error(2018);
        goto restart;
    case LC_DBCS_LEAD:
        if (rc_preprocessing_started) {
            fputc(rc_input_cursor[-1], rc_output);
            fputc(*rc_input_cursor++, rc_output);
        } else {
            rc_message_format = rc_message(2018);
            rc_format(rc_diagnostic_buffer, rc_message_format, (int)rc_input_cursor[-1]);
            rc_error(2018);
            rc_message_format = rc_message(2018);
            rc_format(rc_diagnostic_buffer, rc_message_format, (int)*rc_input_cursor++);
            rc_error(2018);
        }
        goto restart;
    case LC_RBRACE: token = TK_RBRACE; goto emit_token;
    case LC_RBRACKET: token = TK_RBRACKET; goto emit_token;
    case LC_COLON: token = TK_COLON; goto emit_token;
    case LC_COMMA: token = TK_COMMA; goto emit_token;
    case LC_RPAREN: token = TK_RPAREN; goto emit_token;
    case LC_DIGIT:
        rc_token_kind = rc_parse_number(rc_input_cursor[-1]);
        return (unsigned char)rc_token_info[rc_token_kind].category;
    case LC_LBRACE: token = TK_LBRACE; goto emit_token;
    case LC_LBRACKET: token = TK_LBRACKET; goto emit_token;
    case LC_LPAREN: token = TK_LPAREN; goto emit_token;
    case LC_QUESTION: token = TK_QUESTION; goto emit_token;
    case LC_SEMICOLON: token = TK_SEMICOLON; goto emit_token;
    case LC_TILDE: token = TK_TILDE; goto emit_token;
    case LC_DOT:
dot_lookahead:
        lookahead = rc_character_class[*rc_input_cursor++];
        if (lookahead == LC_END) {
            status = rc_resume_input();
            if (status != 16) goto dot_lookahead;
        } else if (lookahead == LC_ESCAPE) {
            if (rc_splice_line()) goto dot_lookahead;
            --rc_input_cursor;
        } else if (lookahead == LC_DOT) {
            if (!rc_match_character('.')) {
                rc_message_format = rc_message(2142);
                rc_format(rc_diagnostic_buffer, rc_message_format);
                rc_error(2142);
            }
            token = TK_ELLIPSIS;
            goto emit_token;
        } else if (lookahead == LC_DIGIT) {
            rc_macro_name[0] = '.';
            rc_macro_name[1] = rc_input_cursor[-1];
            rc_token_kind = rc_read_floating(rc_macro_name + 2);
            return (unsigned char)rc_token_info[rc_token_kind].category;
        }
        --rc_input_cursor;
        token = TK_DOT;
        goto emit_token;
    default:
        /* Internal categories 36..38 are not externally valid token starts.
         * The original fallback reads an uninitialized pending-token byte. */
        lookahead = primary;
        goto operator_fallback;
    }
operator_fallback:
    if (lookahead == LC_END) {
        status = rc_resume_input();
        if (status != 16) goto dispatch;
    }
    --rc_input_cursor;
emit_token:
    rc_token_kind = token;
    /* Punctuation replaces only the low byte of the prior token value. */
    *(unsigned char *)&rc_token_value = token;
    return (unsigned char)rc_token_info[token].category;
}
