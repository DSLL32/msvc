/* Diagnostic vocabulary used by message_text, original table 0xa3a48. */
#include "diagnostics.h"
#include "pragma.h"
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
