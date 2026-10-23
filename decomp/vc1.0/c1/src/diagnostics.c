/* Diagnostic vocabulary used by message_text, original table 0x6f4b8. */
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include "diagnostics.h"
#include "pragma.h"
#include "source-input.h"
#include "global-ir.h"
#include <recon.h>

typedef struct MessageText {
    unsigned code;
    char *text;
} MessageText;

static MessageText message_vocabulary[] = {
    { 301, "left" },
    { 302, "right" },
    { 303, "UNNAMED" },
    { 304, "<unnamed>" },
    { 305, "trailing ',' used for variable argument list" },
    { 313, "warning" },
    { 314, "error" },
    { 315, "fatal error" },
    { 322, "operand" },
    { 323, "opcode" },
    { 333, "anonymous union" },
    { 336, "private" },
    { 337, "protected" },
    { 339, "compiler intermediate" },
    { 340, "compiler generated" },
    { 341, "source" },
    { 342, "inline function definition" },
    { 343, "include" },
    { 344, "precompiled header" },
    { 345, "overloaded function type" },
    { 350, "Using precompiled header" },
    { 351, "Creating precompiled header" },
    { 352, "Creating new precompiled header" },
    { 0, 0 }
};

/* Original 0x4c350..0x4c372. */
char *message_text(unsigned code)
{
    MessageText *entry = message_vocabulary;
    while (entry->code && entry->code != code)
        ++entry;
    return entry->text;
}

/* Original 0x4ac00..0x4acee. Per-message settings can override the supplied
   level, promote a warning to an error, or disable it after its first use. */
void compiler_warning(unsigned level, unsigned code, ...)
{
    int setting = warning_setting(code);
    char *format;
    va_list arguments;
    if (setting == 14) {
        format = diagnostic_format(code + 4000);
        va_start(arguments, code);
        emit_diagnostic(2000, code + 2000, format, arguments);
        va_end(arguments);
        if (!error_count) ++error_count;
        return;
    }
    switch (setting) {
    case 0: case 5: break;
    case 1: case 2: case 3: case 4: level = setting; break;
    default: return;
    }
    if ((int)level > options.option_W) return;
    format = diagnostic_format(code + 4000);
    va_start(arguments, code);
    emit_diagnostic(4000, code, format, arguments);
    va_end(arguments);
    if (options.option_WX && !error_count && options.option_WX == 1) {
        ++options.option_WX;
        report_syntax_error(220);
        --error_count;
    }
    if (setting == 5) set_warning_level(code, 13);
}

/* Original 0x17000..0x1701f. Preprocess-only mode suppresses warnings;
   otherwise the signed per-diagnostic setting supplies the override. */
int __stdcall warning_setting(unsigned code)
{
    if (options.option_E) return 13;
    return (signed char)warning_levels[code];
}

FILE *diagnostic_message_file; /* original 0x5b224 */
char diagnostic_message_buffer[256]; /* original 0x6e248..0x6e348 */

/* Original 0x4a6e0..0x4a763. Open the external message catalog once;
   a failed open is remembered with the original sentinel FILE pointer. */
char *__stdcall diagnostic_format(unsigned code)
{
    FILE *stream = diagnostic_message_file;
    if (!stream) {
        if (options.error_file) {
            stream = fopen(options.error_file, "r");
            diagnostic_message_file = stream;
        }
        if (!stream) {
            diagnostic_message_file = (FILE *)-1;
            return "";
        }
    } else if (stream == (FILE *)-1) return "";
    diagnostic_message_file = stream;
    fseek(stream, diagnostic_message_offset(code), SEEK_SET);
    return read_diagnostic_message(code, diagnostic_message_buffer, sizeof(diagnostic_message_buffer));
}

DiagnosticMessagePosition diagnostic_message_positions[20]; /* original 0x6e348..0x6e3e8 */

/* Original 0x4a940..0x4a9c4. Start from the nearest earlier cached
   message in the same thousand-code category, or from the catalog start. */
long __stdcall diagnostic_message_offset(unsigned code)
{
    int index, best = -1, distance, nearest = 32767;
    DiagnosticMessagePosition *entry;
    for (index = 0; index < 20; ++index) {
        entry = &diagnostic_message_positions[index];
        if (!entry->code) break;
        distance = (int)(code - (unsigned)entry->code);
        if (!distance) { best = index; break; }
        if (distance > 0 && distance < nearest &&
            (int)code / -1000 == entry->code / -1000) {
            nearest = distance;
            best = index;
        }
    }
    return best >= 0 ? diagnostic_message_positions[best].offset : 0;
}

/* Original 0x4a770..0x4a837. Scan to an exact code, or its category's
   default record, preserving the file position for later lookups. */
char *__stdcall read_diagnostic_message(unsigned code, char *buffer, unsigned bytes)
{
    long offset;
    char *text;
    int found;
    for (;;) {
        offset = ftell(diagnostic_message_file);
        if (!fgets(buffer, bytes, diagnostic_message_file)) {
            report_fatal(83, message_text(314), options.error_file);
            exit(68);
        }
        text = parse_diagnostic_code(buffer, &found);
        if (found == (int)code) {
            remember_diagnostic_position(offset, code);
            break;
        }
        if (!(found % 1000) && found / -1000 == (int)code / -1000) break;
    }
    return unquote_diagnostic_message(text);
}

/* Original 0x4a890..0x4a8e7. The unconsumed suffix contains the
   quoted format; blank/comment lines simply yield code zero. */
char *__stdcall parse_diagnostic_code(char *text, int *code)
{
    unsigned value = 0;
    while (isdigit(*text)) {
        value = value * 10 + *text - '0';
        ++text;
    }
    *code = (int)value;
    return text;
}

/* Original 0x4a8f0..0x4a932. Keep the first position for each code in
   the fixed cache; a full cache leaves the scanning behavior unchanged. */
void __stdcall remember_diagnostic_position(long offset, unsigned code)
{
    int index;
    for (index = 0; index < 20; ++index) {
        if (!diagnostic_message_positions[index].code) break;
        if ((unsigned)diagnostic_message_positions[index].code == code) return;
    }
    if (index < 20) {
        diagnostic_message_positions[index].code = (int)code;
        diagnostic_message_positions[index].offset = offset;
    }
}

/* Original 0x4a840..0x4a889. Decode catalog newline/tab escapes in
   place, retaining unknown escapes and returning the quoted payload. */
char *__stdcall unquote_diagnostic_message(char *text)
{
    char *start, *destination;
    while (*text++ != '"') ;
    start = destination = text;
    while (*text != '"') {
        if (*text == '\\') {
            ++text;
            if (*text == 'n') *text = '\n';
            else if (*text == 't') *text = '\t';
            else *destination++ = '\\';
        }
        *destination++ = *text++;
    }
    *destination = 0;
    return start;
}

/* Original 0x4acf0..0x4aecc. Format one source diagnostic, then send
   identical text to stderr and the optional error-listing stream. */
void emit_diagnostic(unsigned category, unsigned code, char *format, char *arguments)
{
    char output[2052], *cursor = output, *severity, *source;
    unsigned length;
    if ((int)source_line_number > 0 && source_filename) {
        format_text(cursor, "%Fs(%ld) : ", source_filename, source_line_number);
        while (*cursor) ++cursor;
    }
    if (category) {
        /* Other category values leave the native local unspecified. */
        switch (category) {
        case 1000: severity = message_text(315); break;
        case 2000: severity = message_text(314); break;
        case 4000: severity = message_text(313); break;
        }
        source = severity;
        do { *cursor = *source++; } while (*cursor++);
        --cursor;
        format_text(cursor, " %c%d: ", 'C', category + code);
        while (*cursor) ++cursor;
    }
    format_text_arguments(cursor, format, arguments);
    cursor = output;
    while (*cursor) ++cursor;
    length = cursor - output;
    fwrite(output, length, 1, stderr);
    fwrite("\n", 1, 1, stderr);
    fflush(stderr);
    if (options.option_Fs && diagnostic_trace_file) {
        fwrite(output, length, 1, diagnostic_trace_file);
        fwrite("\n", 1, 1, diagnostic_trace_file);
        fflush(diagnostic_trace_file);
    }
}

/* Original 0x4af20..0x4af38. Preserve the variadic wrapper around
   the compiler's small formatting engine. */
void format_text(char *destination, char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    format_text_arguments(destination, format, arguments);
    va_end(arguments);
}

/* Original 0x4af40..0x4b08b. Flat-pointer F and long l modifiers
   precede c/d/x/s conversions. Unsupported forms retain their last
   introducer character, matching this formatter's limited vocabulary. */
void format_text_arguments(char *destination, char *format, char *arguments)
{
    char *text, *end;
    unsigned length, index;
    int long_value, value, radix;
    while (*format) {
        if (*format != '%') {
            *destination++ = *format++;
            continue;
        }
        long_value = 0;
        while (format[1] == 'F' || format[1] == 'l') {
            if (format[1] == 'l') long_value = 1;
            ++format;
        }
        switch (format[1]) {
        case 'c':
            *destination++ = *arguments;
            arguments += 4;
            format += 2;
            break;
        case 'd': case 'x':
            radix = format[1] == 'd' ? 10 : 16;
            value = long_value ? *(long *)arguments : *(int *)arguments;
            arguments += 4;
            format += 2;
            destination += format_integer(value, destination, radix);
            break;
        case 's':
            text = *(char **)arguments;
            arguments += 4;
            if (text) {
                end = text;
                while (*end) ++end;
                length = end - text;
                for (index = 0; index <= length; ++index) destination[index] = text[index];
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

/* Original0x2d350..0x2d37b. The extension path deliberately passes
   only the level and diagnostic number; the error path retains varargs. */
void report_declaration_context_error(unsigned warning, unsigned error, ...)
{
    va_list arguments;
    if (options.extensions_enabled) compiler_warning(4,warning);
    else {
        va_start(arguments,error);
        report_syntax_error_arguments(error,arguments);
        va_end(arguments);
    }
}

/* Original0x4ab90..0x4abba, switch targets at0x4abbc. */
int effective_warning_level(unsigned level, unsigned code)
{
    int setting = warning_setting(code);
    switch (setting) {
    case 0: case 5: return level;
    case 1: case 2: case 3: case 4: return setting;
    case 14: return 1;
    default: return 0;
    }
}
