/* Unreconstructed application boundaries retain their original call slots. */
#include "files.h"
#include "types.h"
#include "image.h"
#include "pack.h"
#include <recon.h>

void write_debug_directory(void) { __asm { int 3 } }
void free_type_tables(void) { __asm { int 3 } }
void print_statistics(void) { __asm { int 3 } }
void flush_packed_file(void) { __asm { int 3 } }
unsigned int get_file_size(int file) { __asm { int 3 } return 0; }
void *map_file_region(int file, unsigned int offset, unsigned int size) { __asm { int 3 } return NULL; }
void close_file(int file, int release) { __asm { int 3 } }
void warning(unsigned int message, const char *first, const char *second) { __asm { int 3 } }

void open_stream_file(CachedFile *file, int flags, int mode, int created) { __asm { int 3 } }
void release_cached_file(CachedFile *file) { __asm { int 3 } }


int seek_stream_file(CachedFile *file, int offset, int origin) { __asm { int 3 } return 0; }
int grow_mapping(CachedFile *file, int size) { __asm { int 3 } return 0; }
unsigned stream_read(CachedFile *file, void *buffer, unsigned size) { __asm { int 3 } return 0; }
void initialize_single_module(CodeViewDirectoryEntry *entry, ModuleInfo *info) { __asm { int 3 } }
void read_module_types(CodeViewDirectoryEntry *entry, int precompiled) { __asm { int 3 } }
void prepare_precompiled_types(ModuleInfo *info) { __asm { int 3 } }
void read_module_symbols(unsigned index, CodeViewDirectoryEntry *entry) { __asm { int 3 } }
void read_module_lines(unsigned index, CodeViewDirectoryEntry *entry) { __asm { int 3 } }
void read_module_global_types(unsigned index, CodeViewDirectoryEntry *entry) { __asm { int 3 } }
void pack_symbols(void *symbols, CodeViewDirectoryEntry *entry) { __asm { int 3 } }
void decode_legacy_lines(void *lines, unsigned size) { __asm { int 3 } }
void rewrite_legacy_lines(void *lines, CodeViewDirectoryEntry *entry, void *module_buffer, ModuleInfo *info) { __asm { int 3 } }
void decode_new_lines(void *lines, unsigned size, unsigned short *symbols, unsigned short *references) { __asm { int 3 } }
void rewrite_new_lines(void *lines, CodeViewDirectoryEntry *entry, ModuleInfo *info, unsigned short *symbols, unsigned short *references) { __asm { int 3 } }
void rewrite_module_global_types(void *types, CodeViewDirectoryEntry *entry, ModuleInfo *info) { __asm { int 3 } }
