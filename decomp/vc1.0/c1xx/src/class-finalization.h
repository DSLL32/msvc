#ifndef CLASS_FINALIZATION_H
#define CLASS_FINALIZATION_H
#include "lexer.h"
#include <recon.h>
void browser_end_tag(void);
void __stdcall build_conversion_type_sets(CppSymbol *tag);
void __stdcall prepare_class_virtual_tables(CppSymbol *tag);
void __stdcall initialize_class_virtual_tables(CppSymbol *tag);
void __stdcall build_class_virtual_tables(CppSymbol *tag);
void __stdcall inherit_base_virtual_tables(CppSymbol *tag, BaseClassLink *base, int *virtual_base);
int __stdcall has_new_virtual_members(CppSymbol *tag);
CppSymbol *__stdcall create_virtual_pointer(CppSymbol *tag, int kind);
VirtualTable *__stdcall new_virtual_table(unsigned kind, CppSymbol *tag, unsigned offset, CppSymbol *pointer);
VirtualTable *__stdcall build_virtual_base_table(CppSymbol *tag, CppSymbol *pointer);
void __stdcall finish_nonvirtual_class_size(CppSymbol *tag);
void __stdcall declare_implicit_special_members(CppSymbol *tag);
void __stdcall finalize_class_virtual_tables(CppSymbol *tag);
void __stdcall complete_class_virtual_layout(CppSymbol *tag);
void __stdcall declare_class_virtual_tables(CppSymbol *tag);
void __stdcall declare_default_constructor_closures(CppSymbol *tag);
void __stdcall declare_deleting_destructors(CppSymbol *tag);
void emit_deferred_class_terminator(void);
void __stdcall build_class_member_index(CppSymbol *tag);
void reset_previous_base_class(void);
void __stdcall complete_base_class_layout(CppSymbol *tag);
void __stdcall finish_class_size(CppSymbol *tag);
void __stdcall assign_virtual_base_group_offsets(CppSymbol *tag);
void __stdcall resolve_virtual_overrides(CppSymbol *tag);
void __stdcall diagnose_virtual_ambiguities(CppSymbol *tag);
void __stdcall coalesce_virtual_table_paths(CppSymbol *tag);
void __stdcall persist_class_virtual_tables(CppSymbol *tag);
int __stdcall class_has_pure_virtual(CppSymbol *tag);
#endif
