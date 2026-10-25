#ifndef CV510_WRITER_H
#define CV510_WRITER_H
#include "source-index.h"
#include <recon.h>

typedef struct PackedSignature {
    unsigned char magic[4];
    unsigned directory;
} PackedSignature;
/* Packed CodeView symbol-hash header and the block streams used by writers. */
typedef struct SymbolHashHeader {
    unsigned short name_method, address_method;
    unsigned symbol_bytes, name_bytes, address_bytes;
} SymbolHashHeader;
typedef struct NameHashPair {
    unsigned offset, hash;
} NameHashPair;
typedef struct NameHashOverflow {
    struct NameHashOverflow *next;
    NameHashPair entries[5];
} NameHashOverflow;
typedef struct NameHashBucket {
    unsigned short count, reserved;
    NameHashOverflow *overflow;
    NameHashPair entries[12];
} NameHashBucket;
typedef struct AddressHashEntry {
    PublicSymbol *symbol;
    unsigned stream_offset;
} AddressHashEntry;
typedef struct AddressHashGroup {
    int count, written;
    AddressHashEntry *entries;
} AddressHashGroup;
int __cdecl compare_symbol_address(const void *first, const void *second);
unsigned char *fill_type_arena(TypeArena *arena, unsigned value, unsigned size);
unsigned insert_symbol_page_padding(PublicSymbol **buckets, unsigned reserve);
void build_symbol_name_hash(SymbolKeyProc key, unsigned short *method,
    unsigned *bytes, TypeArena *stream, unsigned count, PublicSymbol **buckets);
void build_symbol_address_hash(unsigned short *method, unsigned *bytes,
    TypeArena *stream, unsigned count, PublicSymbol **buckets);
TypeArenaBlock *first_hash_output_block(TypeArena *stream);
TypeArenaBlock *next_hash_output_block(TypeArenaBlock *block);
extern PackedSignature packed_signature;
extern unsigned output_capacity, output_used;
extern unsigned char *output_buffer;
extern int output_file;
extern unsigned total_packed_source_bytes, packed_file_length;
extern unsigned output_padding_size;
extern unsigned char output_padding[4];
void begin_buffered_output(int file);
unsigned short buffered_write(const void *data, unsigned size);
unsigned short end_buffered_output(void);
int __cdecl compare_packed_directory_entries(const void *first, const void *second);
void write_subsection(unsigned index);
void write_publics(CodeViewDirectoryEntry *entry, unsigned offset);
void write_globals(CodeViewDirectoryEntry *entry, unsigned offset);
void write_static_symbols(CodeViewDirectoryEntry *entry, unsigned offset);
unsigned write_global_types(void);
void prepare_type_offsets(void);
unsigned write_cached_file(int file, const void *buffer, unsigned size);
int resize_cached_file(int file, unsigned size);
#endif
