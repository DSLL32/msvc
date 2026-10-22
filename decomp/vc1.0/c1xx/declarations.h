#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include "hash-table.h"
#include "types.h"
#include "scopes.h"

typedef struct ClassInfo {
    unsigned field00, field04, field08, field0c;
    unsigned field10, field14, field18, field1c, field20, field24;
    short type_index;
    unsigned short pointer_attributes;
    unsigned field2c;
    unsigned short debug_index;
    unsigned char alignment, flags, flags2;
} ClassInfo;

typedef union CppSymbolState {
    unsigned flags;
    struct { unsigned char low_flags, token, high_flags[2]; } parts;
} CppSymbolState;

/* C++ declarations extend the common frontend symbol with two trailing words. */
struct CppSymbol {
    unsigned char kind, flags;
    unsigned short declaration_flags;
    CppSymbol *next;
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
        ClassInfo *class_info;
        CppSymbol *parameter_next;
    } detail;
    unsigned field24, field28, field2c;
    CppSymbolState cpp;
};
/* A transient qualified declarator uses the link slot before it joins a
   symbol list. The resolution holds a reference to its current binding;
   these are encountered prefixes, not complete allocated-object layouts. */
typedef struct QualifierResolution {
    void **binding;
} QualifierResolution;
typedef struct QualifiedDeclarator {
    unsigned field00, field04, field08;
    QualifierResolution *resolution;
} QualifiedDeclarator;
typedef struct DeclarationType {
    unsigned char storage[3];
    CType *type;
} DeclarationType;
extern unsigned char declaration_storage[3];
extern unsigned char declaration_mode;
/* This declaration cache is also the global/symbol IR stream selector. */
extern unsigned char global_record_kind;
#define declaration_mode_cache global_record_kind
extern unsigned declaration_kind;
void __stdcall save_declaration_context(unsigned char *mode, SymbolScope **scope);
void __stdcall restore_declaration_context(unsigned char mode, SymbolScope *scope);
CppSymbol *__stdcall new_declarator(char *name);
CppSymbol *__stdcall add_declaration_parameter(int prepend, CppSymbol **list, char *name, CType *type);
CppSymbol *__stdcall attach_function_parameters(CppSymbol *declaration, CppSymbol *parameters);
extern CppSymbol *parsed_parameter_symbols;
extern unsigned function_declaration_kind;
CppSymbol *finish_parameter_scope(void);
CppSymbol *__stdcall attach_plain_function_parameters(CppSymbol *declaration, CppSymbol *parameters);
CppSymbol *__stdcall build_function_declarator(CppSymbol *declaration, CppSymbol *parameters);
CppSymbol *__stdcall apply_declaration_type(DeclarationType *type, CppSymbol *declaration);
extern void *deferred_declarations[60];
extern int current_class_context, allow_incomplete_class;
void __stdcall resolve_deferred_declaration(void *declaration);
void __stdcall merge_declaration_type(CppSymbol *symbol, CType *base, int is_typedef);
int __stdcall is_current_class(CppSymbol *symbol);
char *__stdcall format_symbol_name(CppSymbol *symbol);
void report_declaration_diagnostic(unsigned warning_code, unsigned error_code, ...);
unsigned short __stdcall type_qualifiers(CType *type);
CType *__stdcall strip_based_shape(CType *type);
unsigned short __stdcall classify_incomplete_type(CType *type);
void __stdcall warn_ignored_distance(CppSymbol *symbol);
unsigned short __stdcall normalize_symbol_distance(unsigned short attributes, CppSymbol *symbol);
void browser_symbol_rollover(void);
void __stdcall normalize_symbol_type(CType *base, CppSymbol *symbol);
void __stdcall append_declarator(CppSymbol *list, CppSymbol *declaration);
void __stdcall prepend_declarator(CppSymbol *list, CppSymbol *declaration);
CppSymbol *__stdcall find_scope_symbol(SymbolScope *scope, char *name, int flags);
typedef char DeclarationTypeSizeMustBe8[(sizeof(DeclarationType) == 8) ? 1 : -1];
typedef struct DeclarationList {
    CppSymbol *head;
} DeclarationList;
void __stdcall add_function_shape(CppSymbol *declaration, unsigned short attributes, DeclarationList *parameters);
extern HashTable *declaration_table;
extern CppSymbol *translation_sentinel, *translation_placeholder;
extern char *anonymous_symbol_name;
extern Vector *translation_symbols;
void initialize_declaration_table(void);
typedef char CppSymbolSizeMustBe52[(sizeof(CppSymbol) == 52) ? 1 : -1];
/* The encountered class-info prefix extends through flags2 at byte 52. */
typedef char ClassInfoPrefixSizeMustBe56[(sizeof(ClassInfo) == 56) ? 1 : -1];
#endif
