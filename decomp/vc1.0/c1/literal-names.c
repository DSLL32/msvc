/* Encoded names used for pooled string literal symbols. */
#include <ctype.h>
#include "literal-names.h"
#include "global-ir.h"
#include "parser.h"

/* Original 0x2ddf0..0x2de40. Name formatting shares the symbol-name
   workspace; the bound applies before adding its terminating NUL. */
char *__stdcall string_pool_name(StringToken *string, int wide)
{
    char *end = encode_string_pool_name(symbol_name_buffer, string->text, string->bytes, wide);
    if (!end) return 0;
    if (end - symbol_name_buffer > 247) {
        fatal_error(66);
        end = symbol_name_buffer + 247;
    }
    *end = '\0';
    return symbol_name_buffer;
}

/* Original 0x2dec0..0x2dfb4. The external spelling includes a length,
   checksum and bounded literal prefix. Wide characters emit high byte first. */
char *__stdcall encode_string_pool_name(char *destination, unsigned char *text,
    unsigned bytes, int wide)
{
    unsigned count;
    unsigned short character;
    if (!options.option_Gf &&
        (!function_symbol || !(function_symbol->storage[1] & 1) ||
         (function_symbol->storage[0] & 7) == 3)) return 0;
    *destination++ = '?';
    *destination++ = '?';
    *destination++ = '_';
    *destination++ = 'C';
    *destination++ = '@';
    *destination++ = '_';
    *destination++ = wide ? '1' : '0';
    destination = encode_name_number(destination, bytes);
    destination = encode_name_number(destination, checksum_bytes(text, bytes, 0));
    if (wide) {
        count = bytes > 20 ? 20 : bytes;
        while (count--) {
            character = *(unsigned short *)text;
            text += 2;
            destination = encode_name_character(destination, (char)(character >> 8));
            destination = encode_name_character(destination, (char)character);
        }
    } else {
        count = bytes > 32 ? 32 : bytes;
        while (count--) destination = encode_name_character(destination, *text++);
    }
    *destination++ = '@';
    return destination;
}

/* Original 0x2de40..0x2debe. Small values have one-character spellings;
   other values use hexadecimal digits A..P followed by '@'. */
char *__stdcall encode_name_number(char *destination, unsigned value)
{
    unsigned mask = 0xf0000000;
    int shift = 28;
    if (!value) {
        *destination++ = 'A';
        *destination++ = '@';
        return destination;
    }
    if (value <= 10) {
        *destination++ = (char)(value + '0' - 1);
        return destination;
    }
    while (!(value & mask) && shift) {
        shift -= 4;
        mask >>= 4;
    }
    while (shift) {
        *destination++ = (char)(((value & mask) >> shift) + 'A');
        shift -= 4;
        mask >>= 4;
    }
    *destination++ = (char)((value & 15) + 'A');
    *destination++ = '@';
    return destination;
}

/* Original 0x2dfc0..0x2e159. Retain the CRT's locale-sensitive character
   classes and the target's signed high-nibble escape for other bytes. */
char *__stdcall encode_name_character(char *destination, char value)
{
    unsigned char letter;
    if (isalnum(value) || value == '_' || value == '$') {
        *destination++ = value;
        return destination;
    }
    *destination++ = '?';
    letter = value & 0x7f;
    if (isalpha(letter)) {
        *destination++ = letter;
        return destination;
    }
    switch (value) {
    case ',': *destination++ = '0'; break;
    case '/': *destination++ = '1'; break;
    case '\\': *destination++ = '2'; break;
    case ':': *destination++ = '3'; break;
    case '.': *destination++ = '4'; break;
    case ' ': *destination++ = '5'; break;
    case '\n': *destination++ = '6'; break;
    case '\t': *destination++ = '7'; break;
    case '\'': *destination++ = '8'; break;
    case '-': *destination++ = '9'; break;
    default:
        *destination++ = '$';
        *destination++ = (char)((value >> 4) + 'A');
        *destination++ = (char)((value & 15) + 'A');
        break;
    }
    return destination;
}
