/* Authored cleanup lifecycle; no original runtime or CLI-reach claim. */
#include <recon-tool.h>
#include <recon.h>
int writer_cleanup_unit(void);
extern const ReconTest cleanup_tests[];
static const uint32_t order[]={0x1c250,0x1aaa8,0x168c4,0x161ec,0x1aa80,0x1c274,0x1ab1c,0xdee0,0x1c268,0x1aae0,0x16900,0x16208,0x1c2a0,0x1ab84};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0021-writer-cleanup-unit",
    .description="Source-correlated retained write_fini with real fixup/carve lifecycle; native unit only",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=writer_cleanup_unit,
    .tests=cleanup_tests,.test_count=2,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order)
};
