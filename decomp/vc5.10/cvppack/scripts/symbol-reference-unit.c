/* Native-only initialized fields/own padding/list examples, not original oracles. */
#define RECON_SHIM_IMPL
#include "pack.h"
#include <recon.h>
#undef main
extern unsigned public_arena_remaining;
extern unsigned char *public_arena_cursor;

int main(void)
{
    unsigned char record[56], arena[64];
    PublicSymbol *buckets[4096], *first, *second;
    unsigned hash, address = 0;
    unsigned short name, segment = 0, bucket;
    memset(record, 0, sizeof(record));
    memset(buckets, 0, sizeof(buckets));
    memset(arena, 0xa5, sizeof(arena));
    *(unsigned short *)record = 54;
    *(unsigned short *)(record + 2) = 0x100b;
    *(unsigned *)(record + 32) = 0x1234;
    *(unsigned short *)(record + 36) = 3;
    record[39] = 3;
    memcpy(record + 40, "abc", 3);
    bucket = symbol_record_key(record, &hash, &name, &segment, &address, 4096);
    public_arena_cursor = arena;
    public_arena_remaining = sizeof(arena);
    record_symbol_reference(record, buckets, 2, 108, 0x400);
    first = buckets[bucket];
    if ((void *)first != arena || first->next || first->hash != hash ||
        first->offset != 0x1234 || first->name_offset != 39 || first->segment != 3 ||
        *(unsigned short *)(arena + 16) != 14 || *(unsigned short *)(arena + 18) != 0x400 ||
        *(unsigned *)(arena + 20) != hash || *(unsigned *)(arena + 24) != 108 ||
        *(unsigned short *)(arena + 28) != 2 || arena[30] != 0xa5 || arena[31] != 0xa5)
        return 1;
    record_symbol_reference(record, buckets, 7, 304, 0x400);
    second = buckets[bucket];
    if ((void *)second != arena + 32 || second->next != first ||
        *(unsigned *)(arena + 56) != 304 || *(unsigned short *)(arena + 60) != 7 ||
        arena[62] != 0xa5 || arena[63] != 0xa5 || public_arena_remaining ||
        public_arena_cursor != arena + sizeof(arena)) return 2;
    puts("initialized-fields own-padding bucket-chain");
    return 0;
}
