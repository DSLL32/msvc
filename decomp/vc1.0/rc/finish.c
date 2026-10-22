/* Original 0x01007dd0..0x01007e84. Common success/fatal-error cleanup. */
#include "resource.h"
#include <stdarg.h>
#include <stdlib.h>

void __cdecl resource_finish(char *format, ...)
{
    va_list arguments;
    if (format) {
        fprintf(diagnostics, "\n");
        va_start(arguments, format);
        vfprintf(diagnostics, format, arguments);
        va_end(arguments);
        fprintf(diagnostics, "\n");
    }
    _fcloseall();
    if (font_count)
        remove("rc$x.fdr");
    if (format && !error_count)
        remove(resource_filename);
    if (temporary_input)
        remove(temporary_input);
    if (temporary_output)
        remove(temporary_output);
    exit(format != 0);
}
