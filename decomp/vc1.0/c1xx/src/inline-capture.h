#ifndef INLINE_CAPTURE_H
#define INLINE_CAPTURE_H
#include "class-members.h"
#include <recon.h>

typedef int (__stdcall *ClassMemberVisitor)(CppSymbol *member, void *context);
void __stdcall visit_class_members(CppSymbol *tag, ClassMemberVisitor visitor, void *context);
int __stdcall has_copy_initializers(CppSymbol *tag);
void __stdcall emit_copy_base_initializer(CppSymbol *tag, int *count);
int __stdcall emit_copy_direct_base_initializer(CppSymbol *member, void *context);
int __stdcall emit_copy_field_initializer(CppSymbol *member, void *context);
extern int captured_class_error_count, pending_class_name_index;
void begin_pending_class_names(void);
char *next_pending_class_name(void);
void __cdecl format_work_text(WorkingStack *stack, char *format, ...);
unsigned short __stdcall generated_member_pointer_kind(int context);
void __stdcall emit_generated_default_constructor(CppSymbol *symbol, int context, unsigned short kind);
void __stdcall emit_generated_copy_constructor(CppSymbol *symbol, int context, unsigned short kind);
void __stdcall emit_generated_assignment(CppSymbol *symbol, int context, unsigned short kind);
char *__stdcall pointer_kind_spelling(unsigned short kind);
void __stdcall format_diagnostic_symbol(short text, CppSymbol *symbol);
char *__stdcall generated_member_name(int kind);
void __stdcall print_inline_declaration(CppSymbol *symbol, FILE *stream);
extern int printing_inline_declaration;
#endif
