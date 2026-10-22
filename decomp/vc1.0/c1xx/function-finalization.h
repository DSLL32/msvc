#ifndef FUNCTION_FINALIZATION_H
#define FUNCTION_FINALIZATION_H
#include "function-definitions.h"
void __stdcall finish_function_body(int recovering);
void reset_function_symbol_tables(void);
extern CppSymbol *initializer_data_symbol;
void __stdcall emit_member_and_base_destructors(CppSymbol *tag);
void __stdcall emit_vbase_destructor_epilogue(CppSymbol *tag);
void finish_function_output(void);
void reset_materialized_value_index(void);
void finish_label_symbols(void);
int __stdcall serialize_scope(SymbolScope *scope);
void finish_inline_function(void);
void __stdcall record_function_definition(CppSymbol *function);
void __stdcall emit_function_symbols(CppSymbol *symbols);
int defer_global_symbol(CppSymbol *symbol);
#endif
