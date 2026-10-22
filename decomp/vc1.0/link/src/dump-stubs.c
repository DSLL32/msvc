/* Named frontiers of the dump-mode graph. */
#include "dump.h"
#include <recon.h>

void dump_symbol_map(void) { __asm { int 3 } }



void dump_fpo(IMAGE_SECTION_HEADER *section) { __asm { int 3 } }
