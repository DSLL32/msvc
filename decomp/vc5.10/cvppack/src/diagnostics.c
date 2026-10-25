/* Original fatal diagnostic [004054d0,00405593). */
#include "cvpack.h"
#include <recon.h>

void fatal_error(unsigned int message, const char *first, const char *second)
{
    char buffer[300];
    const char *format;
    int prefix = 0;
    if (show_logo && banner_pending) print_banner();
    if (message != 1029) {
        format = message_text(8001);
        if (!format) format = "CVPACK : fatal error CK%04u: ";
        prefix = sprintf(buffer, format, message, ' ');
    }
    sprintf(buffer + prefix, message_text(message), first, second);
    puts(buffer);
    if (input_file != -1) {
        close_file(input_file, 1);
        input_file = -1;
    }
    exit(1);
}

/* Nonfatal diagnostic [004055a0,0040562f), cdecl three-argument ABI. */
void warning(unsigned int message, const char *first, const char *second)
{
    char buffer[300];
    const char *format;
    int prefix;
    if (show_logo && banner_pending) print_banner();
    format = message_text(8002);
    if (!format) format = "CVPACK : warning CK%04u: ";
    prefix = sprintf(buffer, format, message, ' ');
    sprintf(buffer + prefix, message_text(message), first, second);
    puts(buffer);
}
