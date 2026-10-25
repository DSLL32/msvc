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























void prepare_old_types(void * types,unsigned long size) { __asm { int 3 } }

void pack_old_types(void * types,CvDirectoryEntry * entry,void * module_record,ModuleInfo * module) { __asm { int 3 } }



void pack_new_lines(void * lines,CvDirectoryEntry * entry,ModuleInfo * module) { __asm { int 3 } }









void *type_server_fill_alloc(unsigned long size, unsigned char value) { __asm { int 3 } return NULL; }
TypeServerRecord *__stdcall type_server_record_init(TypeServerRecord *node, TypeServerRecord *next, unsigned char *record, unsigned short type) { __asm { int 3 } return NULL; }






























void __fastcall queue_type_match(unsigned short type, TypeCacheEntry *named) { __asm { int 3 } }






void *allocate_small_type(void) { __asm { int 3 } return NULL; }
void recycle_small_type(void *buffer) { __asm { int 3 } }


void convert_old_pointer(TypeRecord *entry) { __asm { int 3 } }
void convert_old_based_pointer(TypeRecord *entry) { __asm { int 3 } }
void convert_old_structure(TypeRecord *entry) { __asm { int 3 } }
void convert_old_array(TypeRecord *entry) { __asm { int 3 } }
void convert_old_procedure(TypeRecord *entry) { __asm { int 3 } }
void convert_old_leaf_80(TypeRecord *entry) { __asm { int 3 } }
void convert_old_leaf_5c(TypeRecord *entry) { __asm { int 3 } }
void convert_old_leaf_72(TypeRecord *entry) { __asm { int 3 } }
void convert_old_leaf_8d(TypeRecord *entry) { __asm { int 3 } }
void convert_old_leaf_8e(TypeRecord *entry) { __asm { int 3 } }
void convert_old_barray(TypeRecord *entry) { __asm { int 3 } }
void convert_old_modifier(TypeRecord *entry) { __asm { int 3 } }
void convert_old_leaf_a7(TypeRecord *entry) { __asm { int 3 } }
