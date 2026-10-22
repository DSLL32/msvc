/* Named frontiers of the editor and its built-in bind/rebase modes. */
#include "editor.h"
#include <recon.h>
int rebase_main(int argc,char **argv,char **environment) { __asm { int 3 } return 0; }
int bind_main(int argc,char **argv,char **environment) { __asm { int 3 } return 0; }


void edit_section(char *text) { __asm { int 3 } }
