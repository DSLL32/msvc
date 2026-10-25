#define RECON_SHIM_IMPL
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <recon-tool.h>
#include "types.h"
#include "pack.h"
#include <recon.h>
#undef main

/* Native-only focused duplicate/conflict branches, not an original oracle. */

extern unsigned global_symbol_bytes, global_symbol_count, duplicate_public_symbols;

static int run_global_symbol(void)
{
    unsigned char initial[] = "\027\000\003\020\024\020\000\000\020tagRPCOLEMESSAGE";
    unsigned char record[sizeof(initial)];
    type_table_local = calloc(4096, sizeof(void *));
    memcpy(record, initial, sizeof(initial));
    if (insert_global_symbol(record, symbol_record_key) != 1 ||
        *(unsigned short *)(record + 2) != 8 ||
        global_symbol_count != 1 || global_symbol_bytes != 28) return 120;
    memcpy(record, initial, sizeof(initial));
    if (insert_global_symbol(record, symbol_record_key) ||
        *(unsigned short *)(record + 2) != 8 || duplicate_public_symbols != 1 ||
        global_symbol_count != 1 || global_symbol_bytes != 28) return 121;
    memcpy(record, initial, sizeof(initial));
    ++*(unsigned *)(record + 4);
    if (insert_global_symbol(record, symbol_record_key) != 2 ||
        *(unsigned short *)(record + 2) != 0x1003 || duplicate_public_symbols != 1 ||
        global_symbol_count != 1 || global_symbol_bytes != 28) return 122;
    return 0;
}

static int test_global_symbol(void) { return run_global_symbol()==0; }

/* Native-only output ownership examples, not replacement original oracles. */

static int run_lookup_type(void)
{
    TypeRecord records[3], sentinel;
    TypeRecord *result;
    unsigned output;
    memset(records, 0, sizeof(records));
    memset(&sentinel, 0, sizeof(sentinel));
    type_records = records;
    module_type_base = 0x1000;
    module_type_low = 1;
    empty_type_record = &sentinel;
    records[0].status = records[1].status = 0x1000;
    records[0].global_index = 0x1001;
    records[1].global_index = 0x1002;
    output = 0xfeedface;
    result = lookup_global_type(0, &output);
    if (result != &records[2] || output != 0x1001) return 1;
    output = 0xfeedface;
    result = lookup_global_type(2, &output);
    if (result != &records[2] || output) return 2;
    module_type_low = 0;
    output = 0xfeedface;
    result = lookup_global_type(0xffff - module_type_base, &output);
    if (result != &sentinel || output) return 3;
    puts("recursive-parent-output direct-zero sentinel-zero");
    return 0;
}

static int test_lookup_type(void) { return run_lookup_type()==0; }

/* Native-only recursive comparison examples; not additional original oracles. */

static int run_recursive_compare(void)
{
    TypeRecord records[2];
    unsigned char data[12], other[12], child[8];
    unsigned char *block[512];
    memset(records, 0, sizeof(records));
    memset(data, 0, sizeof(data));
    memset(other, 0, sizeof(other));
    memset(child, 0, sizeof(child));
    memset(block, 0, sizeof(block));
    type_records = records;
    module_type_base = 0x1000;
    module_type_low = 1;
    global_type_blocks[0] = block;
    records[0].data = data;
    records[1].data = child;
    *(unsigned short *)data = *(unsigned short *)other = 10;
    if (!compare_recursive_type(&records[0], 0x1000, other, 0x1002) ||
        records[0].status != 2 || records[0].mapped_index != 0x1002) return 1;
    other[11] = 1;
    if (compare_recursive_type(&records[0], 0x1000, other, 0x1002) ||
        (records[0].status & 1)) return 2;
    other[11] = 0;
    records[0].status = 0;
    records[0].reference_count = 1;
    records[0].reference_offsets[0] = 4;
    *(unsigned *)(data + 4) = 0x1001;
    *(unsigned *)(other + 4) = 0x1002;
    block[2] = child;
    *(unsigned short *)child = 6;
    if (!compare_recursive_type(&records[0], 0x1000, other, 0x1003) ||
        records[0].status != 2 || records[1].status != 2 ||
        records[1].mapped_index != 0x1002) return 3;
    records[0].status = 0;
    records[1].status = 1;
    records[1].mapped_index = 0x1003;
    if (compare_recursive_type(&records[0], 0x1000, other, 0x1003) ||
        (records[0].status & 1)) return 4;
    puts("equal different recursive conflicting-alias");
    return 0;
}

static int test_recursive_compare(void) { return run_recursive_compare()==0; }

/* Native-only hash/storage boundaries, not a replacement original oracle. */

static int run_recursive_hash(void)
{
    TypeRecord record;
    unsigned char data[48], flags = 0xe0;
    unsigned short offsets[6] = { 4, 12, 20, 28, 36, 44 };
    unsigned i, named, generic, extended, cached;
    memset(&record, 0, sizeof(record));
    memset(data, 0, sizeof(data));
    *(unsigned short *)data = 27;
    *(unsigned short *)(data + 2) = 0x1005;
    data[22] = 6;
    memcpy(data + 23, "Widget", 6);
    record.data = data;
    named = hash_recursive_type(&record);
    memset(&record, 0, sizeof(record));
    for (i = 0; i < sizeof(data); ++i) data[i] = (unsigned char)i;
    *(unsigned short *)data = 14;
    *(unsigned short *)(data + 2) = 0x1001;
    record.data = data;
    record.reference_count = 1;
    record.reference_offsets[0] = 8;
    record.reference_offsets[8] = 0x80;
    generic = hash_recursive_type(&record);
    memset(&record, 0, sizeof(record));
    *(unsigned short *)data = 46;
    record.data = data;
    record.reference_count = 6;
    record.extended_references = offsets;
    record.reference_flags = &flags;
    record.flags = 1;
    extended = hash_recursive_type(&record);
    record.data = NULL;
    record.recursive_hash = 0xabc;
    cached = hash_recursive_type(&record);
    if (named != 3769u || generic != 3682u || extended != 2374u || cached != 2748u) return 1;
    return 0;
}

static int test_recursive_hash(void) { return run_recursive_hash()==0; }

/* Native-only source-line ownership example, not additional original coverage. */
static int run_source_lines(void)
{
    unsigned char input[7] = { 1, 3, 5, 7, 9, 11, 13 };
    CodeViewDirectoryEntry entry;
    ModuleInfo info;
    memset(&entry, 0, sizeof(entry));
    memset(&info, 0, sizeof(info));
    entry.subsection = 0x127;
    entry.module = info.module = 2;
    entry.size = sizeof(input);
    rewrite_module_global_types(input, &entry, &info);
    if (entry.data == input || entry.data != info.source_lines ||
        entry.size != sizeof(input) || info.source_lines_size != sizeof(input) ||
        memcmp(entry.data, input, sizeof(input)) || info.symbols || info.symbols_size ||
        info.precompiled_types || info.precompiled_bytes || info.next) return 1;
    puts("source-lines copied-all-bytes directory-module-alias");
    return 0;
}

static int test_source_lines(void) { return run_source_lines()==0; }

/* Native-only initialized fields/own padding/list examples, not original oracles. */
extern unsigned public_arena_remaining;
extern unsigned char *public_arena_cursor;

static int run_symbol_reference(void)
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

static int test_symbol_reference(void) { return run_symbol_reference()==0; }

/* Native-only scope stack/list examples, not additional original oracles. */

static int run_symbol_scopes(void)
{
    unsigned char buffer[76];
    unsigned offsets[7] = { 4, 16, 32, 48, 52, 56, 72 };
    unsigned short lengths[7] = { 10, 14, 14, 2, 2, 14, 2 };
    unsigned short kinds[7] = { 5, 0x100a, 0x107, 6, 6, 0x100b, 6 };
    unsigned i;
    memset(buffer, 0, sizeof(buffer));
    for (i = 0; i < 7; ++i) {
        *(unsigned short *)(buffer + offsets[i]) = lengths[i];
        *(unsigned short *)(buffer + offsets[i] + 2) = kinds[i];
    }
    if (!link_symbol_scopes(buffer, sizeof(buffer)) ||
        *(unsigned *)(buffer + 8) != 16 ||
        *(unsigned *)(buffer + 20) || *(unsigned *)(buffer + 24) != 52 ||
        *(unsigned *)(buffer + 28) != 56 ||
        *(unsigned *)(buffer + 36) != 16 || *(unsigned *)(buffer + 40) != 48 ||
        *(unsigned *)(buffer + 60) || *(unsigned *)(buffer + 64) != 72) return 1;
    *(unsigned *)(buffer + 64) = 0;
    if (link_symbol_scopes(buffer, 72)) return 2;
    memset(buffer, 0, 8);
    *(unsigned short *)(buffer + 4) = 2;
    *(unsigned short *)(buffer + 6) = 6;
    if (!link_symbol_scopes(buffer, 8)) return 3;
    puts("nested next missing-end unmatched-end");
    return 0;
}

static int test_symbol_scopes(void) { return run_symbol_scopes()==0; }

/* Native-only reference storage boundaries; not a new original oracle. */

static int run_type_reference(void)
{
    TypeRecord record;
    unsigned i;
    memset(&record, 0, sizeof(record));
    record.reference_offsets[8] = 0xa5;
    for (i = 0; i < 5; ++i) add_type_reference(&record, (unsigned short)(i + 1));
    if (record.reference_count != 5 || record.flags & 1) return 120;
    add_type_reference(&record, 260);
    if (record.reference_count != 6 || !(record.flags & 1) ||
        record.reference_flags[0] != 0xa5 || record.extended_references[5] != 260) return 121;
    for (i = 0; i < 5; ++i) if (record.extended_references[i] != i + 1) return 122;
    for (i = 6; i < 40; ++i) add_type_reference(&record, (unsigned short)(i + 256));
    record.reference_flags[0] = 0xa5;
    add_type_reference(&record, 296);
    if (record.reference_count != 41 || record.reference_flags[0]) return 123;
    for (i = 6; i <= 40; ++i) if (record.extended_references[i] != i + 256) return 124;
    memset(&record, 0, sizeof(record));
    add_type_reference(&record, 256);
    if (record.reference_count != 1 || !(record.flags & 1) ||
        record.extended_references[0] != 256) return 125;
    return 0;
}

static int test_type_reference(void) { return run_type_reference()==0; }

/* Native-only nonzero identity control; not an original behavior oracle. */

static int run_type_flag(void)
{
    TypeRecord record;
    memset(&record, 0, sizeof(record));
    discard_types = 1;
    record.mapped_index = 12345;
    allocate_global_type(&record);
    if (record.mapped_index || record.status || discard_types != 1) return 120;
    type_records = &record;
    type_record_count = 1;
    module_type_base = 0x1000;
    record.mapped_index = 12345;
    if (compact_type_index(0x1000) || record.mapped_index || record.status != 2) return 121;
    record.mapped_index = 12345;
    record.status = 0;
    if (compact_legacy_index(0x200) || record.mapped_index || record.status != 2) return 122;
    return discard_types == 1 ? 0 : 123;
}

static int test_type_flag(void) { return run_type_flag()==0; }

/* Native-only unresolved-kind control, outside the reconstructed graph. */

static int run_pointer_gap(int argc, char **argv)
{
    unsigned char bytes[32];
    TypeRecord record;
    memset(bytes, 0, sizeof(bytes));
    memset(&record, 0, sizeof(record));
    *(unsigned short *)bytes = 18;
    *(unsigned short *)(bytes + 2) = 0x1002;
    *(unsigned *)(bytes + 4) = 0x74;
    *(unsigned *)(bytes + 8) = argc > 1 ? 4 : 10;
    *(unsigned short *)(bytes + 14) = 0xffff;
    record.data = bytes;
    module_type_low = 2;
    compact_legacy_pointer(&record);
    if (argc > 1) return 120;
    return *(unsigned *)(bytes + 4) == 0x74 ? 0 : 121;
}

static int test_pointer_gap(void) { return run_pointer_gap(1,(char **)0)==0; }

static int test_unsupported_pointer_kind(void)
{ int status,pid=fork(); if(pid<0)return 0; if(!pid){run_pointer_gap(2,(char **)0);_exit(99);} return waitpid(pid,&status,0)==pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGTRAP; }
/* Native initial scalar tags, independently retained from the original table;
 * callback identities are checked by the common strided-table audit. */
static int test_callback_table_tags(void)
{
 static const unsigned short kinds[39] = {
    0x1006,0x0207,0x100a,0x100b,0x0006,0x1007,0x1008,0x1009,0x0209,0x1001,0x100e,0x100f,0x020f,0x0100,0x0107,0x0104,0x0105,0x0101,0x0102,0x0103,0x0109,0x1003,0x100d,0x1010,0x1011,0x000a,0x010c,0x0108,0x0208,0x0106,0x0206,0x010a,0x020a,0x1002,0x0005,0x0007,0x0001,0x0009,0x1000
 };
 unsigned i;
 for(i=0;i<39;++i)
  if(symbol_rewriters.entries[i].kind!=kinds[i] || symbol_rewriters.entries[i].reserved) return 0;
 return 1;
}
/* The last record must start on the next 48K page, leaving four zero-fill
 * bytes. Verify the destructive pointer-to-offset slot transition directly. */
extern void prepare_type_offsets(void);
static int test_type_offset_page(void)
{
    unsigned char first[8], middle[0xbff4], last[8];
    unsigned char *slots[3];
    *(unsigned short *)first = sizeof(first) - 2;
    *(unsigned short *)middle = sizeof(middle) - 2;
    *(unsigned short *)last = sizeof(last) - 2;
    slots[0] = first; slots[1] = middle; slots[2] = last;
    global_type_blocks[0] = slots;
    symbol_rewriters.state.next_global_type = 0x1003;
    prepare_type_offsets();
    return (unsigned)slots[0] == 0 && (unsigned)slots[1] == 8 &&
        (unsigned)slots[2] == 0xc000 && *(unsigned short *)first == 6 &&
        *(unsigned short *)middle == 0xbff2 && *(unsigned short *)last == 6;
}
const ReconTest cvpack_tests[13] = {
 {"global-symbol",1,test_global_symbol},
 {"lookup-type",1,test_lookup_type},
 {"recursive-compare",1,test_recursive_compare},
 {"recursive-hash",1,test_recursive_hash},
 {"source-lines",1,test_source_lines},
 {"symbol-reference",1,test_symbol_reference},
 {"symbol-scopes",1,test_symbol_scopes},
 {"type-reference",1,test_type_reference},
 {"type-flag",1,test_type_flag},
 {"pointer-gap",1,test_pointer_gap},
 {"callback-table-tags",1,test_callback_table_tags},
 {"unsupported-pointer-kind",1,test_unsupported_pointer_kind},
 {"type-offset-page",1,test_type_offset_page}
};
