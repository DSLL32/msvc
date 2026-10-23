#include "files.h"
#include <recon.h>

void warning(int code, const char *first, const char *second) { __asm { int 3 } }
int release_file_slot(FileSlot *slot) { __asm { int 3 } return 0; }
int open_mapped_file(FileSlot *slot, int mode, unsigned char created)
{ __asm { int 3 } return 0; }
int close_file(int handle, unsigned char release) { __asm { int 3 } return 0; }
void read_debug_directory(void) { __asm { int 3 } }
void compact_types(void) { __asm { int 3 } }
void pack_module(unsigned short module) { __asm { int 3 } }
void strip_debug(int handle) { __asm { int 3 } }
void write_debug_directory(void) { __asm { int 3 } }
void print_statistics(void) { __asm { int 3 } }
void free_type_tables(void) { __asm { int 3 } }
int update_image_checksum(const char *name, unsigned long length, unsigned char read_only)
{ __asm { int 3 } return 0; }

long seek_buffered_file(FileSlot *slot, long offset, int origin) { __asm { int 3 } return 0; }
