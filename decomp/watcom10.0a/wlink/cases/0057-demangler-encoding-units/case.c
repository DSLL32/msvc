/* Source/static/native unit, no original invocation claim. */
#include <recon-tool.h>
#include <recon.h>
extern int demangler_encoding_unit( void );
extern const ReconTest demangler_encoding_tests[];
static const uint32_t order[]={0x37310};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0057-demangler-encoding-units",.description="Old allocating demangler/scope APIs and unsigned LEB128",
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=demangler_encoding_unit,
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .tests=demangler_encoding_tests,.test_count=3
};
