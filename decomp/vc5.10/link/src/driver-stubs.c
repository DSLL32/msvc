#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "link.h"
#include "options.h"
#include "../crt/crt-imports.h"
#include <recon.h>

BOOL WINAPI console_control(DWORD event) { __asm { int 3 } return FALSE; }
unsigned char debug_flags[256];
void __stdcall initialize_debug(char *options, const char *environment)
{
    int option;
    if (!options) options = getenv(environment);
    if (!options) return;
    while (*options && !(isdigit)((signed char)*options)) ++options;
    while (sscanf(options, "%d", &option) == 1) {
        debug_flags[option] = 1;
        while (*options && (isdigit)((signed char)*options)) ++options;
        while (*options && !(isdigit)((signed char)*options)) ++options;
    }
}
int __stdcall get_debug_flag(unsigned index)
{
    return (signed char)debug_flags[index];
}
int __stdcall run_helper(int argc, char **argv) { __asm { int 3 } return 255; }
int __stdcall save_exception(unsigned char passthrough, DWORD code, EXCEPTION_POINTERS *info,
                            void **delay, EXCEPTION_RECORD *record, CONTEXT *context)
{ __asm { int 3 } return 1; }
void __stdcall report_delayed_import(DWORD code, void *info, int fatal) { __asm { int 3 } }
ByteBuffer logo_text;
void __stdcall clear_buffer(ByteBuffer *);
void __cdecl print_logo(void)
{
    const char *description=selected_tool_name;
    switch (selected_tool_kind) {
    case 1: description="32-Bit Incremental Linker"; break;
    case 2: description="32-Bit Library Manager"; break;
    case 3: description="COFF Binary File Dumper"; break;
    case 4: description="COFF Binary File Editor"; break;
    case 5: description="32-Bit Import Library Converter"; break;
    }
    printf("Microsoft (R) %s Version 5.10.7303\n"
           "Copyright (C) Microsoft Corp 1992-1997. All rights reserved.\n\n",description);
    if (logo_text.data) {
        if (logo_text.data[logo_text.used-1]!='\n') append_bytes(&logo_text,"\n",1);
        append_bytes(&logo_text,"",1);
        printf("%s",logo_text.data);
        clear_buffer(&logo_text);
    }
    fflush(stdout);
    logo_pending=0;
}
