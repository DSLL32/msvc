/* Native source/disassembly qualification, no original trace/oracle claim. */
#include <recon-tool.h>
#include <recon.h>
int resource_error_unit(void);
extern const ReconTest resource_tests[];
static const uint32_t order[] = { 0x215b0 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
    .name="0012-resource-units",
    .description="Authored1994 WRes v2/Win16 resources and native reader assertions; no original execution claim",
    .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=resource_error_unit,
    .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
    .tests=resource_tests,.test_count=5
};
