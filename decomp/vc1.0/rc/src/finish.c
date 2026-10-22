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

/* Original 0x01007cd0..0x01007d2f, registered BOOL WINAPI callback (RET 4).
 * The control event is unused. Keep this distinct from resource_finish: native
 * interrupt cleanup always deletes the partial output and exits with status 1.
 */
BOOL WINAPI interrupt_resource(DWORD control)
{
    _fcloseall();
    if (font_count)
        remove("rc$x.fdr");
    remove(resource_filename);
    if (temporary_input)
        remove(temporary_input);
    if (temporary_output)
        remove(temporary_output);
    exit(1);
    return TRUE;
}
