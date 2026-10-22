/* Formatted byte output in RC's CRT, original 0x01010dd0..0x010115eb.
 * Its two adjacent switch tables occupy 0x010115ec..0x010116e4. */
#include "format.h"
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <recon.h>
#pragma intrinsic(strlen)

enum FormatState { NORMAL, PERCENT, FLAG, WIDTH, DOT, PRECISION, SIZE, TYPE };
enum FormatClass { OTHER, PCENT, POINT, STAR, ZERO, DIGIT, SIGN, MODIFIER, CONVERSION };
enum FormatFlags {
    PLUS = 1, SPACE = 2, LEFT = 4, PAD_ZERO = 8,
    LONG_VALUE = 16, SHORT_VALUE = 32, SIGNED_VALUE = 64,
    ALTERNATE = 128, NEGATIVE = 256, OCTAL_ZERO = 512, WIDE_VALUE = 2048
};
struct CountedString { short length, capacity; char *buffer; };
extern void (__cdecl *_cfltcvt_tab[])(void);

int __cdecl _output(FILE *stream, const char *format, va_list arguments)
{
    enum FormatState state = NORMAL;
    enum FormatClass kind;
    int count = 0, flags = 0, width = 0, precision = -1;
    int prefix_length = 0, length = 0, wide = 0, skip = 0;
    int capitals = 0, digit_adjust = 0, radix, padding, index, converted;
    char prefix[2], buffer[512], multibyte[2], *text = buffer, *cursor;
    char character;
    wchar_t *wide_text;
    unsigned long value, floating[2];
    struct CountedString *string;

    while ((character = *format++) != 0 && count >= 0) {
        switch (character) {
        case '%': kind = PCENT; break;
        case '.': kind = POINT; break;
        case '*': kind = STAR; break;
        case '0': kind = ZERO; break;
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': kind = DIGIT; break;
        case ' ': case '#': case '+': case '-': kind = SIGN; break;
        case 'F': case 'L': case 'N': case 'h': case 'l': case 'w': kind = MODIFIER; break;
        case 'C': case 'E': case 'G': case 'S': case 'X': case 'Z':
        case 'c': case 'd': case 'e': case 'f': case 'g': case 'i':
        case 'n': case 'o': case 'p': case 's': case 'u': case 'x':
            kind = CONVERSION; break;
        default: kind = OTHER; break;
        }
        switch (kind) {
        case PCENT: state = (state == NORMAL || state == TYPE) ? PERCENT : NORMAL; break;
        case POINT: state = state >= PERCENT && state <= WIDTH ? DOT : NORMAL; break;
        case STAR:
            state = (state == PERCENT || state == FLAG) ? WIDTH :
                    (state == DOT ? PRECISION : NORMAL);
            break;
        case ZERO:
            state = (state == PERCENT || state == FLAG) ? FLAG :
                    (state == WIDTH ? WIDTH :
                     ((state == DOT || state == PRECISION) ? PRECISION : NORMAL));
            break;
        case DIGIT:
            state = state >= PERCENT && state <= WIDTH ? WIDTH :
                    ((state == DOT || state == PRECISION) ? PRECISION : NORMAL);
            break;
        case SIGN: state = (state == PERCENT || state == FLAG) ? FLAG : NORMAL; break;
        case MODIFIER: state = state >= PERCENT && state <= SIZE ? SIZE : NORMAL; break;
        case CONVERSION: state = state >= PERCENT && state <= SIZE ? TYPE : NORMAL; break;
        default: state = NORMAL; break;
        }
        switch (state) {
        case NORMAL:
            wide = 0;
            if (_pctype[(unsigned char)character] & 0x8000) {
                write_char(character, stream, &count);
                character = *format++;
            }
            write_char(character, stream, &count);
            break;
        case PERCENT:
            capitals = skip = width = prefix_length = flags = wide = 0;
            precision = -1;
            break;
        case FLAG:
            switch (character) {
            case ' ': flags |= SPACE; break;
            case '#': flags |= ALTERNATE; break;
            case '+': flags |= PLUS; break;
            case '-': flags |= LEFT; break;
            case '0': flags |= PAD_ZERO; break;
            }
            break;
        case WIDTH:
            if (character == '*') {
                width = get_int_arg(&arguments);
                if (width < 0) {
                    flags |= LEFT;
                    width = -width;
                }
            } else
                width = width * 10 + character - '0';
            break;
        case DOT:
            precision = 0;
            break;
        case PRECISION:
            if (character == '*') {
                precision = get_int_arg(&arguments);
                if (precision < 0)
                    precision = -1;
            } else
                precision = precision * 10 + character - '0';
            break;
        case SIZE:
            if (character == 'h') flags |= SHORT_VALUE;
            if (character == 'l') flags |= LONG_VALUE;
            if (character == 'w') flags |= WIDE_VALUE;
            break;
        case TYPE:
            switch (character) {
            case 'C':
                if (!(flags & (SHORT_VALUE | LONG_VALUE | WIDE_VALUE)))
                    flags |= WIDE_VALUE;
            case 'c':
                if (flags & (LONG_VALUE | WIDE_VALUE)) {
                    length = wctomb(buffer, get_short_arg(&arguments));
                    if (length < 0)
                        skip = 1;
                } else {
                    buffer[0] = (char)get_int_arg(&arguments);
                    length = 1;
                }
                text = buffer;
                break;
            case 'E': case 'G':
                capitals = 1;
                character += 'a' - 'A';
            case 'e': case 'f': case 'g':
                flags |= SIGNED_VALUE;
                text = buffer;
                if (precision < 0)
                    precision = 6;
                else if (!precision && character == 'g')
                    precision = 1;
                floating[0] = ((unsigned long *)arguments)[0];
                floating[1] = ((unsigned long *)arguments)[1];
                arguments += 8;
                ((void (__cdecl *)(void *, char *, int, int, int))_cfltcvt_tab[0])
                    (floating, text, character, precision, capitals);
                if ((flags & ALTERNATE) && !precision)
                    ((void (__cdecl *)(char *))_cfltcvt_tab[3])(text);
                if (character == 'g' && !(flags & ALTERNATE))
                    ((void (__cdecl *)(char *))_cfltcvt_tab[1])(text);
                if (*text == '-') {
                    flags |= NEGATIVE;
                    ++text;
                }
                length = strlen(text);
                break;
            case 'Z':
                string = (struct CountedString *)get_int_arg(&arguments);
                if (string && string->buffer) {
                    text = string->buffer;
                    if (flags & WIDE_VALUE) {
                        length = (unsigned int)(int)string->length >> 1;
                        wide = 1;
                    } else {
                        length = string->length;
                        wide = 0;
                    }
                } else {
                    text = "(null)";
                    length = strlen(text);
                }
                break;
            case 'S':
                if (!(flags & (SHORT_VALUE | LONG_VALUE | WIDE_VALUE)))
                    flags |= WIDE_VALUE;
            case 's':
                index = precision == -1 ? INT_MAX : precision;
                text = (char *)get_int_arg(&arguments);
                if (flags & (LONG_VALUE | WIDE_VALUE)) {
                    if (!text)
                        text = (char *)L"(null)";
                    wide = 1;
                    wide_text = (wchar_t *)text;
                    length = 0;
                    while (length < index && *wide_text) {
                        converted = wctomb(multibyte, *wide_text);
                        if (!converted)
                            break;
                        length += converted;
                        ++wide_text;
                    }
                } else {
                    if (!text)
                        text = "(null)";
                    cursor = text;
                    while (index-- && *cursor)
                        ++cursor;
                    length = cursor - text;
                }
                break;
            case 'n':
                cursor = (char *)get_int_arg(&arguments);
                if (flags & SHORT_VALUE)
                    *(short *)cursor = (short)count;
                else
                    *(int *)cursor = count;
                skip = 1;
                break;
            case 'd': case 'i':
                flags |= SIGNED_VALUE;
                radix = 10;
                goto number;
            case 'o':
                radix = 8;
                if (flags & ALTERNATE)
                    flags |= OCTAL_ZERO;
                goto number;
            case 'p':
                precision = 8;
            case 'X':
                digit_adjust = 7;
                goto hexadecimal;
            case 'x':
                digit_adjust = 39;
hexadecimal:
                radix = 16;
                if (flags & ALTERNATE) {
                    prefix[0] = '0';
                    prefix[1] = (char)(digit_adjust + 'Q');
                    prefix_length = 2;
                }
                goto number;
            case 'u':
                radix = 10;
number:
                value = get_int_arg(&arguments);
                if (flags & SHORT_VALUE)
                    value = (flags & SIGNED_VALUE) ? (short)value : (unsigned short)value;
                if ((flags & SIGNED_VALUE) && (long)value < 0) {
                    value = -value;
                    flags |= NEGATIVE;
                }
                if (precision < 0)
                    precision = 1;
                else
                    flags &= ~PAD_ZERO;
                if (!value)
                    prefix_length = 0;
                text = buffer + sizeof(buffer) - 1;
                while (precision-- > 0 || value) {
                    index = value % radix + '0';
                    value /= radix;
                    if (index > '9')
                        index += digit_adjust;
                    *text-- = (char)index;
                }
                length = buffer + sizeof(buffer) - 1 - text;
                ++text;
                if ((flags & OCTAL_ZERO) && (!length || *text != '0')) {
                    *--text = '0';
                    ++length;
                }
                break;
            }
            if (skip)
                break;
            if (flags & SIGNED_VALUE) {
                if (flags & NEGATIVE) {
                    prefix[0] = '-';
                    prefix_length = 1;
                } else if (flags & PLUS) {
                    prefix[0] = '+';
                    prefix_length = 1;
                } else if (flags & SPACE) {
                    prefix[0] = ' ';
                    prefix_length = 1;
                }
            }
            padding = width - prefix_length - length;
            if (!(flags & (LEFT | PAD_ZERO)))
                write_multi_char(' ', padding, stream, &count);
            write_string(prefix, prefix_length, stream, &count);
            if ((flags & PAD_ZERO) && !(flags & LEFT))
                write_multi_char('0', padding, stream, &count);
            if (wide && length > 0) {
                wide_text = (wchar_t *)text;
                /* The target counts converted bytes above but iterates that
                 * count as wide elements here; retain the original behavior. */
                for (index = 0; index < length; ++index) {
                    converted = wctomb(multibyte, *wide_text++);
                    if (converted <= 0)
                        break;
                    write_string(multibyte, converted, stream, &count);
                }
            } else
                write_string(text, length, stream, &count);
            if (flags & LEFT)
                write_multi_char(' ', padding, stream, &count);
            break;
        }
    }
    return count;
}

void __cdecl write_char(int character, FILE *stream, int *count)
{
    if (putc(character, stream) == EOF)
        *count = -1;
    else
        ++*count;
}
void __cdecl write_multi_char(int character, int repetitions, FILE *stream, int *count)
{
    while (repetitions-- > 0)
        write_char(character, stream, count);
}
void __cdecl write_string(char *text, int length, FILE *stream, int *count)
{
    while (length-- > 0)
        write_char(*text++, stream, count);
}
int __cdecl get_int_arg(va_list *arguments)
{
    int value = *(int *)*arguments;
    *arguments += sizeof(int);
    return value;
}
short __cdecl get_short_arg(va_list *arguments)
{
    short value = *(short *)*arguments;
    *arguments += sizeof(int);
    return value;
}
int __cdecl wctomb(char *text, wchar_t character)
{
    __asm int 3
    return 0;
}
