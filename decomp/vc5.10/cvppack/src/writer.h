#ifndef CV510_WRITER_H
#define CV510_WRITER_H
#include "source-index.h"
#include <recon.h>

typedef struct PackedSignature {
    unsigned char magic[4];
    unsigned directory;
} PackedSignature;
extern PackedSignature packed_signature;
extern unsigned output_capacity, output_used;
extern unsigned char *output_buffer;
extern int output_file;
extern unsigned total_packed_source_bytes, packed_file_length;
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
