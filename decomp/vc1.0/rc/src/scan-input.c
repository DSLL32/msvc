/* The older scanf engine linked into RC. Preserve its character counter,
 * byte-oriented scansets, length modifiers and separate stream helper graph. */
#include "driver.h"
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>
#include <stddef.h>
typedef char ScanFileSize[(sizeof(FILE) == 32) ? 1 : -1];
typedef char ScanFileCountOffset[(offsetof(FILE, _cnt) == 4) ? 1 : -1];
typedef char ScanFileFlagsOffset[(offsetof(FILE, _flag) == 12) ? 1 : -1];
extern char *_decimal_point;
extern void (__cdecl *_cfltcvt_tab[])(void);
int __cdecl _inc(FILE *);
void __cdecl _un_inc(int, FILE *);
int __cdecl _whiteout(int *, FILE *);
int __cdecl _hextodec(int);
#undef isspace
int __cdecl isspace(int);

#define INPUT_GET() (++characters, _inc(stream))
#define INPUT_UNGET(c) (--characters, _un_inc((c), stream))

/* Original 0x01012770..0x010131ff, switch table 0x01013200..0x01013264. */
int __cdecl _input(FILE *stream, const char *format, va_list arguments)
{
    int assignments = 0, characters = 0, character = 0;
    signed char completed = 0;
    char floating[352], multibyte[2], *number_end, *output;
    WCHAR wide_character;
    BYTE bitmap[32], previous, low, high, element;
    signed char number_size, wide_mode;
    BYTE suppress, inversion, wide_output, bad, negative;
    unsigned long value;
    unsigned int conversion;
    int width, width_present, digits, found, index;
    va_list saved_arguments;
    void *destination = 0;
    const unsigned char *set;

    while (*format) {
        if ((unsigned short)__mb_cur_max > 1 ?
            _isctype((unsigned char)*format, 8) : (_pctype[(unsigned char)*format] & 8)) {
            --characters;
            _un_inc(_whiteout(&characters, stream), stream);
            do {
                ++format;
            } while (isspace((unsigned char)*format));
        }
        if (*format != '%') {
            element = (unsigned char)*format++;
            character = INPUT_GET();
            if (element != character)
                goto mismatch;
            if (character < 256 && (_pctype[character] & 0x8000)) {
                element = (unsigned char)*format++;
                index = INPUT_GET();
                if (element != index) {
                    INPUT_UNGET(index);
                    goto mismatch;
                }
                --characters;
            }
            goto next_directive;
        }
        bad = previous = suppress = inversion = wide_output = negative = 0;
        number_size = 1;
        wide_mode = 0;
        width = width_present = digits = 0;
        value = 0;
        for (;;) {
            element = (unsigned char)*++format;
            if (isdigit(element)) {
                ++width_present;
                width = width * 10 + element - '0';
                continue;
            }
            switch (element) {
            case 'N': case 'F': continue;
            case '*': ++suppress; continue;
            case 'L': ++number_size; continue;
            case 'h': --number_size; --wide_mode; continue;
            case 'l': ++number_size; ++wide_mode; continue;
            case 'w': ++wide_mode; continue;
            }
            break;
        }
        if (!suppress) {
            saved_arguments = arguments;
            destination = va_arg(arguments, void *);
        }
        if (!wide_mode)
            wide_mode = (*format == 'C' || *format == 'S') ? 1 : -1;
        conversion = (unsigned char)*format | 0x20;
        if (conversion != 'n') {
            if (conversion == 'c' || conversion == '{')
                character = INPUT_GET();
            else
                character = _whiteout(&characters, stream);
        }
        if (width_present && !width)
            goto mismatch;
        switch (conversion) {
        case 'c':
            if (!width_present) {
                ++width_present;
                ++width;
            }
            wide_output = wide_mode > 0;
            set = (const unsigned char *)"]";
            inversion = 0xff;
            goto scan_set;
        case 's':
            wide_output = wide_mode > 0;
            set = (const unsigned char *)" \t-\r]";
            inversion = 0xff;
            goto scan_set;
        case '{':
            wide_output = wide_mode > 0;
            set = (const unsigned char *)++format;
            if (*set == '^') {
                ++set;
                inversion = 0xff;
            }
scan_set:
            for (index = 0; index != sizeof(bitmap); ++index)
                bitmap[index] = 0;
            if (conversion == '{' && *set == ']') {
                previous = ']';
                ++set;
                bitmap[']' >> 3] = 1 << (']' & 7);
            }
            while (*set != ']') {
                element = *set++;
                if (element == '-' && previous && *set != ']') {
                    element = *set++;
                    if (element > previous) {
                        low = previous;
                        high = element;
                    } else {
                        low = element;
                        high = previous;
                    }
                    do {
                        bitmap[low >> 3] |= 1 << (low & 7);
                        ++low;
                    } while (high >= low);
                    previous = 0;
                } else {
                    previous = element;
                    bitmap[element >> 3] |= 1 << (element & 7);
                }
            }
            if (!*set)
                goto finished;
            if (conversion == '{')
                format = (const char *)set;
            output = destination;
            found = 0;
            INPUT_UNGET(character);
            while (!width_present || width--) {
                character = INPUT_GET();
                if (character == EOF || !((bitmap[character >> 3] ^ inversion) & (1 << (character & 7)))) {
                    INPUT_UNGET(character);
                    break;
                }
                ++found;
                if (suppress)
                    continue;
                if (wide_output) {
                    multibyte[0] = (char)character;
                    if (character < 256 && (_pctype[character] & 0x8000))
                        multibyte[1] = (char)INPUT_GET();
                    mbtowc(&wide_character, multibyte, (unsigned short)__mb_cur_max);
                    *(WCHAR *)output = wide_character;
                    output += sizeof(WCHAR);
                } else
                    *output++ = (char)character;
            }
            if (!found)
                goto finished;
            if (!suppress) {
                ++assignments;
                if (conversion != 'c') {
                    if (wide_output)
                        *(WCHAR *)output = 0;
                    else
                        *output = 0;
                }
            }
            break;
        case 'e': case 'f': case 'g':
            number_end = floating;
            if (character == '-') {
                *number_end++ = '-';
                --width;
                character = INPUT_GET();
            } else if (character == '+') {
                --width;
                character = INPUT_GET();
            }
            if (!width_present || width > 349)
                width = 349;
            while (isdigit(character) && width--) {
                ++digits;
                *number_end++ = (char)character;
                character = INPUT_GET();
            }
            if (character == *_decimal_point && width--) {
                character = INPUT_GET();
                *number_end++ = *_decimal_point;
                while (isdigit(character) && width--) {
                    ++digits;
                    *number_end++ = (char)character;
                    character = INPUT_GET();
                }
            }
            if (digits && (character == 'e' || character == 'E') && width--) {
                *number_end++ = 'e';
                character = INPUT_GET();
                if (character == '-' || character == '+') {
                    if (character == '-')
                        *number_end++ = '-';
                    if (!width--)
                        ++width;
                    else
                        character = INPUT_GET();
                }
                while (isdigit(character) && width--) {
                    ++digits;
                    *number_end++ = (char)character;
                    character = INPUT_GET();
                }
            }
            INPUT_UNGET(character);
            if (!digits)
                goto finished;
            if (!suppress) {
                ++assignments;
                *number_end = 0;
                ((void (__cdecl *)(int, void *, char *))_cfltcvt_tab[2])(
                    number_size - 1, destination, floating);
            }
            break;
        case 'n':
            value = characters;
            goto store_count;
        case 'p':
            number_size = 1;
            /* Fall through to the ordinary signed-input loop. */
        case 'd': case 'u': case 'o':
            if (character == '-' || character == '+') {
                negative = character == '-';
                if (!--width && width_present)
                    bad = 1;
                else
                    character = INPUT_GET();
            }
            goto scan_integer;
        case 'i':
            conversion = 'd';
            /* Fall through: leading zero selects octal or hexadecimal. */
        case 'x':
            if (character == '-' || character == '+') {
                negative = character == '-';
                if (!--width && width_present)
                    bad = 1;
                else
                    character = INPUT_GET();
            }
            if (character == '0') {
                character = INPUT_GET();
                if (character == 'x' || character == 'X') {
                    character = INPUT_GET();
                    conversion = 'x';
                } else {
                    digits = 1;
                    if (conversion != 'x')
                        conversion = 'o';
                    else {
                        INPUT_UNGET(character);
                        character = '0';
                    }
                }
            }
scan_integer:
            while (!bad) {
                if (conversion == 'x' || conversion == 'p') {
                    if (isxdigit(character)) {
                        value <<= 4;
                        character = _hextodec(character);
                    } else
                        ++bad;
                } else if (isdigit(character)) {
                    if (conversion == 'o') {
                        if (character < '8')
                            value <<= 3;
                        else
                            ++bad;
                    } else
                        value *= 10;
                } else
                    ++bad;
                if (!bad) {
                    ++digits;
                    value += character - '0';
                    if (width_present && !--width)
                        ++bad;
                    else
                        character = INPUT_GET();
                } else
                    INPUT_UNGET(character);
            }
            if (negative)
                value = -value;
            if (conversion == 'F')
                digits = 0;
            if (!digits)
                goto finished;
            if (!suppress) {
                ++assignments;
store_count:
                if (!suppress) {
                    if (number_size)
                        *(unsigned long *)destination = value;
                    else
                        *(unsigned short *)destination = (unsigned short)value;
                }
            }
            break;
        default:
            if ((unsigned char)*format != character)
                goto mismatch;
            --completed;
            if (!suppress)
                arguments = saved_arguments;
            break;
        }
        ++completed;
        ++format;
next_directive:
        if (character == EOF && !(format[0] == '%' && format[1] == 'n'))
            break;
    }
finished:
    return character == EOF && !assignments && !completed ? EOF : assignments;
mismatch:
    INPUT_UNGET(character);
    goto finished;
}

/* Named helper frontiers; they retain their individual original ABIs. */
/* Original 0x010132b0..0x010132da. */
int __cdecl _inc(FILE *stream)
{
    if (--stream->_cnt >= 0)
        return (unsigned char)*stream->_ptr++;
    return _filbuf(stream);
}
/* Original 0x010132e0..0x010132f8. */
void __cdecl _un_inc(int character, FILE *stream)
{
    if (character != EOF)
        ungetc(character, stream);
}
/* Original 0x01013300..0x0101332b. */
int __cdecl _whiteout(int *count, FILE *stream)
{
    int character;
    do {
        ++*count;
        character = _inc(stream);
    } while (isspace(character));
    return character;
}
/* Original 0x01013270..0x010132b0. */
int __cdecl _hextodec(int character)
{
    return isdigit(character) ? character : (character & ~0x20) - 7;
}
/* Original 0x01013480..0x010134b2. */
int __cdecl isspace(int character)
{
    if ((unsigned short)__mb_cur_max > 1)
        return _isctype(character, 8);
    return _pctype[character] & 8;
}
/* This separate multibyte conversion vertex remains unreached. */
int __cdecl mbtowc(wchar_t *wide, const char *text, size_t length)
{
    __asm int 3
    return 0;
}
