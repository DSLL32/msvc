#ifndef BUILTINS_H
#define BUILTINS_H
#include "scopes.h"
#include <recon.h>
struct FrontendSymbol {
    unsigned char kind, flags;
    unsigned short field02;
    struct FrontendSymbol *next;
    char *name;
    CType *type;
    unsigned index, attributes;
    unsigned short references;
    unsigned char storage[3], field1d;
    unsigned short field1e;
    union {
        struct { unsigned short operation, flags; } intrinsic;
        unsigned value;
        char *text;
        struct FrontendSymbol *parameter_next;
    } detail;
    union {
        unsigned value;
        struct { unsigned char kind, reserved[3]; } segment;
        /* Tag variant: full CodeView index, alignment and tag flags. */
        struct { unsigned short debug_index; unsigned char alignment, flags; } tag;
        struct {
            unsigned char offset, width;
            unsigned short reserved;
        } bitfield;
    } metadata;
    union {
        struct FrontendSymbol *members, *initializer;
        struct HashTable *enumerators;
    } associated;
};
/* Enum constants share the first 24 symbol bytes, then store their value. */
typedef struct EnumeratorSymbol {
    unsigned char kind, flags;
    unsigned short field02;
    FrontendSymbol *next;
    char *name;
    CType *type;
    unsigned index, attributes, value;
} EnumeratorSymbol;
typedef struct BuiltinDefinition {
    char *name;
    unsigned operation;
    unsigned short return_type, return_shape, flags, argument_flags;
} BuiltinDefinition;
extern BuiltinDefinition builtin_definitions[51];
extern unsigned char builtin_group_boundaries[29];
extern unsigned next_symbol_index, browser_symbol_limit;
extern unsigned char current_storage[3];
int __stdcall builtin_enabled(unsigned index);
int __stdcall find_builtin_index(char *name);
void __stdcall make_intrinsic_external(FrontendSymbol *symbol);
void __stdcall mark_builtin(FrontendSymbol *symbol, unsigned index);
void __stdcall insert_scope_symbol(SymbolScope *scope, FrontendSymbol *symbol);
void browser_index_overflow(void);
void __stdcall browser_symbol_definition(FrontendSymbol *symbol);
#endif
