#include "cvtres.h"
#include <recon.h>

/* INT3 is the deliberately unimplemented original-function boundary. */
void error(unsigned short code, ...) { __asm { int 3 } }
void warning(unsigned short code, ...) { __asm { int 3 } }
long __stdcall seek_file(FILE *file, long offset, int origin) { __asm { int 3 } return 0; }
int __stdcall convert_resources(FILE *input, FILE *output, unsigned long size,
                                int rewrite, unsigned long first_id)
{ __asm { int 3 } return 0; }
