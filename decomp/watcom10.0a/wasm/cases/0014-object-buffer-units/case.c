/* Authored typed buffer checks; no original-runtime or CLI claim. */
#include <recon-tool.h>
#include <recon.h>
int object_smoke(void);
extern const ReconTest object_tests[];
static const uint32_t order[]={0x1c010};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0014-object-buffer-units",
    .description="Typed WOMP record-buffer width/index roundtrips and borrowed storage; native units only",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=object_smoke,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
    .tests=object_tests,.test_count=2
};
