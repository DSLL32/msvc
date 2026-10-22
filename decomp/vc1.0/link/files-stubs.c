/* File operations not yet reached by an accepted frontier. */
#include "files.h"
void grow_mapping(FileSlot *file,long length) { __asm { int 3 } }
void grow_buffered_file(FileSlot *file) { __asm { int 3 } }
Page * choose_file_page(void) { __asm { int 3 } return NULL; }
