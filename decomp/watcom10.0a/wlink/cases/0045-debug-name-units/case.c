/* Source/static/native-unit qualification; no original execution claim. */
#include <recon-tool.h>
#include <recon.h>
extern int debug_name_unit( void );
extern const ReconTest debug_name_unit_tests[];
static const uint32_t order[] = { 0x2d89c };
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION, .struct_size=sizeof(ReconCase),
 .name="0045-debug-name-units", .description="Typed old policy/helper units; no observed original CLI claim",
 .execution=RECON_DIRECT, .terminal=RECON_EXIT, .direct=debug_name_unit,
 .first_entry_order=order, .first_entry_count=RECON_COUNT(order),
 .tests=debug_name_unit_tests, .test_count=3
};
