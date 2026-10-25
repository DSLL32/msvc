#include "files.h"
#include "types.h"
#include "image.h"
#include "pdb.h"
#include "strip.h"
#include "output.h"
#include "oldsymbols.h"
#include <recon.h>




void cache_closed_file(FileSlot *slot) { __asm { int 3 } }














unsigned long write_old_packed_types(void) { __asm { int 3 } return 0; }













void read_packed_modules(void) { __asm { int 3 } }

void grow_mapping(FileSlot *slot, long size) { __asm { int 3 } }







































void *type_server_fill_alloc(unsigned long size, unsigned char value) { __asm { int 3 } return NULL; }
TypeServerRecord *__stdcall type_server_record_init(TypeServerRecord *node, TypeServerRecord *next, unsigned char *record, unsigned short type) { __asm { int 3 } return NULL; }






























void __fastcall queue_type_match(unsigned short type, TypeCacheEntry *named) { __asm { int 3 } }




















void convert_old_barray(TypeRecord *entry) { __asm { int 3 } }














void size_old_block32(void) { __asm { int 3 } }


void size_old_label32(void) { __asm { int 3 } }

void size_old_register_relative32(void) { __asm { int 3 } }




void size_old_bprel32(void) { __asm { int 3 } }



void size_old_data32(void) { __asm { int 3 } }

void size_old_procedure32(void) { __asm { int 3 } }




void convert_old_symbol_block32(void) { __asm { int 3 } }

void convert_old_symbol_procedure32(void) { __asm { int 3 } }


void convert_old_symbol_bprel32(void) { __asm { int 3 } }

void convert_old_symbol_data32(void) { __asm { int 3 } }

void convert_old_symbol_label32(void) { __asm { int 3 } }



void convert_old_symbol_register_relative32(void) { __asm { int 3 } }










unsigned char *get_old_symbol(unsigned short offset) { __asm { int 3 } return 0; }
unsigned short convert_old_based_symbol(unsigned char *output,unsigned char *input) { __asm { int 3 } return 0; }
