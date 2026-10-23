/* Preprocessor directive dispatch and conditional source regions. */
#include "directives-internal.h"
#include "characters.h"
#include "control-ir.h"
#include <recon.h>

unsigned conditional_branch_states[32]; /* original 0x956e4 */
short pch_include_count = -1; /* original 0xa235c: record/playback include depth */

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
