#ifndef LINK_RECONSTRUCTION_H
#define LINK_RECONSTRUCTION_H
#include <windows.h>
#include <stdio.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif


/* Encountered diagnostic prefix: actual4670c4 copy uses36 bytes including NUL.
 * Next known original scalar is at4672cc; this does not assert that the whole
 * intervening520 bytes belong to this buffer. Longer native writes stay held. */
extern char current_phase[36];
extern FILE *diagnostic_output;
extern char *program_name;
extern const char *selected_tool_name;
extern int selected_tool_kind;
extern unsigned char logo_pending;

BOOL WINAPI console_control(DWORD event);
void __stdcall initialize_debug(char *options, const char *environment);
void __stdcall initialize_file_cache(unsigned, unsigned, unsigned, unsigned);
void __cdecl initialize_memory(void);
extern DWORD allocation_page_size, maximum_physical_memory;
int __stdcall get_debug_flag(unsigned index);
int __stdcall run_linker(int, char **);
int __stdcall run_librarian(int, char **);
int __stdcall run_dumper(int, char **);
int __stdcall run_editor(int, char **);
int __stdcall run_helper(int, char **);
int __stdcall save_exception(unsigned char, DWORD, EXCEPTION_POINTERS *, void **,
                             EXCEPTION_RECORD *, CONTEXT *);
void __stdcall report_delayed_import(DWORD, void *, int);
void __cdecl print_logo(void);
int __cdecl link_main(int argc, char **argv, char **environment);

#ifdef __cplusplus
}
#endif
#endif
