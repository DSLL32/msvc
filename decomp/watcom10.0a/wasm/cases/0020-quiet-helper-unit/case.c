/* Retained helper qualification, not a CLI/original execution claim. */
#include <recon-tool.h>
#include <recon.h>
int quiet_unit(void);
static const uint32_t order[]={0xf724};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0020-quiet-helper-unit",
    .description="Source-backed retained BeQuiet return1 and unchanged typed owners; native unit only",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=quiet_unit,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order)
};
