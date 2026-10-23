#ifndef CONSTRUCTOR_INITIALIZERS_H
#define CONSTRUCTOR_INITIALIZERS_H
#include "function-definitions.h"
#include <recon.h>
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
IrToken *__stdcall make_destructor_call(CppSymbol *tag, IrToken *object,
    int kind, int direct, int flags);
IrToken *__stdcall make_array_destructor_call(IrToken *object);
void __stdcall require_deleting_destructor(CppSymbol *tag);
void __stdcall emit_scalar_deleting_cleanup(CppSymbol *tag);
void __stdcall emit_virtual_base_destructors(CppSymbol *tag, IrToken *object);
IrToken *__stdcall make_class_delete_call(QualifiedDeclarator *declaration, IrToken *object, CppSymbol *tag);
IrToken *__stdcall make_global_delete_call(IrToken *object);
void __stdcall require_class_member_definitions(CppSymbol *tag, char *name);
void __stdcall restore_expression_symbols(ExpressionSymbolList *symbols);
IrToken *__stdcall adjusted_this_expression(CppSymbol *parameter);
CType *__stdcall replace_type_pointer_kind(CType *type, unsigned short kind);
int __stdcall initialize_lvalue(IrToken *target, IrToken *value);
unsigned __stdcall classify_initializer_conversion(CType *type, IrToken *value);
unsigned __stdcall classify_expression_conversion(CType *type, IrToken *value,
    unsigned flags, OverloadCandidate *context);
CType *__stdcall parameter_reference_type(IrToken *reference);
unsigned __stdcall classify_type_conversion(CType *target, CType *source,
    unsigned flags, OverloadCandidate *context);
CppSymbol *__stdcall resolve_overloaded_initializer(IrToken *expression, CType *type, int mode);
unsigned __stdcall classify_special_type_conversion(CType *target, CType *source,
    unsigned flags, OverloadCandidate *context);
unsigned __stdcall classify_derived_type_conversion(CType *target, CType *source,
    unsigned flags, OverloadCandidate *context);
int __stdcall classify_user_conversion(unsigned char flags, CType *source, CType *target,
    OverloadCandidate *context, unsigned *rank);
unsigned __stdcall classify_special_scalar_conversion(CType *target, CType *source,
    unsigned char flags, OverloadCandidate *context);
int __stdcall has_pointer_or_member_pointer_shape(CType *type);
unsigned __stdcall classify_class_reference_conversion(CType *target, CType *source);
unsigned __stdcall classify_null_pointer_shape(unsigned rank, TypeShape *target, TypeShape *source);
unsigned __stdcall rank_pointer_attributes(unsigned rank, unsigned char target, unsigned char source, TypeShape *shape);
unsigned __stdcall rank_conversion_qualifiers(unsigned rank, unsigned short target, unsigned short source);
unsigned __stdcall classify_nested_shape_conversion(TypeShape *target, TypeShape *source);
unsigned __stdcall rank_reference_attributes(unsigned rank, CType *target, CType *source);
unsigned __stdcall classify_member_pointer_conversion(CType *source, CType *target, int reverse);
int __stdcall classify_source_class_conversion(unsigned char flags, CType *source, CType *target, OverloadCandidate *context, unsigned *rank);
int __stdcall type_has_base_relation(CType *derived, CType *base);
IrToken * __stdcall convert_derived_initializer(CType *type, IrToken *expression);
int __stdcall same_unqualified_class_type(CType *left, CType *right);
CppSymbol * __stdcall find_copy_constructor(CType *type);
IrToken * __stdcall construct_copy_initializer(CppSymbol *constructor, CType *type, IrToken *expression);
IrToken * __stdcall convert_scalar_initializer(unsigned char operation, CType *type, IrToken *expression);
void __stdcall emit_vtable_pointer_initializers(CppSymbol *tag, int table_kind, int use_member_address);
void __stdcall emit_vtordisp_initializers(CppSymbol *tag, int initialize);
QualifierBinding * __stdcall make_table_qualifier_path(VirtualTable *table, CppSymbol *tag);
void __stdcall initialize_qualified_member(QualifierBinding *path, IrToken *value);
unsigned __stdcall virtual_table_object_offset(VirtualTable *table);
IrToken * __stdcall make_vtordisp_value(CppSymbol *tag, VirtualTableGroup *group);
QualifierBinding * __stdcall make_vtordisp_qualifier_path(CppSymbol *tag, VirtualTableGroup *group);
IrToken *__stdcall clear_most_derived_argument(IrToken *call);
int __stdcall based_pointer_has_indirect_base(TypeShape *shape);
CppSymbol *__stdcall find_source_conversion(CppSymbol *tag, CType *target, unsigned *rank);
void __stdcall record_user_conversion(OverloadCandidate *context, CppSymbol *conversion,
    CType *target, unsigned rank);
CppSymbol *__stdcall find_user_conversion(CType *target, CType *source, unsigned *rank);
CppSymbol *__stdcall find_class_conversion(CType *target, CType *source, unsigned *rank);
int __stdcall is_class_or_reference_type(CType *type);
CppSymbol *__stdcall find_constructor_overloads(CppSymbol *tag);
extern struct InlineVector *classes_pending_table_output;
#endif
