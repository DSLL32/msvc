#include "recon-tool.h"
extern int fixture_main(int,char**);
extern int fixture_add(int,int);
static int check_add(void){return fixture_add(3,7)==10;}
static const uint32_t order[]={0x1000};
RECON_EXPORT const ReconCase case_descriptor={RECON_ABI_VERSION,sizeof(ReconCase),"0003-timeout","focused harness fixture",order,RECON_COUNT(order),0,0,RECON_IO,RECON_EXIT,0,0,fixture_main,0,0,0};
