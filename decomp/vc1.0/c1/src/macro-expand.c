/* Macro replacement, predefined macros, and recursive rescan frames. */
#include "macro-expand.h"
#include "lexer.h"
#include "source-input.h"
#include "characters.h"
#include "pp-expression.h"
#include <recon.h>

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
    if (!options.extensions_enabled) {
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

/* Original 0x1d570..0x1d8a1; class dispatch 0x1d8a4..0x1d95c.
   Collect actual arguments before activating their replacement/rescan frame. */
void __stdcall expand_macro_arguments(MacroDefinition *definition, int parameter_count)
{
    char *saved_expansion = macro_expansion_start, *begin, *saved_output, *saved_start;
    int depth = 0, argument_mode;
    unsigned flags;
    unsigned char character, class;
    *macro_expansion_next++ = source_text_cursor[-1];
    for (;;) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
            macro_scan_error(56);
        for (;;) {
            if (!skip_macro_space()) return;
            if (character_class[(unsigned char)*source_text_cursor] != CC_SLASH) {
                begin = macro_expansion_next;
                break;
            }
            ++source_text_cursor;
            if (skip_source_comment()) continue;
            begin = macro_expansion_next;
            *macro_expansion_next++ = '/';
            break;
        }
        argument_mode = 0;
        for (;;) {
            character = (unsigned char)*source_text_cursor++;
            class = character_class[character];
            switch (class) {
            case CC_SPACE: case CC_CARRIAGE_RETURN: case CC_NEWLINE:
                --source_text_cursor;
                if (!skip_macro_space()) return;
                continue;
            case CC_SLASH:
                if (skip_source_comment()) {
                    if (macro_expansion_next[-1] == ' ') continue;
                    character = ' ';
                }
                break;
            case CC_ESCAPE_OR_END:
                saved_output = macro_expansion_next;
                saved_start = macro_expansion_start;
                flags = recover_special_character();
                if (flags & 0xa) return;
                macro_expansion_start = saved_start;
                macro_expansion_next = saved_output;
                if (!(flags & 0x50)) continue;
                break;
            case CC_APOSTROPHE: case CC_QUOTE:
                macro_expansion_next = copy_macro_quoted_literal(macro_expansion_next,
                    macro_work_buffer + macro_work_bytes - 5, character);
                continue;
            case CC_COMMA:
                if (!depth) goto finish_argument;
                break;
            case CC_RIGHT_PAREN:
                if (--depth < 0) goto finish_argument;
                break;
            case CC_LEFT_PAREN:
                ++depth;
                break;
            case CC_MACRO_STRING:
                append_macro_string('"', read_macro_string());
                continue;
            case CC_MACRO_TOKEN:
                append_macro_string('\'', read_macro_string());
                continue;
            case CC_IDENTIFIER: case CC_WIDE_PREFIX:
                *macro_expansion_next++ = character;
                character = (unsigned char)*source_text_cursor++;
                while (character_flags[character] & CF_IDENTIFIER) {
                    if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
                        macro_scan_error(56);
                    *macro_expansion_next++ = character;
                    character = (unsigned char)*source_text_cursor++;
                }
                if (character_class[character] != CC_MACRO_PARAMETER) {
                    --source_text_cursor;
                    continue;
                }
                argument_mode = 1;
                /* An adjacent formal uses the concatenated-identifier form. */
            case CC_MACRO_PARAMETER:
                append_macro_text(select_macro_argument(&argument_mode));
                continue;
            }
            *macro_expansion_next++ = character;
        }
finish_argument:
        if (character_class[(unsigned char)macro_expansion_next[-1]] == CC_SPACE &&
            character_class[(unsigned char)macro_expansion_next[-2]] != CC_MACRO_END)
            --macro_expansion_next;
        if (macro_expansion_next > begin) {
            ++macro_actual_argument_count;
            save_macro_argument(begin, macro_expansion_next);
        }
        *macro_expansion_next++ = character;
        if (depth < 0) break;
    }
    macro_input_mark = saved_expansion;
    if (macro_actual_argument_count > parameter_count) compiler_warning(1, 2, interned_name);
    else if (macro_actual_argument_count < parameter_count) compiler_warning(1, 3, interned_name);
    if (definition->replacement) {
        push_macro_frame(definition);
        advance_macro_argument();
    } else {
        macro_expansion_start = macro_input_mark;
        macro_expansion_next = macro_replacement_start;
    }
}

/* Original 0x1da00..0x1da74. Each saved actual argument is a signed-short
   byte span, its text, and the zero/'A' marker that resumes argument expansion. */
void __stdcall save_macro_argument(char *begin, char *end)
{
    unsigned length = end - begin, index;
    MacroArgument *argument;
    if (macro_work_buffer + (macro_work_bytes >> 1) - 2 <= macro_expansion_start + length)
        macro_scan_error(56);
    argument = (MacroArgument *)macro_expansion_start;
    for (index = 0; index < length; ++index) argument->text[index] = begin[index];
    argument->text[length] = 0;
    argument->text[length + 1] = 'A';
    argument->bytes = (short)(length + 4);
    macro_expansion_start += length + 4;
}

/* Original 0x1dd10..0x1dd3d. Rescan each saved actual before beginning
   the macro replacement text itself. */
void advance_macro_argument(void)
{
    MacroFrame *frame = &macro_frames[macro_expansion_active];
    if (frame->argument_count > frame->argument_index) {
        ++frame->argument_index;
        activate_macro_argument((unsigned char)frame->argument_index);
    } else begin_macro_replacement();
}

/* Original 0x1dce0..0x1dd0b. Saved actuals are a span-linked byte stream
   and arguments are numbered from one. */
void __stdcall activate_macro_argument(unsigned char index)
{
    MacroArgument *argument = (MacroArgument *)macro_frames[macro_expansion_active].saved_expansion_start;
    while (--index) argument = (MacroArgument *)((char *)argument + argument->bytes);
    source_text_cursor = argument->text;
}

/* Original 0x1dd80..0x1dde9. Isolated formals use the saved expanded
   actuals; formals joined to identifiers retain their original spelling. */
char *__stdcall select_macro_argument(int *mode)
{
    MacroFrame *frame = &macro_frames[macro_expansion_active];
    MacroArgument *argument = (MacroArgument *)frame->saved_expansion_start;
    unsigned char index = (unsigned char)*source_text_cursor++;
    int following;
    if (frame->argument_count < (signed char)index) return 0;
    following = macro_identifier_follows();
    if (!*mode && !following) index += (unsigned char)macro_frames[macro_expansion_active].argument_count;
    *mode = following;
    while (--index) argument = (MacroArgument *)((char *)argument + argument->bytes);
    return argument->text;
}

/* Original 0x1dd40..0x1dd7c. Peek for an identifier character or the
   next formal marker without consuming it. */
int macro_identifier_follows(void)
{
    unsigned char character = (unsigned char)*source_text_cursor++;
    int result = character_class[character] == CC_MACRO_PARAMETER ||
        (character_flags[character] & CF_IDENTIFIER) != 0;
    --source_text_cursor;
    return result;
}

/* Original 0x1dbb0..0x1dbf6. Append a replacement without its NUL,
   preserving the five-byte reserve used by the macro scanner. */
void __stdcall append_macro_text(char *text)
{
    if (!text) return;
    while (*text) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
            macro_scan_error(56);
        *macro_expansion_next++ = *text++;
    }
}

/* Original0x1bda0..0x1bded. Quote file-name bytes without a terminating
   NUL, escaping backslashes and keeping multibyte lead/trail pairs together. */
char *__stdcall quote_macro_filename(char *destination, char *filename)
{
    unsigned char *source = (unsigned char *)filename;
    *destination++ = '"';
    while ((*destination = *source) != 0) {
        if (character_class[*source] == CC_ESCAPE_OR_END)
            *++destination = '\\';
        else if (character_class[*source] == CC_LEAD_BYTE)
            *++destination = *++source;
        ++source;
        ++destination;
    }
    *destination++ = '"';
    return destination;
}

/* Original0x1dfb0..0x1dfec. Stringification selects the original actual
   argument rather than its expanded copy in the second argument span. */
char *read_macro_string(void)
{
    signed char index = *source_text_cursor++;
    MacroFrame *frame = &macro_frames[macro_expansion_active];
    MacroArgument *argument = (MacroArgument *)frame->saved_expansion_start;
    if (index > frame->argument_count) return 0;
    while (--index) argument = (MacroArgument *)((char *)argument + argument->bytes);
    return argument->text;
}

/* Original0x1da80..0x1dba6. Stringify original actuals, preserving quoted
   substrings, escaping their backslashes and skipping macro end markers. */
void __stdcall append_macro_string(int delimiter, char *text)
{
    unsigned char quote = (unsigned char)delimiter, character, category;
    unsigned active_quote;
    int in_quote = 0;
    if (!text) return;
    *macro_expansion_next++ = quote;
    for (;;) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
            macro_scan_error(56);
        character = (unsigned char)*text;
        category = character_class[character];
        switch (category) {
        case CC_ESCAPE_OR_END:
            if (!character) {
                *macro_expansion_next++ = quote;
                return;
            }
            if (character == '\\' && in_quote) *macro_expansion_next++ = '\\';
            break;
        case CC_QUOTE:
            if (character_class[quote] == CC_QUOTE) *macro_expansion_next++ = '\\';
            /* Both quote forms delimit a substring unless producing a
               character constant, for which the original omits this state. */
        case CC_APOSTROPHE:
            if (character_class[quote] != CC_APOSTROPHE) {
                if (!in_quote) { in_quote = 1; active_quote = category; }
                else if (active_quote == category) in_quote = 0;
            }
            break;
        case CC_MACRO_END:
            text += 2;
            continue;
        case CC_LEAD_BYTE:
            ++text;
            *macro_expansion_next++ = character;
            break;
        }
        *macro_expansion_next++ = *text++;
    }
}

/* Original0x1e100..0x1e11a. Attribute an unterminated expansion to the
   saved invocation line before entering the common fatal path. */
void __stdcall macro_scan_error(unsigned code)
{
    source_line_number = macro_definition_line;
    fatal_error(code);
}
