#include "cvtres.h"
#include <recon.h>

/* INT3 is the deliberately unimplemented original-function boundary. */
void error(unsigned short code, ...) { __asm { int 3 } }
void warning(unsigned short code, ...) { __asm { int 3 } }
int __stdcall convert_ce_image(int ce, FILE *input, FILE *output, ResourceEntry *entry, int type) { __asm { int 3 } return 0; }
