/* The three original RC diagnostics have distinct source-line conventions
 * and error numbers; keep their function vertices and call sites separate. */
#include "resource-data.h"
#include <recon.h>

/* Original 0x010094f0..0x01009536. */
void __stdcall lexical_error(char *format, char *context)
{
    fprintf(diagnostics, "\n%s(%d) : error RW2001 : ", source_filename, source_line);
    fprintf(diagnostics, format, context);
    resource_finish("\n");
}
/* Original 0x010096e0..0x01009726. */
void __stdcall parser_error(char *format, char *context)
{
    fprintf(diagnostics, "\n%s(%d) : error RW2002: ", source_filename, current_token.line);
    fprintf(diagnostics, format, context);
    resource_finish("\n");
}
/* Original 0x0100a5e0..0x0100a626. */
void __stdcall resource_data_error(char *format, char *context)
{
    fprintf(diagnostics, "\n%s(%d) : error RW2003: ", source_filename, current_token.line);
    fprintf(diagnostics, format, context);
    resource_finish("\n");
}
