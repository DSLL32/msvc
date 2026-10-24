#include "link.h"
#include "symbols.h"
#include "options.h"
#include <recon.h>
void link_cleanup(void) { __asm { int 3 } }
