#include "c2.h"
#include <recon.h>

extern int c2_errors;

void c2_fatal(int message, ...)
{
    va_list arguments;
    char *format;
    _fcloseall();
    format = lookup_message(message + 1000, 0);
    va_start(arguments, message);
    emit_diagnostic(1000, message, format, arguments);
    va_end(arguments);
    exit(++c2_errors);
}

char message_buffer[384];

char *__fastcall lookup_message(unsigned message, int allow_missing)
{
    initialize_messages();
    return load_message(message, message_buffer, sizeof(message_buffer), allow_missing);
}

MessageEntry messages[7] = {
    { 301, NULL }, { 302, NULL }, { 303, NULL },
    { 310, NULL }, { 311, NULL }, { 312, NULL }, { 0, NULL }
};
int messages_initialized;

void initialize_messages(void)
{
    MessageEntry *entry;
    char *text;
    char *(__cdecl *copy)(const char *);
    if (!messages_initialized) {
        messages_initialized = 1;
        if (messages[0].id) {
            copy = _strdup;
            for (entry = messages; entry->id; ++entry) {
                text = lookup_message(entry->id, 1);
                if (text)
                    entry->text = copy(text);
                else if (entry->id == 301)
                    entry->text = "warning";
                else if (entry->id == 302)
                    entry->text = "error";
                else if (entry->id == 303)
                    entry->text = "fatal error";
            }
        }
    }
}

char *__fastcall load_message(unsigned message, char *buffer, int capacity, int allow_missing)
{
    if (LoadStringA(GetModuleHandleA(NULL), message, buffer, capacity))
        return unescape_message(buffer);
    return NULL;
}

char *__fastcall unescape_message(char *buffer)
{
    char *read = buffer, *write = buffer;
    while (*read) {
        if (*read == '\\') {
            ++read;
            if (*read == 'n')
                *read = '\n';
            else if (*read == 't')
                *read = '\t';
            else
                *write++ = '\\';
        }
        *write++ = *read++;
    }
    *write = '\0';
    return buffer;
}

int diagnostic_location_enabled;
unsigned diagnostic_position, diagnostic_token_position;
char *diagnostic_file;
extern int c2_dos_messages;

void emit_diagnostic(int category, int message, char *format, va_list arguments)
{
    char text[2304], *output = text, *file, *kind;
    unsigned line = diagnostic_position;
    size_t (__cdecl *write)(const void *, size_t, size_t, FILE *);
    if (diagnostic_location_enabled) {
        if (!line)
            line = diagnostic_token_position + 1;
        line = resolve_diagnostic_location(line, &file);
    } else
        file = diagnostic_file;
    if (file && *file) {
        format_diagnostic(output, "%s(%ld) : ", file, line);
        output += strlen(output);
    }
    if (category) {
        switch (category) {
        case 1000: kind = c2_get_diagnostic_string(303); break;
        case 2000: kind = c2_get_diagnostic_string(302); break;
        case 4000: kind = c2_get_diagnostic_string(301); break;
        default: return;
        }
        strcpy(output, kind);
        output += strlen(kind);
        format_diagnostic(output, " %c%d: ", c2_dos_messages ? 'F' : 'C', category + message);
        output += strlen(output);
    }
    format_diagnostic_args(output, format, arguments);
    write = fwrite;
    write(text, strlen(text), 1, stderr);
    write("\n", 1, 1, stderr);
}

char *__fastcall c2_get_diagnostic_string(int message)
{
    char *text;
    initialize_messages();
    text = find_cached_message(message);
    return text ? text : "";
}

char *__fastcall find_cached_message(int message)
{
    MessageEntry *entry = messages;
    while (entry->id && entry->id != (unsigned)message)
        ++entry;
    return entry->text;
}

void format_diagnostic(char *buffer, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    format_diagnostic_args(buffer, format, arguments);
    va_end(arguments);
}

void format_diagnostic_args(char *buffer, const char *format, va_list arguments)
{
    char *output = buffer, *text;
    int long_argument, value;
    while (*format) {
        if (*format != '%') {
            *output++ = *format++;
            continue;
        }
        long_argument = 0;
    conversion:
        switch (format[1]) {
        case 'F':
            ++format;
            goto conversion;
        case 'l':
            long_argument = 1;
            ++format;
            goto conversion;
        case 's':
            text = va_arg(arguments, char *);
            if (*text) {
                strcpy(output, text);
                output += strlen(text);
            }
            format += 2;
            break;
        case 'c':
            *output++ = (char)va_arg(arguments, int);
            format += 2;
            break;
        case 'd':
        case 'x':
            value = long_argument ? (int)va_arg(arguments, long) : va_arg(arguments, int);
            output += format_integer(value, output, format[1] == 'd' ? 10 : 16);
            format += 2;
            break;
        default:
            *output++ = *format++;
            break;
        }
    }
    *output = '\0';
}

int __fastcall format_integer(int value, char *buffer, unsigned base)
{
    char digits[32], *next = digits;
    unsigned number = (unsigned)value;
    int length = 0;
    *next++ = '\0';
    if (value < 0 && base == 10) {
        *buffer++ = '-';
        number = 0 - number;
        length = 1;
    }
    do {
        *next++ = "0123456789abcdef"[number % base];
        number /= base;
    } while (number);
    while ((*buffer++ = *--next) != '\0')
        ++length;
    return length;
}
