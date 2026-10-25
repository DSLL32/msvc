#include <recon-tool.h>
extern int cvpack_direct(void);
static const uint32_t order[]={
 0x7000, 0x74c0, 0x198a0, 0x1a8e0, 0x19a10, 0x19cf0, 0x19b20, 0x19ac0, 0x1a990, 0x19db0, 0x1a1f0, 0x1a910, 0x1a7f0, 0x1a540, 0x1a6a0, 0x93a0, 0x1a2e0, 0x1a400, 0x1a350, 0x1a5c0, 0x1a120, 0x1a850, 0x19ed0, 0x19f50, 0x13730, 0x13820, 0x13920, 0x13780
};
RECON_EXPORT const ReconCase case_descriptor={
 RECON_ABI_VERSION,sizeof(ReconCase),"cfig-symbol-api",
 "Internal serializer API: all 423 genuine narrow declaration records from unchanged shipped CFIG386, exact outputs; no based-pointer CLI/wide-record claim.",
 order,RECON_COUNT(order),NULL,0,RECON_DIRECT,RECON_EXIT,0,
 NULL,NULL,cvpack_direct,NULL,0
};
