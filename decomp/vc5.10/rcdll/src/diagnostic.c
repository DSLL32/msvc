#include <string.h>
#include "command.h"
#include "resources.h"
#include <recon.h>
#ifdef _MSC_VER
#pragma intrinsic(strcmp)
#endif

int rc_line,rc_last_line,rc_message_count;

void __stdcall rc_resource_file_warning(unsigned code,const WCHAR *name)
{
    rc_format(rc_error_text,2048,rc_message(code),rc_resource_filename,
              rc_resource_token.line,name);
    rc_emit_notice(rc_error_text);
}

void __stdcall rc_emit(char *message)
{
    if (rc_diagnostic) rc_diagnostic(0,0,message);
    if (rc_window && SendMessageA(rc_window,0x419,0,(LPARAM)message))
        rc_fatal("\n");
    if (rc_line==rc_last_line) {
        ++rc_message_count;
        if (rc_message_count>4 && strcmp(message,"\n"))
            rc_fatal("\n");
    } else {
        rc_message_count=0;
        rc_last_line=rc_line;
    }
}


/* Notices notify the same clients as errors but do not advance the repeated
 * diagnostic counters used by rc_emit. */
void __stdcall rc_emit_notice(char *message)
{
    if(rc_diagnostic)rc_diagnostic(0,0,message);
    if(rc_window&&SendMessageA(rc_window,0x419,0,(LPARAM)message))
        rc_fatal("\n");
}
