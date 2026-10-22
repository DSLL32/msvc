/* Object operations not yet entered by an accepted frontier. */
#include "object.h"
#include "comdat.h"
void initialize_mac_object_symbols(unsigned long count) { __asm { int 3 } }
void finish_mac_object_symbols(void) { __asm { int 3 } }
void process_mac_object_symbols(SymbolTable *table,PIMAGE_SYMBOL symbols,char *strings) { __asm { int 3 } }

void validate_mac_code_relocation(unsigned short type) { __asm { int 3 } }
void append_mac_fixup(MacFixupList *list,Contribution *contribution,unsigned long offset) { __asm { int 3 } }
void record_mac_symbol_reference(unsigned long symbol,Section *section,unsigned long flags) { __asm { int 3 } }
void record_mac_data_reference(Contribution *contribution,unsigned long offset,unsigned long symbol) { __asm { int 3 } }

void process_mac_static_symbol(Contribution *contribution,PIMAGE_SYMBOL symbol,unsigned long index) { __asm { int 3 } }
void record_mac_external(External *external,unsigned long index) { __asm { int 3 } }
void format_input_reference(char *display,const char *library,const char *file) { __asm { int 3 } }
void process_weak_external(SymbolTable *table,PIMAGE_SYMBOL *cursor,PIMAGE_SYMBOL symbol,External *external,Module *module,unsigned char *new_externals,unsigned char created,unsigned char *imports,unsigned short member) { __asm { int 3 } }
unsigned read_coff_symbol(int file,PIMAGE_SYMBOL symbol) { __asm { int 3 } return 0; }
void expand_hash(HashTable *table,void *context) { __asm { int 3 } }
void format_undefined_references(TextBuffer *buffer,External *external) { __asm { int 3 } }
