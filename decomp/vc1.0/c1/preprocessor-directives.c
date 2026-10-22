/* Preprocessor directive control flow. */
#include <string.h>
#include <ctype.h>
#include "preprocessor.h"
#include "characters.h"
#include "parser.h"
#include "control-ir.h"

/* Conditional-compilation state, shared with source/include restoration. */
unsigned conditional_branch_state[32]; /* original 0x638a0 */
short pch_conditional_depth; /* original 0x6de1c */

/* Original 0x205b0..0x20d78; directive dispatch table 0x20d78..0x20db0. */
void preprocess_directive(void)
{
    char *name, *left, *right, *message;
    unsigned char character, class, directive;
    int condition, saved_output, result, index;
    unsigned number;
    char *filename;

    if (macro_expansion_active) return;
    if (!include_depth && pch_mode == 1 && !pch_conditional_depth)
        finish_pch_capture();
    name = interned_name;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        class = character_class[character];
        if (class == CC_SPACE && (character == ' ' || character == '\t')) continue;
        switch (class) {
        case CC_CARRIAGE_RETURN: continue;
        case CC_SLASH:
            if (skip_source_comment()) continue;
            break;
        case CC_ESCAPE_OR_END:
            if (recover_special_character() & ~0x50U) continue;
            break;
        case CC_NEWLINE:
            --source_text_cursor;
            return;
        case CC_IDENTIFIER: case CC_WIDE_PREFIX:
            scan_source_identifier();
            name = interned_name;
            break;
        default:
            report_syntax_error(19, character);
            skip_source_line();
            return;
        }
        break;
    }
    scanner_flags |= 0x10000;
classify_directive:
    directive = classify_directive(name);
    switch (directive) {
    case 0: /* if, including the expression following a skipped elif */
        if (!include_depth && pch_mode) {
            capture_directive_text();
            result = pch_find_conditional();
            if (result) {
                if (pch_mode == 1) break;
            } else {
                saved_output = options.option_E;
                options.option_E = 0;
                scanner_flags |= 0x8000;
                condition = evaluate_preprocessor_expression();
                scanner_flags &= ~0x8000UL;
                options.option_E = saved_output;
                finish_directive("#if/#elif");
            }
            /* PCH playback ignores the live value and supplies its recorded
               condition. The original local is unspecified on that path. */
            condition = pch_resolve_condition((short)condition);
        } else {
            saved_output = options.option_E;
            options.option_E = 0;
            scanner_flags |= 0x8000;
            condition = evaluate_preprocessor_expression();
            scanner_flags &= ~0x8000UL;
            options.option_E = saved_output;
            finish_directive("#if/#elif");
        }
        if (!condition) {
            result = skip_inactive_directives(5);
            if (result != 3) {
                if (!options.option_EP) emit_line_directive();
                if (result == 2) { name = "if"; goto classify_directive; }
                break;
            }
        }
        index = push_conditional_level();
        conditional_branch_state[index] = condition ? 1 : 3;
        if (!options.option_EP && !condition) emit_line_directive();
        break;
    case 4: case 5: /* ifdef / ifndef */
        if (!include_depth && pch_mode) {
            capture_directive_text();
            result = pch_find_conditional();
            if (result) {
                if (pch_mode == 1) break;
            } else {
                character = skip_directive_space();
                class = character_class[character];
                if (class != CC_IDENTIFIER && class != CC_WIDE_PREFIX) fatal_error(16);
                scan_source_identifier();
                condition = lookup_macro() != 0;
                finish_directive(directive == 4 ? "#ifdef" : "#ifndef");
                if (directive == 5) condition = !condition;
            }
            condition = pch_resolve_condition((short)condition);
        } else {
            character = skip_directive_space();
            class = character_class[character];
            if (class != CC_IDENTIFIER && class != CC_WIDE_PREFIX) fatal_error(16);
            scan_source_identifier();
            condition = lookup_macro() != 0;
            finish_directive(directive == 4 ? "#ifdef" : "#ifndef");
            if (directive == 5) condition = !condition;
        }
        if (!condition) {
            result = skip_inactive_directives(5);
            if (result != 3) {
                if (result == 2) { name = "if"; goto classify_directive; }
                break;
            }
        }
        index = push_conditional_level();
        conditional_branch_state[index] = condition ? 1 : 3;
        break;
    case 1: /* elif after a branch already selected */
        index = conditional_depth--;
        if (index < 0) fatal_error(18);
        do { result = skip_inactive_directives(5); } while (result != 4);
        if (!options.option_EP) emit_line_directive();
        break;
    case 2: /* else after a branch already selected */
        if (conditional_depth >= 0) {
            index = conditional_depth--;
            if (conditional_branch_state[index] != 1) fatal_error(19);
        } else fatal_error(19);
        finish_directive("#else");
        skip_inactive_directives(6);
        if (!options.option_EP) emit_line_directive();
        break;
    case 3: /* endif */
        index = conditional_depth--;
        if (index < 0) fatal_error(20);
        if (!options.option_EP) emit_line_directive();
        if (options.z_a) skip_source_line();
        else finish_directive("#endif");
        break;
    case 6: /* define */
        define_source_macro();
        break;
    case 7: /* include */
        filename = read_include_filename();
        if (filename)
            open_forced_include(filename, current_token_code != TOK_STRING);
        break;
    case 8: /* pragma */
        if (!include_depth && pch_mode) {
            capture_directive_text();
            if (pch_find_conditional() == 1) break;
        }
        process_pragma();
        break;
    case 9: /* undef */
        character = skip_directive_space();
        class = character_class[character];
        if (class != CC_IDENTIFIER && class != CC_WIDE_PREFIX)
            compiler_warning(1, 6);
        else { scan_source_identifier(); undefine_source_macro(); }
        finish_directive("#undef");
        break;
    case 10: /* line */
        if (!include_depth && pch_mode) finish_pch_capture();
        saved_output = options.option_E;
        options.option_E = 0;
        read_parser_token();
        if (current_token_code < 7 || current_token_code > 10) {
            report_syntax_error(5, token_names[current_token_code]);
            options.option_E = saved_output;
            skip_source_line();
            break;
        }
        number = current_token_value.literal->value.integer;
        if ((int)number <= 0 || (int)number > 0x7fff) {
            compiler_warning(options.z_a ? 4 : 1, 112);
            if ((int)current_token_value.literal->value.integer <= 0)
                current_token_value.literal->value.integer = 1;
        }
        source_line_number = current_token_value.literal->value.integer - 1;
        read_parser_token();
        options.option_E = saved_output;
        if (current_token_code != TOK_NEWLINE) {
            if (current_token_code != TOK_STRING) {
                report_syntax_error(130, token_names[current_token_code]);
                skip_source_line();
                break;
            }
            if (current_token_value.bytes[6] & 1) report_syntax_error(2);
            left = current_token_value.identifier;
            right = source_filename;
            while (*left && *left == *right) { ++left; ++right; }
            if (*left != *right) {
                source_buffer_cursor = source_filename;
                text_work_cursor = (unsigned char *)current_token_value.identifier;
                do { *source_buffer_cursor++ = *text_work_cursor++; } while (source_buffer_cursor[-1]);
                if (!options.option_E) {
                    source_file_index = allocate_permanent(4);
                    *source_file_index = 0xffff;
                    if (parameter_nesting) record_line_information(2);
                    if (browser_filename) { browser_close_file(); browser_open_file(); }
                }
            }
            text_work_cursor = (unsigned char *)current_token_value.identifier;
        }
        if (!options.option_E) record_line_information(1);
        else emit_line_directive();
        finish_directive("#line");
        break;
    case 11:
        fatal_error(21, name);
        break;
    case 12: /* error */
        message = token_text;
        for (;;) {
            character = (unsigned char)*source_text_cursor++;
            class = character_class[character];
            if (class == CC_ESCAPE_OR_END) { recover_special_character(); continue; }
            if (class == CC_NEWLINE) break;
            if (class == CC_LEAD_BYTE) {
                *message++ = character;
                character = (unsigned char)*source_text_cursor++;
            }
            *message++ = character;
        }
        --source_text_cursor;
        *message = '\0';
        report_syntax_error(189, token_text);
        finish_directive("#error");
        break;
    case 13: /* ident */
        saved_output = options.option_E;
        options.option_E = 0;
        read_parser_token();
        options.option_E = saved_output;
        if (current_token_code != TOK_STRING) warn_ignored_token();
        else {
            pending_pragma_kind = 3;
            pending_pragma_text = (unsigned char *)current_token_value.identifier;
            record_line_information(11);
        }
        /* The original uses the #error spelling for this trailing check. */
        finish_directive("#error");
        break;
    }
    scanner_flags &= ~0x10000UL;
}

/* Encountered directive vocabulary. The original partitions the suffixes
   by first letter; store complete names alongside their directive codes. */
static struct DirectiveName {
    char *name;
    unsigned char code;
} directive_names[] = {
    { "define", 6 }, { "error", 12 }, { "endif", 3 },
    { "else", 2 }, { "elif", 1 }, { "include", 7 },
    { "ifndef", 5 }, { "ifdef", 4 }, { "if", 0 },
    { "ident", 13 }, { "line", 10 }, { "pragma", 8 }, { "undef", 9 }
};

/* Original 0x1c420..0x1c4a8. No identifier interning or allocation occurs. */
unsigned char __stdcall classify_directive(char *name)
{
    unsigned i;
    char *left, *right;
    if ((signed char)*name < '_') return 11;
    for (i = 0; i < sizeof(directive_names) / sizeof(directive_names[0]); ++i) {
        left = name;
        right = directive_names[i].name;
        while (*left && *left == *right) { ++left; ++right; }
        if (*left == *right) return directive_names[i].code;
    }
    return 11;
}

/* Original 0x203d0..0x2048c. Protect the include name at the upper end
   of the scanner workspace before opening another source buffer. */
char *read_include_filename(void)
{
    int saved_output = options.option_E;
    char *name;
    options.option_E = 0;
    scanner_flags |= 0x4000;
    read_parser_token();
    scanner_flags &= ~0x4000UL;
    options.option_E = saved_output;
    if (current_token_code == TOK_STRING) {
        if (current_token_value.bytes[6] & 1) report_syntax_error(2);
        name = token_buffer_end;
        do { *--name = *--text_work_cursor; }
        while (text_work_cursor != (unsigned char *)current_token_value.identifier);
        return name;
    }
    if (current_token_code == TOK_LESS) {
        name = read_angle_include_filename();
        if (name) return name;
    } else report_syntax_error(6, token_names[current_token_code]);
    skip_source_line();
    scanner_flags &= ~0x10000UL;
    return 0;
}

/* Original 0x20210..0x202bb. Angle includes use raw characters and trim
   trailing space before preserving the filename at the scanner buffer end. */
char *read_angle_include_filename(void)
{
    char *start = source_buffer_cursor;
    char *name;
    unsigned char character = skip_directive_space();
    if (character == '\n') {
        --source_text_cursor;
        report_syntax_error(12);
        return 0;
    }
    while (character != '>') {
        if (character == '\n') {
            --source_text_cursor;
            report_syntax_error(13);
            return 0;
        }
        *source_buffer_cursor++ = character;
        character = next_source_character();
    }
    if (source_buffer_cursor != start)
        source_buffer_cursor = trim_filename_space(start, source_buffer_cursor);
    *source_buffer_cursor++ = '\0';
    name = token_buffer_end;
    do { *--name = *--source_buffer_cursor; } while (source_buffer_cursor != start);
    return name;
}

/* Original 0x229b0..0x22a28. Skip horizontal directive space and comments,
   leaving the first nonspace character consumed for the directive reader. */
unsigned char skip_directive_space(void)
{
    unsigned char character, class;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        if (character <= '/') {
            if (character == '/') {
                if (skip_source_comment()) continue;
                return character;
            }
            if (character <= ' ') {
                class = character_class[character];
                if (class <= CC_CARRIAGE_RETURN) continue;
                if (class == CC_ESCAPE_OR_END) {
                    recover_special_character();
                    continue;
                }
            }
            return character;
        }
        if (character == '\\' && consume_line_escape()) continue;
        return character;
    }
}

/* Original 0x21360..0x213e1. Return the beginning of a trailing whitespace
   run, or the supplied end when the last character is not whitespace. */
char *trim_filename_space(char *begin, char *end)
{
    char *cursor = begin, *trailing = 0;
    if (!end) end = strchr(begin, '\0');
    while (cursor < end) {
        if (isspace((unsigned char)*cursor)) {
            if (!trailing) trailing = cursor;
        } else trailing = 0;
        ++cursor;
    }
    return trailing ? trailing : end;
}
