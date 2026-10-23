/* Add one module reference to its symbol hash bucket [0040f400,0040f49e). */
#include "pack.h"
#include <recon.h>

typedef struct SymbolReferenceRecord {
    unsigned short length, kind;
    unsigned checksum, offset;
    unsigned short module;
    /* The original leaves the final two alignment bytes unwritten. */
} SymbolReferenceRecord;

void record_symbol_reference(const unsigned char *record, PublicSymbol **buckets,
                             unsigned short module, unsigned offset, unsigned short kind)
{
    union { const unsigned char *record; unsigned short name_offset; } saved;
    unsigned hash, address = 0;
    unsigned short segment = 0, bucket;
    PublicSymbol *node;
    SymbolReferenceRecord *reference;
    /* The original name result reuses the now-dead record-pointer DWORD. */
    saved.record = record;
    bucket = symbol_record_key(saved.record, &hash, &saved.name_offset,
                               &segment, &address, 0x1000);
    node = allocate_public_block(32);
    node->hash = hash;
    node->name_offset = saved.name_offset;
    node->segment = segment;
    node->offset = address;
    reference = (SymbolReferenceRecord *)node->record;
    reference->length = 14;
    reference->kind = kind;
    reference->checksum = hash;
    reference->offset = offset;
    reference->module = module;
    node->next = buckets[bucket];
    buckets[bucket] = node;
}
