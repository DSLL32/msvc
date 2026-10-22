/* Named import-library operations not yet reached by a recorded frontier. */
#include "import-library.h"
void read_definition_file(Image *image,const char *path) { __asm { int 3 } }
void write_import_descriptor(const ImportMachine *machine,Image *image) { __asm { int 3 } }
void write_null_import_descriptor(Image *image) { __asm { int 3 } }
void write_null_thunk_member(Image *image) { __asm { int 3 } }
void write_helper_thunk_member(const ImportMachine *machine,Image *image) { __asm { int 3 } }
void write_export_object(Image *image,unsigned long slots,unsigned long exports,const ImportMachine *machine,unsigned char linked) { __asm { int 3 } }
void write_import_members(Image *image,const ImportMachine *machine) { __asm { int 3 } }
void finish_import_archive_indexes(SymbolTable *table) { __asm { int 3 } }
void record_export_ordinal(unsigned long ordinal) { __asm { int 3 } }
void find_export_matches(SymbolTable *table,const char *name,unsigned short *count,char **names,char **modules) { __asm { int 3 } }
