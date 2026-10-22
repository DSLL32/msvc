/* The small formatter used by line directives and preprocessor diagnostics. */
#include "preprocessor.h"
#include <stdarg.h>
#include <recon.h>
/* Original [010072a0,010073e0). Supported conversions are c, d, x and s;
 * F is an ignored far-pointer qualifier, l selects a 32-bit long argument.
 * Unsupported conversions copy the current format character literally. */
void rc_format(char *output, const char *format, ...)
{
    va_list arguments;
    int long_argument;
    long number;
    const char *string;
    va_start(arguments, format);
    while (*format) {
        if (*format != '%') {
            *output++ = *format++;
            continue;
        }
        long_argument = 0;
        while (format[1] == 'F' || format[1] == 'l') {
            if (format[1] == 'l') long_argument = 1;
            ++format;
        }
        switch (format[1]) {
        case 'c':
            *output++ = (char)va_arg(arguments, int);
            format += 2;
            break;
        case 'd':
        case 'x':
            number = long_argument ? va_arg(arguments, long) : va_arg(arguments, int);
            output += rc_number_text(number, output, format[1] == 'd' ? 10 : 16);
            format += 2;
            break;
        case 's':
            string = va_arg(arguments, const char *);
            while (*string) *output++ = *string++;
            *output = 0;
            format += 2;
            break;
        default:
            *output++ = *format++;
        }
    }
    *output = 0;
    va_end(arguments);
}

/* Original [01010ca0,01010d1f). Decimal negatives get a minus sign; other
 * radices format the unsigned two's-complement bit pattern. */
int __stdcall rc_number_text(long value, char *text, unsigned radix)
{
    char reversed[32];
    unsigned magnitude = (unsigned)value;
    unsigned used = 0;
    int length = 0;
    if (value < 0 && radix == 10) {
        *text++ = '-';
        magnitude = 0U - magnitude;
        length = 1;
    }
    do {
        reversed[used++] = "0123456789abcdef"[magnitude % radix];
        magnitude /= radix;
    } while (magnitude);
    length += used;
    while (used) *text++ = reversed[--used];
    *text = 0;
    return length;
}
