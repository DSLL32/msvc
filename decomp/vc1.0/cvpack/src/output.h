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
void initialize_output(int);
unsigned short write_output(const void *,unsigned short);
unsigned short flush_output(int);
void write_subsection(unsigned long);
void write_global_publics(CvDirectoryEntry *,unsigned long);
void write_global_symbols(CvDirectoryEntry *,unsigned long);
void write_static_symbols(CvDirectoryEntry *,unsigned long);
unsigned long write_packed_types(void);
unsigned long write_old_packed_types(void);
int __cdecl compare_output_directory(const void *,const void *);
#endif
