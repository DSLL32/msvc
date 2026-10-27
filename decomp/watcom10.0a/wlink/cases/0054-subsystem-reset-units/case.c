/* Source/static/native units; no original execution claim. */
#include <recon-tool.h>
#include <recon.h>
extern int subsystem_reset_unit( void );
extern const ReconTest subsystem_reset_tests[];
static const uint32_t order[]={0x106a0,0x116c0,0x155b0,0x187a0,0x19680,0x1dec0,0x1f810,0x23f70,0x31ed0};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0054-subsystem-reset-units",.description="Exact old subsystem reset field sets and preserved borrowed owners",
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=subsystem_reset_unit,
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .tests=subsystem_reset_tests,.test_count=2
};
