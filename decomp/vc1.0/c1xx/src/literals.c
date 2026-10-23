/* String literal scanning and its shared lexical value. */
#include "literals-internal.h"
#include "characters.h"
#include "source-input.h"
#include "numeric-scanning.h"
#include "function-definitions.h"
#include <stdlib.h>
#include <recon.h>

unsigned char *__stdcall copy_bytes_forward(unsigned char *destination,
    const unsigned char *source, unsigned bytes);

char *__stdcall mangle_string_literal(ParserValue *literal, int wide);
IrToken *__stdcall make_string_literal_object(IrToken *token, char *name, int shared);

/* Original 0x5cbc0..0x5cc27. Reuse an existing pooled literal symbol
   when its content has a shareable name; otherwise create a fresh object. */
IrToken *__stdcall materialize_string_literal(IrToken *token)
{
    ParserValue *literal;
    char *name;
    CppSymbol *symbol;
    ++function_expression_sequence;
    literal = (ParserValue *)&token->value;
    name = mangle_string_literal(literal, (token->tail.string.auxiliary_flags & 1) ? 1 : 0);
    if (name) {
        name = intern_identifier(name);
        symbol = find_scope_symbol(global_scope, name, 0);
        if (symbol) return symbol_reference(symbol);
        return make_string_literal_object(token, name, 1);
    }
    return make_string_literal_object(token, 0, 0);
}

/* Original 0x2c530..0x2c5f9. Give the completed literal its actual
   character-array type and retain text permanently when capture requires it. */
void __stdcall record_string_literal(IrToken *token)
{
    TypeDescription *description = &type_description_stack[++type_description_depth];
    unsigned count, bytes;
    unsigned char *copy;
    description->flags = 0;
    description->attributes = 0;
    description->related = 0;
    description->field08 = description->field0a = 0;
    description->flags = (token->tail.string.auxiliary_flags & 1) ? 0x4042 : 0x41;
    description->attributes = 0;
    count = token->tail.string.length / ((token->tail.string.auxiliary_flags & 1) ? 2 : 1);
    description->related = intern_type_shape(new_type_shape(2, 1, &count));
    token->type = intern_type(description);
    --type_description_depth;
    if (token_allocation_flags & 0x20) {
        bytes = (token->tail.string.length + 3U) & ~3U;
        copy = allocate_permanent(bytes);
        copy_bytes_forward(copy, (unsigned char *)token->value.text, token->tail.string.length);
        token->value.text = (char *)copy;
    }
}

/* Original 0x250d0..0x25177. Replace the first literal's terminator
   with the next literal, preserving the destination's width and Pascal form. */
void __stdcall concatenate_strings(ParserValue *destination, ParserValue *source)
{
    int offset = destination->string.length - ((destination->string.flags & 1) ? 2 : 1);
    unsigned bytes = source->string.length;
    unsigned length = offset + bytes;
    unsigned char *out, *in;
    if ((unsigned)destination->string.text + length > (unsigned)(expression_storage + 2048)) {
        compiler_warning(1, 9);
        return;
    }
    if ((source->string.flags & 1) != (destination->string.flags & 1))
        compiler_warning(1, 14);
    destination->string.length = (unsigned short)length;
    if (destination->string.flags & 2) {
        destination->string.text[0] = 'P';
        if (destination->string.flags & 2) finish_pascal_literal(destination);
    }
    out = (unsigned char *)destination->string.text + offset;
    in = (unsigned char *)source->string.text;
    while (bytes--) *out++ = *in++;
}

/* Original 0x25900..0x25c2c. Escape values are words: narrow numeric
   escapes sign-extend their byte, while wide escapes retain all sixteen bits. */
unsigned short __stdcall decode_literal_escape(unsigned char character, int wide)
{
    unsigned value, count;
    unsigned char next;
    int octal;
restart:
    if (character_flags[character] & CF_OCTAL) {
        octal = 1;
        value = integer_digit_value(character);
        count = 1;
        next = next_source_character();
        while (count < 3 && (character_flags[next] & CF_OCTAL)) {
            value = value * 8 + integer_digit_value(next);
            ++count;
            next = next_source_character();
        }
        goto numeric_value;
    }
    switch (character) {
    case 'a': return 7;
    case 'b': return 8;
    case 'f': return 12;
    case 'n': return options.option_m ? 13 : 10;
    case 'r': return options.option_m ? 10 : 13;
    case 't': return 9;
    case 'v': return 11;
    case '\'': case '"': return character;
    case '?':
        if (!(recover_trigraph() & ~0x40U)) return character;
        character = next_source_character();
        goto restart;
    case '\\':
        if (!consume_line_escape()) return character;
        character = next_source_character();
        goto restart;
    case 'x':
        octal = 0;
        next = next_source_character();
        value = 0;
        if (!(character_flags[next] & CF_HEX)) report_syntax_error(153);
        else do {
            value = (value << 4) + integer_digit_value(next);
            next = next_source_character();
        } while (character_flags[next] & CF_HEX);
        goto numeric_value;
    }
    value = character;
    if (character_class[character] == CC_LEAD_BYTE) {
        value <<= 8;
        next = (unsigned char)*source_text_cursor++;
        while (!next) {
            recover_special_character();
            next = (unsigned char)*source_text_cursor++;
        }
        value |= next;
        compiler_warning(1, 129, value);
    } else compiler_warning(1, 129, (signed char)character);
    return (unsigned short)value;
numeric_value:
    if (!octal || !options.option_E) {
        if (!wide && (unsigned char)value != value) report_syntax_error(22, value);
        else if ((unsigned short)value != value) report_syntax_error(188, value);
        if (octal && (character_flags[next] & CF_DECIMAL)) compiler_warning(4, 125);
    }
    --source_text_cursor;
    return wide ? (unsigned short)value : (unsigned short)(signed char)value;
}

/* Original 0x24ea0..0x250cb, stdcall8. Retain raw capture, escaped
   delimiters, DBCS pairs, wide code units, Pascal prefixes and newline recovery.
   The segment encoder remains a separate original graph node. */
void __stdcall scan_string_literal(int wide, char delimiter)
{
    unsigned char *cursor = text_work_cursor, *segment, *checked, *previous;
    unsigned character, recovery, value;
    unsigned char classification;
    int odd;
    current_token_value.string.text = (char *)text_work_cursor;
    current_token_value.string.flags =
        (current_token_value.string.flags & ~1U) | (wide & 1);
    current_token_value.string.flags &= ~2U;
    for (;;) {
        segment = cursor;
        character = (unsigned char)*source_text_cursor++;
        classification = character_class[character];
        if (classification == CC_ESCAPE_OR_END) {
            recovery = recover_special_character();
            if (recovery & ~0x50U) continue;
            if ((recovery & 0x10) && !(token_allocation_flags & 0x4000) &&
                !(token_allocation_flags & 0x20000000UL)) {
                character = next_source_character();
                if (options.option_m && (character == 'p' || character == 'P') &&
                    cursor == text_work_cursor) {
                    current_token_value.string.flags |= 2;
                } else {
                    value = decode_literal_escape((unsigned char)character, wide) & 0xffff;
                    character = value;
                    if (wide && (value & 0xff) != value) {
                        cursor = check_literal_cursor(cursor);
                        *cursor++ = (unsigned char)value;
                        *cursor++ = (unsigned char)(value >> 8);
                        continue;
                    }
                }
            }
        } else if (classification == CC_NEWLINE) {
            --source_text_cursor;
            report_syntax_error(1);
            if (character_class[cursor[-1]] == CC_CARRIAGE_RETURN) --cursor;
            character = (unsigned short)(short)delimiter;
            goto possible_delimiter;
        } else if (classification == CC_APOSTROPHE || classification == CC_QUOTE) {
            goto possible_delimiter;
        } else if (classification == CC_LEAD_BYTE) {
            cursor = check_literal_cursor(cursor);
            *cursor++ = (unsigned char)character;
            character = next_source_character();
        }
store_character:
        checked = check_literal_cursor(cursor);
        *checked = (unsigned char)character;
        checked[1] = 0;
        cursor = encode_literal_segment(segment, wide, checked + 1 - segment);
        continue;
possible_delimiter:
        if ((int)delimiter != (unsigned short)character) goto store_character;
        if (!(token_allocation_flags & 0x20000000UL) || cursor[-1] != '\\')
            goto literal_end;
        previous = cursor - 1;
        odd = 0;
        if (previous >= (unsigned char *)current_token_value.string.text) {
            do {
                if (*previous != '\\') break;
                odd = !odd;
                --previous;
            } while (previous >= (unsigned char *)current_token_value.string.text);
            if (previous >= (unsigned char *)current_token_value.string.text &&
                character_class[*previous] == CC_LEAD_BYTE)
                odd = !odd;
        }
        if (odd) goto store_character;
literal_end:
        checked = check_literal_cursor(cursor);
        *checked = 0;
        cursor = encode_literal_segment(segment, wide, checked + 1 - segment);
        current_token_value.string.length = (unsigned short)
            (cursor - (unsigned char *)current_token_value.string.text);
        if (current_token_value.string.flags & 2)
            finish_pascal_literal(&current_token_value);
        text_work_cursor = cursor;
        return;
    }
}

/* Original 0x24e00..0x24e1c. Diagnose a full 2048-byte text workspace
   and restart output at its first byte, retaining this distinct check node. */
unsigned char *__stdcall check_literal_cursor(unsigned char *cursor)
{
    if (cursor >= expression_storage + 2048) {
        report_syntax_error(26);
        cursor = expression_storage;
    }
    return cursor;
}

/* Original 0x24e20..0x24e9f, stdcall12. Capture mode retains the source
   bytes. Otherwise wide segments convert one multibyte character through a
   separate aligned 16-bit temporary; even a conversion error retains the
   original two-byte copy from that temporary. */
unsigned char *__stdcall encode_literal_segment(unsigned char *segment,
                                                int wide, unsigned bytes)
{
    unsigned char *temporary;
    if ((token_allocation_flags & 0x20000000UL) || !wide)
        return segment + bytes;
    temporary = check_literal_cursor((unsigned char *)
        (((unsigned long)(segment + bytes + 1) + 3) & ~3UL));
    /* void * preserves the target CRT's 16-bit wchar destination without
       imposing a host wchar_t representation on reconstructed source. */
    if (mbtowc((void *)temporary, (char *)segment, bytes) < 0)
        report_syntax_error(2);
    *segment++ = temporary[0];
    *segment++ = temporary[1];
    return segment;
}
