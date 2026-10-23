#ifndef REFERENCE_INITIALIZERS_H
#define REFERENCE_INITIALIZERS_H
#include "numeric-conversion.h"
#include <recon.h>
IrToken *__stdcall convert_reference_initializer(CType *type, IrToken *expression);
int __stdcall reference_requires_temporary(CType *referent, IrToken *expression, int *extension_binding);
IrToken *__stdcall bind_reference_lvalue(CType *referent, CType *reference, IrToken *expression);
IrToken *__stdcall retain_reference_temporary(CType *referent, CType *reference, IrToken *expression);
IrToken *__stdcall initialize_reference_temporary(CType *referent, CType *reference, IrToken *expression);
int __stdcall reference_types_compatible(CType *referent, CType *source);
int __stdcall reference_lvalue_bindable(IrToken *expression, int *extension_binding);
int __stdcall reference_source_is_array(CType *type);
CType *__stdcall strip_reference_type(CType *type);
int __stdcall reference_tag_compatible(CType *referent, CType *source);
int __stdcall reference_lvalue_needs_extension(IrToken *expression);
#endif
