/* Compiler-generated C++ runtime declarations. */
#include "macros.h"
#include "parameters.h"
#include "keywords.h"
#include <recon.h>

void __stdcall declare_runtime_function(char *name, unsigned short pointer_attributes,
    unsigned short function_attributes, unsigned reserved_attributes, int runtime_kind);
CType *__stdcall runtime_void_pointer(unsigned short attributes);
CType *__stdcall runtime_callback_type(unsigned short pointer_attributes, unsigned short function_attributes);
void __stdcall declare_vector_helper(char *name, unsigned runtime_kind);
CppSymbol *__stdcall runtime_function_symbol(int index);
CppSymbol *purecall_symbol; /* original 0x9f2b0 */

/* Original 0x38bd0..0x38bf8. */
void declare_runtime_helpers(void)
{
    declare_vector_helper(name_vector_ctor, 8);
    declare_vector_helper(name_vector_ctor_vb, 9);
    declare_vector_helper(name_vector_dtor, 10);
}

/* Original 0x38c90..0x38ca8. */
void __stdcall declare_vector_helper(char *name, unsigned runtime_kind)
{
    declare_runtime_function(name, 1, 0x71, 0x61, runtime_kind);
}

/* Original 0x38cb0..0x38de4. The fourth argument occupies an original ABI
   slot but is not consumed by this version of the frontend. */
void __stdcall declare_runtime_function(char *name, unsigned short pointer_attributes,
    unsigned short function_attributes, unsigned reserved_attributes, int runtime_kind)
{
    unsigned char saved_mode;
    SymbolScope *saved_scope;
    CppSymbol *parameters = 0, *declaration;
    DeclarationType result;
    save_declaration_context(&saved_mode, &saved_scope);
    add_declaration_parameter(1, &parameters, 0, runtime_void_pointer(pointer_attributes));
    add_declaration_parameter(1, &parameters, 0, type_4064_second);
    add_declaration_parameter(1, &parameters, 0, type_0064);
    add_declaration_parameter(1, &parameters, 0,
        runtime_callback_type(pointer_attributes, function_attributes));
    declaration = new_declarator(name);
    declaration->type->attributes = 0x60;
    declaration_kind = 4;
    declaration = attach_function_parameters(declaration, parameters);
    result.storage[0] = (declaration_storage[0] & ~7) | 0x41;
    result.storage[1] = declaration_storage[1];
    result.storage[2] = declaration_storage[2];
    result.type = (CType *)copy_type_description(type_0080);
    declaration->flags |= 0x80;
    declaration->cpp.parts.low_flags = (unsigned char)runtime_kind;
    if (runtime_kind == -1) declaration->cpp.flags &= ~0x80000;
    else declaration->cpp.flags |= 0x80000;
    apply_declaration_type(&result, declaration);
    --type_description_depth;
    restore_declaration_context(saved_mode, saved_scope);
}

/* Original 0x38df0..0x38e08. */
CType *__stdcall runtime_void_pointer(unsigned short attributes)
{
    return make_qualified_type(type_0080, 1, attributes, 0);
}

/* Original 0x38e10..0x38e72. The callback accepts one void pointer and
   returns void, with the same attributes on the function and pointer shapes. */
CType *__stdcall runtime_callback_type(unsigned short pointer_attributes,
    unsigned short function_attributes)
{
    CppSymbol *parameters = 0;
    DeclarationList list;
    CType *function;
    add_declaration_parameter(1, &parameters, 0, runtime_void_pointer(pointer_attributes));
    list.head = parameters;
    declaration_kind = 4;
    function = make_qualified_type(type_0080, 4, function_attributes, &list);
    return make_qualified_type(function, 3, function_attributes, 0);
}

/* Original 0x38c00..0x38c0d. */
void initialize_purecall(void)
{
    purecall_symbol = runtime_function_symbol(0);
}

/* The runtime-symbol factory's one entry has an indexed twelve-byte stride.
   The trailing word belongs to the original descriptor, though this caller
   consumes only its name and low calling-convention word. */
typedef struct RuntimeFunctionSpec {
    char *name;
    unsigned attributes, reserved;
} RuntimeFunctionSpec;
RuntimeFunctionSpec runtime_function_specs[] = {
    { "_purecall", 0x11, 0 }
};
typedef char RuntimeFunctionSpecSizeMustBe12[(sizeof(RuntimeFunctionSpec) == 12) ? 1 : -1];

/* Original 0x38c10..0x38c64. */
CppSymbol *__stdcall runtime_function_symbol(int index)
{
    RuntimeFunctionSpec *spec = &runtime_function_specs[index];
    CppSymbol *symbol = new_symbol(intern_identifier(spec->name), 0);
    symbol->kind = 7;
    symbol->type = make_function_type((unsigned short)spec->attributes, type_0064);
    symbol->cpp.parts.token = 1;
    symbol->declaration_flags = (symbol->declaration_flags & ~0x100) | 0x200;
    return symbol;
}
