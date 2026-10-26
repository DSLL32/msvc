/* Authored native termination contract, not an original-runtime oracle. */
#include <recon-tool.h>
#include <recon.h>
int terminal_setup(void);
int terminal_unit(void);
static const uint32_t order[]={0xf72c,0x16738,0x16710,0x1e668,0x204b0,0x20548,0x204c0,0x21570,0x204e4,0x20508,0x205b0,0x1e520,0x16688};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0019-disk-error-unit",
    .description="Real message resources and source-backed exit255 helper; unit-only qualification",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,
    .setup=terminal_setup,.direct=terminal_unit,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order)
};
