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
    0x1d49, /* insert_resource */
    0x30b4, /* init_directory */
    0x30e9, /* init_data_entry */
    0x1b30, /* write_bytes */
    0x1b68, /* copy_bytes */
    0x2225, /* convert_ce_image */
    0x1a7a, /* free_block */
    0x310a, /* load_message */
    0x2195, /* convert_rows */
    0x209d, /* read_pixel */
    0x1fdf, /* color_to_gray */
    0x2147, /* write_pixel */
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "resources-rich-ce",
    .description = "Saved original invocation; exact streams, status, complete artifacts and unseeded App order. Migration only.",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_EXIT,
    .setup = cvtres_case_setup, .io = cvtres_case_invoke,
};
