#ifndef CONSTRUCTOR_INITIALIZERS_H
#define CONSTRUCTOR_INITIALIZERS_H
#include "function-definitions.h"
unsigned short __stdcall constructor_pointer_attributes(CppSymbol *unused_function);
void __stdcall prepare_class_initialization(CppSymbol *tag);
int get_base_initialization_access(void);
void __stdcall set_base_initialization_access(int enabled);
void __stdcall emit_virtual_base_initializers(CppSymbol *tag, IrToken **initializers,
    unsigned short attributes);
IrToken *__stdcall take_member_initializer(IrToken **initializers, char *name);
void __stdcall emit_array_copy_initialization(CppSymbol *member, unsigned displacement,
    IrToken *initializers, unsigned short attributes);
void __stdcall emit_class_array_initialization(CppSymbol *member, CppSymbol *tag,
    IrToken *initializers, unsigned displacement, int copying, unsigned short attributes);
int __stdcall initialize_member_expression(unsigned displacement, CType *type,
    IrToken *value, CppSymbol *member);
IrToken *__stdcall make_subobject_pointer(CppSymbol *tag, unsigned short attributes,
    unsigned displacement);
IrToken *__stdcall make_constructor_call(CppSymbol *tag, IrToken *object,
    IrToken *arguments, int emit, int member_subobject);
void __stdcall emit_class_table_initializers(CppSymbol *tag, int initialize);
int __stdcall has_class_initialization(CType *type);
void __stdcall require_deleting_destructor(CppSymbol *tag);
void __stdcall require_class_member_definitions(CppSymbol *tag, char *name);
void __stdcall restore_expression_symbols(ExpressionSymbolList *symbols);
IrToken *__stdcall adjusted_this_expression(CppSymbol *parameter);
CType *__stdcall replace_type_pointer_kind(CType *type, unsigned short kind);
int __stdcall initialize_lvalue(IrToken *target, IrToken *value);
#endif
