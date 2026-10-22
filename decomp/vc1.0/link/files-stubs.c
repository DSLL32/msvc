/* File operations not yet reached by an accepted frontier. */
#include "files.h"
int open_buffered_file(FileSlot *file,int mode,int permission,unsigned char fresh)
{ __asm { int 3 } return 0; }
unsigned read_buffered_file(FileSlot *file,void *buffer,unsigned size)
{ __asm { int 3 } return 0; }
long seek_buffered_file(FileSlot *file,long offset,int origin)
{ __asm { int 3 } return 0; }
int release_file_slot(FileSlot *file) { __asm { int 3 } return 0; }
void grow_mapping(FileSlot *file,long length) { __asm { int 3 } }
int close_mapped_file(FileSlot *file) { __asm { int 3 } return 0; }
unsigned write_buffered_file(FileSlot *file,const void *buffer,unsigned size) { __asm { int 3 } return 0; }
