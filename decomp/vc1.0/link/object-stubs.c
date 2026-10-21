/* Object operations not yet entered by an accepted frontier. */
#include "object.h"
#include "comdat.h"
void initialize_mac_object_symbols(unsigned long count) { __asm { int 3 } }
void finish_mac_object_symbols(void) { __asm { int 3 } }
void process_mac_object_symbols(SymbolTable *table,PIMAGE_SYMBOL symbols,char *strings) { __asm { int 3 } }
void process_object_symbols(Image *image,Module *module,unsigned char *new_externals,unsigned char *symbols_changed,PIMAGE_SYMBOL symbols,unsigned short member) { __asm { int 3 } }
void release_coff_strings(char *strings) { __asm { int 3 } }
void release_coff_symbols(PIMAGE_SYMBOL symbols) { __asm { int 3 } }
unsigned char select_comdat(SymbolTable *table,Contribution *contribution,PIMAGE_SYMBOL symbols,unsigned short section,char **name) { __asm { int 3 } return 0; }

PIMAGE_RELOCATION allocate_coff_relocations(unsigned long count) { __asm { int 3 } return 0; }
unsigned read_coff_relocations(int file,PIMAGE_RELOCATION records,unsigned long count) { __asm { int 3 } return 0; }
void release_coff_relocations(PIMAGE_RELOCATION records) { __asm { int 3 } }
void add_relocation_edge(SymbolTable *table,Contribution *contribution,PIMAGE_SYMBOL symbols,PIMAGE_RELOCATION relocation) { __asm { int 3 } }
void validate_mac_code_relocation(unsigned short type) { __asm { int 3 } }
void append_mac_fixup(MacFixupList *list,Contribution *contribution,unsigned long offset) { __asm { int 3 } }
void record_mac_symbol_reference(unsigned long symbol,Section *section,unsigned long flags) { __asm { int 3 } }
void record_mac_data_reference(Contribution *contribution,unsigned long offset,unsigned long symbol) { __asm { int 3 } }
