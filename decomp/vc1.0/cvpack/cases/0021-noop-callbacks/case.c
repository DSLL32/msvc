#include <recon-tool.h>
extern int cvpack_direct(void);
static const uint32_t order[]={0x7000,0x13260,0x14a50};
RECON_EXPORT const ReconCase case_descriptor={
 RECON_ABI_VERSION,sizeof(ReconCase),"noop-callbacks",
 "Internal return test of the two genuine zero-argument C6 no-op callbacks; no wide-record or CLI dispatcher coverage.",
 order,RECON_COUNT(order),NULL,0,RECON_DIRECT,RECON_EXIT,0,
 NULL,NULL,cvpack_direct,NULL,0
};
