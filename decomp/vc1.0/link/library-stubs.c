/* Library-manager paths not yet reached by the recorded cases. */
#include "library.h"
void release_library_list(LibraryList *libraries) { __asm { int 3 } }
unsigned char remove_library_member(Image *image,const char *name) { __asm { int 3 } return 0; }
void convert_omf_object(Argument *input,unsigned char link,ImageOptions *options) { __asm { int 3 } }
