#include <stdarg.h>
#include "command.h"
#include <recon.h>

char rc_message_buffer[4096];
char *rc_error_template;

char *__stdcall rc_message(DWORD id)
{
    if (!FormatMessageA(0xa00,dll_instance,id,0,rc_message_buffer,4096,NULL))
        return "";
    return rc_message_buffer;
}

void __cdecl rc_format(char *output, DWORD capacity, const void *format, ...)
{
    va_list arguments;
    va_start(arguments,format);
    FormatMessageA(0x400,format,0,0,output,capacity,&arguments);
    va_end(arguments);
}
