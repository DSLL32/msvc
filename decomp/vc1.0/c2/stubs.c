#include "options.h"
#include "input.h"
#include "backend.h"
BOOL WINAPI compiler_control(DWORD event) { __asm int 3 }
void compiler_cleanup(void) { __asm int 3 }
void initialize_debugging(void) { __asm int 3 }
void initialize_codegen(void) { __asm int 3 }
void initialize_registers(void) { __asm int 3 }
void initialize_types(void) { __asm int 3 }
void initialize_globals(void) { __asm int 3 }
void compile_functions(void) { __asm int 3 }
void finish_output(void) { __asm int 3 }
void finish_debugging(void) { __asm int 3 }
void finish_globals(void) { __asm int 3 }
void __fastcall compiler_internal_error(char *source, unsigned code) { __asm int 3 }
void __cdecl compiler_report(unsigned code, ...) { __asm int 3 }
__declspec(naked) void report_context_error(void) { __asm int 3 }
int __fastcall suffix_matches(char *pattern, char *text, unsigned action) { __asm int 3 }
