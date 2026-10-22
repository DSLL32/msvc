#ifndef CLASS_FINALIZATION_H
#define CLASS_FINALIZATION_H
#include "lexer.h"
void browser_end_tag(void);
void __stdcall build_conversion_type_sets(CppSymbol *tag);
void __stdcall prepare_class_virtual_tables(CppSymbol *tag);
void __stdcall initialize_class_virtual_tables(CppSymbol *tag);
void __stdcall build_class_virtual_tables(CppSymbol *tag);
void __stdcall declare_implicit_special_members(CppSymbol *tag);
void __stdcall finalize_class_virtual_tables(CppSymbol *tag);
void __stdcall declare_default_constructor_closures(CppSymbol *tag);
void __stdcall declare_deleting_destructors(CppSymbol *tag);
void emit_deferred_class_terminator(void);
void __stdcall build_class_member_index(CppSymbol *tag);
#endif
