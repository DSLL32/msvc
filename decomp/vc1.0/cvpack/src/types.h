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
    /* Later type passes interpret these fields; the initializer leaves zero. */
    unsigned char state[18];
    unsigned short flags;
} TypeRecord;

typedef struct TypeDerivation {
    unsigned short type, count, capacity, reserved;
    unsigned short *list;
} TypeDerivation;

typedef char GrowingBufferSize[(sizeof(GrowingBuffer)==24)?1:-1];
typedef char TypeRecordSize[(sizeof(TypeRecord)==28)?1:-1];
typedef char TypeDerivationSize[(sizeof(TypeDerivation)==12)?1:-1];

extern void **name_hash_buckets, **type_hash_buckets;
extern unsigned short *name_hash_counts, *type_hash_counts;
extern void **type_hash_extra_a, **type_hash_extra_b, **type_hash_extra_c, **type_hash_extra_d;
extern TypeDerivation *derived_types;
extern unsigned short derived_type_capacity, type_page_count;
extern unsigned char *type_work_a, *type_work_b;
extern GrowingBuffer type_record_storage, type_symbol_storage;
extern TypeRecord *empty_type;

void *checked_zero(unsigned long);
void *checked_alloc(unsigned long);
void initialize_buffer(GrowingBuffer *,unsigned long);
#endif
