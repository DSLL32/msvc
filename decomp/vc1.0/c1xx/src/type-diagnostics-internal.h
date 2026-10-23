#ifndef TYPE_DIAGNOSTICS_INTERNAL_H
#define TYPE_DIAGNOSTICS_INTERNAL_H
#include "type-diagnostics.h"
#include "declarations.h"
#include <recon.h>
TypeDiagnosticSlot *__stdcall find_type_diagnostic_slot(short text);
char *__stdcall reserve_type_diagnostic_bytes(short text, int append, unsigned bytes);
short __stdcall format_declaration_base_type(short text, CType *type);
short __stdcall format_declarator_shape(CppSymbol *symbol, CType *type);
void __stdcall release_type_diagnostic(short text);
typedef void (__stdcall *TypeDiagnosticWriter)(short text, char *value);
short __stdcall format_shape_attributes(short text, TypeDiagnosticWriter writer,
    unsigned short attributes, void *base, TypeShape *shape);
int __stdcall format_enclosing_declarator_scope(short text, int mode);
void __stdcall append_type_diagnostic_char(short text, int character);
void __stdcall format_this_qualifiers(short text, CType *type);
short __stdcall format_declarator_shapes(short text, TypeShape *shape);
short __stdcall format_based_declaration(unsigned short attributes, void *base);
short __stdcall format_pointer_declarator(short text, TypeShape *shape);
void __stdcall prepend_type_diagnostic_char(short text, int character);
short __stdcall format_function_parameters(short text, FunctionParameters *parameters, unsigned flags);
void __stdcall prepend_type_diagnostic(short text, char *value);
#endif
