/* Source/static/native unit, no original invocation claim. */
#include <recon-tool.h>
#include <recon.h>
extern int state_tail_unit( void );
extern const ReconTest state_tail_tests[];
static const uint32_t order[]={0x12ae0};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0056-state-tail-units",.description="Old DOS/object/symbol reset boundaries and owned spill zeroing",
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=state_tail_unit,
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .tests=state_tail_tests,.test_count=2
};
