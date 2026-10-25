/* Radix, suffix and value selection for source numeric literals. */
#include "numeric-scanning.h"
#include "binary-expressions.h"
#include "source-input.h"
#include "characters.h"
#include "floating.h"
#include <recon.h>

/* Original 0x253a0..0x25585. Fractions/exponents retain a separate scanner;
   hexadecimal accumulation deliberately has no added length guard. */
unsigned char __stdcall scan_number(unsigned char character)
{
    unsigned char token = TOK_INT_CONSTANT, following;
    unsigned value;
    int radix;
    char *destination = token_text;
    if (character == '0') {
        character = read_numeric_character();
        if ((character & 0xdf) == 'X') {
            character = read_numeric_character();
            radix = 16;
            while (character_flags[character] & CF_HEX) {
                *destination = character;
                character = read_numeric_character();
                ++destination;
            }
            if (destination == token_text) report_syntax_error(153);
            goto suffix;
        }
        radix = 8;
        *destination++ = '0';
    } else radix = 10;
    while (character_flags[character] & CF_DECIMAL) {
        *destination++ = character;
        character = read_numeric_character();
        if (destination > token_text + 128) fatal_error(64);
    }
    if (character == '.' || (character & 0xdf) == 'E') {
        --source_text_cursor;
        return scan_float_tail(destination);
    }
suffix:
    character &= 0xdf;
    if (character == 'L') {
        following = read_numeric_character() & 0xdf;
        if (following == 'U') token = TOK_ULONG_CONSTANT;
        else { token = TOK_LONG_CONSTANT; --source_text_cursor; }
    } else if (character == 'U') {
        following = read_numeric_character() & 0xdf;
        if (following == 'L') token = TOK_ULONG_CONSTANT;
        else { token = TOK_UINT_CONSTANT; --source_text_cursor; }
    } else --source_text_cursor;
    following = character_class[(unsigned char)*source_text_cursor];
    if (following == CC_IDENTIFIER || following == CC_WIDE_PREFIX)
        report_syntax_error(59, "bad suffix on number");
    *destination = 0;
    value = convert_integer_digits(token_text, destination - token_text, radix);
    if (token == TOK_INT_CONSTANT)
        token = radix == 10 ? classify_decimal_integer(value) : classify_radix_integer(value);
    else if (token == TOK_LONG_CONSTANT) token = classify_long_integer(value);
    else if (token == TOK_UINT_CONSTANT) token = classify_unsigned_integer(value);
    current_token_value.expression = new_literal_constant(token, &value);
    return token;
}

/* Original 0x25590..0x25747. The suffix is retained in the numeric
   spelling for the representation converter. Macro rescans defer conversion. */
unsigned char __stdcall scan_float_tail(char *destination)
{
    unsigned char character, following, token;
    Extended *value;
    character = read_numeric_character();
    /* Original0x9186c is the -xc cell (rule0xa0198), not -Og. */
    if (options.option_xc && !macro_rescan_depth) {
        compiler_warning(1, 12);
        options.option_xc = 0;
    }
    if ((character_flags[character] & CF_DECIMAL) || character == '.') {
        do {
            *destination++ = character;
            if (destination > token_text + 128) fatal_error(64);
            character = read_numeric_character();
        } while (character_flags[character] & CF_DECIMAL);
    }
    if ((character & 0xdf) == 'E') {
        *destination++ = character;
        character = read_numeric_character();
        if (character == '+' || character == '-') {
            *destination++ = character;
            character = read_numeric_character();
        }
        if (!(character_flags[character] & CF_DECIMAL)) {
            if (!macro_rescan_depth) report_syntax_error(21, character);
            *destination++ = '0';
        } else {
            do {
                *destination = character;
                character = read_numeric_character();
                ++destination;
            } while (character_flags[character] & CF_DECIMAL);
        }
    }
    following = character & 0xdf;
    if (following == 'F') {
        token = 4;
        *destination++ = character;
    } else if (following == 'L') {
        token = 6;
        *destination++ = character;
    } else {
        --source_text_cursor;
        token = 5;
    }
    following = character_class[(unsigned char)*source_text_cursor];
    if (following == CC_IDENTIFIER || following == CC_WIDE_PREFIX)
        report_syntax_error(59, "bad suffix on number");
    *destination = 0;
    if (macro_rescan_depth > 0) {
        macro_work_midpoint = destination;
        return 147;
    }
    if (convert_floating_literal(token_text, &scanned_floating_value)) {
        report_syntax_error(177);
        scanned_floating_value = extended_zero;
    }
    value = save_extended_constant(&scanned_floating_value);
    current_token_value.expression = new_literal_constant(token, &value);
    return token;
}

/* Original 0x260e0..0x2611c, ending with a tail jump to the general
   character reader. A recovered space terminates numeric text at a sentinel. */
unsigned char read_numeric_character(void)
{
    char *cursor = source_text_cursor++;
    if (!*cursor) {
        if (recover_special_character() == 0x20) {
            ++source_text_cursor;
            return 0;
        }
        return (unsigned char)*source_text_cursor++;
    }
    --source_text_cursor;
    return next_source_character();
}

/* Original 0x25750..0x2585a. The decimal bound comparison includes its
   terminating zero, as does the native inline string comparison. */
unsigned __stdcall convert_integer_digits(char *digits, int length, int radix)
{
    unsigned value = 0;
    int digit, comparison, i;
    const char *limit = "4294967295";
    if (length > 8) {
        while (*digits == '0') { ++digits; --length; }
        if (!length) return 0;
        if (length > 8) {
            switch (radix) {
            case 8:
                if (length > 10 &&
                    !(length == 11 && (unsigned char)*digits <= '3'))
                    goto overflow;
                break;
            case 10:
                if (length == 9) break;
                if (length != 10) goto overflow;
                comparison = 0;
                for (i = 0; i < 11; ++i)
                    if ((unsigned char)digits[i] != (unsigned char)limit[i]) {
                        comparison = (unsigned char)digits[i] -
                            (unsigned char)limit[i];
                        break;
                    }
                if (comparison > 0) goto overflow;
                break;
            case 16:
                goto overflow;
            }
        }
    }
    for (;;) {
        digit = integer_digit_value((unsigned char)*digits);
        if (digit >= radix)
            report_syntax_error(41, (unsigned char)*digits, radix);
        ++digits;
        value += digit;
        if (!*digits) return value;
        value *= radix;
    }
overflow:
    report_syntax_error(177);
    return 0;
}

/* Original 0x258d0..0x258f2. Classification uses the low byte while the
   returned arithmetic retains the full incoming argument word. */
int __stdcall integer_digit_value(unsigned character)
{
    if (character_flags[(unsigned char)character] & CF_DECIMAL)
        return (int)(character - '0');
    return (int)((character & 0xdf) - ('A' - 10));
}

/* Original 0x25880..0x25895. Unsuffixed decimal values above LONG_MAX
   select unsigned long, rather than the unsigned-int radix alternative. */
unsigned char __stdcall classify_decimal_integer(unsigned value)
{
    return (value & 0x80000000UL) ? TOK_ULONG_CONSTANT : TOK_INT_CONSTANT;
}

/* Original 0x25860..0x25874. Unsuffixed octal/hexadecimal values use
   unsigned int when the target's sign bit is set. */
unsigned char __stdcall classify_radix_integer(unsigned value)
{
    return (value & 0x80000000UL) ? TOK_UINT_CONSTANT : TOK_INT_CONSTANT;
}

/* Original 0x25340..0x25398. Preserve the spelling of a deferred
   preprocessor number, including signs immediately following e/E. The
   caller has already supplied a digit or dot prefix in token_text. */
unsigned __stdcall read_number_text(char *destination)
{
    unsigned char character, kind;
    for (;;) {
        character = next_source_character();
        if (character == '.') {
            *destination++ = character;
            continue;
        }
        kind = character_class[character];
        if (kind == CC_DIGIT || kind == CC_IDENTIFIER || kind == CC_WIDE_PREFIX) {
            *destination++ = character;
            continue;
        }
        if ((character == '+' || character == '-') &&
            ((unsigned char)destination[-1] & 0xdf) == 'E') {
            *destination++ = character;
            continue;
        }
        break;
    }
    --source_text_cursor;
    *destination = 0;
    return destination - token_text;
}

/* Original0x7d53a..0x7d704, stdcall4. Prefixes and radix suffixes share the scanner
   workspace. C L/U suffixes still select the lexer token's signedness/width. */
unsigned char __stdcall scan_assembler_number(unsigned char character)
{
    unsigned char token = TOK_INT_CONSTANT, following;
    unsigned char *cursor = (unsigned char *)token_text, *end;
    unsigned suffix;
    int radix;
    if (character == '0') {
        character = read_numeric_character();
        if ((character & 0xdf) == 'X') {
            character = read_numeric_character();
            radix = 16;
            while (character_flags[character] & CF_HEX) {
                *cursor++ = character;
                character = read_numeric_character();
            }
            end = cursor;
            if (end == (unsigned char *)token_text) report_syntax_error(153);
            goto width_suffix;
        }
        radix = 8;
        *cursor++ = '0';
    } else radix = 10;
    while (character_flags[character] & CF_HEX) {
        *cursor++ = character;
        character = read_numeric_character();
    }
    suffix = cursor[-1];
    end = cursor - 1;
    if (!(character_flags[suffix] & CF_RADIX_SUFFIX) || (character & 0xdf) == 'H') {
        suffix = character;
        end = cursor;
        if (character_flags[suffix] & CF_RADIX_SUFFIX) character = read_numeric_character();
    }
    if (character_flags[suffix] & CF_RADIX_SUFFIX) {
        switch (suffix & 0xdf) {
        case 'B': radix = 2; break;
        case 'D': radix = 10; break;
        case 'H': radix = 16; break;
        case 'O': case 'Q': radix = 8; break;
        }
    }
width_suffix:
    if ((character & 0xdf) == 'L') {
        following = read_numeric_character();
        if ((following & 0xdf) == 'U') {
            token = TOK_ULONG_CONSTANT;
            goto convert;
        }
        token = TOK_LONG_CONSTANT;
    } else if ((character & 0xdf) == 'U') {
        following = read_numeric_character();
        if ((following & 0xdf) == 'L') {
            token = TOK_ULONG_CONSTANT;
            goto convert;
        }
        token = TOK_UINT_CONSTANT;
    }
    --source_text_cursor;
convert:
    *end = 0;
    if (radix == 2) {
        if (end - (unsigned char *)token_text < 33)
            current_token_value.words.low = parse_asm_binary_digits(token_text);
        else report_syntax_error(177);
    } else current_token_value.words.low =
        convert_integer_digits(token_text,end - (unsigned char *)token_text,radix);
    return token;
}
