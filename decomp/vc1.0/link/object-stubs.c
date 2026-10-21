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
void attach_comdat_node(Module *module,Contribution *contribution,const char *name) { __asm { int 3 } }
void classify_contribution(Contribution *contribution,ImageOptions *options) { __asm { int 3 } }
void read_contribution_relocations(Image *image,Contribution *contribution,PIMAGE_SYMBOL symbols) { __asm { int 3 } }
char * format_module_name(Module *module,char *name) { __asm { int 3 } return 0; }
void process_directives(char *text,Image *image,const char *name,unsigned char library) { __asm { int 3 } }
ComdatRoot * add_comdat_root(const char *name,External *external,Contribution *contribution,ComdatRoot **list) { __asm { int 3 } return 0; }
