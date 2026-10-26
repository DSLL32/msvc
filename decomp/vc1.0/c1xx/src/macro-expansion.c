/* Macro substitution and rescan driver, retaining its separate helper graph. */
#include "macro-expansion.h"
#include "source-input.h"
#include "characters.h"
#include "text-format.h"
#include "function-definitions.h"
#include "keywords.h"
#include <recon.h>

int __stdcall get_file_timestamp(int handle, long *timestamp);

/* Original0x1e3a0..0x1e3bb, cdecl. The original ignores query failure
   and returns the then-unwritten local; retain that defect in both builds. */
long current_source_timestamp(void)
{
    long timestamp;
    get_file_timestamp(source_handle, &timestamp);
    return timestamp;
}

MacroExpansionFrame macro_expansion_frames[256]; /* original0x9578c */
char *macro_argument_start; /* original0x9576c */
int macro_argument_state; /* original0x95770 */
int macro_intertoken_space; /* original0x95774 */
unsigned macro_expansion_line; /* original0x95778 */
char macro_file_timestamp[26]; /* original0x918c8 */

/* Original 0x1ffa0..0x20085, stdcall12. Copy a quoted macro
   token, preserving source recovery, escaped bytes and lead-byte state. */
char *__stdcall copy_macro_literal(char *cursor, char *limit, unsigned char delimiter)
{
    unsigned char character, classification;
    unsigned flags;
    *cursor++ = delimiter;
    while (cursor < limit) {
        character = (unsigned char)*source_text_cursor++;
        classification = character_class[character];
        switch (classification) {
        case CC_ESCAPE_OR_END:
            flags = recover_special_character();
            if (!(flags & 0x50)) continue;
            if (flags & 0x10) {
                *cursor++ = source_text_cursor[-1];
                *cursor++ = next_source_character();
                continue;
            }
            break;
        case CC_NEWLINE:
            report_syntax_error(1);
            --source_text_cursor;
            character = delimiter;
        case CC_APOSTROPHE: case CC_QUOTE:
            if (character == delimiter) {
                *cursor++ = character;
                return cursor;
            }
            break;
        case CC_LEAD_BYTE:
            *cursor++ = character;
            cpp_parser_flags |= 0x02000000UL;
            character = next_source_character();
            cpp_parser_flags &= ~0x02000000UL;
            break;
        }
        *cursor++ = character;
    }
    return limit - 1;
}

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

/* Original 0x21040..0x2116a. Whitespace can terminate an argument scan,
   resume a source window, or append one separator to the expansion buffer. */
int skip_macro_space(void)
{
    int available = 0;
    unsigned status;
    unsigned char classification;
    token_allocation_flags &= ~0x00800000UL;
    macro_intertoken_space = 0;
    for (;;) {
        classification = character_class[(unsigned char)*source_text_cursor++];
        if (classification == CC_SPACE || classification == CC_CARRIAGE_RETURN) {
            macro_intertoken_space = 1;
            continue;
        }
        if (classification == CC_ESCAPE_OR_END) {
            status = recover_special_character();
            if (status & 0x0a) break;
            if (status & ~0x50U) continue;
        } else if (classification == CC_NEWLINE) {
            if (token_allocation_flags & 0x00010000UL) {
                --source_text_cursor;
                break;
            }
            ++source_line_number;
            if (options.option_E) {
                if (preprocessor_stream)
                    fputc((unsigned char)source_text_cursor[-1], preprocessor_stream);
                else write_work_character((unsigned char)source_text_cursor[-1], compiler_stack);
            }
            token_allocation_flags |= 0x00800000UL;
            macro_intertoken_space = 1;
            continue;
        }
        --source_text_cursor;
        available = 1;
        break;
    }
    if (macro_intertoken_space) {
        if ((unsigned)(macro_work_buffer + macro_work_bytes - 5) <=
            (unsigned)macro_work_midpoint) macro_expansion_error(56);
        if (macro_work_midpoint[-1] != ' ') *macro_work_midpoint++ = ' ';
    }
    return available;
}

/* Original 0x20c60..0x20cdd. Frame zero is reserved; each nested
   expansion saves its real source/argument cursors and the replacement mark. */
void __stdcall push_macro_expansion(MacroDefinition *definition)
{
    MacroExpansionFrame *frame;
    if (++macro_expansion_active >= 256) fatal_error(9);
    ++macro_rescan_depth;
    frame = &macro_expansion_frames[macro_expansion_active];
    frame->definition = definition;
    frame->argument_start = macro_argument_start;
    frame->argument_state = (unsigned char)macro_argument_state;
    frame->flags = 0;
    frame->source_cursor = source_text_cursor;
    macro_work_midpoint = macro_replacement_start;
    frame->replacement_start = macro_work_midpoint;
}

/* Original 0x20ce0..0x20d2c. Without extensions, counted formal-name
   prefixes precede the replacement body and must be skipped first. */
void rescan_macro_replacement(void)
{
    MacroDefinition *definition = macro_expansion_frames[macro_expansion_active].definition;
    int remaining;
    source_text_cursor = definition->replacement;
    if (!options.configuration.extensions_enabled) {
        remaining = definition->parameter_count;
        while (remaining-- > 0)
            source_text_cursor += (unsigned char)*source_text_cursor;
    }
    ++definition->flags;
}

/* Original 0x209c0..0x20a57. The outermost replacement remains
   active until its emitted R sentinel restores the saved source cursor. */
int finish_macro_expansion(void)
{
    MacroExpansionFrame *frame;
    if (!--macro_rescan_depth) {
        if ((unsigned)(macro_work_buffer + macro_work_bytes - 5) <=
            (unsigned)macro_work_midpoint) macro_expansion_error(56);
        *macro_work_midpoint++ = 0;
        *macro_work_midpoint++ = 'R';
        source_text_cursor = macro_expansion_frames[macro_expansion_active].replacement_start;
        browser_macro_end_suppressed = 1;
        return 1;
    }
    frame = &macro_expansion_frames[macro_expansion_active];
    source_text_cursor = frame->source_cursor;
    macro_expansion_start = frame->argument_start;
    --frame->definition->flags;
    --macro_expansion_active;
    return 0;
}

/* Original 0x205d0..0x20900; class dispatch 0x20904..0x209bb.
   The shared C1 frontend algorithm is retained after checking this whole
   C1XX body and its real buffer/frame aliases. Collect actual arguments
   before activating their replacement/rescan frame. */
void __stdcall expand_macro_arguments(MacroDefinition *definition, int parameter_count)
{
    char *saved_expansion = macro_expansion_start, *begin, *saved_output, *saved_start;
    int depth = 0, argument_mode;
    unsigned flags;
    unsigned char character, classification;
    *macro_work_midpoint++ = source_text_cursor[-1];
    for (;;) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_work_midpoint)
            macro_expansion_error(56);
        for (;;) {
            if (!skip_macro_space()) return;
            if (character_class[(unsigned char)*source_text_cursor] != CC_SLASH) {
                begin = macro_work_midpoint;
                break;
            }
            ++source_text_cursor;
            if (skip_source_comment()) continue;
            begin = macro_work_midpoint;
            *macro_work_midpoint++ = '/';
            break;
        }
        argument_mode = 0;
        for (;;) {
            character = (unsigned char)*source_text_cursor++;
            classification = character_class[character];
            switch (classification) {
            case CC_SPACE: case CC_CARRIAGE_RETURN: case CC_NEWLINE:
                --source_text_cursor;
                if (!skip_macro_space()) return;
                continue;
            case CC_SLASH:
                if (skip_source_comment()) {
                    if (macro_work_midpoint[-1] == ' ') continue;
                    character = ' ';
                }
                break;
            case CC_ESCAPE_OR_END:
                saved_output = macro_work_midpoint;
                saved_start = macro_expansion_start;
                flags = recover_special_character();
                if (flags & 0xa) return;
                macro_expansion_start = saved_start;
                macro_work_midpoint = saved_output;
                if (!(flags & 0x50)) continue;
                break;
            case CC_APOSTROPHE: case CC_QUOTE:
                macro_work_midpoint = copy_macro_literal(macro_work_midpoint,
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
                emit_macro_string('"', read_macro_parameter());
                continue;
            case CC_MACRO_TOKEN:
                emit_macro_string('\'', read_macro_parameter());
                continue;
            case CC_IDENTIFIER: case CC_WIDE_PREFIX:
                *macro_work_midpoint++ = character;
                character = (unsigned char)*source_text_cursor++;
                while (character_flags[character] & CF_IDENTIFIER) {
                    if (macro_work_buffer + macro_work_bytes - 5 <= macro_work_midpoint)
                        macro_expansion_error(56);
                    *macro_work_midpoint++ = character;
                    character = (unsigned char)*source_text_cursor++;
                }
                if (character_class[character] != CC_MACRO_PARAMETER) {
                    --source_text_cursor;
                    continue;
                }
                argument_mode = 1;
                /* An adjacent formal uses the concatenated-identifier form. */
            case CC_MACRO_PARAMETER:
                emit_macro_text(select_macro_argument(&argument_mode));
                continue;
            }
            *macro_work_midpoint++ = character;
        }
finish_argument:
        if (character_class[(unsigned char)macro_work_midpoint[-1]] == CC_SPACE &&
            character_class[(unsigned char)macro_work_midpoint[-2]] != CC_MACRO_END)
            --macro_work_midpoint;
        if (macro_work_midpoint > begin) {
            ++macro_argument_state;
            save_macro_argument_text(begin, macro_work_midpoint);
        }
        *macro_work_midpoint++ = character;
        if (depth < 0) break;
    }
    macro_argument_start = saved_expansion;
    if (macro_argument_state > parameter_count) compiler_warning(1, 2, interned_name);
    else if (macro_argument_state < parameter_count) compiler_warning(1, 3, interned_name);
    if (definition->replacement) {
        push_macro_expansion(definition);
        advance_macro_argument();
    } else {
        macro_expansion_start = macro_argument_start;
        macro_work_midpoint = macro_replacement_start;
    }
}


/* Actual arguments are span-linked records in the first buffer half.
   Their variable text starts immediately after the signed WORD span. */
typedef struct MacroArgument {
    short bytes;
    char text[1];
} MacroArgument;
typedef char MacroArgumentTextOffsetMustBe2[
    offsetof(MacroArgument, text) == 2 ? 1 : -1];

/* Original 0x21000..0x2103c. A replacement byte selects the
   one-based argument record; the frame count comparison uses signed bytes. */
char *read_macro_parameter(void)
{
    MacroExpansionFrame *frame = &macro_expansion_frames[macro_expansion_active];
    MacroArgument *argument = (MacroArgument *)frame->argument_start;
    unsigned char index = (unsigned char)*source_text_cursor++;
    if ((signed char)frame->argument_state < (signed char)index) return 0;
    while (--index) argument = (MacroArgument *)((char *)argument + argument->bytes);
    return argument->text;
}


/* Original 0x20a60..0x20ad3. Copy from the separate work half, then
   append the NUL/A rescan marker and advance the argument-storage cursor. */
void __stdcall save_macro_argument_text(char *start, char *end)
{
    unsigned length = end - start, index;
    MacroArgument *argument;
    if ((unsigned)(macro_work_buffer + (macro_work_bytes >> 1) - 2) <=
        (unsigned)(macro_expansion_start + length)) macro_expansion_error(56);
    argument = (MacroArgument *)macro_expansion_start;
    for (index = 0; index < length; ++index) argument->text[index] = start[index];
    argument->text[length] = 0;
    argument->text[length + 1] = 'A';
    argument->bytes = (short)(length + 4);
    macro_expansion_start += length + 4;
}

/* Original 0x20d60..0x20d8c. The frame's count/index bytes compare
   signed, while the selected actual is passed as a byte index. */
void advance_macro_argument(void)
{
    MacroExpansionFrame *frame = &macro_expansion_frames[macro_expansion_active];
    if ((signed char)frame->argument_state > (signed char)frame->flags) {
        ++frame->flags;
        activate_macro_argument(frame->flags);
    } else rescan_macro_replacement();
}

/* Original 0x20d30..0x20d5a. Actuals are numbered from one and
   traversed by their signed WORD spans. */
void __stdcall activate_macro_argument(unsigned char index)
{
    MacroArgument *argument = (MacroArgument *)
        macro_expansion_frames[macro_expansion_active].argument_start;
    while (--index) argument = (MacroArgument *)((char *)argument + argument->bytes);
    source_text_cursor = argument->text;
}

/* Original 0x20e40..0x20ffb. Assemble identifier text across formal
   substitutions, preserving any counted marker that prevents re-expansion. */
int __stdcall scan_macro_name(unsigned char first)
{
    char *begin = macro_work_midpoint, *argument;
    int valid = 1, argument_mode = 0, disabled_length = -1;
    unsigned char character = first;
    for (;;) {
        if (character_class[character] == CC_MACRO_PARAMETER) {
            argument = select_macro_argument(&argument_mode);
            if (argument) {
                while (*argument) {
                    if (!(character_flags[(unsigned char)*argument] & CF_IDENTIFIER)) valid = 0;
                    if (macro_work_buffer + macro_work_bytes - 5 <= macro_work_midpoint)
                        macro_expansion_error(56);
                    *macro_work_midpoint++ = *argument++;
                }
            }
        } else *macro_work_midpoint++ = character;
        for (;;) {
            character = (unsigned char)*source_text_cursor++;
            while (character_flags[character] & CF_IDENTIFIER) {
                if (macro_work_buffer + macro_work_bytes - 5 <= macro_work_midpoint)
                    macro_expansion_error(56);
                *macro_work_midpoint++ = character;
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
        if (disabled_length != -1 && macro_work_midpoint - begin == disabled_length) {
            valid = 0;
            *macro_work_midpoint++ = 4;
            *macro_work_midpoint++ = (unsigned char)disabled_length;
            *macro_work_midpoint = 0;
        }
        if (valid) {
            *macro_work_midpoint = 0;
            interned_name = intern_identifier(begin);
            return 1;
        }
    }
    return valid;
}

/* Original 0x20dd0..0x20e38. Joined formals use their original text;
   isolated formals select the expanded copy following the original spans. */
char *__stdcall select_macro_argument(int *mode)
{
    MacroExpansionFrame *frame = &macro_expansion_frames[macro_expansion_active];
    MacroArgument *argument = (MacroArgument *)frame->argument_start;
    unsigned char index = (unsigned char)*source_text_cursor++;
    int following;
    if ((signed char)frame->argument_state < (signed char)index) return 0;
    following = macro_identifier_follows();
    if (!*mode && !following)
        index += macro_expansion_frames[macro_expansion_active].argument_state;
    *mode = following;
    while (--index) argument = (MacroArgument *)((char *)argument + argument->bytes);
    return argument->text;
}

/* Original 0x20d90..0x20dcb. Peek without consuming the next byte. */
int macro_identifier_follows(void)
{
    unsigned char character = (unsigned char)*source_text_cursor++;
    int result = character_class[character] == CC_MACRO_PARAMETER ||
        (character_flags[character] & CF_IDENTIFIER) != 0;
    --source_text_cursor;
    return result;
}

/* Original 0x20ae0..0x20c06, stdcall8. Stringification removes
   inhibition markers and escapes quote/backslash bytes inside literal tokens.
   The original early load of its unwritten quote-class local is discarded;
   every consumed comparison is preceded by the opening-quote assignment. */
void __stdcall emit_macro_string(unsigned char delimiter, char *text)
{
    unsigned char character;
    unsigned classification, literal_class;
    int quoted = 0;
    if (!text) return;
    *macro_work_midpoint++ = delimiter;
    for (;;) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_work_midpoint)
            macro_expansion_error(56);
        character = (unsigned char)*text;
        classification = character_class[character];
        switch (classification) {
        case CC_ESCAPE_OR_END:
            if (!character) goto finish_string;
            if (character == '\\' && quoted) *macro_work_midpoint++ = '\\';
            break;
        case CC_QUOTE:
            if (character_class[delimiter] == CC_QUOTE) *macro_work_midpoint++ = '\\';
            /* The two quote classes share the literal-state update. */
        case CC_APOSTROPHE:
            if (character_class[delimiter] == CC_APOSTROPHE) break;
            if (quoted) {
                if (literal_class == classification) quoted = 0;
            } else {
                quoted = 1;
                literal_class = classification;
            }
            break;
        case CC_MACRO_END:
            text += 2;
            continue;
        case CC_LEAD_BYTE:
            *macro_work_midpoint++ = *text++;
            break;
        }
        *macro_work_midpoint++ = *text++;
    }
finish_string:
    *macro_work_midpoint++ = delimiter;
}

/* Original 0x20c10..0x20c55. Append replacement bytes without their
   terminating NUL, keeping the scanner's five-byte reserve. */
void __stdcall emit_macro_text(char *text)
{
    if (!text) return;
    while (*text) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_work_midpoint)
            macro_expansion_error(56);
        *macro_work_midpoint++ = *text++;
    }
}

/* Original 0x21170..0x2118a, stdcall4. Diagnose a failed expansion at
   its invocation line, rather than the source cursor's later position. */
void __stdcall macro_expansion_error(unsigned code)
{
    source_line_number = macro_expansion_line;
    fatal_error(code);
}
