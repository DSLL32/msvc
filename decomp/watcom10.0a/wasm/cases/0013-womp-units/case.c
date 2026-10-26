/* Authored typed serialization assertions; no original-runtime claim. */
#include <recon-tool.h>
#include <recon.h>
int womp_smoke(void);
extern const ReconTest womp_tests[];
static const uint32_t order[] = { 0x1c2d4 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0013-womp-units",
    .description="Typed Intel/MS386/PharLap WOMP serialization through actual object APIs; native units only",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=womp_smoke,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
    .tests=womp_tests,.test_count=6
};
