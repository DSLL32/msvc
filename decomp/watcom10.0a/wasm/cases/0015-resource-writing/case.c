/* Source/disassembly and native-unit provenance only, no original oracle. */
#include <recon-tool.h>
#include <recon.h>
int conversion_smoke(void);
extern const ReconTest writer_tests[];
static const uint32_t order[]={0x21b30};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0015-resource-writing",
    .description="Typed v2/Win16 resource serialization and low-byte32-bit names; native units only",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=conversion_smoke,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
    .tests=writer_tests,.test_count=4
};
