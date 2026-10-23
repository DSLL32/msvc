#ifndef INLINE_CAPTURE_H
#define INLINE_CAPTURE_H
#include "class-members.h"
#include <recon.h>

extern int captured_class_error_count, pending_class_name_index;
void begin_pending_class_names(void);
char *next_pending_class_name(void);
void __cdecl format_work_text(WorkingStack *stack, char *format, ...);
unsigned short __stdcall generated_member_pointer_kind(int context);
void __stdcall emit_generated_default_constructor(CppSymbol *symbol, int context, unsigned short kind);
void __stdcall emit_generated_copy_constructor(CppSymbol *symbol, int context, unsigned short kind);
void __stdcall emit_generated_assignment(CppSymbol *symbol, int context, unsigned short kind);
char *__stdcall pointer_kind_spelling(unsigned short kind);
short reserve_type_diagnostic(void);
void __stdcall format_diagnostic_symbol(short text, CppSymbol *symbol);
char *__stdcall generated_member_name(int kind);
void __stdcall print_inline_declaration(CppSymbol *symbol, unsigned *flags);
#endif
