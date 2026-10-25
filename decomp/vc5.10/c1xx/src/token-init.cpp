#include <stddef.h>
#include "frontend.h"
#include <recon.h>

/* The encountered canonical record view only. No intervening bytes or tail
 * are initialized, copied, or interpreted here. Later consumers must refine
 * the view before using another field. The original allocation stays 48. */
struct CompilerTokenRecord {
    BYTE kind;
    BYTE unmodeled_01_07[7];
    BYTE tag;
};
typedef char Token_kind_offset[offsetof(CompilerTokenRecord, kind) == 0 ? 1 : -1];
typedef char Token_tag_offset[offsetof(CompilerTokenRecord, tag) == 8 ? 1 : -1];

/* Token-code membership in the canonical record table (original d6718).
 * Entries are booleans, not a copy of a containing data section. */
static const BYTE token_has_canonical_record[245] = {
    0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, /* token 00..0f */
    1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, /* token 10..1f */
    0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, /* token 20..2f */
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, /* token 30..3f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* token 40..4f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, /* token 50..5f */
    0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, /* token 60..6f */
    0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, /* token 70..7f */
    0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, /* token 80..8f */
    1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, /* token 90..9f */
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, /* token a0..af */
    1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, /* token b0..bf */
    1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, /* token c0..cf */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, /* token d0..df */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, /* token e0..ef */
    0, 0, 0, 1, 1 /* token f0..f4 */
};

extern "C" void __cdecl initialize_preprocessor(void)
{
    unsigned code;
    CompilerPool *pool = &frontend_bss.pools[7]; /* original bf498 */
    for (code = 1; code <= 244; ++code) {
        CompilerTokenRecord *record;
        if (token_has_canonical_record[code]) {
            if (pool->remaining < 48)
                pool->Refill(48);
            record = (CompilerTokenRecord *)pool->cursor;
            pool->cursor += 48;
            pool->remaining -= 48;
            if (record) {
                record->kind = (BYTE)code;
                record->tag = (BYTE)code;
            }
            frontend_data.canonical_token_records[code - 1] = record;
        } else {
            frontend_data.canonical_token_records[code - 1] = NULL;
        }
    }
}
