/* Preprocessor directive dispatch and conditional source regions. */
#include "directives-internal.h"
#include "characters.h"
#include "control-ir.h"
#include <ctype.h>
#include <string.h>
#include <recon.h>

unsigned conditional_branch_states[32]; /* original 0x956e4 */
short pch_include_count = -1; /* original 0xa235c: record/playback include depth */

PchInputRecord *pch_input_record; /* original 0x8ccc0 */
short pch_saved_page_count; /* original 0xa2360 */
int pch_input_changed; /* original 0xa2364 */
int pch_abort_on_file_change; /* original 0xa2368 */

/* Original 0x6b530..0x6b649, stdcall4. Record include events while
   capturing, or consume a matching replay include and its file-time events.
   A changed replay input either marks the capture or resumes ordinary input. */
int __stdcall process_pch_include(char *filename)
{
    short previous_count;
    if (!pch_mode) return 0;
    if (pch_mode == 1) {
        previous_count = pch_include_count--;
        if (!previous_count) finish_pch_capture();
        else record_pch_string_event(2, permanent_page_count, filename);
        if (options.pch_auto_file && compare_filenames(options.pch_auto_file, filename))
            pch_include_count = 0;
        return 0;
    }
    if (pch_mode == 2 && pch_input_record->kind == 2 &&
        compare_filenames(pch_input_record->text, filename)) {
        ++pch_include_count;
        pch_saved_page_count = (short)pch_input_record->value;
        for (;;) {
            pch_input_record = (PchInputRecord *)((char *)pch_input_record +
                pch_input_record->bytes + 12);
            if (pch_input_record->kind != 5) return 1;
            if (source_file_timestamp(pch_input_record->text) != pch_input_record->value) {
                pch_input_changed = 1;
                if (pch_abort_on_file_change) return resume_pch_source_input();
            }
        }
    }
    return resume_pch_source_input();
}

/* Original 0x246e0..0x24736, cdecl. Ignore the remainder of a
   directive, consuming comments and DBCS pairs but leave its final newline. */
void skip_source_line(void)
{
    unsigned char classification;
    for (;;) {
        classification = character_class[(unsigned char)*source_text_cursor++];
        switch (classification) {
        case CC_SLASH:
            skip_source_comment();
            break;
        case CC_ESCAPE_OR_END:
            recover_special_character();
            break;
        case CC_NEWLINE:
            --source_text_cursor;
            return;
        case CC_LEAD_BYTE:
            ++source_text_cursor;
        }
    }
}

/* Original 0x24460..0x2454a, cdecl. Ignore inactive source until
   a line-leading directive name, keeping literal contents from being mistaken
   for directives and preserving source-line accounting and optional output. */
unsigned char read_skipped_directive(void)
{
    unsigned char character, classification;
    token_allocation_flags &= ~0x10000UL;
    for (;;) {
        character = next_directive_character();
classify_character:
        classification = character_class[character];
        if (classification == CC_NEWLINE) {
            ++source_line_number;
            if (options.option_E) {
                if (preprocessor_stream) fputc('\n', preprocessor_stream);
                else write_work_character('\n', compiler_stack);
            }
            character = next_directive_character();
            if (character != '#') goto classify_character;
            character = next_directive_character();
            classification = character_class[character];
            if (classification == CC_IDENTIFIER || classification == CC_WIDE_PREFIX) {
                scan_source_identifier();
                token_allocation_flags |= 0x10000UL;
                return classify_directive(interned_name);
            }
            goto classify_character;
        }
        if (classification == CC_ESCAPE_OR_END) {
            if (recover_special_character() & 0x10) ++source_text_cursor;
        } else if (classification == CC_APOSTROPHE || classification == CC_QUOTE) {
            skip_directive_literal(character);
        }
    }
}

/* Original code 0x241d0..0x24272, six-way dispatch 0x24274..0x2428b.
   Skip nested conditional groups until this depth's elif, else or endif.
   Mode6 diagnoses attempts to introduce another alternative after an else. */
int __stdcall skip_conditional_block(unsigned flags)
{
    unsigned depth = 0;
    unsigned char alternative = PP_UNKNOWN, directive;
    for (;;) {
        directive = read_skipped_directive();
        switch (directive) {
        case PP_IF: case PP_IFDEF: case PP_IFNDEF:
            ++depth;
            continue;
        case PP_ELSE:
            alternative = PP_ELSE;
        case PP_ELIF:
            if (depth) {
                alternative = PP_UNKNOWN;
                continue;
            }
            if (flags == 6) {
                fatal_error(22);
                continue;
            }
            if (alternative != PP_ELSE) return 2;
            if (options.configuration.extensions_enabled) skip_source_line();
            else finish_directive("#else");
            return 3;
        case PP_ENDIF:
            if (!depth) {
                finish_directive("#endif");
                return 4;
            }
            --depth;
        }
    }
}

/* Original 0x23600..0x23624, cdecl. Conditional directives share
   the source stack's signed depth, with at most thirty-two active entries. */
int push_conditional_state(void)
{
    ++conditional_depth;
    if ((unsigned)conditional_depth >= 32) fatal_error(52);
    return conditional_depth;
}

/* Original 0x247b0..0x24830, cdecl. Return the beginning of the
   final whitespace run. A null end requests the original string terminator. */
char *trim_filename_space(char *begin, char *end)
{
    char *space = 0;
    if (!end) end = strchr(begin, 0);
    while (begin < end) {
        if (isspace((unsigned char)*begin)) {
            if (!space) space = begin;
        } else space = 0;
        ++begin;
    }
    return space ? space : end;
}

/* Original 0x23630..0x236da, cdecl. Build the angle-delimited name
   in the source-name workspace, trim trailing whitespace, then copy it back
   immediately before the current input position for the include opener. */
char *read_angle_include_name(void)
{
    char *begin = source_buffer_cursor, *name;
    unsigned char character = next_directive_character();
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
        *source_buffer_cursor++ = (char)character;
        character = next_source_character();
    }
    if (source_buffer_cursor != begin)
        source_buffer_cursor = trim_filename_space(begin, source_buffer_cursor);
    *source_buffer_cursor++ = 0;
    name = source_buffer_end;
    do { *--name = *--source_buffer_cursor; }
    while (source_buffer_cursor != begin);
    return name;
}

/* Original 0x237f0..0x238ab, cdecl. Tokenize without /E output or
   escape interpretation. Quoted names are moved back into the source buffer;
   angle names use their own character reader. Invalid forms discard the line. */
char *read_include_name(void)
{
    int saved_output = options.option_E;
    char *name;
    options.option_E = 0;
    token_allocation_flags |= 0x4000;
    read_parser_token();
    token_allocation_flags &= ~0x4000U;
    options.option_E = saved_output;
    if (current_token_code == 3) {
        if (current_token_value.string.flags & 1) report_syntax_error(2);
        name = source_buffer_end;
        do { *--name = (char)*--text_work_cursor; }
        while (text_work_cursor != (unsigned char *)current_token_value.string.text);
        return name;
    }
    if (current_token_code == 0x4d) {
        name = read_angle_include_name();
        if (name) return name;
    } else {
        report_syntax_error(6, token_spellings[current_token_code]);
    }
    skip_source_line();
    token_allocation_flags &= ~0x10000UL;
    return 0;
}

/* Original 0x239d0..0x24195; fourteen-way switch data at0x24198..0x241cf.
   Recover every directive arm, including PCH condition transport, rather than
   specializing this dispatcher to the generated member text's #line case. */
void preprocess_directive(void)
{
    char *directive_name, *filename, *left, *right, *destination;
    unsigned char character, classification, directive;
    int saved_output, condition, skipped, depth;
    /* Playback consumes the recorded value and ignores this local's input.
       The original also leaves it unwritten on that playback-only path. */
    if (macro_expansion_active) return;
    if (!include_depth && pch_mode == 1 && !pch_include_count)
        finish_pch_capture();
    directive_name = interned_name;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        classification = character_class[character];
        if (classification == CC_SPACE && (character == ' ' || character == '\t'))
            continue;
        if (classification == CC_CARRIAGE_RETURN) continue;
        if (classification == CC_NEWLINE) {
            --source_text_cursor;
            return;
        }
        if (classification == CC_SLASH) {
            if (skip_source_comment()) continue;
            break;
        }
        if (classification == CC_ESCAPE_OR_END) {
            if (recover_special_character() & ~0x50U) continue;
            break;
        }
        if (classification == CC_IDENTIFIER || classification == CC_WIDE_PREFIX) {
            scan_source_identifier();
            directive_name = interned_name;
            break;
        }
        report_syntax_error(19, (unsigned)character);
        skip_source_line();
        return;
    }
    token_allocation_flags |= 0x10000UL;
dispatch_directive:
    directive = classify_directive(directive_name);
    switch (directive) {
    case PP_IF:
        if (!include_depth && pch_mode) {
            capture_directive_text();
            if (!process_pch_directive()) {
                saved_output = options.option_E;
                options.option_E = 0;
                token_allocation_flags |= 0x8000;
                condition = evaluate_preprocessor_expression();
                token_allocation_flags &= ~0x8000UL;
                options.option_E = saved_output;
                finish_directive("#if/#elif");
            } else if (pch_mode == 1) break;
            condition = record_pch_condition((short)condition);
        } else {
            saved_output = options.option_E;
            options.option_E = 0;
            token_allocation_flags |= 0x8000;
            condition = evaluate_preprocessor_expression();
            token_allocation_flags &= ~0x8000UL;
            options.option_E = saved_output;
            finish_directive("#if/#elif");
        }
        if (condition || (skipped = skip_conditional_block(5)) == 3) {
            depth = push_conditional_state();
            conditional_branch_states[depth] = condition ? 1 : 3;
            if (!options.configuration.option_EP && !condition) emit_line_directive();
        } else {
            if (!options.configuration.option_EP) emit_line_directive();
            if (skipped == 2) {
                directive_name = "if";
                condition = 0;
                goto dispatch_directive;
            }
        }
        break;
    case PP_ELIF:
        depth = conditional_depth;
        conditional_depth = (int)((unsigned)depth - 1);
        if (depth < 0) fatal_error(18);
        do { skipped = skip_conditional_block(5); } while (skipped != 4);
        if (!options.configuration.option_EP) emit_line_directive();
        break;
    case PP_ELSE:
        depth = conditional_depth;
        if (depth < 0) fatal_error(19);
        else {
            conditional_depth = (int)((unsigned)depth - 1);
            if (conditional_branch_states[depth] != 1) fatal_error(19);
        }
        finish_directive("#else");
        skip_conditional_block(6);
        if (!options.configuration.option_EP) emit_line_directive();
        break;
    case PP_ENDIF:
        depth = conditional_depth;
        conditional_depth = (int)((unsigned)depth - 1);
        if (depth < 0) fatal_error(20);
        if (!options.configuration.option_EP) emit_line_directive();
        if (options.configuration.extensions_enabled) skip_source_line();
        else finish_directive("#endif");
        break;
    case PP_IFDEF:
    case PP_IFNDEF:
        if (!include_depth && pch_mode) {
            capture_directive_text();
            if (!process_pch_directive()) {
                character = next_directive_character();
                classification = character_class[character];
                if (classification != CC_IDENTIFIER && classification != CC_WIDE_PREFIX)
                    fatal_error(16);
                scan_source_identifier();
                condition = lookup_macro() != 0;
                finish_directive(directive == PP_IFDEF ? "#ifdef" : "#ifndef");
                if (directive == PP_IFNDEF) condition = !condition;
            } else if (pch_mode == 1) break;
            condition = record_pch_condition((short)condition);
        } else {
            character = next_directive_character();
            classification = character_class[character];
            if (classification != CC_IDENTIFIER && classification != CC_WIDE_PREFIX)
                fatal_error(16);
            scan_source_identifier();
            condition = lookup_macro() != 0;
            finish_directive(directive == PP_IFDEF ? "#ifdef" : "#ifndef");
            if (directive == PP_IFNDEF) condition = !condition;
        }
        if (!condition && (skipped = skip_conditional_block(5)) != 3) {
            if (skipped == 2) {
                directive_name = "if";
                condition = 0;
                goto dispatch_directive;
            }
        } else {
            depth = push_conditional_state();
            conditional_branch_states[depth] = condition ? 1 : 3;
        }
        break;
    case PP_DEFINE:
        define_source_macro();
        break;
    case PP_INCLUDE:
        filename = read_include_name();
        if (filename) open_forced_include(filename, current_token_code != 3);
        break;
    case PP_PRAGMA:
        if (!include_depth && pch_mode) {
            capture_directive_text();
            if (process_pch_directive() != 1) process_pragma();
        } else process_pragma();
        break;
    case PP_UNDEF:
        character = next_directive_character();
        classification = character_class[character];
        if (classification == CC_IDENTIFIER || classification == CC_WIDE_PREFIX) {
            scan_source_identifier();
            undefine_source_macro();
        } else compiler_warning(1, 6);
        finish_directive("#undef");
        break;
    case PP_LINE:
        if (!include_depth && pch_mode) finish_pch_capture();
        saved_output = options.option_E;
        options.option_E = 0;
        read_parser_token();
        if (current_token_code < 7 || current_token_code > 10) {
            report_syntax_error(5, token_spellings[current_token_code]);
            options.option_E = saved_output;
            skip_source_line();
            goto directive_done;
        }
        if ((int)current_token_value.expression->value.integer < 1 ||
            (int)current_token_value.expression->value.integer > 0x7fff) {
            compiler_warning(options.configuration.extensions_enabled ? 4 : 1, 112);
            if ((int)current_token_value.expression->value.integer < 1)
                current_token_value.expression->value.integer = 1;
        }
        source_line_number = current_token_value.expression->value.integer - 1;
        read_parser_token();
        options.option_E = saved_output;
        if (current_token_code != 1) {
            if (current_token_code != 3) {
                report_syntax_error(130, token_spellings[current_token_code]);
                skip_source_line();
                goto directive_done;
            }
            if (current_token_value.string.flags & 1) report_syntax_error(2);
            left = current_token_value.string.text;
            right = source_filename;
            while (*left == *right && *left) { ++left; ++right; }
            if (*left != *right) {
                source_buffer_cursor = source_filename;
                text_work_cursor = (unsigned char *)current_token_value.string.text;
                do { *source_buffer_cursor++ = *text_work_cursor++; }
                while (source_buffer_cursor[-1]);
                if (!options.option_E) {
                    source_file_index = allocate_permanent(4);
                    *source_file_index = 0xffff;
                    if (declaration_mode) record_line_information(2);
                    if (browser_filename) {
                        browser_include_end();
                        browser_open_file();
                    }
                }
            }
            text_work_cursor = (unsigned char *)current_token_value.string.text;
        }
        if (!options.option_E) record_line_information(1);
        else emit_line_directive();
        finish_directive("#line");
        break;
    case PP_UNKNOWN:
        fatal_error(21, directive_name);
        break;
    case PP_ERROR:
        destination = token_text;
        for (;;) {
            character = (unsigned char)*source_text_cursor++;
            classification = character_class[character];
            if (classification == CC_ESCAPE_OR_END) {
                recover_special_character();
                continue;
            }
            if (classification == CC_NEWLINE) break;
            if (classification == CC_LEAD_BYTE) {
                *destination++ = character;
                character = (unsigned char)*source_text_cursor++;
            }
            *destination++ = character;
        }
        --source_text_cursor;
        *destination = 0;
        report_syntax_error(189, token_text);
        goto finish_error_directive;
    case PP_IDENT:
        saved_output = options.option_E;
        options.option_E = 0;
        read_parser_token();
        options.option_E = saved_output;
        if (current_token_code == 3) {
            pending_pragma_kind = 3;
            pending_pragma_text = (unsigned char *)current_token_value.string.text;
            record_line_information(11);
        } else warn_ident_requires_string();
finish_error_directive:
        /* The native #ident arm uses the same diagnostic name as #error. */
        finish_directive("#error");
        break;
    default:
        break;
    }
directive_done:
    token_allocation_flags &= ~0x10000UL;
}

/* The native first-character/suffix index is represented as the complete
   logical vocabulary reached by this classifier. Identifier scanning has
   already established its input domain; directive spelling is case sensitive. */
typedef struct DirectiveKeyword {
    char *name;
    unsigned char kind;
} DirectiveKeyword;
typedef char DirectiveKeywordSizeMustBe8[(sizeof(DirectiveKeyword) == 8) ? 1 : -1];
typedef char DirectiveKeywordKindOffsetMustBe4[(offsetof(DirectiveKeyword, kind) == 4) ? 1 : -1];
DirectiveKeyword directive_keywords[13] = {
    { "define", PP_DEFINE }, { "error", PP_ERROR },
    { "endif", PP_ENDIF }, { "else", PP_ELSE }, { "elif", PP_ELIF },
    { "include", PP_INCLUDE }, { "ifndef", PP_IFNDEF },
    { "ifdef", PP_IFDEF }, { "if", PP_IF }, { "ident", PP_IDENT },
    { "line", PP_LINE }, { "pragma", PP_PRAGMA }, { "undef", PP_UNDEF }
};

/* Original 0x1f470..0x1f4f7. Search only the current first-character
   group, comparing its suffix inline without adding a CRT strcmp edge. */
unsigned char __stdcall classify_directive(char *name)
{
    signed char first = (signed char)*name;
    char *keyword, *input;
    unsigned index;
    if (first < '_') return PP_UNKNOWN;
    for (index = 0; index < 13; ++index) {
        if (first != directive_keywords[index].name[0]) continue;
        keyword = directive_keywords[index].name + 1;
        input = name + 1;
        while (*keyword == *input && *keyword) { ++keyword; ++input; }
        if (*keyword == *input) return directive_keywords[index].kind;
    }
    return PP_UNKNOWN;
}

/* Original 0x243c0..0x243ef, stdcall4. Leave the newline unread for
   the main lexer; extra directive text warns with its caller's context. */
void __stdcall finish_directive(char *name)
{
    if (next_directive_character() != '\n') {
        compiler_warning(1, 67, name);
        skip_source_line();
    } else {
        --source_text_cursor;
    }
}

/* Original 0x25f90..0x26007. Skip horizontal/CR whitespace, comments,
   source-buffer escapes and continued lines, leaving a real newline visible. */
unsigned char next_directive_character(void)
{
    unsigned char character, classification;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        if (character <= '/') {
            if (character == '/') {
                if (skip_source_comment()) continue;
                return '/';
            }
            if (character > ' ') return character;
            classification = character_class[character];
            if (classification <= CC_CARRIAGE_RETURN) continue;
            if (classification == CC_ESCAPE_OR_END) {
                recover_special_character();
                continue;
            }
            return character;
        }
        if (character == '\\' && consume_line_escape()) continue;
        return character;
    }
}

/* Original 0x243f0..0x2445a, stdcall4. Quotes in an inactive
   directive region still protect embedded directive-looking text. Leave the
   physical newline for the conditional-source scanner. */
void __stdcall skip_directive_literal(unsigned char delimiter)
{
    unsigned char character;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        switch (character_class[character]) {
        case CC_ESCAPE_OR_END:
            if (recover_special_character() & 0x10) ++source_text_cursor;
            break;
        case CC_NEWLINE:
            --source_text_cursor;
            return;
        case CC_APOSTROPHE:
        case CC_QUOTE:
            if (character == delimiter) return;
            break;
        case CC_LEAD_BYTE:
            next_source_character();
            break;
        }
    }
}

/* Original0x3c8f0..0x3c90c, cdecl. The diagnostic names the actual
   token code, which is distinct from the pragma parser's token category. */
void warn_ident_requires_string(void)
{
    compiler_warning(1, 79, token_spellings[current_token_code]);
}
