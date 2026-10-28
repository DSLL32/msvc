#include "omf.h"
#include <recon.h>

void __cdecl omf_error(const char * a0,...) { __asm { int 3 } }

void __cdecl read_omf_lheadr(void) { __asm { int 3 } }
