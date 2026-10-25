#include "files.h"
#include "types.h"
#include "image.h"
#include "pdb.h"
#include "strip.h"
#include "output.h"
#include <recon.h>




void cache_closed_file(FileSlot *slot) { __asm { int 3 } }














unsigned long write_old_packed_types(void) { __asm { int 3 } return 0; }





void grow_page_table(FileSlot *slot) { __asm { int 3 } }







void read_packed_modules(void) { __asm { int 3 } }

void grow_mapping(FileSlot *slot, long size) { __asm { int 3 } }












void read_lines(unsigned long directory,const CvDirectoryEntry * entry) { __asm { int 3 } }





void prepare_symbols(void * symbols,unsigned long size) { __asm { int 3 } }

void pack_new_symbols(void * symbols,CvDirectoryEntry * entry) { __asm { int 3 } }


void prepare_old_types(void * types,unsigned long size) { __asm { int 3 } }

void pack_old_types(void * types,CvDirectoryEntry * entry,void * module_record,ModuleInfo * module) { __asm { int 3 } }



void pack_new_lines(void * lines,CvDirectoryEntry * entry,ModuleInfo * module) { __asm { int 3 } }




unsigned short __fastcall compact_old_type(unsigned short type) { __asm { int 3 } return 0; }

void read_old_global_types(void *types, unsigned long size) { __asm { int 3 } }


void *type_server_fill_alloc(unsigned long size, unsigned char value) { __asm { int 3 } return NULL; }
TypeServerRecord *__stdcall type_server_record_init(TypeServerRecord *node, TypeServerRecord *next, unsigned char *record, unsigned short type) { __asm { int 3 } return NULL; }






























void __fastcall queue_type_match(unsigned short type, TypeCacheEntry *named) { __asm { int 3 } }






void *allocate_small_type(void) { __asm { int 3 } return NULL; }
void recycle_small_type(void *buffer) { __asm { int 3 } }
