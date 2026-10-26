/* Native source/disassembly units; no CLI or original-runtime claim. */
#include <recon-tool.h>
#include <recon.h>
int collection_smoke(void);
extern const ReconTest collection_tests[];
static const uint32_t order[]={0x1c280};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0016-collections",
    .description="Real array blocks/callbacks, borrowed queue/fixup links and successful realloc ownership; native units only",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=collection_smoke,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
    .tests=collection_tests,.test_count=4
};
