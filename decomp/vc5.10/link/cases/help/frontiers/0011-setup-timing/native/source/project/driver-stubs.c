#include <windows.h>
#include "link.h"
#include <recon.h>

BOOL WINAPI console_control(DWORD event) { __asm { int 3 } return FALSE; }
void __cdecl initialize_debug(char *options, const char *environment) { __asm { int 3 } }
void __cdecl initialize_messages(unsigned a, unsigned b, unsigned c, unsigned d) { __asm { int 3 } }
void __cdecl initialize_memory(void) { __asm { int 3 } }
int __cdecl run_linker(int argc, char **argv) { __asm { int 3 } return 255; }
int __cdecl run_librarian(int argc, char **argv) { __asm { int 3 } return 255; }
int __cdecl run_dumper(int argc, char **argv) { __asm { int 3 } return 255; }
int __cdecl run_editor(int argc, char **argv) { __asm { int 3 } return 255; }
int __cdecl run_helper(int argc, char **argv) { __asm { int 3 } return 255; }
int __stdcall save_exception(unsigned char passthrough, DWORD code, EXCEPTION_POINTERS *info,
                            void **delay, EXCEPTION_RECORD *record, CONTEXT *context)
{ __asm { int 3 } return 1; }
void __stdcall report_delayed_import(DWORD code, void *info, int fatal) { __asm { int 3 } }
void __cdecl cleanup_emulation(void) { __asm { int 3 } }
