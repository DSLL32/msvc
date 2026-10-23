/* Preprocessor directive control flow. */
#include <string.h>
#include <ctype.h>
#include "preprocessor.h"
#include "pch.h"
#include "macro-expand.h"
#include "characters.h"
#include "lexer.h"
#include "parser.h"
#include "control-ir.h"
#include "literal-names.h"
#include <recon.h>

/* Conditional-compilation state, shared with source/include restoration. */
unsigned conditional_branch_state[32]; /* original 0x638a0 */

/* Original 0x205b0..0x20d78; directive dispatch table 0x20d78..0x20db0. */
void preprocess_directive(void)
{
    char *name, *left, *right, *message;
    unsigned char character, class, directive;
    int condition, saved_output, result, index;
    unsigned number;
    char *filename;

    if (macro_expansion_active) return;
    if (!include_depth && pch_mode == 1 && !pch_include_count)
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
        if (options.extensions_enabled) skip_source_line();
        else finish_directive("#endif");
        break;
    case 6: /* define */
        define_source_macro();
        break;
    case 7: /* include */
        filename = read_include_filename();
        if (filename)
            open_include_file(filename, current_token_code != TOK_STRING);
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
            compiler_warning(options.extensions_enabled ? 4 : 1, 112);
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

/* Original 0x20490..0x205a8. Both forced inputs and #include use this
   entry; angle includes bypass the including file's directory. */
int __stdcall open_include_file(char *filename, int system_include)
{
    char *name = filename, *saved_name;
    if (!include_depth && options.pch_create_file &&
        compare_filenames(options.pch_create_file, name)) pch_include_state = 1;
    if (!include_depth && pch_skip_include(name)) return 0;
    finish_directive("#include");
    if (!options.option_E) record_line_information(1);
    if (!strchr(options.path_characters, (unsigned char)*name) &&
        !(strchr(options.path_characters, ':') && name[1] == ':')) {
        if (!system_include && open_include_in_source_directory(name)) return 1;
        search_include_directories(name);
        return 1;
    }
    saved_name = source_buffer_cursor;
    do { *source_buffer_cursor++ = *name++; } while (name[-1]);
    if (!open_source_file(saved_name, 0))
        report_fatal(83, message_text(343), saved_name);
    /* The absolute-name branch passes its consumed input cursor, as in the
       original, rather than the preserved copy used to open the source. */
    if (pch_mode == 1) pch_record_include(name);
    return 1;
}

/* Original 0x20fa0..0x20fd0. The line-ending reader remains responsible
   for consuming the newline after a directive's trailing-token check. */
void __stdcall finish_directive(char *name)
{
    if (skip_directive_space() != '\n') {
        compiler_warning(1, 67, name);
        skip_source_line();
    } else --source_text_cursor;
}

/* Original 0x20e70..0x20f9d. Search /I directories in command-line order,
   returning the scanner workspace to its prior position after a failed open. */
void __stdcall search_include_directories(char *filename)
{
    Vector *directories = options.include_directories;
    short index = 0;
    int attempted = 0;
    char *directory, *input, *start;
    /* The original only assigns this cursor while copying a nonempty
       directory, retaining its prior value for an empty directory string. */
    char *last;
    while (index < directories->count) {
        directory = directories->items[index++];
        attempted = 1;
        start = source_buffer_cursor;
        if (directory && !(directory[0] == '.' && directory[1] == '/' && !directory[2])) {
            input = directory;
            while (*input) {
                *source_buffer_cursor++ = *input;
                last = input++;
            }
            if (!strchr(options.path_characters, (unsigned char)*last))
                *source_buffer_cursor++ = '\\';
        }
        input = filename;
        do { *source_buffer_cursor++ = *input++; } while (input[-1]);
        if (open_source_file(start, 0)) {
            if (pch_mode == 1) pch_record_include(start);
            return;
        }
        source_buffer_cursor = start;
    }
    if (attempted) report_fatal(83, message_text(343), filename);
    else fatal_error(34, filename);
}

/* Original 0x201e0..0x20205. Conditional levels are shared across source
   includes, whose input records remember the depth at their entry. */
int push_conditional_level(void)
{
    if ((unsigned)++conditional_depth >= 32) fatal_error(52);
    return conditional_depth;
}

int macro_parameter_count; /* original 0x63940 */

/* Original 0x1c650..0x1c776. Recognize an adjacent formal list before
   reading a macro's replacement; object-like macros retain parameter count 0. */
void define_source_macro(void)
{
    unsigned char character, class;
    unsigned bytes;
    character = skip_directive_space();
    class = character_class[character];
    if (class != CC_IDENTIFIER && class != CC_WIDE_PREFIX) {
        report_syntax_error(7);
        skip_source_line();
        return;
    }
    scan_source_identifier();
    macro_parameter_count = 0;
    macro_replacement_text = macro_work_buffer;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        class = character_class[character];
        switch (class) {
        case CC_CARRIAGE_RETURN:
            continue;
        case CC_SPACE:
            break;
        case CC_ESCAPE_OR_END:
            if (recover_special_character() & ~0x50U) continue;
            report_syntax_error(8, character);
            break;
        case CC_SLASH:
            if (!skip_source_comment()) report_syntax_error(8, '/');
            break;
        case CC_NEWLINE:
            --source_text_cursor;
            define_macro(0, 0, 0);
            return;
        case CC_LEFT_PAREN:
            read_macro_parameters();
            if (!macro_parameter_count) macro_parameter_count = -1;
            break;
        default:
            report_syntax_error(8, character);
            break;
        }
        break;
    }
    bytes = read_macro_replacement();
    if (!options.extensions_enabled) {
        bytes += macro_replacement_text - macro_work_buffer;
        define_macro(macro_work_buffer, bytes, macro_parameter_count);
    } else define_macro(macro_replacement_text, bytes, macro_parameter_count);
}

/* Original 0x20db0..0x20e54; dispatch table 0x20e54..0x20e6c.
   Return 2 at an elif expression, 3 after else, or 4 after the closing endif. */
int __stdcall skip_inactive_directives(int mode)
{
    unsigned nested = 0;
    unsigned char directive;
    for (;;) {
        directive = find_conditional_directive();
        switch (directive) {
        case 0: case 4: case 5:
            ++nested;
            break;
        case 1: case 2:
            if (nested) break;
            if (mode == 6) { fatal_error(22); break; }
            if (directive == 1) return 2;
            if (options.extensions_enabled) skip_source_line();
            else finish_directive("#else");
            return 3;
        case 3:
            if (nested) { --nested; break; }
            finish_directive("#endif");
            return 4;
        }
    }
}

/* Original 0x21040..0x210fb. Scan discarded source without expanding
   macros, recognizing a directive only at the beginning of a physical line. */
unsigned char find_conditional_directive(void)
{
    unsigned char character, class;
    scanner_flags &= ~0x10000UL;
    for (;;) {
        character = skip_directive_space();
classify_character:
        class = character_class[character];
        if (class == CC_ESCAPE_OR_END) {
            if (recover_special_character() & 0x10) ++source_text_cursor;
        } else if (class == CC_NEWLINE) {
            ++source_line_number;
            if (options.option_E) fputc('\n', preprocessor_stream);
            character = skip_directive_space();
            if (character != '#') goto classify_character;
            character = skip_directive_space();
            class = character_class[character];
            if (class != CC_IDENTIFIER && class != CC_WIDE_PREFIX) goto classify_character;
            scan_source_identifier();
            scanner_flags |= 0x10000;
            return classify_directive(interned_name);
        } else if (class == CC_APOSTROPHE || class == CC_QUOTE)
            skip_inactive_literal(character);
    }
}

/* Original 0x20fd0..0x2103a. Quotes in discarded source still protect
   directive-looking text. Leave an unterminated literal's newline unread. */
void __stdcall skip_inactive_literal(int delimiter)
{
    unsigned char character, class;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        class = character_class[character];
        if (class == CC_ESCAPE_OR_END) {
            if (recover_special_character() & 0x10) ++source_text_cursor;
        } else if (class == CC_NEWLINE) {
            --source_text_cursor;
            return;
        } else if (class == CC_APOSTROPHE || class == CC_QUOTE) {
            if (character == (unsigned char)delimiter) return;
        } else if (class == CC_LEAD_BYTE) next_source_character();
    }
}

/* The replacement workspace reserves five bytes for scanner markers. */
#define CHECK_REPLACEMENT_SPACE(cursor_) do { \
    if (macro_work_buffer + macro_work_bytes - 5 <= (cursor_)) fatal_error(11, token_text); \
} while (0)

/* Original 0x1cae0..0x1ced9. Read the complete replacement token sequence,
   encoding formal references and preserving the deferred end marker. */
unsigned read_macro_replacement(void)
{
    char *destination = macro_replacement_text, *identifier;
    unsigned char character, class, lookahead_class;
    unsigned recovery;
    int stringize = 0, characterize = 0, parameter;
read_initial:
    character = skip_directive_space();
    for (;;) {
        CHECK_REPLACEMENT_SPACE(destination);
        class = character_class[character];
        switch (class) {
        case CC_SLASH:
            if (!skip_source_comment()) goto copy_character;
            /* A comment separates adjacent tokens just as whitespace does. */
        case CC_SPACE: case CC_CARRIAGE_RETURN:
            character = skip_directive_space();
            if (character_class[character] != CC_NEWLINE) *destination++ = ' ';
            continue;
        case CC_ESCAPE_OR_END:
            recovery = recover_special_character();
            if (!(recovery & 0x50)) {
                character = (unsigned char)*source_text_cursor++;
                continue;
            }
            if (!(recovery & 0x10)) goto copy_character;
            *destination++ = character;
            character = next_source_character();
            class = character_class[character];
            if (class == CC_APOSTROPHE || class == CC_QUOTE) {
                --destination;
                report_syntax_error(295, character);
                goto read_initial;
            }
            goto copy_character;
        case CC_APOSTROPHE: case CC_QUOTE:
            destination = copy_macro_quoted_literal(destination,
                macro_work_buffer + macro_work_bytes - 5, character);
            character = (unsigned char)*source_text_cursor++;
            continue;
        case CC_INVALID:
            report_syntax_error(18, character);
            character = (unsigned char)*source_text_cursor++;
            continue;
        case CC_HASH:
            for (;;) {
                lookahead_class = character_class[(unsigned char)*source_text_cursor++];
                if (lookahead_class != CC_ESCAPE_OR_END) break;
                if (!(recover_special_character() & ~0x50U)) break;
            }
            if (lookahead_class == CC_HASH) {
                if (destination == macro_replacement_text) {
                    report_syntax_error(160);
                    continue;
                }
                if (destination[-1] == ' ') --destination;
                character = skip_directive_space();
                if (character_class[character] == CC_NEWLINE) {
                    --source_text_cursor;
                    report_syntax_error(161);
                }
                continue;
            }
            if (lookahead_class == CC_AT) {
                if (macro_parameter_count && options.extensions_enabled) characterize = 1;
                else goto literal_hash;
            } else if (!macro_parameter_count) {
literal_hash:
                *destination++ = '#';
                character = (unsigned char)source_text_cursor[-1];
                continue;
            } else {
                --source_text_cursor;
                stringize = 1;
            }
            character = skip_directive_space();
            class = character_class[character];
            if (class != CC_IDENTIFIER && class != CC_WIDE_PREFIX) {
                stringize = characterize = 0;
                report_syntax_error(162);
                continue;
            }
            /* A # or #@ consumes the following formal identifier here. */
        case CC_IDENTIFIER: case CC_WIDE_PREFIX:
            identifier = destination;
            for (;;) {
                CHECK_REPLACEMENT_SPACE(destination);
                *destination++ = character;
read_identifier_character:
                character = (unsigned char)*source_text_cursor++;
                if (character_flags[character] & CF_IDENTIFIER) continue;
                if (character_class[character] == CC_ESCAPE_OR_END &&
                    (recover_special_character() & ~0x50U)) goto read_identifier_character;
                break;
            }
            *destination = 0;
            parameter = find_macro_parameter(identifier);
            if (parameter >= 1) {
                identifier[0] = stringize ? 2 : (characterize ? 3 : 1);
                identifier[1] = (char)parameter;
                destination = identifier + 2;
            } else if (stringize || characterize) report_syntax_error(162);
            stringize = characterize = 0;
            continue;
        case CC_NEWLINE:
            --source_text_cursor;
            if (destination == (options.extensions_enabled ? macro_replacement_text : macro_work_buffer)) return 0;
            CHECK_REPLACEMENT_SPACE(destination);
            *destination++ = 0;
            *destination++ = 'D';
            return destination - macro_replacement_text;
        default:
copy_character:
            *destination++ = character;
            character = (unsigned char)*source_text_cursor++;
            continue;
        }
    }
}
#undef CHECK_REPLACEMENT_SPACE

/* Original 0x1cee0..0x1cf43. Formal descriptors contain a byte span,
   followed by a terminated spelling. Replacement markers use indices from 1. */
int __stdcall find_macro_parameter(char *name)
{
    char *descriptor = macro_work_buffer, *left, *right;
    int index;
    for (index = 1; index <= macro_parameter_count; ++index) {
        left = name;
        right = descriptor + 1;
        while (*left && *left == *right) { ++left; ++right; }
        if (*left == *right) return index;
        descriptor += (unsigned char)*descriptor;
    }
    return -1;
}

/* Original 0x1c780..0x1c8b5. Encode each formal as its byte span and
   terminated spelling, rejecting duplicate names and empty list elements. */
void read_macro_parameters(void)
{
    char *begin, *write;
    unsigned char character, class;
    for (;;) {
        begin = write = macro_replacement_text + 1;
        for (;;) {
            character = skip_directive_space();
            class = character_class[character];
            if (class == CC_COMMA || class == CC_RIGHT_PAREN) {
                if (write > begin) {
                    if (find_macro_parameter(begin) >= 1) report_syntax_error(9, begin);
                    else {
                        *macro_replacement_text = (unsigned char)(write - macro_replacement_text);
                        macro_replacement_text = write;
                        ++macro_parameter_count;
                    }
                } else if (class == CC_COMMA || macro_parameter_count > 0)
                    report_syntax_error(10, character);
                if (class == CC_RIGHT_PAREN) return;
                break;
            }
            if (class != CC_IDENTIFIER && class != CC_WIDE_PREFIX) {
                report_syntax_error(10, character);
                continue;
            }
            if (begin != write) report_syntax_error(10, character);
            *write++ = character;
            for (;;) {
                character = (unsigned char)*source_text_cursor++;
                while (character_flags[character] & CF_IDENTIFIER) {
                    *write++ = character;
                    character = (unsigned char)*source_text_cursor++;
                }
                if (!character) {
                    if (fill_source_buffer()) macro_scan_error(4);
                    continue;
                }
                if (character == '\\' && consume_line_escape()) continue;
                --source_text_cursor;
                *write++ = 0;
                break;
            }
        }
    }
}

/* Original 0x1c5b0..0x1c647. Remove a user macro from its interned-name
   bucket, preserving PCH events and browser references for the old definition. */
void undefine_source_macro(void)
{
    MacroDefinition **bucket, *definition, *previous = 0;
    if (!include_depth && process_pch_string_event(7, 0, interned_name)) return;
    if (interned_name == name_defined) {
        report_macro_error("#undef");
        return;
    }
    bucket = &macro_buckets[(unsigned)interned_name & 0x1ff];
    for (definition = *bucket; definition; definition = definition->next) {
        if (definition->name == interned_name) break;
        previous = definition;
    }
    if (!definition) return;
    if (definition->parameter_count < -2) {
        report_macro_error("#undef");
        return;
    }
    if (previous) previous->next = definition->next;
    else *bucket = definition->next;
    if (browser_filename) browser_macro_reference(definition);
}

/* Original0x202c0..0x2039d; dispatch data0x203a0..0x203c3.
   Save directive spelling with collapsed space outside quoted text, retaining
   escapes and the newline for automatic-PCH conditional dependency matching. */
void capture_directive_text(void)
{
    unsigned char character = skip_directive_space();
    int quote = 0;
    reset_work_stack(pch_dependency_stack);
    for (;;) {
        switch (character_class[character]) {
        case CC_SPACE: case CC_CARRIAGE_RETURN:
space:
            if (quote) break;
            character = skip_directive_space();
            if (character_class[character] != CC_NEWLINE)
                put_initializer_byte(' ',pch_dependency_stack);
            continue;
        case CC_SLASH:
            if (skip_source_comment()) goto space;
            break;
        case CC_ESCAPE_OR_END:
            put_initializer_byte((signed char)character,pch_dependency_stack);
            character = (unsigned char)*source_text_cursor++;
            break;
        case CC_NEWLINE:
            put_initializer_byte((signed char)character,pch_dependency_stack);
            --source_text_cursor;
            return;
        case CC_APOSTROPHE: quote = quote == 2 ? 0 : 2; break;
        case CC_QUOTE: quote = quote == 1 ? 0 : 1; break;
        }
        put_initializer_byte((signed char)character,pch_dependency_stack);
        character = (unsigned char)*source_text_cursor++;
    }
}

/* Original 0x2d380..0x2d39c. Both directive and pragma parsers
   report the same token spelling before discarding unexpected input. */
void warn_ignored_token(void)
{
    compiler_warning(1, 79, token_names[current_token_code]);
}
