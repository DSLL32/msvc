/* Named import-library operations not yet reached by a recorded frontier. */
#include "import-library.h"
void read_definition_file(Image *image,const char *path) { __asm { int 3 } }
void write_helper_thunk_member(const ImportMachine *machine,Image *image) { __asm { int 3 } }
void record_export_ordinal(unsigned long ordinal) { __asm { int 3 } }
void find_export_matches(SymbolTable *table,const char *name,unsigned short *count,char **names,char **modules) { __asm { int 3 } }
void append_export_directive(const char *text) { __asm { int 3 } }
