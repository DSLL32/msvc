#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "../crt/crt-imports.h"
#include "pass-one.h"
#include "link.h"
#include <recon.h>

/* 48a73c is a WORD count. The 10,000 DWORD slots at 48a740 end at
 * the independently encountered 494380 owner; admitted IDs are 0..9999. */
WORD suppressed_message_count;
DWORD suppressed_messages[10000];

BYTE __stdcall is_message_suppressed(unsigned message)
{
    WORD index;
    if (message>9999) return 1;
    for (index=0; index<suppressed_message_count; ++index)
        if (suppressed_messages[index]==message) return 1;
    return 0;
}
typedef int (WINAPI *LoadStringProcedure)(HINSTANCE,UINT,LPSTR,int);
HMODULE message_user32;
LoadStringProcedure load_message_string;
char message_text[256];

const char *__stdcall get_message_string(unsigned message)
{
    if (!message_user32) {
        message_user32=LoadLibraryA("USER32.DLL");
        if (!message_user32) return NULL;
    }
    if (!load_message_string) {
        load_message_string=(LoadStringProcedure)GetProcAddress(message_user32,"LoadStringA");
        if (!load_message_string) return NULL;
    }
    return load_message_string(NULL,message,message_text,sizeof(message_text)) ? message_text : NULL;
}

void __stdcall print_link_diagnostic(const char *file, unsigned kind,
                                    unsigned message, va_list arguments)
{
    const char *text;
    if (is_message_suppressed(message)) return;
    if (logo_pending) print_logo();
    fflush(NULL);
    if (!file) file=program_name;
    if (kind!=4) printf("%s :",file);
    if (kind!=3 && kind!=4) {
        text=get_message_string(kind);
        if (text) printf("%s",text);
    }
    if (message!=6021 && kind!=4) printf(" LNK%04u:",message);
    text=get_message_string(message);
    if (text) vprintf(text,arguments);
    fputc('\n',stdout);
    fflush(stdout);
}

void __cdecl link_message(unsigned message, ...)
{
    va_list arguments;
    va_start(arguments, message);
    print_link_diagnostic(NULL, 4, message, arguments);
    va_end(arguments);
}
