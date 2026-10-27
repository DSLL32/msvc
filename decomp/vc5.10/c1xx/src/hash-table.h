#ifndef C1XX_HASH_TABLE_H
#define C1XX_HASH_TABLE_H
#include "c1xx.h"
#include <recon.h>

/* Insertion owns all three fields of this12-byte chained entry. Keys and
 * values remain borrowed; callbacks establish their pointee domains. */
struct CompilerHashEntry {
    const void *key;
    void *value;
    CompilerHashEntry *next;
};
typedef char CompilerHashEntry_size[sizeof(CompilerHashEntry) == 12 ? 1 : -1];
/* Original 24-byte generic hash-table header. Hash/comparison selectors
 * index the distinct callback families at cf110/cf0d0. The two mode bits
 * are known stores; their broader API interpretation remains pending. */
struct CompilerHashTable {
    unsigned arena_kind;              /* +0 */
    unsigned comparison_kind;         /* +4 */
    unsigned hash_kind;               /* +8 */
    WORD bucket_count;                /* +c */
    unsigned char mode0 : 1;          /* +e bit0 */
    unsigned char mode1 : 1;          /* +e bit1; remaining bits/+f unmodeled */
    CompilerHashEntry **buckets;      /* +10 */
    unsigned entry_count;             /* +14; insertion increments it */
};
typedef char CompilerHashTable_size[sizeof(CompilerHashTable) == 24 ? 1 : -1];
extern "C" CompilerHashEntry *__cdecl insert_hash_entry(CompilerHashTable *, const void *, void *);
#endif
