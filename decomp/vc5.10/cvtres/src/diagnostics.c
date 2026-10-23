#include "cvtres.h"
#include <stdarg.h>
#include <recon.h>

void warning(unsigned short code, ...)
{
    va_list arguments;
    va_start(arguments, code);
    printf(load_message(8002), code, ' ');
    vprintf(load_message(code), arguments);
    printf("\n");
    va_end(arguments);
}

void error(unsigned short code, ...)
{
    va_list arguments;
    va_start(arguments, code);
    printf(load_message(8001), code, ' ');
    vprintf(load_message(code), arguments);
    printf("\n");
    va_end(arguments);
}
