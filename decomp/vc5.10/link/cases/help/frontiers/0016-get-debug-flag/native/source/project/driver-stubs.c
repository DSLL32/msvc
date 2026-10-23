#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "link.h"
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
void __cdecl initialize_memory(void) { __asm { int 3 } }
int __stdcall run_linker(int argc, char **argv) { __asm { int 3 } return 255; }
int __stdcall run_librarian(int argc, char **argv) { __asm { int 3 } return 255; }
int __stdcall run_dumper(int argc, char **argv) { __asm { int 3 } return 255; }
int __stdcall run_editor(int argc, char **argv) { __asm { int 3 } return 255; }
int __stdcall run_helper(int argc, char **argv) { __asm { int 3 } return 255; }
int __stdcall save_exception(unsigned char passthrough, DWORD code, EXCEPTION_POINTERS *info,
                            void **delay, EXCEPTION_RECORD *record, CONTEXT *context)
{ __asm { int 3 } return 1; }
void __stdcall report_delayed_import(DWORD code, void *info, int fatal) { __asm { int 3 } }
void __cdecl cleanup_emulation(void) { __asm { int 3 } }
