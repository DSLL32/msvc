/* Migrated cached original IO; exact status/streams/outputs and application first entries. */
#include <recon-tool.h>
extern int cl_case_run(int,char **);
static const uint32_t order[]={0x1743,0x57b4,0x4cd1,0x2136,0x23bb,0x22e9,0x2de5,0x54c5,0x2b8a,0x2db4,0x5334,0x5069,0x5092,0x5495};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="usage",.description="Migrated cached original IO; exact status/streams/outputs and application first entries.",
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .execution=RECON_IO,.terminal=RECON_EXIT,.io=cl_case_run
};
