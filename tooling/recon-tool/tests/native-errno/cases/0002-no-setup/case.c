#include "case.h"
static int no_setup(int argc,char **argv) { (void)argc;(void)argv;return errno_app(77)?0:9; }
RECON_EXPORT const ReconCase case_descriptor={RECON_ABI_VERSION,sizeof(ReconCase),"errno-no-setup","constructor/IO/atexit errno without setup",order,1,0,0,RECON_IO,RECON_EXIT,0,0,no_setup,0,0,0};
