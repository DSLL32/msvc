#ifndef OBJECT_INITIALIZERS_H
#define OBJECT_INITIALIZERS_H
#include "parser.h"
#include <recon.h>
extern CppSymbol *initializer_storage_symbol;
extern unsigned initializer_offset;
extern unsigned initializer_runtime_start;
extern int initializer_runtime_override, initializer_value_error_reported;
extern unsigned initializer_constructor_offset, initializer_constructor_count;
CppSymbol *__stdcall prepare_initializer_symbol(CppSymbol *symbol);
void __stdcall initialize_object_value(CppSymbol *symbol, IrToken *value);
void __stdcall initialize_class_object(CppSymbol *symbol, IrToken *value);
IrToken *__stdcall prepare_constant_initializer(CppSymbol *symbol, IrToken *value);
int __stdcall is_character_array_initializer(CppSymbol *symbol, IrToken *value);
unsigned short __stdcall fit_string_initializer(CppSymbol *symbol, IrToken *value);
int __stdcall check_static_initializer(CppSymbol *symbol, IrToken **value);
void __stdcall begin_runtime_initializer(CppSymbol *symbol);
int __stdcall is_scalar_initializer_symbol(CppSymbol *symbol);
void __stdcall emit_initializer_assignment(IrToken *target, IrToken *value);
void __stdcall finish_initializer_symbol(CppSymbol *symbol);
IrToken *__stdcall normalize_initializer_expression(IrToken *value);
void __stdcall write_literal_initializer(CppSymbol *symbol, IrToken *value, void *context);
void __stdcall emit_runtime_symbol_initializer(CppSymbol *symbol, IrToken *value);
int __stdcall remaining_string_initializer_bytes(CppSymbol *symbol, IrToken *value);
void __stdcall emit_initializer_constant(IrToken *target);
IrToken *__stdcall prepare_constructor_arguments(CppSymbol *tag, IrToken *value);
IrToken *__stdcall make_object_constructor_call(unsigned char mode, unsigned short attributes,
    IrToken *placement, CppSymbol *symbol, IrToken *arguments, IrToken *object, IrToken *array_bound);
extern CppSymbol *allocation_base_symbol;
CppSymbol *__stdcall lookup_destructor_member(CType *type);
CppSymbol *__stdcall lookup_vector_delete_destructor(CType *type);
IrToken *__stdcall initialize_allocated_scalar(IrToken *object, IrToken *initializer);
IrToken *__stdcall initialize_allocated_class(int allocated, CppSymbol *tag,
    IrToken *object, IrToken *arguments);
IrToken *__stdcall initialize_allocated_array(IrToken *object, int allocated,
    unsigned prefix_bytes, IrToken *element_count, int element_bytes);
void __stdcall finish_local_static_initialization(CppSymbol *symbol);
void __stdcall begin_initializer_function(CppSymbol *symbol);
void __stdcall guard_local_static_initialization(CppSymbol *symbol);
CppSymbol *__stdcall create_generated_object(unsigned char storage, CType *type);
#endif
