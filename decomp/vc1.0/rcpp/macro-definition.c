/* Definition parsing, original [0100a200,0100a340). The same work buffer
 * first holds packed formal names and then the encoded replacement text. */
#include "directives.h"
int __stdcall rc_find_macro_parameter(const char *);
void __stdcall rc_check_macro_storage(char *);
char *__stdcall rc_copy_macro_quote(char *, unsigned char);

void rc_parse_define(void)
{
    unsigned char ch;
    unsigned length;
    int arguments;
    ch = rc_read_nonspace();
    if (rc_character_class[ch] != LC_IDENTIFIER) {
        rc_message_format = rc_message(2007);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_error(2007);
        rc_skip_line();
        return;
    }
    rc_read_identifier(ch);
    rc_define_argument_count = 0;
    rc_define_storage = rc_text_buffer;
    for (;;) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_CR:
            continue;
        case LC_SPACE:
            break;
        case LC_END:
            if (rc_resume_input() != 16) continue;
            goto bad_character;
        case LC_SLASH:
            if (!rc_skip_comment()) {
                rc_message_format = rc_message(2008);
                rc_format(rc_diagnostic_buffer, rc_message_format, '/');
                rc_error(2008);
            }
            break;
        case LC_NEWLINE:
            --rc_input_cursor;
            rc_define_macro(NULL, 0, 0);
            return;
        case LC_LPAREN:
            rc_read_macro_parameters();
            if (!rc_define_argument_count) rc_define_argument_count = -1;
            break;
        default:
        bad_character:
            rc_message_format = rc_message(2008);
            rc_format(rc_diagnostic_buffer, rc_message_format, (int)ch);
            rc_error(2008);
            break;
        }
        arguments = rc_define_argument_count;
        length = rc_read_macro_body();
        rc_define_macro(rc_define_storage, length, arguments);
        return;
    }
}

/* Original [0100a7c0,0100aade), switch data [0100aae0,0100ab84).
 * Parameter references use compact two-byte markers: 1 is ordinary
 * substitution, 2 stringizing, and 3 the Microsoft characterizing extension. */
unsigned rc_read_macro_body(void)
{
    char *out = rc_define_storage, *identifier;
    unsigned char ch, look;
    int stringify = 0, characterize = 0, parameter;
    ch = rc_read_nonspace();
    for (;;) {
        rc_check_macro_storage(out);
        switch (rc_character_class[ch]) {
        case LC_SLASH:
            if (!rc_skip_comment()) goto copy_character;
            /* Comments, CR and whitespace collapse to a single space. */
        case LC_SPACE: case LC_CR:
            ch = rc_read_nonspace();
            if (rc_character_class[ch] != LC_NEWLINE) *out++ = ' ';
            continue;
        case LC_END:
            if (rc_resume_input() != 16) break;
            *out++ = ch;
            ch = rc_get_logical_character();
            goto copy_character;
        case LC_CHARACTER: case LC_STRING:
            out = rc_copy_macro_quote(out, ch);
            break;
        case LC_BAD:
            rc_message_format = rc_message(2018);
            rc_format(rc_diagnostic_buffer, rc_message_format, (int)ch);
            rc_error(2018);
            break;
        case LC_HASH:
            for (;;) {
                look = *rc_input_cursor++;
                if (rc_character_class[look] != LC_END) break;
                if (rc_resume_input() == 16) break;
            }
            if (rc_character_class[look] == LC_HASH) {
                if (out == rc_define_storage) {
                    rc_message_format = rc_message(2160);
                    rc_format(rc_diagnostic_buffer, rc_message_format);
                    rc_error(2160);
                    continue;
                }
                if (out[-1] == ' ') --out;
                ch = rc_read_nonspace();
                if (rc_character_class[ch] == LC_NEWLINE) {
                    --rc_input_cursor;
                    rc_message_format = rc_message(2161);
                    rc_format(rc_diagnostic_buffer, rc_message_format);
                    rc_error(2161);
                }
                continue;
            }
            if (rc_character_class[look] == LC_AT) characterize = 1;
            else { --rc_input_cursor; stringify = 1; }
            ch = rc_read_nonspace();
            if (rc_character_class[ch] == LC_IDENTIFIER) goto identifier;
            rc_message_format = rc_message(2162);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2162);
            continue;
        case LC_IDENTIFIER:
        identifier:
            identifier = out;
            for (;;) {
                rc_check_macro_storage(out);
                *out++ = ch;
            read_identifier_character:
                ch = *rc_input_cursor++;
                if (rc_character_flags[ch] & 16) continue;
                if (rc_character_class[ch] == LC_END && rc_resume_input() != 16)
                    goto read_identifier_character;
                break;
            }
            *out = 0;
            parameter = rc_find_macro_parameter(identifier);
            if (parameter >= 1) {
                identifier[0] = stringify ? 2 : characterize ? 3 : 1;
                identifier[1] = (unsigned char)parameter;
                out = identifier + 2;
            } else if (stringify || characterize) {
                rc_message_format = rc_message(2162);
                rc_format(rc_diagnostic_buffer, rc_message_format);
                rc_error(2162);
            }
            stringify = characterize = 0;
            continue;
        case LC_NEWLINE:
            --rc_input_cursor;
            if (out == rc_define_storage) return 0;
            rc_check_macro_storage(out);
            *out++ = 0;
            *out = 'D';
            return out + 1 - rc_define_storage;
        default:
        copy_character:
            *out++ = ch;
            break;
        }
        ch = *rc_input_cursor++;
    }
}

/* Original [0100ac00,0100ac39). Keep five bytes available for terminators
 * and the largest encoded parameter reference. */
void __stdcall rc_check_macro_storage(char *out)
{
    if (out >= rc_text_buffer + 2043) {
        rc_message_format = rc_message(1065);
        rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name);
        rc_fatal(1065);
    }
}

/* Original [0100ab90,0100abf3). Formal names have a one-byte record size
 * followed by a NUL-terminated name; indices in replacement text start at 1. */
int __stdcall rc_find_macro_parameter(const char *name)
{
    unsigned char *record = (unsigned char *)rc_text_buffer;
    const char *a, *b;
    int i;
    for (i = 1; i <= rc_define_argument_count; ++i) {
        a = name;
        b = (const char *)record + 1;
        while (*a == *b && *a) { ++a; ++b; }
        if (*a == *b) return i;
        record += *record;
    }
    return -1;
}

/* Original [0100a340,0100a548). Formal names are collected directly into
 * the packed definition workspace; duplicate names are diagnosed and omitted. */
void rc_read_macro_parameters(void)
{
    char *name, *end;
    unsigned char ch, category;
    for (;;) {
        name = rc_define_storage + 1;
        end = name;
        for (;;) {
            ch = rc_read_nonspace();
            category = rc_character_class[ch];
            if (category == LC_COMMA || category == LC_RPAREN) {
                if (name < end) {
                    if (end - name >= 32) {
                        name[31] = 0;
                        rc_message_format = rc_message(4111);
                        rc_format(rc_diagnostic_buffer, rc_message_format, name);
                        rc_warning(4011);
                        end = name + 32;
                    }
                    if (rc_find_macro_parameter(name) >= 1) {
                        rc_message_format = rc_message(2009);
                        rc_format(rc_diagnostic_buffer, rc_message_format, name);
                        rc_error(2009);
                    } else {
                        *rc_define_storage = (unsigned char)(end - rc_define_storage);
                        rc_define_storage = end;
                        ++rc_define_argument_count;
                    }
                } else if (category == LC_COMMA || rc_define_argument_count > 0) {
                    rc_message_format = rc_message(2010);
                    rc_format(rc_diagnostic_buffer, rc_message_format, (int)ch);
                    rc_error(2010);
                }
                if (category == LC_RPAREN) return;
                break;
            }
            if (category != LC_IDENTIFIER) {
                rc_message_format = rc_message(2010);
                rc_format(rc_diagnostic_buffer, rc_message_format, (int)ch);
                rc_error(2010);
                continue;
            }
            if (name != end) {
                rc_message_format = rc_message(2010);
                rc_format(rc_diagnostic_buffer, rc_message_format, (int)ch);
                rc_error(2010);
            }
            *end++ = ch;
            for (;;) {
                ch = *rc_input_cursor++;
                while (rc_character_flags[ch] & 16) {
                    *end++ = ch;
                    ch = *rc_input_cursor++;
                }
                if (!ch) {
                    if (rc_refill_input()) {
                        rc_message_format = rc_message(1004);
                        rc_format(rc_diagnostic_buffer, rc_message_format);
                        rc_fatal(1004);
                    }
                    continue;
                }
                if (ch == '\\' && rc_splice_line()) continue;
                --rc_input_cursor;
                *end++ = 0;
                break;
            }
        }
    }
}

/* Original [0100ac40,0100ad25). Preserve a quote and its escapes as source
 * text while removing spliced newlines. Limit overflow returns the sentinel
 * position; the enclosing replacement scanner then diagnoses exhaustion. */
char *__stdcall rc_copy_macro_quote(char *out, unsigned char quote)
{
    unsigned char ch;
    *out++ = quote;
    while (out <= rc_text_buffer + 2043) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_END:
            if (rc_resume_input() != 16) continue;
            *out++ = '\\';
            ch = rc_get_logical_character();
            if (ch == '\\' && rc_splice_line()) continue;
            break;
        case LC_NEWLINE:
            rc_message_format = rc_message(2001);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2001);
            --rc_input_cursor;
            ch = quote;
            /* Fall through to the closing-quote check. */
        case LC_CHARACTER: case LC_STRING:
            if (ch == quote) { *out++ = ch; return out; }
            break;
        case LC_DBCS_LEAD:
            *out++ = ch;
            ch = rc_get_logical_character();
            break;
        }
        *out++ = ch;
    }
    return rc_text_buffer + 2043;
}
