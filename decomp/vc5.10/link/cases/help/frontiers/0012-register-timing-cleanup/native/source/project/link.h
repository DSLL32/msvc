#ifndef LINK_RECONSTRUCTION_H
#define LINK_RECONSTRUCTION_H
#include <windows.h>
#include <recon.h>

BOOL WINAPI console_control(DWORD event);
void __cdecl initialize_debug(char *options, const char *environment);
void __cdecl initialize_messages(unsigned, unsigned, unsigned, unsigned);
void __cdecl initialize_memory(void);
int __cdecl run_linker(int, char **);
int __cdecl run_librarian(int, char **);
int __cdecl run_dumper(int, char **);
int __cdecl run_editor(int, char **);
int __cdecl run_helper(int, char **);
int __stdcall save_exception(unsigned char, DWORD, EXCEPTION_POINTERS *, void **,
                             EXCEPTION_RECORD *, CONTEXT *);
void __stdcall report_delayed_import(DWORD, void *, int);
void __cdecl cleanup_emulation(void);
int __cdecl link_main(int argc, char **argv, char **environment);

#endif
