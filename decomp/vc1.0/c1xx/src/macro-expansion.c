/* Macro substitution and rescan driver, retaining its separate helper graph. */
#include "macro-expansion.h"
#include "source-input.h"
#include "characters.h"
#include "text-format.h"
#include <recon.h>

MacroExpansionFrame macro_expansion_frames[256]; /* original0x9578c */
char *macro_argument_start; /* original0x9576c */
int macro_argument_state; /* original0x95770 */
int macro_intertoken_space; /* original0x95774 */
unsigned macro_expansion_line; /* original0x95778 */
char macro_file_timestamp[26]; /* original0x918c8 */

/* Original0x20090..0x205b3, builtin dispatch0x205b4..0x205cf, stdcall4.
   An empty object macro still counts as expansion without entering a rescan
   frame. Nested replacements reuse this driver, while active macro names are
   emitted with a counted marker that prevents immediate recursive expansion. */
int __stdcall expand_macro(MacroDefinition *definition)
{
    int expanded = 0, count;
    unsigned i, length;
    unsigned char character, classification;
    char *text;
    macro_rescan_depth = 0;
    macro_replacement_start = macro_work_midpoint;
    macro_expansion_line = source_line_number;
expand_definition:
    macro_argument_start = macro_expansion_start;
    macro_argument_state = 0;
    count = definition->parameter_count;
    if (count < -2) {
        if (browser_filename) reference_browser_macro(definition);
        push_macro_expansion(definition);
        ++macro_expansion_frames[macro_expansion_active].definition->flags;
        switch (count) {
        case -9:
            emit_macro_text("1");
            break;
        case -8:
            if (!options.configuration.option_Brepro) {
                text = format_timestamp(current_source_timestamp());
                for (i = 0; i < 25; ++i) macro_file_timestamp[i] = text[i];
                for (i = 0; i < 8; ++i) macro_file_timestamp[11 + i] = text[16 + i];
                for (i = 0; i < 5; ++i) macro_file_timestamp[19 + i] = text[10 + i];
                macro_file_timestamp[24] = 0;
            } else {
                macro_file_timestamp[0] = '1';
                macro_file_timestamp[1] = 0;
            }
            emit_macro_string('"', macro_file_timestamp);
            break;
        case -7:
            emit_macro_text("1");
            break;
        case -6:
            emit_macro_string('"', macro_time);
            break;
        case -5:
            emit_macro_string('"', macro_date);
            break;
        case -4:
            macro_work_midpoint = quote_source_filename(macro_work_midpoint,
                options.configuration.option_Brepro ? "Brepro.cxx" : source_filename);
            break;
        case -3:
            format_text(token_text, "%d", source_line_number);
            emit_macro_text(token_text);
        }
        if (finish_macro_expansion()) return 1;
    } else if (!count) {
        expanded = 1;
        if (browser_filename) reference_browser_macro(definition);
        if (definition->replacement) {
            push_macro_expansion(definition);
            rescan_macro_replacement();
        } else macro_work_midpoint = macro_replacement_start;
    } else {
        if (count == -1) count = 0;
        for (;;) {
            if (!skip_macro_space()) goto rescan_expansion;
            if (character_class[(unsigned char)*source_text_cursor] != CC_SLASH) break;
            ++source_text_cursor;
            if (!skip_source_comment()) {
                --source_text_cursor;
                break;
            }
        }
        /* Failure to find further source leaves its current position intact. */
        if (character_class[(unsigned char)*source_text_cursor] == CC_LEFT_PAREN) {
            expanded = 1;
            ++source_text_cursor;
            token_allocation_flags |= 0x2000;
            expand_macro_arguments(definition, count);
            token_allocation_flags &= ~0x2000U;
            if (browser_filename) reference_browser_macro(definition);
        } else if (token_allocation_flags & 0x800000UL) {
            *--source_text_cursor = '\n';
            token_allocation_flags &= ~0x800000UL;
            --source_line_number;
        } else if (macro_intertoken_space) *--source_text_cursor = ' ';
    }
rescan_expansion:
    while (macro_rescan_depth) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_work_midpoint)
            macro_expansion_error(56);
        character = (unsigned char)*source_text_cursor++;
        classification = character_class[character];
        switch (classification) {
        case CC_MACRO_PARAMETER: case CC_IDENTIFIER: case CC_WIDE_PREFIX:
            macro_replacement_start = macro_work_midpoint;
            if (scan_macro_name(character)) {
                definition = lookup_macro();
                if (definition) {
                    if (!definition->flags) goto expand_definition;
                    for (length = 0; interned_name[length]; ++length) ;
                    *macro_work_midpoint++ = 4;
                    *macro_work_midpoint++ = (char)length;
                }
            }
            if ((token_allocation_flags & 0x8000) && interned_name == name_defined)
                check_defined_macro(interned_name);
            break;
        case CC_MACRO_STRING:
            emit_macro_string('"', read_macro_parameter());
            break;
        case CC_MACRO_TOKEN:
            emit_macro_string('\'', read_macro_parameter());
            break;
        case CC_APOSTROPHE: case CC_QUOTE:
            macro_work_midpoint = copy_macro_literal(macro_work_midpoint,
                macro_work_buffer + macro_work_bytes - 5, character);
            break;
        case CC_DOT:
            if (!(character_flags[(unsigned char)*source_text_cursor] & CF_DECIMAL)) {
                *macro_work_midpoint++ = '.';
                break;
            }
            character = '.';
        case CC_DIGIT:
            for (;;) {
                *macro_work_midpoint++ = (char)character;
                if (macro_work_buffer + macro_work_bytes - 5 <= macro_work_midpoint)
                    macro_expansion_error(56);
                character = (unsigned char)*source_text_cursor++;
                if (character_flags[character] & CF_NUMBER) continue;
                /* Original204a0 tests the byte at the output cursor itself. */
                if ((character == '+' || character == '-') &&
                    (*macro_work_midpoint == 'e' || *macro_work_midpoint == 'E')) continue;
                --source_text_cursor;
                break;
            }
            break;
        case CC_SPACE:
            while (character_flags[(unsigned char)*source_text_cursor++] & CF_SPACE) ;
            --source_text_cursor;
            *macro_work_midpoint++ = ' ';
            break;
        case CC_ESCAPE_OR_END:
            if (!(recover_special_character() & 0x50)) break;
        default:
            *macro_work_midpoint++ = (char)character;
        }
    }
    return expanded;
}

/* Original 0x24550..0x246d1. The spelling test is independent of the
   interned-name identity; scan the operand without expanding its definition. */
int __stdcall check_defined_macro(char *name)
{
    int defined = 0, parentheses = 0;
    unsigned char character, classification;
    char *spelling = "defined";
    /* The original compiler expands this comparison in the caller. */
    while (*name && *name == *spelling) { ++name; ++spelling; }
    if (*name != *spelling) return 0;
    if (!skip_macro_space() && !macro_rescan_depth) return 0;
    if (*source_text_cursor == '(') {
        ++source_text_cursor;
        parentheses = 1;
        if (!skip_macro_space() && !macro_rescan_depth) return 0;
    }
    character = (unsigned char)*source_text_cursor;
    classification = character_class[character];
    if (macro_rescan_depth > 0) {
        if (classification == CC_MACRO_PARAMETER ||
            classification == CC_IDENTIFIER || classification == CC_WIDE_PREFIX) {
            ++source_text_cursor;
            scan_macro_name(character);
            defined = lookup_macro() != 0;
        }
    } else if (classification == CC_IDENTIFIER || classification == CC_WIDE_PREFIX) {
        ++source_text_cursor;
        if (macro_expansion_active > 0) scan_macro_identifier();
        else scan_source_identifier();
        defined = lookup_macro() != 0;
    } else report_syntax_error(parentheses ? 4 : 3);
    classification = character_class[(unsigned char)*source_text_cursor];
    if ((classification == CC_SPACE || classification == CC_ESCAPE_OR_END) &&
        !skip_macro_space()) return defined;
    if (parentheses) {
        if (character_class[(unsigned char)*source_text_cursor] == CC_RIGHT_PAREN) {
            ++source_text_cursor;
            --parentheses;
        }
    }
    if (parentheses > 0 && !macro_rescan_depth) compiler_warning(1, 4);
    return defined;
}
