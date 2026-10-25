#ifndef CV510_TYPES_H
#define CV510_TYPES_H
#include "cvpack.h"
#include <recon.h>

typedef struct TypeArenaBlock {
    unsigned used;
    unsigned char *data;
    struct TypeArenaBlock *next;
} TypeArenaBlock;
typedef struct TypeArena {
    unsigned capacity;
    union {
        unsigned fields[5];
        struct {
            unsigned block_count;
            TypeArenaBlock *last;
            unsigned char *cursor;
            unsigned remaining;
            TypeArenaBlock *first;
        };
    };
} TypeArena;
typedef struct TypeRecord {
    unsigned char *data;
    unsigned mapped_index, global_index;
    unsigned short reference_count, recursive_hash;
    union {
        unsigned char reference_offsets[12];
        struct {
            unsigned short *extended_references;
            unsigned reserved_reference;
            unsigned char *reference_flags;
        };
    };
    union {
        unsigned short status;
        struct { unsigned char low_flags, flags; };
    };
    unsigned short tail;
} TypeRecord;
extern unsigned char *type_table_primary, *type_table_primary_counts;
extern unsigned char *type_table_secondary, *type_table_secondary_counts;
extern unsigned char *type_table_local, *type_table_global;
extern unsigned char *type_table_forward, *type_table_pending;
extern unsigned char *type_hash_buckets, *type_references, *type_positions;
extern unsigned short derived_type_capacity;
extern unsigned type_table_used;
extern TypeArena type_arena, type_large_arena;
extern TypeRecord *empty_type_record;
/* Cached counted type names; nodes are retained when their contents reset. */
typedef struct CachedTypeName {
    unsigned char *name, *record;
    unsigned type_index, reserved;
    struct CachedTypeName *next;
} CachedTypeName;
typedef struct TypePatch {
    unsigned reserved0;
    unsigned char *data;
    unsigned reserved8;
    unsigned char flags, reserved_tail[3];
} TypePatch;
typedef struct PendingType {
    unsigned index;
    TypePatch *patch;
    struct PendingType *next;
} PendingType;
extern PendingType *pending_types_head, *pending_types_tail;
unsigned __fastcall compact_type_index(unsigned index);
extern unsigned type_record_count, module_type_base;
extern TypeRecord *type_records;
extern unsigned short recursive_type_pending, type_recursion_depth;
extern unsigned type_recursion_chain, type_recursion_stack[256];
extern int discard_types;
unsigned map_primitive_type(unsigned index);
TypeRecord *lookup_global_type(unsigned index, unsigned *from_global);
void convert_legacy_record(unsigned index);
unsigned char __fastcall compact_legacy_pointer(TypeRecord *record);
int __fastcall match_forward_type(TypeRecord *record, CachedTypeName **match, int insert);
void __fastcall associate_forward_type(unsigned index, CachedTypeName *record);
unsigned __fastcall compact_type_list(unsigned index, unsigned count);
typedef unsigned (__fastcall *TypeIndexMapper)(unsigned index);
extern TypeIndexMapper type_index_mappers[3];
void __fastcall add_type_reference(TypeRecord *record, unsigned short offset);
void append_type_field_list(unsigned char *field, TypeRecord *record);
unsigned resolve_recursive_type(TypeRecord *record, unsigned index);
typedef struct RecursiveTypeNode {
    unsigned char *data;
    unsigned index;
    struct RecursiveTypeNode *next;
} RecursiveTypeNode;
unsigned __fastcall hash_recursive_type(TypeRecord *record);
unsigned short __fastcall hash_type_reference(TypeRecord *record, unsigned short ordinal);
int __fastcall compare_recursive_type(TypeRecord *record, unsigned index,
    unsigned char *other_data, unsigned other_index);
unsigned __fastcall intern_recursive_type(unsigned index);
void __fastcall mark_recursive_type(TypeRecord *record);
void __fastcall index_recursive_type(unsigned char *data, unsigned index);
void __fastcall intern_type(TypeRecord *record);
typedef struct InternedType {
    unsigned index;
    unsigned char *data;
    struct InternedType *next;
} InternedType;
unsigned short __fastcall hash_type_record(const unsigned char *data);
extern unsigned char type_hash_current, type_hash_previous, type_hash_mix;
int find_local_named_type(unsigned short kind, const unsigned char *name, CachedTypeName **match, int scoped);
int lookup_named_type(unsigned short kind, const unsigned char *name, CachedTypeName **match);
typedef struct StructureAlias {
    struct StructureAlias *next;
    unsigned short length, kind;
    unsigned index;
    unsigned char name[1];
} StructureAlias;
extern unsigned short retain_structure_names;
extern unsigned char **global_type_blocks[2048];
extern StructureAlias *structure_aliases, *last_structure_alias;
unsigned char *__fastcall copy_type_arena(TypeArena *arena, const unsigned char *data, unsigned size);
void __fastcall allocate_global_type(TypeRecord *record);
void record_base_classes(unsigned index, unsigned list);
void record_base_relation(unsigned index, unsigned base);
void release_type_payload(TypeRecord *record);
unsigned char *allocate_small_type(void);
unsigned char *allocate_medium_type(void);
void recycle_small_type(unsigned char *data);
void recycle_medium_type(unsigned char *data);
unsigned __fastcall compact_legacy_index(unsigned index);
void flush_type_stack(void);
void *allocate_cleared(unsigned size);
void *allocate_bytes(unsigned size);
void initialize_arena(TypeArena *arena, unsigned capacity);
void finalize_derived_types(void);
unsigned char *allocate_type_buffer(unsigned size);
#endif
