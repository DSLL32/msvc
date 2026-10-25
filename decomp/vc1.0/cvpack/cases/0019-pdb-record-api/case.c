#include <recon-tool.h>
extern int cvpack_direct(void);
static const uint32_t order[]={
 0x7000, 0x74c0, 0x198a0, 0x1a8e0, 0x19a10, 0x19cf0, 0x19b20, 0x19ac0, 0x1a990, 0x1aa30, 0x1ad30, 0x93a0, 0x19ed0, 0x1ac60, 0x1b968, 0x1aef0, 0x1b14c, 0x1bd20, 0x1bd00, 0x1bcc0, 0x1bccc, 0x1bca8, 0x1bcb4, 0x1badc, 0x1bbe8
};
RECON_EXPORT const ReconCase case_descriptor={
 RECON_ABI_VERSION,sizeof(ReconCase),"pdb-record-api",
 "Internal provider API: real constructor, default read mode, every genuine SUPERPAD type record and hash chain, normal close.",
 order,RECON_COUNT(order),NULL,0,RECON_DIRECT,RECON_EXIT,0,
 NULL,NULL,cvpack_direct,NULL,0
};
