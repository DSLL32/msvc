/* Source/static/native-unit qualification; no original execution claim. */
#include <recon-tool.h>
#include <recon.h>
extern int cache_spill_unit( void );
extern const ReconTest cache_spill_tests[];
static const uint32_t order[] = { 0x25458 };
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION, .struct_size=sizeof(ReconCase),
 .name="0042-cache-spill-units",
 .description="Owned cache and spill files; no allocation failure or original CLI claim",
 .execution=RECON_DIRECT, .terminal=RECON_EXIT, .direct=cache_spill_unit,
 .first_entry_order=order, .first_entry_count=RECON_COUNT(order),
 .tests=cache_spill_tests, .test_count=5
};
