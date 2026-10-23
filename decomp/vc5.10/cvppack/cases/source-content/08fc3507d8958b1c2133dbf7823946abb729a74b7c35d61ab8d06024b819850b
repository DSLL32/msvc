/* Unreconstructed application boundaries retain their original call slots. */
#include "files.h"
#include "types.h"
#include "image.h"
#include "pack.h"
#include "widen.h"
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
void prepare_precompiled_types(ModuleInfo *info) { __asm { int 3 } }


void decode_legacy_lines(void *lines, unsigned size) { __asm { int 3 } }
void rewrite_legacy_lines(void *lines, CodeViewDirectoryEntry *entry, void *module_buffer, ModuleInfo *info) { __asm { int 3 } }
void read_type_database(const char *path) { __asm { int 3 } }
unsigned short write_segment_record(unsigned char *buffer, unsigned short segment) { __asm { int 3 } return 0; }
int link_symbol_scopes(unsigned char *buffer, unsigned size) { __asm { int 3 } return 0; }
void record_symbol_references(unsigned char *buffer, unsigned size, unsigned short module) { __asm { int 3 } }
void enter_symbol_scope(void) { __asm { int 3 } }
void leave_symbol_scope(void) { __asm { int 3 } }
void rewrite_symbol_type4(void) { __asm { int 3 } }
void rewrite_bp_relative_symbol(void) { __asm { int 3 } }
void rewrite_global_data_symbol(void) { __asm { int 3 } }
void rewrite_register16_symbol(void) { __asm { int 3 } }
void rewrite_data16_symbol(void) { __asm { int 3 } }
void rewrite_global_data16_symbol(void) { __asm { int 3 } }
void rewrite_type_name_symbol(void) { __asm { int 3 } }
void rewrite_object_name_symbol(void) { __asm { int 3 } }
void rewrite_procedure16_symbol(void) { __asm { int 3 } }
void rewrite_procedure32_symbol(void) { __asm { int 3 } }
void rewrite_mips_procedure_symbol(void) { __asm { int 3 } }
void rewrite_module_global_types(void *types, CodeViewDirectoryEntry *entry, ModuleInfo *info) { __asm { int 3 } }

const char *module_name(ModuleInfo *info) { __asm { int 3 } return NULL; }

int create_widen(WidenTi **object) { __asm { int 3 } return 0; }
void release_widen(WidenTi *object) { __asm { int 3 } }
int query_widen(WidenTi *object, WidenOutput *output, const void *input, unsigned size) { __asm { int 3 } return 0; }
int emit_widen(WidenTi *object, WidenOutput *output, const void *input, unsigned size, unsigned flags) { __asm { int 3 } return 0; }
void *resize_bytes(void *buffer, unsigned size) { __asm { int 3 } return NULL; }




TypeRecord *lookup_global_type(unsigned index, unsigned char *from_global) { __asm { int 3 } return NULL; }
void convert_legacy_record(unsigned index) { __asm { int 3 } }
unsigned char __fastcall compact_legacy_pointer(TypeRecord *record) { __asm { int 3 } return 0; }
int __fastcall match_forward_type(TypeRecord *record, TypeRecord **match, int insert) { __asm { int 3 } return 0; }
void __fastcall associate_forward_type(unsigned index, TypeRecord *record) { __asm { int 3 } }
unsigned __fastcall compact_type_list(unsigned index, unsigned count) { __asm { int 3 } return 0; }
unsigned resolve_recursive_type(TypeRecord *record, unsigned index) { __asm { int 3 } return 0; }
void __fastcall intern_type(TypeRecord *record) { __asm { int 3 } }



unsigned short numeric_leaf_size(const unsigned char *record) { __asm { int 3 } return 0; }

void read_legacy_module_types(void *types, unsigned size) { __asm { int 3 } }



int record_widen(WidenTi *object, void *output, const void *input, unsigned *size) { __asm { int 3 } return 0; }
void use_precompiled_types(void *leaf) { __asm { int 3 } }
void register_type_server(void *leaf) { __asm { int 3 } }
void __fastcall find_type_server(void *name, unsigned mode, void *first, void *second, void *third) { __asm { int 3 } }
