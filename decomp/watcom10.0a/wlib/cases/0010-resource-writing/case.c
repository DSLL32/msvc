/* Authored source/disassembly expectations; no original-runtime trace claim. */
#include <recon-tool.h>
extern int scalar_roundtrip_unit( void );
extern const ReconTest writer_tests[];
static const uint32_t order[] = { 0x1b5ec, 0x1b618, 0x1b644, 0x1c210, 0x1c050, 0x1c090 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "resource-writing",
    .description = "Source-grounded native resource writer/read roundtrips and16/32-bit name forms",
    .execution = RECON_DIRECT, .terminal = RECON_EXIT,
    .direct = scalar_roundtrip_unit,
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .tests = writer_tests, .test_count = 4
};
