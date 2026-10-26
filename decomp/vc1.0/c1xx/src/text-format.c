/* The frontend's own text formatter, separate from the linked CRT. */
#include <io.h>
#include "text-format.h"
#include "mangling.h"
#include <recon.h>

/* Original 0x736d0..0x736e7. Retain the variadic wrapper and its
   separate argument-list formatter, rather than substituting CRT sprintf. */
void format_text(char *destination, char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vformat_text(destination, format, arguments);
    va_end(arguments);
}

/* Original 0x736f0..0x7383a. This small formatter recognizes only c/d/x/s
   and the F/l modifiers. Both integer widths occupy one 32-bit argument in
   this target, so the two original l/non-l paths perform the same operation.
   Unknown conversions copy the current percent/modifier byte literally. */
void vformat_text(char *destination, char *format, va_list arguments)
{
    char *source, *copy, *target;
    unsigned length, bytes, words, radix;
    while (*format) {
        if (*format != '%') {
            *destination++ = *format++;
            continue;
        }
        while (format[1] == 'F' || format[1] == 'l') ++format;
        switch (format[1]) {
        case 'c':
            *destination++ = (char)va_arg(arguments, int);
            format += 2;
            break;
        case 'd':
        case 'x':
            radix = format[1] == 'd' ? 10 : 16;
            format += 2;
            destination += format_integer(va_arg(arguments, unsigned), destination, radix);
            break;
        case 's':
            source = va_arg(arguments, char *);
            if (source) {
                length = 0;
                while (source[length]) ++length;
                bytes = length + 1;
                copy = source;
                target = destination;
                /* Include the NUL and preserve the original forward word
                   copy before trailing bytes, including overlapping ranges. */
                for (words = bytes / 4; words; --words) {
                    *(unsigned *)target = *(unsigned *)copy;
                    target += 4;
                    copy += 4;
                }
                for (bytes &= 3; bytes; --bytes) *target++ = *copy++;
                destination += length;
            }
            format += 2;
            break;
        default:
            *destination++ = *format++;
            break;
        }
    }
    *destination = 0;
}

/* Original 0x74380..0x743f8. Build reversed lowercase digits after a
   zero sentinel, then copy through that sentinel. Decimal treats the 32-bit
   input as signed; its magnitude still uses unsigned negation for INT_MIN. */
int format_integer(unsigned value, char *cursor, unsigned radix)
{
    char digits[32], character;
    unsigned used = 1;
    int length = 0;
    digits[0] = 0;
    if ((int)value < 0 && radix == 10) {
        value = 0U - value;
        *cursor++ = '-';
        length = 1;
    }
    do {
        digits[used++] = "0123456789abcdef"[value % radix];
        value /= radix;
    } while (value);
    do {
        character = digits[--used];
        *cursor++ = character;
        if (character) ++length;
    } while (character);
    return length;
}

/* Original0x73680..0x736cc, cdecl variadic. The frontend's own
   formatter feeds descriptor2 directly rather than the CRT FILE buffers. */
void write_ide_progress(char *format, ...)
{
    char buffer[512];
    unsigned length;
    va_list arguments;
    va_start(arguments, format);
    vformat_text(buffer, format, arguments);
    va_end(arguments);
    for (length = 0; buffer[length]; ++length) {}
    _write(2, buffer, length);
}
