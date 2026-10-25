#include <recon-tool.h>
#include "startup.h"
static const uint32_t order[]={0x47c77,0x47f15,0x66a2e};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,
    .struct_size=sizeof(ReconCase),
    .name="wcc-first-help-frontier",
    .description="Basic help: genuine CRT initialization reaches private MemAlloc(8) before main",
    .first_entry_order=order,
    .first_entry_count=RECON_COUNT(order),
    .execution=RECON_IO,
    .terminal=RECON_FRONTIER,
    .frontier_rva=0x66a2e,
    .io=wcc_native_startup,
};
