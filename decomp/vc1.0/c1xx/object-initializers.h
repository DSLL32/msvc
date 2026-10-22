#ifndef OBJECT_INITIALIZERS_H
#define OBJECT_INITIALIZERS_H
#include "parser.h"
extern CppSymbol *initializer_storage_symbol;
extern unsigned initializer_offset;
extern int initializer_runtime_override, initializer_value_error_reported;
extern unsigned initializer_constructor_offset, initializer_constructor_count;
CppSymbol *__stdcall prepare_initializer_symbol(CppSymbol *symbol);
void __stdcall initialize_object_value(CppSymbol *symbol, IrToken *value);
#endif
