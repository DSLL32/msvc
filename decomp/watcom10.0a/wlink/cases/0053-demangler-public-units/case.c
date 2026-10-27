/* Source/static/native qualification, no original execution claim. */
#include <recon-tool.h>
#include <recon.h>
extern int demangler_public_unit( void );
extern const ReconTest demangler_public_tests[];
static const uint32_t order[]={0x2f7ec};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0053-demangler-public-units",.description="Old basename, callback parser and operator-mangling public contracts",
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=demangler_public_unit,
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .tests=demangler_public_tests,.test_count=3
};
