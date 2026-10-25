#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include <stddef.h>
#include "hash-table.h"
#include "types.h"
#include "scopes.h"
#include <recon.h>

/* Encountered base-list node prefix through its state byte at +0x14.
   The same source-order chain also contains ordinary declaration variants. */
typedef struct BaseClassLink {
    unsigned char kind, flags;
    unsigned short attributes;
    CppSymbol *next;
    char *name;
    short context_index;
    unsigned short field0e;
    unsigned displacement;
    unsigned char state;
} BaseClassLink;
typedef char BaseClassLinkSizeMustBe24[(sizeof(BaseClassLink) == 24) ? 1 : -1];
typedef char BaseClassLinkNameOffsetMustBe8[(offsetof(BaseClassLink, name) == 8) ? 1 : -1];
typedef char BaseClassLinkStateOffsetMustBe20[(offsetof(BaseClassLink, state) == 20) ? 1 : -1];

/* Encountered virtual-dispatch record prefixes. Unused words retain their
   offsets until allocation and initialization paths establish their meaning. */
typedef struct VirtualAdjustment {
    unsigned displacement;
    short virtual_base_class;
    unsigned short reserved;
} VirtualAdjustment;
typedef struct VirtualCallEntry {
    CppSymbol *function;
    VirtualAdjustment this_adjustment;
    unsigned char flags, reserved_flags[3];
} VirtualCallEntry;
typedef struct VirtualBaseEntry {
    unsigned pointer_offset;
    CppSymbol *base;
    unsigned reserved[2];
} VirtualBaseEntry;
typedef union VirtualEntry {
    VirtualCallEntry dispatch;
    VirtualBaseEntry base;
} VirtualEntry;
typedef char VirtualCallEntrySizeMustBe16[(sizeof(VirtualCallEntry) == 16) ? 1 : -1];
typedef char VirtualBaseEntrySizeMustBe16[(sizeof(VirtualBaseEntry) == 16) ? 1 : -1];
typedef char VirtualBaseEntryClassOffsetMustBe4[(offsetof(VirtualBaseEntry, base) == 4) ? 1 : -1];
typedef char VirtualAdjustmentSizeMustBe8[(sizeof(VirtualAdjustment) == 8) ? 1 : -1];
typedef char VirtualAdjustmentClassOffsetMustBe4[(offsetof(VirtualAdjustment, virtual_base_class) == 4) ? 1 : -1];
typedef char VirtualEntryAdjustmentOffsetMustBe4[(offsetof(VirtualCallEntry, this_adjustment) == 4) ? 1 : -1];
typedef char VirtualEntryFlagsOffsetMustBe12[(offsetof(VirtualCallEntry, flags) == 12) ? 1 : -1];
typedef struct VirtualTable {
    unsigned kind;
    CppSymbol *symbol, *pointer;
    unsigned short pointer_attributes, flags;
    int entry_count;
    int entry_capacity;
    VirtualEntry *entries;
    unsigned byte_size, object_offset, virtual_base_offset;
    BaseClassLink *inheritance_path;
    struct VirtualTable *source_table;
    CppSymbol *unshared_owner;
    Vector *shared_owners;
} VirtualTable;
typedef struct VirtualTableGroup {
    CppSymbol *tag;
    unsigned object_offset, virtual_pointer_offset, virtual_base_table_offset;
    CppSymbol *virtual_base_pointer;
    Vector *tables, *base_tables;
    unsigned char flags, reserved_flags[3];
    unsigned has_vtordisp;
} VirtualTableGroup;
typedef char VirtualEntrySizeMustBe16[(sizeof(VirtualEntry) == 16) ? 1 : -1];
typedef char VirtualTableSizeMustBe56[(sizeof(VirtualTable) == 56) ? 1 : -1];
typedef char VirtualTableOwnerOffsetMustBe48[(offsetof(VirtualTable, unshared_owner) == 48) ? 1 : -1];
typedef char VirtualTableVirtualBaseOffsetMustBe36[(offsetof(VirtualTable, virtual_base_offset) == 36) ? 1 : -1];
typedef char VirtualTablePathOffsetMustBe40[(offsetof(VirtualTable, inheritance_path) == 40) ? 1 : -1];
typedef char VirtualTableSourceOffsetMustBe44[(offsetof(VirtualTable, source_table) == 44) ? 1 : -1];
typedef char VirtualTableOwnersOffsetMustBe52[(offsetof(VirtualTable, shared_owners) == 52) ? 1 : -1];
typedef char VirtualTableCountOffsetMustBe16[(offsetof(VirtualTable, entry_count) == 16) ? 1 : -1];
typedef char VirtualTableEntriesOffsetMustBe24[(offsetof(VirtualTable, entries) == 24) ? 1 : -1];
typedef char VirtualTableGroupTagOffsetMustBe0[(offsetof(VirtualTableGroup, tag) == 0) ? 1 : -1];
typedef char VirtualTableGroupOffsetOffsetMustBe4[(offsetof(VirtualTableGroup, object_offset) == 4) ? 1 : -1];
typedef char VirtualTableGroupFlagsOffsetMustBe28[(offsetof(VirtualTableGroup, flags) == 28) ? 1 : -1];
typedef char VirtualTableGroupPointerOffsetMustBe8[(offsetof(VirtualTableGroup, virtual_pointer_offset) == 8) ? 1 : -1];
typedef char VirtualTableGroupTableOffsetMustBe12[(offsetof(VirtualTableGroup, virtual_base_table_offset) == 12) ? 1 : -1];
typedef char VirtualTableGroupPointerSymbolOffsetMustBe16[(offsetof(VirtualTableGroup, virtual_base_pointer) == 16) ? 1 : -1];
typedef char VirtualTableGroupSizeMustBe36[(sizeof(VirtualTableGroup) == 36) ? 1 : -1];
typedef char VirtualTableGroupBaseTablesOffsetMustBe24[(offsetof(VirtualTableGroup, base_tables) == 24) ? 1 : -1];
typedef char VirtualTableGroupContextOffsetMustBe32[(offsetof(VirtualTableGroup, has_vtordisp) == 32) ? 1 : -1];
typedef char VirtualTableGroupTablesOffsetMustBe20[(offsetof(VirtualTableGroup, tables) == 20) ? 1 : -1];
/* The final four class-info bytes pack availability/access/ambiguity bits
   for special members, followed by copy/assignment cv qualification. */
typedef struct ClassSpecialFields {
    unsigned char available, nonprivate, ambiguous, qualifiers;
} ClassSpecialFields;
typedef union ClassSpecialTraits {
    unsigned word;
    ClassSpecialFields parts;
    unsigned char bytes[4];
} ClassSpecialTraits;
typedef char ClassSpecialTraitsSizeMustBe4[(sizeof(ClassSpecialTraits) == 4) ? 1 : -1];
typedef struct ClassMemberBucket {
    CppSymbol **symbols;
    unsigned count;
} ClassMemberBucket;
typedef char ClassMemberBucketSizeMustBe8[(sizeof(ClassMemberBucket) == 8) ? 1 : -1];

typedef struct ClassInfo {
    unsigned size, nonvirtual_size;
    Vector *virtual_tables;
    Vector *virtual_base_tables;
    Vector *virtual_table_groups;
    CppSymbol *virtual_dispatch_pointer; /* +0x14: symbol supplying virtual-call pointer type */
    struct InlineVector *conversion_types, *ambiguous_conversion_types;
    ClassMemberBucket *member_index;
    unsigned member_index_mask;
    short type_index;
    unsigned short pointer_attributes;
    unsigned virtual_base_pointer_offset;
    unsigned short debug_index;
    unsigned char alignment_flags, flags, flags2, inheritance_model;
    unsigned char trivial_special_members, implicit_member_requirements;
    ClassSpecialTraits special_members;
} ClassInfo;

typedef char ClassInfoAlignmentFlagsOffsetMustBe50[(offsetof(ClassInfo, alignment_flags) == 50) ? 1 : -1];
typedef char ClassInfoFlagsOffsetMustBe51[(offsetof(ClassInfo, flags) == 51) ? 1 : -1];
typedef char ClassInfoVbptrOffsetMustBe44[(offsetof(ClassInfo, virtual_base_pointer_offset) == 44) ? 1 : -1];
typedef char ClassInfoMemberIndexOffsetMustBe32[(offsetof(ClassInfo, member_index) == 32) ? 1 : -1];
typedef char ClassInfoMemberMaskOffsetMustBe36[(offsetof(ClassInfo, member_index_mask) == 36) ? 1 : -1];
typedef char ClassInfoVirtualTablesOffsetMustBe8[(offsetof(ClassInfo, virtual_tables) == 8) ? 1 : -1];
typedef char ClassInfoVirtualBaseTablesOffsetMustBe12[(offsetof(ClassInfo, virtual_base_tables) == 12) ? 1 : -1];
typedef char ClassInfoConversionTypesOffsetMustBe24[(offsetof(ClassInfo, conversion_types) == 24) ? 1 : -1];
typedef char ClassInfoAmbiguousConversionsOffsetMustBe28[(offsetof(ClassInfo, ambiguous_conversion_types) == 28) ? 1 : -1];
typedef char ClassInfoVirtualGroupsOffsetMustBe16[(offsetof(ClassInfo, virtual_table_groups) == 16) ? 1 : -1];

typedef char ClassInfoSpecialTraitsOffsetMustBe56[(offsetof(ClassInfo, special_members) == 56) ? 1 : -1];

typedef struct CppSymbolStateParts {
    unsigned char low_flags, token, member_flags, member_qualifiers;
} CppSymbolStateParts;
typedef union CppSymbolState {
    unsigned flags;
    CppSymbolStateParts parts;
} CppSymbolState;

/* Tags own the common forty-byte record; full declarations additionally own
   the trailing C++ declaration state. Scope chains hold either variant. */
typedef struct SymbolVirtualFunction {
    short declaring_class, table_class;
} SymbolVirtualFunction;
typedef char SymbolVirtualFunctionTableClassOffsetMustBe2[(offsetof(SymbolVirtualFunction, table_class) == 2) ? 1 : -1];
/* The reference tracker uses the second auxiliary byte for SEH nesting.
   Virtual members use the separate virtual_function interpretation. */
typedef struct SymbolBitField {
    unsigned char offset, width;
    unsigned short reserved;
} SymbolBitField;
typedef char SymbolBitFieldSizeMustBe4[(sizeof(SymbolBitField) == 4) ? 1 : -1];
typedef char SymbolBitFieldWidthOffsetMustBe1[(offsetof(SymbolBitField, width) == 1) ? 1 : -1];
typedef struct SymbolReferenceState {
    unsigned char scope_depth, seh_depth;
    unsigned short reserved2;
} SymbolReferenceState;
typedef char SymbolReferenceDepthOffsetMustBe1[(offsetof(SymbolReferenceState, seh_depth) == 1) ? 1 : -1];

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
        unsigned value, virtual_table_offset; \
        int thunk_virtual_displacement; \
        unsigned short scope_number; \
        char *text, *segment_class; \
        ClassInfo *class_info; \
        CppSymbol *parameter_next; \
    } detail; \
    union { unsigned value; unsigned char segment_kind; short declaring_class; SymbolVirtualFunction virtual_function; SymbolReferenceState reference; SymbolBitField bit_field; VirtualTable *virtual_table; CppSymbol *scope_members; HashTable *enumerators; } auxiliary;
typedef struct TagSymbol {
    COMMON_SYMBOL_FIELDS
} TagSymbol;
struct CppSymbol {
    COMMON_SYMBOL_FIELDS
    union { unsigned field28; int this_displacement; };
    unsigned field2c;
    CppSymbolState cpp;
};
#undef COMMON_SYMBOL_FIELDS
/* VC1.0 accepts direct-member offsetof constants; compose nested offsets. */
typedef char CppSymbolThunkDetailOffsetMustBe32[(offsetof(CppSymbol, detail) == 32) ? 1 : -1];
typedef char CppSymbolThunkVirtualDisplacementSizeMustBe4[(sizeof(((CppSymbol *)0)->detail.thunk_virtual_displacement) == 4) ? 1 : -1];
typedef char CppSymbolThisDisplacementOffsetMustBe40[(offsetof(CppSymbol, this_displacement) == 40) ? 1 : -1];
typedef char CppSymbolThunkQualifierOffsetMustBe51[(offsetof(CppSymbol, cpp) + offsetof(CppSymbolState, parts) + offsetof(CppSymbolStateParts, member_qualifiers) == 51) ? 1 : -1];
typedef char CppSymbolStorageOffsetMustBe26[(offsetof(CppSymbol, storage) == 26) ? 1 : -1];
typedef char CppSymbolTypeOffsetMustBe12[(offsetof(CppSymbol, type) == 12) ? 1 : -1];
typedef char CppSymbolIndexOffsetMustBe16[(offsetof(CppSymbol, index) == 16) ? 1 : -1];
typedef char CppSymbolAuxiliaryOffsetMustBe36[(offsetof(CppSymbol, auxiliary) == 36) ? 1 : -1];
/* Overload sets share the ordinary symbol prefix through the name slot.
   Their final word holds functions rather than a CType pointer. */
typedef struct OverloadSet {
    unsigned char kind, flags;
    unsigned short declaration_flags;
    CppSymbol *next;
    char *name;
    CppSymbol *functions;
} OverloadSet;
typedef char OverloadSetSizeMustBe16[(sizeof(OverloadSet) == 16) ? 1 : -1];

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
typedef char ClassReferenceContextIndexOffsetMustBe12[
    (offsetof(ClassReference, context_index) == 12) ? 1 : -1];
typedef char ClassInfoTypeIndexOffsetMustBe40[
    (offsetof(ClassInfo, type_index) == 40) ? 1 : -1];
typedef struct QualifierBinding {
    struct QualifierBinding *next;
    struct QualifierBinding *following;
    ClassReference *target;
} QualifierBinding;
typedef char QualifierBindingSizeMustBe12[(sizeof(QualifierBinding) == 12) ? 1 : -1];
typedef char QualifierBindingTargetOffsetMustBe8[(offsetof(QualifierBinding, target) == 8) ? 1 : -1];
typedef struct QualifierResolution {
    /* The leaf contains the currently resolved target. Its next link is
       the enclosing qualifier, inspected separately for implicit this. */
    QualifierBinding *binding, *tail;
} QualifierResolution;
typedef char QualifierResolutionSizeMustBe8[(sizeof(QualifierResolution) == 8) ? 1 : -1];
typedef struct QualifiedDeclarator {
    unsigned char kind, flags;
    unsigned short declaration_flags;
    struct QualifiedDeclarator *next;
    char *name;
    QualifierResolution *resolution;
} QualifiedDeclarator;
typedef char QualifiedDeclaratorSizeMustBe16[(sizeof(QualifiedDeclarator) == 16) ? 1 : -1];
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
extern unsigned function_declaration_kind, active_function_declaration_kind;
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
int __stdcall equivalent_base_type_flags(unsigned short left, unsigned short right);
int __stdcall equivalent_storage_extensions(StorageSpec left, StorageSpec right);
int __stdcall compatible_declaration_storage(CppSymbol *existing, CppSymbol *declaration);
unsigned __stdcall compatible_declaration_attributes(unsigned short left, unsigned short right, int function);
void __stdcall report_named_type_difference(unsigned code, CType *context, char *name,
    CType *source, CType *destination);
int __stdcall merge_declared_symbol(CppSymbol *existing, CppSymbol *declaration);
CppSymbol *__stdcall static_member_alias(CppSymbol *symbol);
int __stdcall is_class_name_type(CType *type);
CppSymbol *__stdcall lookup_declaration(CppSymbol *declaration);
CppSymbol *__stdcall lookup_current_scope_declaration(CppSymbol *declaration);
CppSymbol *__stdcall lookup_outer_declaration(CppSymbol *declaration);
CppSymbol *__stdcall find_outer_name(char *name);
CppSymbol *__stdcall lookup_scoped_name(char *name, unsigned categories);
extern unsigned char symbol_scope_mode, class_lookup_operator;
extern CppSymbol *class_lookup_context;
extern int class_lookup_error;
QualifiedDeclarator *__stdcall lookup_class_name(CppSymbol *tag, char *name,
    unsigned categories, unsigned diagnostic);
int __stdcall symbol_matches_categories(CppSymbol *symbol, unsigned categories);
QualifierResolution *__stdcall find_class_name_paths(CppSymbol *tag, char *name,
    unsigned categories);
void __stdcall record_local_declaration(CppSymbol *declaration);
void __stdcall insert_symbol(CppSymbol *symbol);
void __stdcall insert_current_scope_symbol(CppSymbol *symbol);
void __stdcall insert_scope_symbol(SymbolScope *scope, CppSymbol *symbol);
CppSymbol *__stdcall resolve_declaration(DeclarationType *context, CppSymbol *declaration);
int __stdcall is_static_data_member(CppSymbol *symbol);
void __stdcall unwind_qualified_scopes(int stop_at_class);
extern unsigned type_diagnostic_depth;
short reserve_type_diagnostic(void);
unsigned short begin_type_diagnostic(void);
void end_type_diagnostic(void);
short __stdcall format_declarator(CppSymbol *symbol, CType *type);
char *__stdcall type_diagnostic_text(short text);
void __stdcall browser_declaration(CppSymbol *symbol);
CppSymbol *__stdcall declare_qualified_member(CppSymbol *symbol, QualifiedDeclarator *qualifier, CppSymbol *member);
char *__stdcall format_qualified_name(QualifierBinding *binding, int verbose);
CppSymbol *__stdcall symbol_construction_class(CppSymbol *symbol);
int __stdcall has_destructor_member_model(CType *type);
int __stdcall symbol_has_reference_shape(CppSymbol *symbol);
void __stdcall initialize_zero_object(CppSymbol *symbol);
void __stdcall finalize_object_storage(CppSymbol *symbol);
void __stdcall validate_special_function(CppSymbol *symbol, int definition);
CppSymbol *__stdcall apply_declaration_type(DeclarationType *type, CppSymbol *declaration);
extern void *deferred_declarations[60];
extern int current_class_context, allow_incomplete_class;
void __stdcall check_qualified_member_access(void *declaration);
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
int __stdcall merge_reference_shape(CppSymbol *symbol, ShapeCursor inner,
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
typedef char ClassInfoVirtualDispatchPointerOffsetMustBe20[
    (offsetof(ClassInfo, virtual_dispatch_pointer) == 20) ? 1 : -1];
#endif
