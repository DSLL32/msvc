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
typedef struct HashOutputBlock {
    unsigned used;
    unsigned char *data;
    struct HashOutputBlock *next;
} HashOutputBlock;
typedef struct HashOutputStream {
    /* The first five words are owned by the hash-builder routines. */
    unsigned state[5];
    HashOutputBlock *first;
} HashOutputStream;
unsigned insert_symbol_page_padding(PublicSymbol **buckets, unsigned reserve);
void build_symbol_name_hash(SymbolKeyProc key, unsigned short *method,
    unsigned *bytes, HashOutputStream *stream, unsigned count, PublicSymbol **buckets);
void build_symbol_address_hash(unsigned short *method, unsigned *bytes,
    HashOutputStream *stream, unsigned count, PublicSymbol **buckets);
HashOutputBlock *first_hash_output_block(HashOutputStream *stream);
HashOutputBlock *next_hash_output_block(HashOutputBlock *block);
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
unsigned write_cached_file(int file, const void *buffer, unsigned size);
void resize_cached_file(int file, unsigned size);
#endif
