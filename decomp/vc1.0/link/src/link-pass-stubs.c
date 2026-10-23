/* Output operations not yet entered by accepted frontiers. */
#include "link-pass.h"
#include "codeview.h"
#include <recon.h>
char * replace_extension(const char *name,const char *extension) { __asm { int 3 } return 0; }
void sort_externals(SymbolTable *table) { __asm { int 3 } }
void read_absolute_symbols(const char *file,External **sorted,SymbolTable *table) { __asm { int 3 } }
void resolve_entry_symbol(SymbolTable *table) { __asm { int 3 } }
void dump_symbols(SymbolTable *table) { __asm { int 3 } }
void emit_rom_relocations(void) { __asm { int 3 } }









void end_comdat_node_enumerator(ComdatNodeEnumerator *scan) { __asm { int 3 }  }



void end_comdat_edge_enumerator(ComdatEdgeEnumerator *scan) { __asm { int 3 }  }



void end_comdat_root_enumerator(ComdatRootEnumerator *scan) { __asm { int 3 }  }



void adjust_rom_section_header(Section *section,IMAGE_SECTION_HEADER *header,Image *image) { __asm { int 3 } }




PIMAGE_NT_HEADERS checksum_mapped_image(void *image,unsigned long size,unsigned long *previous,unsigned long *checksum) { __asm { int 3 } return 0; }
