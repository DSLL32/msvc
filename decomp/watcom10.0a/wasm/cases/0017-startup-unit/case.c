/* Native source-helper unit; actual CLI dispatch remains unsupported. */
#include <recon-tool.h>
#include <recon.h>
int startup_smoke(void);
extern const ReconTest startup_tests[];
static const uint32_t order[]={0x7c00,0x161ec,0x1150c,0x7e14,0x7c40,0x80cc,0x16208,0x7c28};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0017-startup-unit",
    .description="Typed direct invocation of retained DOS/OS2 startup/exit helper, real queues; no CLI/original-runtime claim",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=startup_smoke,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
    .tests=startup_tests,.test_count=4
};
