/* The frontend's own text formatter, separate from the linked CRT. */
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
