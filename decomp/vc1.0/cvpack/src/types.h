#ifndef CVPACK_TYPES_H
#define CVPACK_TYPES_H
#include "cvpack.h"
#include <recon.h>

typedef struct BufferBlock {
    unsigned long used;
    unsigned char *data;
    struct BufferBlock *next;
} BufferBlock;

typedef struct GrowingBuffer {
    unsigned long block_size, block_count;
    BufferBlock *last;
    unsigned char *next;
    unsigned long available;
    BufferBlock *first;
} GrowingBuffer;

typedef struct TypeRecord {
    unsigned char *data;
    unsigned short index;
    unsigned short forward_index, recursive_count, reserved;
    union {
        unsigned char small[12];
        struct {
            unsigned short *offsets;
            unsigned long reserved;
            unsigned char *marks;
        } large;
    } recursion;
    unsigned short flags;
} TypeRecord;

/* Linked cache nodes reset between modules; later passes fill their state. */
typedef struct TypeCacheEntry {
    unsigned char *name, *type;
    unsigned short index;
    struct TypeCacheEntry *next;
} TypeCacheEntry;
typedef char TypeCacheEntrySize[(sizeof(TypeCacheEntry)==16)?1:-1];

typedef struct TypeDerivation {
    unsigned short type, count, capacity, reserved;
    unsigned short *list;
} TypeDerivation;

typedef char GrowingBufferSize[(sizeof(GrowingBuffer)==24)?1:-1];
typedef char TypeRecordSize[(sizeof(TypeRecord)==28)?1:-1];
typedef char TypeDerivationSize[(sizeof(TypeDerivation)==12)?1:-1];

extern void **name_hash_buckets, **type_hash_buckets;
extern unsigned short *name_hash_counts, *type_hash_counts;
extern void **global_symbol_hash, **public_symbol_hash, **type_hash_extra_c, **type_hash_extra_d;
extern TypeDerivation *derived_types;
extern unsigned short derived_type_capacity, type_page_count;
extern unsigned char *type_work_a, *type_work_b;
extern GrowingBuffer type_record_storage, type_symbol_storage;
extern TypeRecord *empty_type;

void *checked_zero(unsigned long);
void *checked_alloc(unsigned long);
void initialize_buffer(GrowingBuffer *,unsigned long);
extern int type_recursion_error;
unsigned short map_primitive(unsigned short);
unsigned short __fastcall compact_old_type(unsigned short);
unsigned short __fastcall compact_new_type(unsigned short);
void finish_recursive_types(void);
TypeRecord *get_type_record(unsigned short, unsigned short *);
int __fastcall find_named_type(TypeRecord *, TypeCacheEntry **, int);
void __fastcall queue_type_match(unsigned short, TypeCacheEntry *);
unsigned char __fastcall compact_pointer(TypeRecord *);
unsigned short __fastcall compact_type_list(unsigned short, unsigned short);
void __fastcall add_recursive_offset(TypeRecord *, unsigned short);
void merge_field_list(unsigned char *, TypeRecord *);
unsigned short intern_type(TypeRecord *, unsigned short);
void __fastcall store_type(TypeRecord *);
unsigned short __fastcall hash_type(const unsigned char *);
void release_type_data(TypeRecord *);
int find_forward_type(unsigned short, const unsigned char *, TypeCacheEntry **);
void __fastcall save_type(TypeRecord *);
void record_derived_types(unsigned short, unsigned short);

void reserve_extra_type(unsigned short, unsigned short *);
int has_extra_type(unsigned short);
extern TypeRecord *local_type_records;
extern unsigned short local_type_capacity;
extern unsigned long total_local_type_bytes;
extern unsigned short type_module_count;
void begin_symbol_block(void);
void end_symbol_block(void);
void convert_bp_relative16(void);
void convert_bp_relative32(void);
void convert_register_symbol(void);
void convert_constant_symbol(void);
void convert_object_name(void);
void convert_local_data16(void);
void convert_global_data16(void);
void convert_local_data32(void);
void convert_global_data32(void);
void convert_procedure16(void);
void convert_procedure32(void);
void convert_udt_symbol(void);
unsigned short write_search_symbol(unsigned char *, unsigned short);
int link_symbol_scopes(unsigned char *, unsigned long);
void record_symbol_offsets(unsigned char *, unsigned long, unsigned short);
#endif
