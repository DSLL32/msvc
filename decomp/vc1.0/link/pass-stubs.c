/* Pass-one operations not yet entered by accepted cases. */
#include "pass-one.h"
#include "archive.h"
void reproduce_input(const char *name,unsigned char input) { __asm { int 3 } }
void resolve_weak_externals(SymbolTable *symbols,unsigned long flags) { __asm { int 3 } }
void warn_unused_libraries(Library *library) { __asm { int 3 } }
void convert_object(Argument *argument,unsigned short *machine,Image *image) { __asm { int 3 } }
void convert_omf(Argument *argument,unsigned short *machine,ImageOptions *options) { __asm { int 3 } }
void dump_archive_header(Library *library,IMAGE_ARCHIVE_MEMBER_HEADER header,unsigned long offset) { __asm { int 3 } }
void end_undefined_enumerator(UndefinedEnumerator *scan) { __asm { int 3 } }
void reorder_code_contributions(Image *image) { __asm { int 3 } }
