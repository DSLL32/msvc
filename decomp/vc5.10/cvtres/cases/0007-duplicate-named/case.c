#include <recon-tool.h>
#include "adapter.h"
static const uint32_t order[] = {
    0x14a5, /* main */
    0x1af6, /* seek_file */
    0x1ac9, /* tell_file */
    0x266b, /* convert_resources */
    0x1a91, /* read_bytes */
    0x1a4c, /* allocate_block */
    0x1c39, /* read_identifier */
    0x310a, /* load_message */
    0x1d49, /* insert_resource */
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "duplicate-named",
    .description = "Saved original invocation; exact streams, status, complete artifacts and unseeded App order. Migration only.",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_EXIT,
    .setup = cvtres_case_setup, .io = cvtres_case_invoke,
};
