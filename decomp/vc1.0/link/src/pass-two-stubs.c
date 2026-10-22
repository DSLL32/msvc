/* Per-module output helpers waiting for matching frontiers. */
#include "pass-two.h"
#include <recon.h>






External * find_mac_contribution_external(Contribution *contribution) { __asm { int 3 } return 0; }











void write_mac_loader_prologue(Contribution *contribution) { __asm { int 3 }  }





void record_debug_fixup(unsigned short type,unsigned long rva,unsigned long target) { __asm { int 3 } }
unsigned char is_big_endian(void) { __asm { int 3 } return 0; }
