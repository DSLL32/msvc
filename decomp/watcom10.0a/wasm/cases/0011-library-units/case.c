/* Authored native assertions, not an original CLI or runtime oracle. */
#include <recon-tool.h>
#include <recon.h>
int library_units(void);
int output_unit(void);
int group_index_unit(void);
static const uint32_t order[] = { 0x16590,0x124b0,0x12448 };
static const ReconTest tests[] = {
    { "output-bytes-and-borrowed-inputs",1,output_unit },
    { "group-index-and-borrowed-owners",1,group_index_unit }
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION,
    .struct_size = sizeof(ReconCase),
    .name = "0011-library-units",
    .description = "Source/disassembly-grounded native output and group-index units; no original execution claim",
    .execution = RECON_DIRECT,
    .terminal = RECON_EXIT,
    .direct = library_units,
    .first_entry_order = order,
    .first_entry_count = RECON_COUNT(order),
    .tests = tests,
    .test_count = RECON_COUNT(tests)
};
