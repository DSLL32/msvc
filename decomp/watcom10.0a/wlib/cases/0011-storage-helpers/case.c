/* Source/disassembly-grounded native helper case; no CLI/original trace claim. */
#include <recon-tool.h>
extern int allocation_unit( void );
extern const ReconTest storage_tests[];
static const uint32_t order[] = {0x12021,0x12031};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "storage-helpers", .description = "Nullable allocation and distinct dictionary-finalizer hooks",
    .execution = RECON_DIRECT, .terminal = RECON_EXIT,
    .direct = allocation_unit,
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .tests = storage_tests, .test_count = 2
};
