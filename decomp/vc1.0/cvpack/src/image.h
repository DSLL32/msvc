#ifndef CVPACK_IMAGE_H
#define CVPACK_IMAGE_H
#include "cvpack.h"
#include <recon.h>

extern unsigned long debug_bytes_read;
extern long optional_header_offset;
extern long section_header_offset;
extern long original_file_size;
extern unsigned short cv_version;
extern long debug_section_offset;
extern unsigned long debug_directory_offset;
extern long cv_file_offset;
typedef struct CvDirectoryHeader {
    unsigned short header_size, entry_size;
    unsigned long count, next, flags;
} CvDirectoryHeader;

typedef struct CvDirectoryEntry {
    unsigned short subsection, module;
    long offset;
    unsigned long size;
} CvDirectoryEntry;

typedef char CvDirectoryHeaderSize[(sizeof(CvDirectoryHeader)==16)?1:-1];
typedef char CvDirectoryEntrySize[(sizeof(CvDirectoryEntry)==12)?1:-1];
extern CvDirectoryHeader cv_directory_header;
extern long cv_directory_offset;
extern CvDirectoryEntry *directory_entries;
extern unsigned long directory_entry_count;
typedef struct ModuleInfo {
    unsigned short module;
    unsigned char state[42];
    struct ModuleInfo *next;
} ModuleInfo;

typedef struct ModuleIndex {
    unsigned short module, reserved;
    unsigned long directory;
    ModuleInfo *info;
} ModuleIndex;

typedef char ModuleInfoSize[(sizeof(ModuleInfo)==48)?1:-1];
typedef char ModuleIndexSize[(sizeof(ModuleIndex)==12)?1:-1];
extern ModuleIndex *module_index;
extern ModuleInfo *module_list;
extern unsigned char *libraries, *segment_map, *segment_names;
extern unsigned long libraries_size, segment_map_size, segment_names_size;
void read_subsection(const CvDirectoryEntry *,unsigned char **,unsigned long *);
ModuleInfo *find_module(unsigned short,int);
int __cdecl compare_directory(const void *,const void *);
extern unsigned long directory_size;
extern unsigned long module_size;
extern unsigned long type_size;
extern unsigned long symbol_size;
extern unsigned long global_symbol_size;
extern unsigned long directory_capacity;
extern unsigned long module_capacity;
extern unsigned long type_capacity;
extern unsigned long symbol_capacity;
extern unsigned char ** global_symbol_blocks;
extern unsigned short global_symbol_block;
extern unsigned short global_symbol_block_count;

long seek_file(int, long, int);
int read_file(int, void *, unsigned long);
long tell_file(int);
int write_file(int, const void *, unsigned long);
int truncate_file(int, long);
extern unsigned char cv_signature[4];
unsigned short read_cv_signature(void);
void read_old_directory(void);
void read_new_directory(int);
void initialize_modules(void);
void read_packed_modules(void);
#endif
