/* Named import-library operations not yet reached by a recorded frontier. */
#include "import-library.h"
void read_definition_file(Image *image,const char *path) { __asm { int 3 } }
void parse_export(const char *text,Image *image,unsigned char directives,const char *context) { __asm { int 3 } }
void merge_export_definitions(SymbolTable *exports,SymbolTable *linked,unsigned char directives,Library *libraries) { __asm { int 3 } }
void prepare_export_order(SymbolTable *table) { __asm { int 3 } }
unsigned long count_export_symbols(SymbolTable *table,unsigned long *imports) { __asm { int 3 } return 0; }
unsigned long count_export_name_bytes(SymbolTable *table) { __asm { int 3 } return 0; }
void assign_export_ordinals(SymbolTable *table,unsigned char *ordinals) { __asm { int 3 } }
void write_import_archive_indexes(unsigned long members,SymbolTable *table) { __asm { int 3 } }
void write_import_descriptor(const ImportMachine *machine,Image *image) { __asm { int 3 } }
void write_null_import_descriptor(Image *image) { __asm { int 3 } }
void write_null_thunk_member(Image *image) { __asm { int 3 } }
void write_helper_thunk_member(const ImportMachine *machine,Image *image) { __asm { int 3 } }
void write_export_object(Image *image,unsigned long slots,unsigned long exports,const ImportMachine *machine,unsigned char linked) { __asm { int 3 } }
void write_import_members(Image *image,const ImportMachine *machine) { __asm { int 3 } }
void finish_import_archive_indexes(SymbolTable *table) { __asm { int 3 } }
