#ifndef CVPACK_OUTPUT_H
#define CVPACK_OUTPUT_H
#include "image.h"
#include "types.h"
#include <recon.h>

typedef struct PackedSignature {
    char signature[4];
    unsigned long directory;
} PackedSignature;
typedef char PackedSignatureSize[(sizeof(PackedSignature)==8)?1:-1];
extern unsigned char *output_buffer;
extern unsigned long output_capacity, output_used;
extern int output_handle;
extern PackedSignature packed_signature;
extern long packed_file_size;
extern unsigned long source_line_bytes;
extern unsigned char *source_file_index;
extern unsigned long source_file_index_size;
void *checksum_image(void *,unsigned long,unsigned long *,unsigned long *);
void initialize_output(int);
unsigned short write_output(const void *,unsigned short);
unsigned short flush_output(void);
void write_subsection(unsigned long);
unsigned long pad_symbols(void **,unsigned long);
void build_symbol_name_hash(SymbolHash,unsigned short *,unsigned long *,GrowingBuffer *,unsigned long,void **);
void build_symbol_address_hash(unsigned short *,unsigned long *,GrowingBuffer *,unsigned long,void **);
void *append_fill(GrowingBuffer *,unsigned char,unsigned long);
BufferBlock *first_buffer_block(const GrowingBuffer *);
BufferBlock *next_buffer_block(const BufferBlock *);
void write_global_publics(CvDirectoryEntry *,unsigned long);
void write_global_symbols(CvDirectoryEntry *,unsigned long);
void write_static_symbols(CvDirectoryEntry *,unsigned long);
void build_type_offsets(void);
unsigned long write_packed_types(void);
unsigned long write_old_packed_types(void);
int __cdecl compare_symbol_address(const void *,const void *);
int __cdecl compare_output_directory(const void *,const void *);
#endif
