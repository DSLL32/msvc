#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include <stddef.h>
#include "hash-table.h"
#include "types.h"
#include "scopes.h"

/* Encountered base-list node prefix through its state byte at +0x14.
   The same source-order chain also contains ordinary declaration variants. */
typedef struct BaseClassLink {
    unsigned char kind, flags;
    unsigned short attributes;
    CppSymbol *next;
    unsigned field08;
    short context_index;
    unsigned short field0e;
    unsigned displacement;
    unsigned char state;
} BaseClassLink;
typedef char BaseClassLinkStateOffsetMustBe20[(offsetof(BaseClassLink, state) == 20) ? 1 : -1];

typedef struct ClassInfo {
    unsigned field00, field04, field08, field0c;
    unsigned field10, field14, field18, field1c, field20, field24;
    short type_index;
    unsigned short pointer_attributes;
    unsigned field2c;
    unsigned short debug_index;
    unsigned char alignment_flags, flags, flags2, inheritance_model;
    unsigned short field36;
    unsigned field38;
} ClassInfo;

typedef union CppSymbolState {
    unsigned flags;
    struct { unsigned char low_flags, token, high_flags[2]; } parts;
} CppSymbolState;

/* Tags own the common forty-byte record; full declarations additionally own
   the trailing C++ declaration state. Scope chains hold either variant. */
#define COMMON_SYMBOL_FIELDS \
    unsigned char kind, flags; \
    unsigned short declaration_flags; \
    CppSymbol *next; \
    char *name; \
    CType *type; \
    unsigned index, attributes; \
    unsigned short references; \
    unsigned char storage[3], field1d; \
    unsigned short field1e; \
    union { \
        struct { unsigned short operation, flags; } intrinsic; \
        unsigned value; \
        unsigned short scope_number; \
        char *text; \
        ClassInfo *class_info; \
        CppSymbol *parameter_next; \
    } detail; \
    union { unsigned value; CppSymbol *scope_members; HashTable *enumerators; } auxiliary;
typedef struct TagSymbol {
    COMMON_SYMBOL_FIELDS
} TagSymbol;
struct CppSymbol {
    COMMON_SYMBOL_FIELDS
    unsigned field28, field2c;
    CppSymbolState cpp;
};
#undef COMMON_SYMBOL_FIELDS
/* A transient qualified declarator uses the link slot before it joins a
   symbol list. The resolution holds a reference to its current binding;
   these are encountered prefixes, not complete allocated-object layouts. */
typedef struct ClassReference {
    unsigned char kind, flags;
    unsigned short attributes;
    unsigned field04;
    char *name;
    short context_index;
    unsigned short field0e;
} ClassReference;
typedef struct QualifierBinding {
    struct QualifierBinding *next;
    unsigned field04;
    ClassReference *target;
} QualifierBinding;
typedef struct QualifierResolution {
    /* The leaf contains the currently resolved target. Its next link is
       the enclosing qualifier, inspected separately for implicit this. */
    QualifierBinding *binding;
} QualifierResolution;
typedef struct QualifiedDeclarator {
    unsigned char kind, flags;
    unsigned short declaration_flags;
    struct QualifiedDeclarator *next;
    char *name;
    QualifierResolution *resolution;
} QualifiedDeclarator;
typedef struct StorageSpec {
    unsigned short flags;
    unsigned char extended;
} StorageSpec;
typedef char StorageSpecSizeMustBe4[(sizeof(StorageSpec) == 4) ? 1 : -1];
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
CppSymbol *__stdcall new_symbol(char *name, unsigned char scope);
CppSymbol *__stdcall new_declarator(char *name);
CppSymbol *__stdcall add_declaration_parameter(int prepend, CppSymbol **list, char *name, CType *type);
CppSymbol *__stdcall attach_function_parameters(CppSymbol *declaration, CppSymbol *parameters);
extern unsigned short pending_function_qualifiers;
extern int suppress_implicit_member;
DeclarationType *__stdcall global_declaration_type(DeclarationType *context);
int __stdcall is_member_declarator(CppSymbol *declaration);
CppSymbol *get_current_class_symbol(void);
CType *__stdcall replace_first_type_shape(CType *type, unsigned short kind, unsigned short attributes, void *parameter);
int __stdcall is_unqualified_class_type(CType *type);
void __stdcall attach_type_shape(CppSymbol *declaration, TypeShape *shape);
extern CppSymbol *parsed_parameter_symbols;
extern unsigned function_declaration_kind;
CppSymbol *finish_parameter_scope(void);
CppSymbol *__stdcall attach_plain_function_parameters(CppSymbol *declaration, CppSymbol *parameters);
CppSymbol *__stdcall build_function_declarator(CppSymbol *declaration, CppSymbol *parameters);
extern DeclarationType conversion_declaration;
extern unsigned short current_declaration_scope;
short __stdcall format_storage_spec(short text, StorageSpec storage);
int __stdcall merge_storage_spec(StorageSpec *destination, StorageSpec source);
CType *__stdcall function_return_type(CppSymbol *declaration);
void __stdcall report_symbol_declaration_error(unsigned code, CppSymbol *symbol);
CppSymbol *__stdcall resolve_declaration_alias(CppSymbol *symbol, CppSymbol *declaration);
int __stdcall merge_declared_symbol(CppSymbol *existing, CppSymbol *declaration);
CppSymbol *__stdcall static_member_alias(CppSymbol *symbol);
int __stdcall is_class_name_type(CType *type);
CppSymbol *__stdcall lookup_declaration(CppSymbol *declaration);
CppSymbol *__stdcall lookup_current_scope_declaration(CppSymbol *declaration);
CppSymbol *__stdcall lookup_outer_declaration(CppSymbol *declaration);
CppSymbol *__stdcall find_outer_name(char *name);
void __stdcall record_local_declaration(CppSymbol *declaration);
void __stdcall insert_symbol(CppSymbol *symbol);
void __stdcall insert_current_scope_symbol(CppSymbol *symbol);
void __stdcall insert_scope_symbol(SymbolScope *scope, CppSymbol *symbol);
CppSymbol *__stdcall resolve_declaration(DeclarationType *context, CppSymbol *declaration);
int __stdcall is_static_qualified_symbol(CppSymbol *symbol);
void __stdcall unwind_qualified_scopes(int stop_at_class);
unsigned short begin_type_diagnostic(void);
void end_type_diagnostic(void);
short __stdcall format_declarator(CppSymbol *symbol, CType *type);
char *__stdcall type_diagnostic_text(short text);
void __stdcall browser_declaration(CppSymbol *symbol);
CppSymbol *__stdcall declare_qualified_member(CppSymbol *symbol, QualifiedDeclarator *qualifier, CppSymbol *member);
char *__stdcall format_qualified_name(QualifierBinding *binding, int verbose);
CppSymbol *__stdcall symbol_construction_class(CppSymbol *symbol);
int __stdcall symbol_has_array_shape(CppSymbol *symbol);
void __stdcall initialize_zero_object(CppSymbol *symbol);
void __stdcall finalize_object_storage(CppSymbol *symbol);
void __stdcall validate_special_function(CppSymbol *symbol, int definition);
CppSymbol *__stdcall apply_declaration_type(DeclarationType *type, CppSymbol *declaration);
extern void *deferred_declarations[60];
extern int current_class_context, allow_incomplete_class;
void __stdcall resolve_deferred_declaration(void *declaration);
typedef struct ShapeCursor {
    TypeShape *shape;
    unsigned short attributes, flags;
} ShapeCursor;
typedef char ShapeCursorSizeMustBe8[(sizeof(ShapeCursor) == 8) ? 1 : -1];
int __stdcall prepare_declarator_shapes(CppSymbol *symbol, CType *base,
    TypeShape **work, ShapeCursor *base_shape, TypeShape **prefix,
    TypeShape **head, int is_typedef);
TypeShape *__stdcall extract_declarator_prefix(TypeShape *shape, TypeShape **prefix, CppSymbol *symbol, int is_typedef);
TypeShape *__stdcall intern_shape_on(TypeShape *related, unsigned short kind, unsigned short attributes, void *parameter);
TypeShape *__stdcall extract_base_prefix(TypeShape *shape, TypeShape **prefix, CppSymbol *symbol, int is_typedef, unsigned short *attributes);
TypeShape *__stdcall merge_type_shape_prefix(TypeShape *head, TypeShape *tail);
void __stdcall copy_shape_prefix(CppSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef);
void __stdcall check_declarator_prefix(CppSymbol *symbol, TypeShape *prefix);
void __stdcall finish_function_declarator(CppSymbol *symbol, TypeShape **result);
void __stdcall finish_array_declarator(CppSymbol *symbol, TypeShape **result,
    ShapeCursor current, int is_typedef);
unsigned short __stdcall merge_type_attributes(unsigned short left, unsigned short right);
void __stdcall check_pointer_attributes(CppSymbol *symbol, ShapeCursor *current, unsigned diagnostic);
TypeShape *__stdcall merge_function_shape(CppSymbol *symbol, ShapeCursor inner,
    ShapeCursor *current, ShapeCursor outer, int is_typedef);
int __stdcall merge_member_pointer_shape(CppSymbol *symbol, ShapeCursor inner,
    ShapeCursor *current, ShapeCursor outer);
unsigned char __stdcall member_pointer_class_kind(CppSymbol *class_symbol);
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
typedef char TagSymbolSizeMustBe40[(sizeof(TagSymbol) == 40) ? 1 : -1];
typedef char ClassInfoSizeMustBe60[(sizeof(ClassInfo) == 60) ? 1 : -1];
#endif
