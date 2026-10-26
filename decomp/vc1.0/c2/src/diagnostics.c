#include "diagnostics.h"
#include "options.h"
#include "function-input.h"
#include "control-input.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <recon.h>
/* Original emitter inlines these library operations. Keep that graph in
   the canonical /Od build as well. */
#pragma intrinsic(strlen, strcpy)
/* Original831a8 and85e50..85fd0. The failed-open sentinel prevents
   repeated attempts, while all successful lookups reuse the same buffer. */
FILE *diagnostic_file;
char diagnostic_text[384];
/* Original85fd0..86070, twenty number/file-offset pairs; zero number
   terminates the occupied prefix. No diagnostic resource text is embedded. */
DiagnosticOffset diagnostic_offsets[DIAGNOSTIC_OFFSET_CAPACITY];
typedef char DiagnosticOffsetSizeCheck[sizeof(DiagnosticOffset) == 8 ? 1 : -1];
/* Both original failure arms return the same empty object at80514. */
static char empty_diagnostic[] = "";

/* Original70870..708ed, unsigned ECX, RET0. The zero pushed before
   diagnostic_cached_offset is fseek's origin, not an argument to that helper. */
char *__fastcall diagnostic_format(unsigned number)
{
    FILE *file = diagnostic_file;
    if (!file) {
        if (error_filename) {
            file = fopen(error_filename, "r");
            diagnostic_file = file;
        }
        if (!file) {
            diagnostic_file = (FILE *)-1;
            return empty_diagnostic;
        }
    } else if (file == (FILE *)-1) {
        return empty_diagnostic;
    }
    diagnostic_file = file;
    fseek(file, diagnostic_cached_offset(number), SEEK_SET);
    return read_diagnostic_message(number, diagnostic_text, sizeof(diagnostic_text));
}

/* Original70990..709da, ECX number, RET0. Prefer an exact cached line,
   otherwise the nearest earlier number less than32767 away. */
long __fastcall diagnostic_cached_offset(unsigned number)
{
    int index, best = -1, distance = 32767;
    for (index = 0; index < DIAGNOSTIC_OFFSET_CAPACITY; ++index) {
        int difference;
        if (!diagnostic_offsets[index].number) break;
        difference = (int)(number - diagnostic_offsets[index].number);
        if (!difference) { best = index; break; }
        if (difference > 0 && difference < distance) {
            distance = difference;
            best = index;
        }
    }
    return best < 0 ? 0 : diagnostic_offsets[best].position;
}

char *__fastcall parse_diagnostic_number(char *text, int *number);
void __fastcall remember_diagnostic_offset(long position, unsigned number);
char *__fastcall unquote_diagnostic_message(char *text);

/* Original708f0..70985, ECX number/EDX buffer/stack capacity, RET4.
   The resource contract supplies a quoted exact or same-thousand fallback
   line. The original ignores fgets's result and has no EOF recovery. */
char *__fastcall read_diagnostic_message(unsigned number, char *buffer, unsigned capacity)
{
    char *text;
    for (;;) {
        int found;
        long position = ftell(diagnostic_file);
        fgets(buffer, capacity, diagnostic_file);
        text = parse_diagnostic_number(buffer, &found);
        if ((unsigned)found == number) {
            remember_diagnostic_offset(position, number);
            break;
        }
        if (found % 1000 == 0 && (int)number / -1000 == found / -1000)
            break;
    }
    return unquote_diagnostic_message(text);
}

/* Original70a70..70acb. The resource begins the numeric field at byte0;
   whitespace is not skipped. Original ctype receives signed characters. */
char *__fastcall parse_diagnostic_number(char *text, int *number)
{
    unsigned value = 0;
    while (isdigit((signed char)*text)) {
        value = value * 10 + (signed char)*text - '0';
        ++text;
    }
    *number = (int)value;
    return text;
}

/* Original70a30..70a66, ECX file position/EDX number, RET0. Existing
   numbers retain their first offset; a full cache simply stops learning. */
void __fastcall remember_diagnostic_offset(long position, unsigned number)
{
    unsigned index;
    for (index = 0; index < DIAGNOSTIC_OFFSET_CAPACITY; ++index) {
        if (!diagnostic_offsets[index].number) break;
        if (diagnostic_offsets[index].number == number) return;
    }
    if (index < DIAGNOSTIC_OFFSET_CAPACITY) {
        diagnostic_offsets[index].number = number;
        diagnostic_offsets[index].position = position;
    }
}

/* Original709e0..70a29. Resource lines contain an opening quote. Compact
   n/t escapes in place, retaining unknown backslash escapes verbatim. */
char *__fastcall unquote_diagnostic_message(char *text)
{
    char *result, *destination;
    while (*text++ != '"') ;
    result = destination = text;
    while (*text != '"' && *text) {
        if (*text == '\\') {
            ++text;
            if (*text == 'n') *text = '\n';
            else if (*text == 't') *text = '\t';
            else *destination++ = '\\';
        }
        *destination++ = *text++;
    }
    *destination = 0;
    return result;
}

void __cdecl format_diagnostic_text(char *buffer, char *format, ...);
void __cdecl format_diagnostic_arguments(char *buffer, char *format, va_list arguments);
extern int floating_frontend;

/* Original702b0..7043a, including the three out-of-line category arms.
   Reuse8318c/current_source_location,83190/function_source_location and
   7901c/current_source_filename; these are the input/optimizer's real owners.
   All four original direct callers supply1000,2000 or4000. Other nonzero
   categories leave the label undefined in the original; no fallback invented. */
void __cdecl emit_diagnostic(unsigned category, unsigned number, char *format, va_list arguments)
{
    char buffer[512], *cursor = buffer, *label;
    int line;
    if (!current_source_location) current_source_location = function_source_location;
    line = (int)current_source_location;
    if (line < 0) line = 0;
    if (current_source_filename) {
        format_diagnostic_text(cursor, "%s(%ld) : ", current_source_filename, line);
        cursor += strlen(cursor);
    }
    if (category) {
        switch (category) {
        case 1000: label = "fatal error"; break;
        case 2000: label = "error"; break;
        case 4000: label = "warning"; break;
        }
        strcpy(cursor, label);
        cursor += strlen(label);
        format_diagnostic_text(cursor, " %c%d: ", floating_frontend ? 'F' : 'C', number + category);
        cursor += strlen(cursor);
    }
    format_diagnostic_arguments(cursor, format, arguments);
    fwrite(buffer, strlen(buffer), 1, stderr);
    fwrite("\n", 1, 1, stderr);
}

/* Original707a0..707b8, cdecl variadic adapter. The private formatter uses
   the original four-byte x86 vararg slots; this is not CRT sprintf. */
void __cdecl format_diagnostic_text(char *buffer, char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    format_diagnostic_arguments(buffer, format, arguments);
    va_end(arguments);
}

unsigned __fastcall format_integer(unsigned value, char *text, unsigned radix);

/* Original70670..70792, cdecl(buffer,format,va_list), RET0. This small
   application formatter accepts c/s/d/x and the period F/l modifiers only.
   Unknown conversions retain literal bytes; widths and %% are not printf
   syntax here. Both int and long occupy one four-byte slot on this target. */
void __cdecl format_diagnostic_arguments(char *buffer, char *format, va_list arguments)
{
    while (*format) {
        int long_value = 0;
        if (*format == '%') {
            while (format[1] == 'F' || format[1] == 'l') {
                if (format[1] == 'l') long_value = 1;
                ++format;
            }
            switch (format[1]) {
            case 'c':
                *buffer++ = (char)va_arg(arguments, int);
                format += 2;
                continue;
            case 's': {
                char *text = va_arg(arguments, char *);
                if (text) {
                    strcpy(buffer, text);
                    buffer += strlen(text);
                }
                format += 2;
                continue;
            }
            case 'd': case 'x': {
                unsigned value, radix = format[1] == 'd' ? 10 : 16;
                if (long_value) value = va_arg(arguments, unsigned long);
                else value = va_arg(arguments, unsigned);
                buffer += format_integer(value, buffer, radix);
                format += 2;
                continue;
            }
            }
        }
        *buffer++ = *format++;
    }
    *buffer = 0;
}
