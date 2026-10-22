/* File operations not yet reached by an accepted frontier. */
#include "files.h"
int release_file_slot(FileSlot *file) { __asm { int 3 } return 0; }
void grow_mapping(FileSlot *file,long length) { __asm { int 3 } }
unsigned write_buffered_file(FileSlot *file,const void *buffer,unsigned size) { __asm { int 3 } return 0; }
void grow_buffered_file(FileSlot *file) { __asm { int 3 } }
Page * choose_file_page(void) { __asm { int 3 } return NULL; }
void detach_file_page(Page *page) { __asm { int 3 } }
void advance_file_page(FileSlot *file) { __asm { int 3 } }
