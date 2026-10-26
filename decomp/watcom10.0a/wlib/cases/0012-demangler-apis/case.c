/* Native source/disassembly units, not original-runtime or CLI observations. */
#include <recon-tool.h>
extern int mangled_classification_unit( void );
extern const ReconTest api_tests[];
static const uint32_t order[] = {0x16bbf,0x16b46};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "demangler-apis", .description = "Public C++ demangler queries, callbacks and output ownership",
    .execution = RECON_DIRECT, .terminal = RECON_EXIT,
    .direct = mangled_classification_unit,
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .tests = api_tests, .test_count = 4
};
