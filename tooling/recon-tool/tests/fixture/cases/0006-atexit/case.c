#include "recon-tool.h"
extern int fixture_main(int,char**);
static const uint32_t order[]={0x1000,0x1300};
RECON_EXPORT const ReconCase case_descriptor={RECON_ABI_VERSION,sizeof(ReconCase),"atexit","normal C exit",order,RECON_COUNT(order),0,0,RECON_IO,RECON_EXIT,0,0,fixture_main,0,0,0};
