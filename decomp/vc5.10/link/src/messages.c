#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
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

/* Original 4809ec: the zero-initialized BYTE selected by /WX. */
BYTE warning_errors_enabled;

extern BYTE link_diagnostic_exiting,pdb_enabled,incremental_enabled;
extern const char *output_file_name;
extern char *old_image_name;
void __cdecl close_pdb(void);
void __cdecl close_incremental_heap(void);
void __cdecl close_files(void);
void __cdecl cleanup_pdb(void);
void __cdecl link_unresolved_error(const char *,unsigned,...);

/* The third physical DWORD is consumed as a BYTE cleanup request. The
 * diagnostic number, rather than that request, is passed to exit(). */
void __stdcall exit_link_diagnostic(const char *file,unsigned kind,unsigned remove_output,
                                    unsigned message,va_list arguments)
{
    if(!link_diagnostic_exiting) {
        link_diagnostic_exiting=1;
        if(pdb_enabled)close_pdb();
        print_link_diagnostic(file,kind,message,arguments);
        if(incremental_enabled && (BYTE)remove_output)close_incremental_heap();
        close_files();
        cleanup_pdb();
        if((BYTE)remove_output) {
            if(output_file_name && *output_file_name)_unlink(output_file_name);
            if(incremental_enabled && !_access(old_image_name,0))_unlink(old_image_name);
        }
    }
    if(kind==2)link_unresolved_error(NULL,1218);
    exit(message);
}

void __cdecl link_warning(const char *file,unsigned message,...)
{
    va_list arguments;
    va_start(arguments,message);
    if(warning_errors_enabled) {
        if(!is_message_suppressed(message))
            exit_link_diagnostic(file,2,1,message,arguments);
    } else print_link_diagnostic(file,2,message,arguments);
    va_end(arguments);
}

void __cdecl link_unresolved_error(const char *file,unsigned message,...)
{
    va_list arguments;
    va_start(arguments,message);
    print_link_diagnostic(file,1,message,arguments);
    va_end(arguments);
}

void __cdecl link_fatal(const char *file,unsigned message,...)
{
    va_list arguments;
    va_start(arguments,message);
    exit_link_diagnostic(file,0,1,message,arguments);
    va_end(arguments);
}
