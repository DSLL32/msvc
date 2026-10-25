#include <recon-tool.h>
extern int cvpack_direct(void);
static const uint32_t order[]={
 0x7000, 0x74c0, 0x198a0, 0x1a8e0, 0x19a10, 0x19cf0, 0x19b20, 0x19ac0, 0x1a990, 0x1aa30, 0x1ad30, 0x93a0, 0x19ed0, 0x1ac60, 0x1a400, 0x1abb0, 0x1a000, 0x1ab50, 0x1a670, 0x1ac00, 0x1ace0, 0x1a2e0
};
RECON_EXPORT const ReconCase case_descriptor={
 RECON_ABI_VERSION,sizeof(ReconCase),"file-cache-copy",
 "Internal file API: genuine CLI-created owners, cache reuse/eviction and mapped full SUPERPAD copy/readback.",
 order,RECON_COUNT(order),NULL,0,RECON_DIRECT,RECON_EXIT,0,
 NULL,NULL,cvpack_direct,NULL,0
};
