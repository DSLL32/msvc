#include "definition-parser.h"
#include <recon.h>
extern "C" {
WORD __stdcall read_definition_stub(char *) { __asm { int 3 } return 0; }
}
