#ifndef MEMBER_EXPRESSIONS_H
#define MEMBER_EXPRESSIONS_H
#include "parser.h"
#include <recon.h>
IrToken *__stdcall materialize_member_object(IrToken *object);
IrToken *__stdcall adjust_qualified_member_object(IrToken *object,
    QualifiedDeclarator *member, int pointer_conversion, int derived_to_base,
    CType *object_type);
int __stdcall qualified_member_is_accessible(void *reference);
IrToken *__stdcall adjust_member_binding_object(IrToken *object, QualifierBinding *binding,
    int pointer_conversion, int derived_to_base, CType *object_type);
void report_denied_member_access(void);
IrToken *__stdcall make_member_at_offset(IrToken *object, unsigned displacement, CType *type);
IrToken *__stdcall adjust_pointer_by_offset(IrToken *object, unsigned displacement, CType *type);
IrToken *__stdcall adjust_virtual_member_object(IrToken *object, QualifierBinding *binding,
    CType *type, unsigned displacement, int pointer_conversion);
int __stdcall query_virtual_base_pointer(CppSymbol *base, CppSymbol *derived,
    unsigned *pointer_offset, unsigned *table_offset, CType **pointer_type);
#endif
