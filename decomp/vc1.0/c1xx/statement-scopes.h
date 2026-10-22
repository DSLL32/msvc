#ifndef STATEMENT_SCOPES_H
#define STATEMENT_SCOPES_H
#include "function-definitions.h"
void enter_compound_scope(void);
void record_compound_scope(void);
int __stdcall emit_constructor_initializers(CppSymbol *tag, IrToken *initializers);
void __stdcall begin_destructor_body(CppSymbol *tag);
void __stdcall emit_deleting_destructor_body(CppSymbol *tag);
void __stdcall emit_vector_deleting_destructor_body(CppSymbol *tag);
void __stdcall emit_default_ctor_closure_body(CppSymbol *tag);
#endif
