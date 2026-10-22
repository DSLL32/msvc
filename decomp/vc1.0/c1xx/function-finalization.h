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
#endif
