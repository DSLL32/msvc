/* Named native conversion frontiers not yet reconstructed. */
#include "omf.h"
#include <recon.h>
void omf_error(const char *format,...) { __asm { int 3 } }
void omf_record_lheadr(void) { __asm { int 3 } }
void omf_register_debug_fixup(unsigned index,unsigned long offset) { __asm { int 3 } }
