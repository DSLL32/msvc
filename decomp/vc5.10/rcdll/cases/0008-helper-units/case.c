/* Explicitly authorized native units, without a CLI/original-runtime claim. */
#include <recon-tool.h>
extern int option_suffix_unit(void);
extern const ReconTest helper_tests[];
static const uint32_t order[]={0x2c0b};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0008-helper-units",.description="Typed deterministic native helper units; source/disassembly and original-compiler graph qualification, no CLI reach claim.",
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=option_suffix_unit,
 .tests=helper_tests,.test_count=14
};
