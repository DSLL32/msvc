/* Pass-one operations not yet entered by accepted cases. */
#include "pass-one.h"
#include "archive.h"
void reproduce_input(const char *name,unsigned char input) { __asm { int 3 } }
void resolve_weak_externals(SymbolTable *symbols,unsigned long flags) { __asm { int 3 } }
unsigned char create_import_library(Image *image,const char *library,const char *tool) { __asm { int 3 } return 0; }
void warn_unused_libraries(Library *library) { __asm { int 3 } }
void apply_section_overrides(Image *image) { __asm { int 3 } }
void apply_section_merges(Image *image) { __asm { int 3 } }
void free_pointer(void **pointer) { __asm { int 3 } }
void convert_object(Argument *argument,unsigned short *machine,Image *image) { __asm { int 3 } }
void convert_omf(Argument *argument,unsigned short *machine,ImageOptions *options) { __asm { int 3 } }
void search_library_members(Image *image,Library *library,unsigned char *changed,unsigned char *unresolved) { __asm { int 3 } }
void dump_archive_header(Library *library,IMAGE_ARCHIVE_MEMBER_HEADER header,unsigned long offset) { __asm { int 3 } }
unsigned read_optional_header(int file,IMAGE_OPTIONAL_HEADER *header,unsigned short size) { __asm { int 3 } return 0; }
