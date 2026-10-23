/* Driver diagnostics and help messages. */
#include "cl.h"
#include <recon.h>

/* 00402db4..00402de5 */
void show_banner(int force)
{
    write_message(2, load_string(306), "11.00.7022");
    write_message(2, load_string(307));
}

/* The original shares one mutable 256-byte string buffer, 00405334..00405361. */
static char message_buffer[256];
char *load_string(unsigned id)
{
    if (!LoadStringA(GetModuleHandleA(NULL), id, message_buffer, sizeof(message_buffer)))
        message_buffer[0] = '\0';
    return message_buffer;
}

/* 00405069..00405092 */
int write_message(int fd, const char *text, ...)
{
    unsigned length;
    char *message;
    va_list args;
    va_start(args, text);
    message = format_message(&length, text, args);
    va_end(args);
    return _write(fd, message, length);
}

/* 00405232..00405259: the caller already supplies the variadic argument list. */
int vwrite_message(int fd, const char *text, va_list args)
{
    unsigned length;
    char *message;
    message = format_message(&length, text, args);
    return _write(fd, message, length);
}

/* 00405259..004052a5: unbuffered response output and optional command echo. */
void write_link_message(FILE *stream, const char *text, ...)
{
    unsigned length;
    char *message;
    va_list args;
    va_start(args, text);
    message = format_message(&length, text, args);
    va_end(args);
    if (stream)
        _write(stream->_file, message, length);
    if (debug_exceptions)
        _write(2, message, length);
}

/* Shared expandable output buffer, 00405092..00405232. */
unsigned message_capacity = 2048;
unsigned char *format_buffer;
char *format_message(unsigned *length, const char *text, va_list args)
{
    unsigned char *source = (unsigned char *)text, *out, *replacement;
    char *string;
    unsigned needed, string_length, used;
    if (!format_buffer)
        format_buffer = allocate(message_capacity + 128);
    out = format_buffer;
    while (*source) {
        if (!_mbsncmp(source, (unsigned char *)"%", 1)) {
            source = _mbsinc(source);
            switch (*source) {
            case 's':
                string = va_arg(args, char *);
                string_length = strlen(string);
                needed = string_length + (out - format_buffer);
                if (needed >= message_capacity) {
                    do message_capacity *= 2; while (needed >= message_capacity);
                    replacement = allocate(message_capacity + 128);
                    used = out - format_buffer;
                    memcpy(replacement, format_buffer, used);
                    out = replacement + used;
                    release(format_buffer);
                    format_buffer = replacement;
                }
                strcpy((char *)out, string);
                out += string_length;
                break;
            case 'd':
            case 'x':
                out += format_number(va_arg(args, unsigned), (char *)out,
                    _mbsncmp(source, (unsigned char *)"d", 1) ? 16 : 10);
                break;
            case 'c':
                *out = (unsigned char)va_arg(args, int);
                out = _mbsinc(out);
                break;
            default:
                _mbccpy(out, (unsigned char *)"%");
                out = _mbsinc(out);
                _mbccpy(out, source);
                out = _mbsinc(out);
                break;
            }
        } else {
            _mbccpy(out, source);
            out = _mbsinc(out);
        }
        source = _mbsinc(source);
    }
    *out = '\0';
    *length = out - format_buffer;
    if (*length >= message_capacity + 128)
        fatal(0);
    return (char *)format_buffer;
}

/* 00404fee..00405069: unsigned digits, reversed through a NUL sentinel. */
int format_number(unsigned value, char *out, unsigned base)
{
    unsigned char reverse[40], *digit = reverse + 1;
    int length = 0;
    reverse[0] = '\0';
    do {
        _mbccpy(digit, (unsigned char *)"0123456789abcdef" + value % base);
        digit = _mbsinc(digit);
        value /= base;
    } while (value);
    for (;;) {
        digit = _mbsdec(reverse, digit);
        _mbccpy((unsigned char *)out, digit);
        out = (char *)_mbsinc((unsigned char *)out);
        if (!*digit)
            break;
        ++length;
    }
    return length;
}

/* 004052a5..00405334 */
void show_help(void)
{
    int rows, id, ch;
    rows = _isatty(1) ? console_rows() - 1 : 0x7fffffff;
    if (rows < 1)
        rows = 1;
    for (id = 501; id < 573; ++id) {
        if (id != 501 && (id - 501) % rows == 0) {
            write_message(1, load_string(313));
            do { ch = getchar(); } while (ch != '\n' && ch != EOF);
        }
        write_message(1, load_string(id));
    }
    exit(0);
}

/* 00402b8a..00402bb1: the no-argument usage path exits successfully. */
void show_usage(void)
{
    show_banner(0);
    write_message(1, load_string(305));
    exit(0);
}

/* 004053ba..004053fd: command-line warning prefix, formatted body and newline. */
void warning(unsigned id, ...)
{
    va_list args;
    write_message(2, load_string(302), id);
    va_start(args, id);
    vwrite_message(2, load_string(id), args);
    va_end(args);
    write_message(2, "\n");
}

/* 00405361..004053ba: zero selects the out-of-memory diagnostic. */
void fatal(unsigned id, ...)
{
    va_list args;
    if (!id)
        id = 2000;
    write_message(2, load_string(303), id);
    va_start(args, id);
    vwrite_message(2, load_string(id), args);
    va_end(args);
    write_message(2, "\n");
    driver_exit(2);
}
