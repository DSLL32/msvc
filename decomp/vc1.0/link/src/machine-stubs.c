/* Relocation backends have not yet been reached. */
#include "machine.h"
#include <recon.h>
void apply_m68k_relocations(Contribution *contribution,void *data,
    PIMAGE_SYMBOL symbols,Image *image) { __asm { int 3 } }
