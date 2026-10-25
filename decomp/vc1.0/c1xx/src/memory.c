#include "work-list.h"
/* Permanent zeroed allocation arena, original globals 0x9f1e0/0x9f41c. */
#include <stdlib.h>
#include <stdio.h>
#include "options.h"
#include "pch-format.h"
#include "pch-relocation.h"
#include "operators.h"
#include "diagnostics.h"
#include <recon.h>

unsigned char *permanent_next;
unsigned permanent_available;

/* Original 0x6ad50..0x6ad9a. */
void *__stdcall allocate_permanent(unsigned bytes)
{
    unsigned *clear;
    unsigned i;
    void *result;
    if (permanent_available < bytes) {
        permanent_next = new_permanent_page();
        clear = (unsigned *)permanent_next;
        for (i = 0; i < 0x3bff; ++i)
            clear[i] = 0;
        permanent_next += 4;
        permanent_available = 0xeff8;
    }
    result = permanent_next;
    permanent_next += bytes;
    permanent_available -= bytes;
    return result;
}

/* Pages and relocation maps used to serialize permanent arena pointers. */
typedef struct PermanentPage {
    unsigned char *start;
    BitSet *relocations;
} PermanentPage;
static PermanentPage permanent_pages[512]; /* original 0x98180 */
static PermanentPage *current_page;         /* original 0xa230c */

/* Original 0x6acb0..0x6ad2a. */
void *new_permanent_page(void)
{
    unsigned char *allocation;
    if (permanent_page_count >= 511)
        fatal_error(60);
    current_page = &permanent_pages[permanent_page_count++];
    allocation = malloc(0xfffc);
    if (!allocation)
        fatal_error(60);
    allocation = (unsigned char *)(((unsigned)allocation + 0xfff) & 0xfffff000);
    current_page->start = allocation;
    current_page->relocations = new_bitset(0xeffc);
    return current_page->start;
}

/* Original 0x4e700..0x4e750. */
BitSet *__stdcall new_bitset(unsigned bit_count)
{
    unsigned bytes = (bit_count >> 3) + 2;
    unsigned i;
    BitSet *set = malloc((bit_count >> 3) + 9);
    if (!set)
        fatal_error(60);
    set->bit_count = bit_count;
    for (i = 0; i < bytes; ++i)
        set->bits[i] = 0;
    set->bits[bytes - 1] = 0xff;
    return set;
}

/* Original 0x72d20..0x72d44. */
void *__stdcall checked_malloc(unsigned bytes)
{
    void *result = malloc(bytes);
    if (!result)
        fatal_error(59);
    return result;
}

Arena compiler_arena = { 0x3ff4, 0 }; /* original 0xa2740 */

/* Original 0x72db0..0x72e5b. */
void __stdcall refill_arena(Arena *arena, ArenaPool *pool)
{
    ArenaBlock *block = arena->free_blocks;
    unsigned i;
    if (block)
        arena->free_blocks = block->next;
    else {
        block = malloc(arena->block_bytes + 12);
        if (!block)
            fatal_error((void *)arena == (void *)diagnostic_message_buffer ? 60 : 59);
        block->bytes = arena->block_bytes;
    }
    block->next = 0;
    for (i = 0; i < block->bytes; ++i)
        block->data[i] = 0;
    if (pool->last) {
        pool->last->next = block;
        pool->last->bytes = arena->block_bytes - pool->remaining;
    }
    pool->last = block;
    if (!pool->first)
        pool->first = block;
    pool->remaining = arena->block_bytes;
    pool->next = block->data;
}

/* Original 0x74490..0x744f4. Copy unrounded bytes into a word-aligned allocation. */
void *__stdcall copy_arena_bytes(void *source, int arena, unsigned bytes)
{
    unsigned allocation = (bytes + 3) & ~3;
    unsigned char *destination;
    ArenaPool *pool;
    unsigned i;
    if (!arena)
        destination = allocate_permanent(allocation);
    else {
        if (arena == -1)
            arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)allocation)
            refill_arena(&compiler_arena, pool);
        destination = pool->next;
        pool->remaining -= allocation;
        pool->next += allocation;
    }
    for (i = 0; i < bytes; ++i)
        destination[i] = ((unsigned char *)source)[i];
    return destination;
}

/* Original 0xa1e1c/0xa1e24: release bookkeeping with no identified reader
   in this image. A different global occupies the intervening word. */
unsigned arena_release_word_1, arena_release_word_2;

/* Original 0x72d50..0x72db0. The original root range contains just the
   compiler arena;0xa27c0 is its excluded one-past bound and the
   first byte of the diagnostic catalog buffer, not another Arena. */
void __stdcall release_arena(int index)
{
    ArenaPool *pool = &compiler_arena.pools[index];
    ArenaBlock *last = pool->last;
    if (last) {
        last->bytes = compiler_arena.block_bytes - pool->remaining;
        if (last->bytes) {
            last->next = compiler_arena.free_blocks;
            compiler_arena.free_blocks = pool->first;
            pool->first = 0;
            pool->last = 0;
            pool->next = 0;
            pool->remaining = 0;
            arena_release_word_2 = 0;
            arena_release_word_1 = 0;
        }
    }
}

/* Original 0x6ada0..0x6ada2 is RET 8. Permanent pages do not free individual
   allocations; preserve this separate no-op entry and both stack slots. */
void __stdcall release_permanent(void *block, unsigned bytes)
{
}

/* Original 0x507c0..0x507cd. Entries belong to their arena; popping only
   advances the list head and returns that new head. */
WorkList *__stdcall pop_work_list(WorkList **stack)
{
    *stack = (*stack)->next;
    return *stack;
}

/* Original 0x50770..0x507bd. Only the link is initialized here; callers
   own the payload, including the byte-sized depth entries used by scopes. */
WorkList *__stdcall push_work_list(WorkList **stack, int arena)
{
    WorkList *entry;
    ArenaPool *pool;
    if (!arena)
        entry = allocate_permanent(sizeof(WorkList));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(WorkList))
            refill_arena(&compiler_arena, pool);
        entry = (WorkList *)pool->next;
        pool->remaining -= sizeof(WorkList);
        pool->next += sizeof(WorkList);
    }
    entry->next = *stack;
    *stack = entry;
    return entry;
}

/* Original 0x788f0..0x78929. Unlike checked_malloc, this allocation reports
   fatal error 60 and clears every byte before returning its heap block. */
void *__stdcall allocate_zeroed(unsigned bytes)
{
    unsigned char *block = malloc(bytes);
    unsigned i;
    if (!block) fatal_error(60);
    for (i = 0; i < bytes; ++i) block[i] = 0;
    return block;
}

unsigned __stdcall minimum_unsigned(unsigned left, unsigned right);

/* Original 0x6adb0..0x6ade5. Permanent resizing allocates a new block
   and retains the old one. Copy only the smaller size through the original
   separate unsigned-minimum helper, then return the new allocation. */
void *__stdcall resize_permanent(void *block, unsigned old_bytes, unsigned new_bytes)
{
    unsigned char *copy = allocate_permanent(new_bytes);
    unsigned bytes = minimum_unsigned(new_bytes, old_bytes), index;
    for (index = 0; index < bytes; ++index)
        copy[index] = ((unsigned char *)block)[index];
    return copy;
}

/* Original 0x6ad30..0x6ad40. Retain the native unsigned comparison
   and its separate two-slot helper ABI. */
unsigned __stdcall minimum_unsigned(unsigned left, unsigned right)
{
    return right < left ? right : left;
}

/* Original0x74400..0x74483. Copy a terminated string into a four-byte-rounded
   arena allocation; only the actual string bytes are copied. */
char *__stdcall copy_string(char *text, int arena)
{
    char *destination;
    ArenaPool *pool;
    unsigned length, bytes, i;
    for (length = 0; text[length]; ++length) ;
    bytes = (length + 4) & ~3U;
    if (!arena) destination = allocate_permanent(bytes);
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)bytes) refill_arena(&compiler_arena, pool);
        destination = (char *)pool->next;
        pool->remaining -= bytes;
        pool->next += bytes;
    }
    for (length = 0; text[length]; ++length) ;
    ++length;
    for (i = 0; i < length; ++i) destination[i] = text[i];
    return destination;
}

unsigned char *pch_reference_base; /* original0xa2300: cached page start */
unsigned short pch_reference_page; /* original0xa2304: cached page index */
void __stdcall encode_pch_pointer(void *pointer);

/* Original0x6af60..0x6afe5. A non-NULL slot value marks the slot's own
   arena address, not its target. Foreign slots have no relocation bit. */
int __stdcall mark_pch_pointer(void **slot)
{
    unsigned reference, offset;
    unsigned short page;
    if (!*slot) return 0;
    pch_encoded_pointer = (unsigned)slot - (unsigned)pch_reference_base;
    if (pch_encoded_pointer > 0xeffc) encode_pch_pointer(slot);
    else pch_encoded_pointer = (pch_encoded_pointer & 0xffff) |
        ((unsigned)pch_reference_page << 16);
    reference = pch_encoded_pointer;
    page = (unsigned short)(reference >> 16);
    if (page == 0xffff) return 0;
    offset = (unsigned short)reference;
    permanent_pages[page].relocations->bits[offset >> 3] |=
        (unsigned char)(1 << (offset & 7));
    return 1;
}

/* Original0x6a6c0..0x6a768. Locate the actual permanent page and cache its
   base; the shared result packs a16-bit byte offset followed by page index. */
void __stdcall encode_pch_pointer(void *pointer)
{
    unsigned short index, count;
    unsigned offset;
    if (!pointer) {
        pch_encoded_pointer = 0;
        return;
    }
    pch_reference_page = 0;
    count = (unsigned short)permanent_page_count;
    for (index = 0; index < count; ++index) {
        offset = (unsigned)pointer - (unsigned)permanent_pages[index].start;
        pch_encoded_pointer = offset;
        if (offset < 0xeffc) {
            pch_reference_page = index;
            pch_encoded_pointer = (offset & 0xffff) | ((unsigned)index << 16);
            pch_reference_base = permanent_pages[index].start;
            return;
        }
        pch_reference_page = index + 1;
    }
    pch_encoded_pointer = 0xffffffff;
    pch_reference_page = 0xffff;
    pch_reference_base = 0;
}

/* Original0x6a8f0..0x6aadc. Visit marked slots in page/byte/bit order.
   Each target page keeps a backward chain of the slots referring to it;
   a link crossing a64K boundary starts another chain record. The bitmap's
   extra final0xff byte is a scanning sentinel, not a set of pointer slots. */
void encode_pch_pages(void)
{
    int source_page;
    unsigned byte, bit, reference, location, distance, packed, i;
    unsigned *slot;
    unsigned short target_page;
    BitSet *marks;
    PchRelocationChain *chain;

    for (source_page = 0; source_page < permanent_page_count; ++source_page) {
        marks = permanent_pages[source_page].relocations;
        for (byte = 0; byte <= (marks->bit_count >> 3); ++byte) {
            for (bit = 0; bit < 8; ++bit) {
                if (!(marks->bits[byte] & (1 << bit))) continue;
                slot = (unsigned *)(permanent_pages[source_page].start +
                    byte * 8 + bit);
                pch_encoded_pointer = *slot - (unsigned)pch_reference_base;
                if (pch_encoded_pointer > 0xeffc)
                    encode_pch_pointer((void *)*slot);
                else pch_encoded_pointer = (pch_encoded_pointer & 0xffff) |
                    ((unsigned)pch_reference_page << 16);
                reference = pch_encoded_pointer;
                target_page = (unsigned short)(reference >> 16);
                if (target_page == 0xffff) continue;
                chain = &pch_relocation_chains[target_page];
                location = ((unsigned)source_page << 16) |
                    (((unsigned)slot -
                    (unsigned)permanent_pages[source_page].start) & 0xffff);
                packed = reference & 0xffff;
                if (chain->head) {
                    distance = location - chain->head;
                    if ((int)distance < 0x10000)
                        packed |= (distance & 0xffff) << 16;
                    else {
                        if ((int)pch_relocation_count >= pch_relocation_capacity) {
                            pch_relocation_chains = realloc(pch_relocation_chains,
                                pch_relocation_capacity * 8 + 256);
                            if (!pch_relocation_chains) fatal_error(60);
                            for (i = pch_relocation_capacity;
                                i < (unsigned)pch_relocation_capacity + 32; ++i) {
                                pch_relocation_chains[i].head = 0;
                                pch_relocation_chains[i].page = 0;
                                pch_relocation_chains[i].reserved = 0;
                            }
                            pch_relocation_capacity += 32;
                            chain = &pch_relocation_chains[target_page];
                        }
                        pch_relocation_chains[pch_relocation_count++] = *chain;
                    }
                }
                *slot = packed;
                chain->head = location;
            }
        }
    }
}

extern FILE *pch_file;

/* Original0x6adf0..0x6ae77. The last page contains only the used prefix;
   earlier pages retain their full logical allocation span. */
void write_pch_pages(void)
{
    int page;
    fwrite(pch_relocation_chains, 1, pch_relocation_count * 8, pch_file);
    for (page = 0; page < permanent_page_count - 1; ++page)
        fwrite(permanent_pages[page].start, 1, 0xeffc, pch_file);
    fwrite(permanent_pages[page].start, 1, 0xeffc - permanent_available, pch_file);
}

/* Original0x6aae0..0x6ab42. Operator names are mandatory; operand words
   retain the original low-WORD test before decoding a nonempty slot. */
void restore_pch_operators(void)
{
    BuiltinOperator *entry;
    unsigned reference, operand;
    for (entry = builtin_operators; entry->token; ++entry) {
        reference = (unsigned)entry->name;
        entry->name = (char *)permanent_pages[reference >> 16].start +
            (reference & 0xffff);
        for (operand = 0; operand < 2; ++operand) {
            reference = (unsigned)entry->operands[operand].type;
            if ((unsigned short)reference)
                entry->operands[operand].type = (CType *)(
                    permanent_pages[reference >> 16].start + (reference & 0xffff));
        }
    }
}

/* Original0x6abb0..0x6aca7. Restore the canonical root owners, operator
   table and backward-linked arena slots without changing the serialized file. */
void restore_pch_pointers(void)
{
    unsigned root, reference, location, value;
    int index;
    unsigned char *target;
    unsigned *slot;
    for (root = 0; root < 149; ++root) {
        reference = *pch_root_cells[root];
        if (reference) {
            *pch_root_cells[root] = (unsigned)permanent_pages[reference >> 16].start +
                (reference & 0xffff);
            pch_compiler_image.roots[root] = *pch_root_cells[root];
        }
    }
    restore_pch_operators();
    for (index = 0; index < (int)pch_relocation_count; ++index) {
        location = pch_relocation_chains[index].head;
        target = permanent_pages[pch_relocation_chains[index].page].start;
        while (location) {
            slot = (unsigned *)(permanent_pages[location >> 16].start +
                (location & 0xffff));
            value = *slot;
            *slot = (unsigned)target + (value & 0xffff);
            if (!(unsigned short)(value >> 16)) break;
            location -= (unsigned short)(value >> 16);
        }
    }
    free(pch_relocation_chains);
}

/* Original0x6ae80..0x6af5a. Read saved bytes into real malloc-backed pages;
   they are not file mappings. Only the unused tail of the last page is cleared. */
void read_pch_pages(void)
{
    int page, last;
    unsigned i;
    unsigned char *tail;
    pch_relocation_chains = malloc(pch_relocation_count * 8);
    if (!pch_relocation_chains) fatal_error(60);
    fread(pch_relocation_chains, 1, pch_relocation_count * 8, pch_file);
    last = permanent_page_count - 1;
    permanent_page_count = 1;
    for (page = 0; page < last; ++page) {
        fread(permanent_pages[page].start, 1, 0xeffc, pch_file);
        new_permanent_page();
    }
    fread(permanent_pages[last].start, 1, 0xeffc - permanent_available, pch_file);
    tail = permanent_pages[last].start + 0xeffc - permanent_available;
    for (i = 0; i < permanent_available; ++i) tail[i] = 0;
}
