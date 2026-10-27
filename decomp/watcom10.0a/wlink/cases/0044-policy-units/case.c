/* Source/static/native-unit qualification; no original execution claim. */
#include <recon-tool.h>
#include <recon.h>
extern int policy_unit( void );
extern const ReconTest policy_unit_tests[];
static const uint32_t order[] = { 0x1c1a0 };
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION, .struct_size=sizeof(ReconCase),
 .name="0044-policy-units", .description="Typed old policy/helper units; no observed original CLI claim",
 .execution=RECON_DIRECT, .terminal=RECON_EXIT, .direct=policy_unit,
 .first_entry_order=order, .first_entry_count=RECON_COUNT(order),
 .tests=policy_unit_tests, .test_count=3
};
