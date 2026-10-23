#include "files.h"
#include "types.h"
#include "image.h"
#include "pdb.h"
#include <recon.h>

void warning(int code, const char *first, const char *second) { __asm { int 3 } }
int release_file_slot(FileSlot *slot) { __asm { int 3 } return 0; }
int open_mapped_file(FileSlot *slot, int mode, unsigned char created)
{ __asm { int 3 } return 0; }
int close_file(int handle, unsigned char release) { __asm { int 3 } return 0; }
void strip_debug(int handle) { __asm { int 3 } }
void write_debug_directory(void) { __asm { int 3 } }
void print_statistics(void) { __asm { int 3 } }
void free_type_tables(void) { __asm { int 3 } }
int update_image_checksum(const char *name, unsigned long length, unsigned char read_only)
{ __asm { int 3 } return 0; }


void grow_page_table(FileSlot *slot) { __asm { int 3 } }

Page *remove_lru_page(void) { __asm { int 3 } return NULL; }
void release_page(Page *page) { __asm { int 3 } }


int write_file(int handle, const void *buffer, unsigned long size) { __asm { int 3 } return -1; }
int truncate_file(int handle, long size) { __asm { int 3 } return -1; }
void read_old_directory(void) { __asm { int 3 } }
void read_packed_modules(void) { __asm { int 3 } }

long seek_mapped_file(FileSlot *slot, long offset, int origin) { __asm { int 3 } return -1; }

int read_mapped_file(FileSlot *slot, void *buffer, unsigned long size) { __asm { int 3 } return -1; }
void advance_file_page(FileSlot *slot) { __asm { int 3 } }

void read_old_module(unsigned long directory,ModuleInfo * module) { __asm { int 3 } }


const char *module_name(ModuleInfo * module) { __asm { int 3 } return NULL; }

void read_symbols(unsigned long directory,const CvDirectoryEntry * entry) { __asm { int 3 } }


void read_types(unsigned long directory,const CvDirectoryEntry * entry) { __asm { int 3 } }

void read_lines(unsigned long directory,const CvDirectoryEntry * entry) { __asm { int 3 } }


void read_source_module(unsigned long directory,const CvDirectoryEntry * entry) { __asm { int 3 } }

void save_precompiled(ModuleInfo * module) { __asm { int 3 } }

void prepare_symbols(void * symbols,unsigned long size) { __asm { int 3 } }

void pack_new_symbols(void * symbols,CvDirectoryEntry * entry) { __asm { int 3 } }


void prepare_old_types(void * types,unsigned long size) { __asm { int 3 } }

void pack_old_types(void * types,CvDirectoryEntry * entry,void * module_record,ModuleInfo * module) { __asm { int 3 } }

void index_new_types(void * types,unsigned long size,unsigned short * base,unsigned short * count) { __asm { int 3 } }

void pack_new_types(void * types,CvDirectoryEntry * entry,ModuleInfo * module,unsigned short * base,unsigned short * count) { __asm { int 3 } }

void pack_new_lines(void * lines,CvDirectoryEntry * entry,ModuleInfo * module) { __asm { int 3 } }

void pack_old_lines(void * lines,CvDirectoryEntry * entry,ModuleInfo * module) { __asm { int 3 } }

unsigned short __fastcall convert_new_type(unsigned short type) { __asm { int 3 } return 0; }

unsigned short __fastcall compact_old_type(unsigned short type) { __asm { int 3 } return 0; }
void finish_recursive_types(void) { __asm { int 3 } }

unsigned short numeric_leaf_size(const unsigned char *leaf) { __asm { int 3 } return 0; }
void read_old_global_types(void *types, unsigned long size) { __asm { int 3 } }
void load_precompiled_types(const unsigned char *record) { __asm { int 3 } }
void __fastcall add_old_type(const unsigned char *record, int is_new, int flag, int extra, unsigned short skip) { __asm { int 3 } }

int type_server_close(TypeServer *server) { __asm { int 3 } return 0; }
void *type_server_fill_alloc(unsigned long size, unsigned char value) { __asm { int 3 } return NULL; }
TypeServerRecord *__stdcall type_server_record_init(TypeServerRecord *node, TypeServerRecord *next, unsigned char *record, unsigned short type) { __asm { int 3 } return NULL; }
