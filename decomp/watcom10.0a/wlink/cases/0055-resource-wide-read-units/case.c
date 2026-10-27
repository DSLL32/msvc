/* Source/static/native units, without original invocation claim. */
#include <recon-tool.h>
#include <recon.h>
extern int resource_wide_read_unit( void );
extern const ReconTest resource_wide_read_tests[];
/* Native unit order includes genuine setup, allocation and cleanup edges. */
static const uint32_t order[]={0x36880,0x39138,0x39220,0x33324,0x33304,0x24e98,0x250cc,0x33344,0x24e10};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0055-resource-wide-read-units",.description="Old16-bit resource-name/string read contracts and low-byte projection",
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=resource_wide_read_unit,
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .tests=resource_wide_read_tests,.test_count=3
};
