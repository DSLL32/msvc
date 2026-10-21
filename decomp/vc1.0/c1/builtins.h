#ifndef BUILTINS_H
#define BUILTINS_H
#include "scopes.h"
struct FrontendSymbol {
    unsigned char kind, flags;
    unsigned short field02;
    struct FrontendSymbol *next;
    char *name;
    CType *type;
    unsigned index, attributes;
    unsigned short references;
    unsigned char storage[3], field1d;
    unsigned short field1e, intrinsic_operation, intrinsic_flags;
    unsigned field24, field28;
};
typedef struct BuiltinDefinition {
    char *name;
    unsigned operation;
    unsigned short return_type, return_shape, flags, argument_flags;
} BuiltinDefinition;
extern BuiltinDefinition builtin_definitions[51];
extern unsigned char builtin_group_boundaries[29];
extern unsigned next_symbol_index, browser_symbol_limit;
extern int suppress_floating_intrinsics;
extern unsigned char current_storage[3];
int __stdcall builtin_enabled(unsigned index);
void __stdcall mark_builtin(FrontendSymbol *symbol, unsigned index);
void __stdcall insert_scope_symbol(SymbolScope *scope, FrontendSymbol *symbol);
void browser_index_overflow(void);
void __stdcall browser_builtin(FrontendSymbol *symbol);
#endif
