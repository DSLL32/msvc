/* Object operations not yet entered by an accepted frontier. */
#include "object.h"
#include "comdat.h"
#include <recon.h>

void validate_mac_code_relocation(unsigned short type) { __asm { int 3 } }

void process_mac_static_symbol(Contribution *contribution,PIMAGE_SYMBOL symbol,unsigned long index) { __asm { int 3 } }
void format_input_reference(char *display,const char *library,const char *file) { __asm { int 3 } }
void process_weak_external(SymbolTable *table,PIMAGE_SYMBOL *cursor,PIMAGE_SYMBOL symbol,External *external,Module *module,unsigned char *new_externals,unsigned char created,unsigned char *imports,unsigned short member) { __asm { int 3 } }
void expand_hash(HashTable *table,void *context) { __asm { int 3 } }
void format_undefined_references(TextBuffer *buffer,External *external) { __asm { int 3 } }
