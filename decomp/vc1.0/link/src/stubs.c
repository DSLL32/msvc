#include "link.h"
#include <recon.h>
#define STUB(name,args) int name args { __asm { int 3 } return 0; }
STUB(helper_main,(int argc,char **argv))
STUB(driver_usage,(void))
void link_cleanup(void) { __asm { int 3 } }
void file_failure(char *a,int b,char *c) { __asm { int 3 } }
