/* Per-module output helpers waiting for matching frontiers. */
#include "pass-two.h"
#include <recon.h>






















void record_debug_fixup(unsigned short type,unsigned long rva,unsigned long target) { __asm { int 3 } }
unsigned char is_big_endian(void) { __asm { int 3 } return 0; }
