/* Definition-file operations not yet reached by accepted cases. */
#include "definition-file.h"
#include <recon.h>
unsigned short read_definition_sizes(const char *kind,char *text,unsigned long *reserve,unsigned long *commit) { __asm { int 3 } return 0; }
unsigned short skip_definition_code(void) { __asm { int 3 } return 0; }
unsigned short skip_definition_data(void) { __asm { int 3 } return 0; }
unsigned short read_definition_sections(const char *kind,char *text) { __asm { int 3 } return 0; }
unsigned short read_definition_imports(char *text) { __asm { int 3 } return 0; }
