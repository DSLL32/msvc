#ifndef CV510_TYPES_H
#define CV510_TYPES_H
#include "cvpack.h"
#include <recon.h>

typedef struct TypeArena {
    unsigned capacity;
    unsigned fields[5];
} TypeArena;
typedef struct TypeRecord {
    unsigned char *data;
    unsigned char fields[25];
    unsigned char flags;
    unsigned short tail;
} TypeRecord;
extern unsigned char *type_table_primary, *type_table_primary_counts;
extern unsigned char *type_table_secondary, *type_table_secondary_counts;
extern unsigned char *type_table_local, *type_table_global;
extern unsigned char *type_table_forward, *type_table_pending;
extern unsigned char *type_hash_buckets, *type_references, *type_positions;
extern unsigned short type_table_limit;
extern unsigned type_table_used;
extern TypeArena type_arena, type_large_arena;
extern TypeRecord *empty_type_record;
void *allocate_cleared(unsigned size);
void *allocate_bytes(unsigned size);
void initialize_arena(TypeArena *arena, unsigned capacity);
#endif
