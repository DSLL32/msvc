#ifndef CVPACK_IMAGE_H
#define CVPACK_IMAGE_H
#include "cvpack.h"
#include <recon.h>

extern unsigned long cv_read_offset;
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
    union { long offset; unsigned char *data; } location;
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
    unsigned short reserved;
    unsigned char *object_name;
    unsigned long precompiled_signature;
    unsigned char *record;
    unsigned long record_size;
    unsigned char *symbols;
    unsigned long symbol_size;
    unsigned char state_after_symbols[16];
    struct ModuleInfo *next;
} ModuleInfo;

unsigned short address_in_module(ModuleInfo *, unsigned short, unsigned long);

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
extern unsigned char current_module_pe;
extern unsigned short module_type_base;
extern unsigned short module_type_count;
extern unsigned short module_symbol_count;
typedef struct UDTSymbol {
    struct UDTSymbol *next;
    unsigned short length, kind, type;
    unsigned char name[1];
} UDTSymbol;
extern UDTSymbol *module_udt_symbols;
extern unsigned short emit_udt_symbols;
extern int types_compacted;
extern unsigned long type_signature;
extern unsigned long type_directory;
extern unsigned long symbol_directory;
extern unsigned long public_directory;
extern unsigned long line_directory;
extern unsigned long old_line_directory;
extern int current_precompiled;
extern unsigned short module_flags;
extern ModuleInfo * current_module;
extern void * current_type_map;
extern unsigned char *module_record_buffer;
extern unsigned long total_new_type_bytes;
extern unsigned long total_public_bytes;

void read_old_module(unsigned long,ModuleInfo *);
void read_module(CvDirectoryEntry *,ModuleInfo *);
const char *module_name(ModuleInfo *);
void read_symbols(unsigned long,const CvDirectoryEntry *);
void read_global_types(const CvDirectoryEntry *,int);
void read_types(unsigned long,const CvDirectoryEntry *);
void read_lines(unsigned long,const CvDirectoryEntry *);
void read_publics(unsigned long,const CvDirectoryEntry *);
void read_source_module(unsigned long,const CvDirectoryEntry *);
void save_precompiled(ModuleInfo *);
void prepare_symbols(void *,unsigned long);
void pack_new_symbols(void *,CvDirectoryEntry *);
void pack_public_symbols(void *,CvDirectoryEntry *);
void prepare_old_types(void *,unsigned long);
void pack_old_types(void *,CvDirectoryEntry *,void *,ModuleInfo *);
void index_new_types(void *,unsigned long,unsigned short *,unsigned short *);
void pack_new_types(void *,CvDirectoryEntry *,ModuleInfo *,unsigned short *,unsigned short *);
void pack_new_lines(void *,CvDirectoryEntry *,ModuleInfo *);
void pack_old_lines(void *,CvDirectoryEntry *,ModuleInfo *);
extern unsigned short subsection_reads;
typedef unsigned short (__cdecl *SymbolHash)(const unsigned char *,unsigned long *,unsigned short *,unsigned short *,unsigned long *,unsigned short);
unsigned short __fastcall convert_old_type(unsigned short);
unsigned short __fastcall convert_new_type(unsigned short);
int intern_public(unsigned char *,SymbolHash);
int __fastcall intern_global(unsigned char *,SymbolHash);
unsigned short __cdecl hash_public(const unsigned char *,unsigned long *,unsigned short *,unsigned short *,unsigned long *,unsigned short);
extern unsigned long symbol_scope_depth;
typedef struct PublicSymbol {
    struct PublicSymbol *next;
    unsigned short name_offset, reserved;
    unsigned long hash, offset;
    unsigned short segment;
    unsigned char record[1];
} PublicSymbol;
void *allocate_symbol(unsigned long);
extern unsigned long public_symbols_count, public_symbols_size, duplicate_symbols;
unsigned short numeric_leaf_size(const unsigned char *);
extern unsigned long public_symbol_available;
extern unsigned char *public_symbol_cursor;
extern unsigned long module_type_bytes;
extern int module_types_indexed;
extern unsigned short first_user_type, local_type_count, previous_type_count;
void read_old_global_types(void *, unsigned long);
void read_new_global_types(unsigned long, int);
void reset_type_hash(void);
void reset_local_types(void);
void load_precompiled_types(const unsigned char *);
void import_type_server(const unsigned char *);
void __fastcall add_old_type(const unsigned char *, int, int, int, unsigned short);
#endif
