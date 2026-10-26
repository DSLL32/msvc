/* Diagnostic vocabulary used by message_text, original table 0xa3a48. */
#include <stdlib.h>
#include <ctype.h>
#include "diagnostics.h"
#include "initialization.h"
#include "pragma.h"
#include "source-input.h"
#include "text-format.h"
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
    { 346, "Target Of operator new()" },
    { 350, "Using precompiled header" },
    { 351, "Creating precompiled header" },
    { 352, "Creating new precompiled header" },
    { 0, 0 }
};

/* Original 0x74b00..0x74b22. */
char *message_text(unsigned code)
{
    MessageText *entry = message_vocabulary;
    while (entry->code && entry->code != code)
        ++entry;
    return entry->text;
}

/* Original 0x733b0..0x7349d. Per-message settings can override a
   warning's supplied level, promote it, or disable it after one use. */
void compiler_warning(int level, unsigned code, ...)
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
    if (level > options.option_W) return;
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

/* Original 0x17000..0x1701e. Preprocess-only mode disables warnings;
   otherwise the message override is a signed byte. */
int __stdcall warning_setting(unsigned code)
{
    if (options.option_E) return 13;
    return (signed char)warning_levels[code];
}

FILE *diagnostic_message_file; /* original0x8cc84 */
/* Original0xa27c0..0xa28c0. Also the address-only upper-bound sentinel
   used by the arena code; no separate Arena occupies these bytes. */
char diagnostic_message_buffer[256];

/* Original0x72e90..0x72f13, stdcall4. Open the message catalog once
   and remember a failed open with the original sentinel FILE pointer. */
char *__stdcall diagnostic_format(unsigned code)
{
    FILE *stream = diagnostic_message_file;
    if (!stream) {
        if (options.error_file) {
            stream = fopen(options.error_file,"r");
            diagnostic_message_file = stream;
        }
        if (!stream) {
            diagnostic_message_file = (FILE *)-1;
            return "";
        }
    } else if (stream == (FILE *)-1) return "";
    diagnostic_message_file = stream;
    fseek(stream,diagnostic_message_offset(code),SEEK_SET);
    return read_diagnostic_message(code,diagnostic_message_buffer,
        sizeof(diagnostic_message_buffer));
}

typedef struct DiagnosticMessagePosition {
    int code;
    long offset;
} DiagnosticMessagePosition;
typedef char DiagnosticMessagePositionSizeMustBe8[
    (sizeof(DiagnosticMessagePosition) == 8) ? 1 : -1];
/* Original0xa28c0..0xa2960: twenty code/position cache records. */
DiagnosticMessagePosition diagnostic_message_positions[20];

/* Original0x730f0..0x73174, stdcall4. Reuse the nearest earlier
   cached record in the same thousand-code category, or start at the file head. */
long __stdcall diagnostic_message_offset(unsigned code)
{
    int index, best = -1, distance, nearest = 32767;
    DiagnosticMessagePosition *entry;
    for (index = 0; index < 20; ++index) {
        entry = &diagnostic_message_positions[index];
        if (!entry->code) break;
        distance = (int)(code - (unsigned)entry->code);
        if (!distance) {
            best = index;
            break;
        }
        if (distance > 0 && distance < nearest &&
            (int)code / -1000 == entry->code / -1000) {
            nearest = distance;
            best = index;
        }
    }
    return best >= 0 ? diagnostic_message_positions[best].offset : 0;
}

char *__stdcall parse_diagnostic_code(char *text, int *code);
void __stdcall remember_diagnostic_position(long offset, unsigned code);
char *__stdcall unquote_diagnostic_message(char *text);

/* Original0x72f20..0x72fe7, stdcall12. Scan the external catalog for
   an exact code or its category default, retaining the exact record position. */
char *__stdcall read_diagnostic_message(unsigned code, char *buffer, unsigned bytes)
{
    long offset;
    char *text;
    int found;
    for (;;) {
        offset = ftell(diagnostic_message_file);
        if (!fgets(buffer,bytes,diagnostic_message_file)) {
            report_fatal(83,message_text(314),options.error_file);
            exit(68);
        }
        text = parse_diagnostic_code(buffer,&found);
        if (found == (int)code) {
            remember_diagnostic_position(offset,code);
            break;
        }
        if (!(found % 1000) && found / -1000 == (int)code / -1000) break;
    }
    return unquote_diagnostic_message(text);
}

/* Original0x73040..0x73097, stdcall8. The unconsumed suffix holds
   the quoted message. Blank/comment lines publish the ordinary zero code. */
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

/* Original0x730a0..0x730e2, stdcall8. Retain the first exact
   catalog position for a code; a full cache does not evict prior entries. */
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

/* Original0x72ff0..0x73039, stdcall4. Decode catalog escapes in
   place, preserving a backslash before an otherwise unknown escape. */
char *__stdcall unquote_diagnostic_message(char *text)
{
    char *start, *destination;
    while (*text++ != '"');
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

/* Original0x734a0..0x7367c, cdecl4. Compose the frontend diagnostic,
   then write and flush each enabled destination separately. */
void emit_diagnostic(unsigned category, unsigned code, char *format, va_list arguments)
{
    char output[2048], *cursor = output, *severity, *source;
    unsigned length;
    if ((int)source_line_number > 0 && source_filename) {
        format_text(cursor,"%Fs(%ld) : ",source_filename,source_line_number);
        while (*cursor) ++cursor;
    }
    if (category) {
        switch (category) {
        case 1000: severity = message_text(315); break;
        case 2000: severity = message_text(314); break;
        case 4000: severity = message_text(313); break;
        }
#ifdef RECON_NATIVE_TEST
        /* Original unknown nonzero categories reach0x7351b without
           writing entryESP-2052. Preserve that Win32 bug; native cannot
           reproduce the incidental severity pointer read at this point. */
        if (category != 1000 && category != 2000 && category != 4000)
            __asm { int 3 }
#endif
        source = severity;
        do { *cursor = *source++; } while (*cursor++);
        --cursor;
        format_text(cursor," %c%d: ",'C',category + code);
        while (*cursor) ++cursor;
    }
    vformat_text(cursor,format,arguments);
    cursor = output;
    while (*cursor) ++cursor;
    length = cursor - output;
    fwrite(output,length,1,stderr);
    fwrite("\n",1,1,stderr);
    fflush(stderr);
    if (options.option_Fs && diagnostic_trace_file) {
        fwrite(output,length,1,diagnostic_trace_file);
        fwrite("\n",1,1,diagnostic_trace_file);
        fflush(diagnostic_trace_file);
    }
}

/* Original0x73340..0x7336b, cdecl2;15-target switch7336c..733a8.
   Message overrides select an explicit/default level, disable the diagnostic,
   or treat a promoted error as level1 for the caller's visibility decision. */
int effective_warning_level(int level, unsigned code)
{
    int setting = warning_setting(code);
    switch (setting) {
    case 0: case 5: return level;
    case 1: case 2: case 3: case 4: return setting;
    case 14: return 1;
    default: return 0;
    }
}
