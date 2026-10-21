/* Pass-one operations not yet entered by accepted cases. */
#include "pass-one.h"
void reproduce_input(const char *name,unsigned char input) { __asm { int 3 } }
void check_explicit_modules(Image *image,Library *library) { __asm { int 3 } }
void resolve_weak_externals(SymbolTable *symbols,unsigned long flags) { __asm { int 3 } }
void infer_subsystem(Image *image) { __asm { int 3 } }
void search_libraries(Image *image) { __asm { int 3 } }
unsigned char create_import_library(Image *image,const char *library,const char *tool) { __asm { int 3 } return 0; }
void warn_unused_libraries(Library *library) { __asm { int 3 } }
void apply_section_overrides(Image *image) { __asm { int 3 } }
void apply_section_merges(Image *image) { __asm { int 3 } }
void free_pointer(void **pointer) { __asm { int 3 } }
void begin_library(Library *library,Image *image) { __asm { int 3 } }
void convert_object(Argument *argument,unsigned short *machine,Image *image) { __asm { int 3 } }
void convert_omf(Argument *argument,unsigned short *machine,ImageOptions *options) { __asm { int 3 } }
long tell_file(int file) { __asm { int 3 } return 0; }
