/* Allocation and initial state of the type compactor's tables. */
#include "types.h"
#include <recon.h>

void **name_hash_buckets, **type_hash_buckets;
unsigned short *name_hash_counts, *type_hash_counts;
void **type_hash_extra_a, **type_hash_extra_b, **type_hash_extra_c, **type_hash_extra_d;
TypeDerivation *derived_types;
unsigned short derived_type_capacity, type_page_count;
unsigned char *type_work_a, *type_work_b;
GrowingBuffer type_record_storage, type_symbol_storage;
TypeRecord *empty_type;

void initialize_type_tables(void)
{
    unsigned short *record;
    name_hash_buckets = checked_zero(0x4000);
    name_hash_counts = checked_zero(0x2000);
    type_hash_buckets = checked_zero(0x4000);
    type_hash_counts = checked_zero(0x2000);
    type_hash_extra_a = checked_zero(0x4000);
    type_hash_extra_b = checked_zero(0x4000);
    type_hash_extra_c = checked_zero(0x4000);
    type_hash_extra_d = checked_zero(0x4000);
    derived_types = checked_zero(128 * sizeof(*derived_types));
    type_work_a = checked_zero(0x800);
    type_work_b = checked_zero(0x800);
    derived_type_capacity = 128;
    type_page_count = 0;
    initialize_buffer(&type_record_storage, 0x1000);
    initialize_buffer(&type_symbol_storage, 0x7f00);
    empty_type = checked_zero(sizeof(*empty_type));
    empty_type->data = checked_alloc(6);
    empty_type->flags |= 0x400;
    record = (unsigned short *)empty_type->data;
    record[0] = 6;
    record[1] = 0x201;
    record[2] = 0;
}

void *checked_zero(unsigned long size)
{
    unsigned char *data = malloc(size);
    unsigned long i;
    if (!data) fatal_error(13, NULL, NULL);
    for (i = 0; i < size; ++i) data[i] = 0;
    return data;
}

void initialize_buffer(GrowingBuffer *buffer, unsigned long size)
{
    unsigned long *words = (unsigned long *)buffer;
    int i;
    for (i = 0; i < 6; ++i) words[i] = 0;
    buffer->block_size = size;
}

void *checked_alloc(unsigned long size)
{
    void *data = malloc(size);
    if (!data) fatal_error(13, NULL, NULL);
    return data;
}
