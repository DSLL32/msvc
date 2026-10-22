/* Pass-one operations not yet entered by accepted cases. */
#include "pass-one.h"
#include "archive.h"
#include "resources.h"
void reproduce_input(const char *name,unsigned char input) { __asm { int 3 } }
void resolve_weak_externals(SymbolTable *symbols,unsigned long flags) { __asm { int 3 } }
void warn_unused_libraries(Library *library) { __asm { int 3 } }
void convert_omf(Argument *argument,unsigned short *machine,ImageOptions *options) { __asm { int 3 } }
void dump_archive_header(Library *library,IMAGE_ARCHIVE_MEMBER_HEADER header,unsigned long offset) { __asm { int 3 } }
void end_undefined_enumerator(UndefinedEnumerator *scan) { __asm { int 3 } }
void reorder_code_contributions(Image *image) { __asm { int 3 } }
void resource_usage(int error) { __asm { int 3 } }
int resource_name_to_ansi(char *output,const unsigned short *input,unsigned long count) { __asm { int 3 } return 0; }
int convert_legacy_resource(int input,int output,ResourceRecord *record,unsigned short type,unsigned short name) { __asm { int 3 } return 0; }
void resource_error_prefix(void) { __asm { int 3 } }
