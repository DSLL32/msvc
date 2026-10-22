/* Macro replacement, predefined macros, and recursive rescan frames. */
#include "macro-expand.h"
#include "lexer.h"
#include "source-input.h"
#include "characters.h"
#include "pp-expression.h"

typedef char MacroFrame_must_be_twenty_bytes[(sizeof(MacroFrame) == 20) ? 1 : -1];
MacroFrame macro_frames[256]; /* original 0x63944..0x64d44 */
int macro_expansion_active; /* original 0x63854 */
int macro_rescan_depth; /* original 0x6389c */
unsigned macro_definition_line; /* original 0x6392c */
char *macro_input_mark; /* original 0x63930 */
int macro_actual_argument_count; /* original 0x64d44 */
static char file_macro_timestamp[25]; /* original 0x60814 */

/* Original 0x1d040..0x1d554; predefined dispatch table 0x1d554..0x1d56c. */
int __stdcall expand_macro(MacroDefinition *definition)
{
    int expanded = 0, parameter_count;
    unsigned flags, i, length;
    unsigned char character, class;
    char *text;
    macro_rescan_depth = 0;
    macro_replacement_start = macro_expansion_next;
    macro_definition_line = source_line_number;
expand_definition:
    macro_input_mark = macro_expansion_start;
    macro_actual_argument_count = 0;
    parameter_count = definition->parameter_count;
    if (parameter_count < -2) {
        if (browser_filename) browser_macro_reference(definition);
        push_macro_frame(definition);
        ++macro_frames[macro_expansion_active].definition->disabled;
        switch (parameter_count) {
        case -8: /* __TIMESTAMP__ */
            if (!options.option_Brepro) {
                text = format_timestamp(source_file_timestamp());
                for (i = 0; i < 25; ++i) file_macro_timestamp[i] = text[i];
                for (i = 0; i < 8; ++i) file_macro_timestamp[11+i] = text[16+i];
                for (i = 0; i < 5; ++i) file_macro_timestamp[19+i] = text[10+i];
                file_macro_timestamp[24] = '\0';
            } else { file_macro_timestamp[0] = '1'; file_macro_timestamp[1] = '\0'; }
            append_macro_string('"', file_macro_timestamp);
            break;
        case -7: append_macro_text("1"); break; /* __STDC__ */
        case -6: append_macro_string('"', macro_time); break;
        case -5: append_macro_string('"', macro_date); break;
        case -4:
            macro_expansion_next = quote_macro_filename(macro_expansion_next,
                options.option_Brepro ? "Brepro.c" : source_filename);
            break;
        case -3:
            format_text(token_text, "%d", source_line_number);
            append_macro_text(token_text);
            break;
        }
        if (pop_macro_frame()) return 1;
        goto rescan_or_finish;
    }
    if (!parameter_count) {
        expanded = 1;
        if (browser_filename) browser_macro_reference(definition);
        if (definition->replacement) {
            push_macro_frame(definition);
            begin_macro_replacement();
        } else macro_expansion_next = macro_replacement_start;
        goto rescan_or_finish;
    }
    if (parameter_count == -1) parameter_count = 0;
    for (;;) {
        if (!skip_macro_space()) goto rescan_or_finish;
        if (character_class[(unsigned char)*source_text_cursor] == CC_SLASH) {
            ++source_text_cursor;
            if (skip_source_comment()) continue;
            --source_text_cursor;
        }
        break;
    }
    if (character_class[(unsigned char)*source_text_cursor] == CC_LEFT_PAREN) {
        expanded = 1;
        ++source_text_cursor;
        scanner_flags |= 0x2000;
        expand_macro_arguments(definition, parameter_count);
        scanner_flags &= ~0x2000UL;
        if (browser_filename) browser_macro_reference(definition);
    } else if (scanner_flags & 0x800000) {
        --source_text_cursor;
        scanner_flags &= ~0x800000UL;
        *source_text_cursor = '\n';
        --source_line_number;
    } else if (macro_space_seen) *--source_text_cursor = ' ';
rescan_or_finish:
    if (!macro_rescan_depth) return expanded;
    if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
        macro_scan_error(56);
    character = (unsigned char)*source_text_cursor++;
    class = character_class[character];
    switch (class) {
    case CC_ESCAPE_OR_END:
        flags = recover_special_character();
        if (flags & 0x50) goto emit_character;
        goto rescan_or_finish;
    case CC_APOSTROPHE: case CC_QUOTE:
        macro_expansion_next = copy_macro_quoted_literal(macro_expansion_next,
            macro_work_buffer + macro_work_bytes - 5, character);
        goto rescan_or_finish;
    case CC_DOT:
        if (!(character_flags[(unsigned char)*source_text_cursor] & CF_DECIMAL)) {
            *macro_expansion_next++ = '.';
            goto rescan_or_finish;
        }
        character = '.';
        /* Fall through to the preprocessing-number copier. */
    case CC_DIGIT:
        for (;;) {
            *macro_expansion_next++ = character;
            if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
                macro_scan_error(56);
            character = (unsigned char)*source_text_cursor++;
            if (character_flags[character] & CF_NUMBER) continue;
            /* The original tests the current output byte, rather than the
               preceding byte, when deciding whether an exponent sign fits. */
            if ((character == '+' || character == '-') &&
                (*macro_expansion_next == 'e' || *macro_expansion_next == 'E')) continue;
            --source_text_cursor;
            goto rescan_or_finish;
        }
    case CC_MACRO_PARAMETER: case CC_IDENTIFIER: case CC_WIDE_PREFIX:
        macro_replacement_start = macro_expansion_next;
        if (scan_macro_argument_identifier(character)) {
            definition = lookup_macro();
            if (definition) {
                if (!definition->disabled) goto expand_definition;
                for (length = 0; interned_name[length]; ++length) ;
                *macro_expansion_next++ = 4;
                *macro_expansion_next++ = (unsigned char)length;
            }
        }
        if ((scanner_flags & 0x8000) && interned_name == name_defined)
            evaluate_defined_operator(interned_name);
        goto rescan_or_finish;
    case CC_MACRO_STRING:
        append_macro_string('"', read_macro_string());
        goto rescan_or_finish;
    case CC_MACRO_TOKEN:
        append_macro_string('\'', read_macro_string());
        goto rescan_or_finish;
    case CC_SPACE:
        do { character = (unsigned char)*source_text_cursor++; }
        while (character_flags[character] & CF_SPACE);
        --source_text_cursor;
        character = ' ';
        break;
    }
emit_character:
    *macro_expansion_next++ = character;
    goto rescan_or_finish;
}

/* Original 0x1dc00..0x1dc85. Frame 0 is reserved; the first active frame
   owns the saved physical input cursor used by include-buffer eviction. */
void __stdcall push_macro_frame(MacroDefinition *definition)
{
    MacroFrame *frame;
    if (++macro_expansion_active >= 256) fatal_error(9);
    ++macro_rescan_depth;
    frame = &macro_frames[macro_expansion_active];
    frame->definition = definition;
    frame->saved_expansion_start = macro_input_mark;
    frame->argument_count = (signed char)macro_actual_argument_count;
    frame->argument_index = 0;
    frame->source_cursor = source_text_cursor;
    macro_expansion_next = macro_replacement_start;
    frame->output_start = macro_expansion_next;
}

/* Original 0x1dc90..0x1dcdd. Formal names prefix replacement bytes in
   the non-extension representation; a live replacement disables its macro. */
void begin_macro_replacement(void)
{
    MacroDefinition *definition = macro_frames[macro_expansion_active].definition;
    int remaining = definition->parameter_count;
    source_text_cursor = definition->replacement;
    if (!options.z_a) {
        while (remaining-- > 0)
            source_text_cursor += (unsigned char)*source_text_cursor;
    }
    ++definition->disabled;
}

/* Original 0x1d960..0x1d9f8. The last rescan frame stays active until
   the lexer consumes its result and the terminating restore marker. */
int pop_macro_frame(void)
{
    MacroFrame *frame;
    if (!--macro_rescan_depth) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
            macro_scan_error(56);
        *macro_expansion_next++ = 0;
        *macro_expansion_next++ = 'R';
        source_text_cursor = macro_frames[macro_expansion_active].output_start;
        browser_macro_context = 1;
        return 1;
    }
    frame = &macro_frames[macro_expansion_active];
    source_text_cursor = frame->source_cursor;
    macro_expansion_start = frame->saved_expansion_start;
    --frame->definition->disabled;
    --macro_expansion_active;
    return 0;
}

/* Original 0x1ddf0..0x1dfac. Assemble an identifier from replacement text
   and formal substitutions, retaining the marker that inhibits re-expansion. */
int __stdcall scan_macro_argument_identifier(int first_character)
{
    char *begin = macro_expansion_next, *argument;
    int valid = 1, argument_mode = 0, disabled_length = -1;
    unsigned char character = (unsigned char)first_character;
    for (;;) {
        if (character_class[character] == CC_MACRO_PARAMETER) {
            argument = select_macro_argument(&argument_mode);
            if (argument) {
                while (*argument) {
                    if (!(character_flags[(unsigned char)*argument] & CF_IDENTIFIER)) valid = 0;
                    if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
                        macro_scan_error(56);
                    *macro_expansion_next++ = *argument++;
                }
            }
        } else *macro_expansion_next++ = character;
        for (;;) {
            character = (unsigned char)*source_text_cursor++;
            while (character_flags[character] & CF_IDENTIFIER) {
                if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
                    macro_scan_error(56);
                *macro_expansion_next++ = character;
                character = (unsigned char)*source_text_cursor++;
            }
            if (character_class[character] != CC_MACRO_END) break;
            disabled_length = (unsigned char)*source_text_cursor++;
        }
        if (character_class[character] != CC_MACRO_PARAMETER) break;
        argument_mode = 1;
    }
    --source_text_cursor;
    if (valid) {
        if (disabled_length != -1 && macro_expansion_next - begin == disabled_length) {
            valid = 0;
            *macro_expansion_next++ = 4;
            *macro_expansion_next++ = (unsigned char)disabled_length;
            *macro_expansion_next = 0;
        }
        if (valid) {
            *macro_expansion_next = 0;
            interned_name = intern_identifier(begin);
            return 1;
        }
    }
    return valid;
}

/* Original 0x1cf50..0x1d036. Copy a replacement's quoted token without
   interpreting its escapes, retaining line-boundary and lead-byte handling. */
char *__stdcall copy_macro_quoted_literal(char *destination, char *limit, int delimiter)
{
    unsigned char character, class;
    unsigned flags;
    *destination++ = (unsigned char)delimiter;
    while (destination < limit) {
        character = (unsigned char)*source_text_cursor++;
        class = character_class[character];
        switch (class) {
        case CC_ESCAPE_OR_END:
            flags = recover_special_character();
            if (!(flags & 0x50)) continue;
            if (flags & 0x10) {
                *destination++ = source_text_cursor[-1];
                *destination++ = next_source_character();
                continue;
            }
            break;
        case CC_NEWLINE:
            report_syntax_error(1);
            --source_text_cursor;
            character = (unsigned char)delimiter;
            /* Finish the token with its expected delimiter after the error. */
        case CC_APOSTROPHE: case CC_QUOTE:
            if (character == (unsigned char)delimiter) {
                *destination++ = character;
                return destination;
            }
            break;
        case CC_LEAD_BYTE:
            *destination++ = character;
            parser_flags |= 0x02000000UL;
            character = next_source_character();
            parser_flags &= ~0x02000000UL;
            break;
        }
        *destination++ = character;
    }
    return limit - 1;
}
