/* File operations not yet reached by an accepted frontier. */
#include "files.h"
#include <recon.h>
void grow_mapping(FileSlot *file,long length) { __asm { int 3 } }
void grow_buffered_file(FileSlot *file) { __asm { int 3 } }
