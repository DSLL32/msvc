/* Source/static/native unit qualification, no original invocation claim. */
#include <recon-tool.h>
#include <recon.h>
extern int resource_writer_unit( void );
extern const ReconTest resource_writer_tests[];
static const uint32_t order[]={0x38770};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0052-resource-writer-units",.description="Owned old WRes/MRes writes and finite zero-extension conversion",
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=resource_writer_unit,
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .tests=resource_writer_tests,.test_count=4
};
