/* Source/disassembly-grounded native units; no original CLI trace claim. */
#include <recon-tool.h>
extern int signal_callback_unit( void );
extern const ReconTest resource_tests[];
static const uint32_t order[] = { 0x12a5c };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "resource-units",
    .description = "Native-only typed helper assertions with valid resource streams; no CLI/original-runtime qualification",
    .execution = RECON_DIRECT, .terminal = RECON_EXIT,
    .direct = signal_callback_unit,
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .tests = resource_tests, .test_count = 10
};
