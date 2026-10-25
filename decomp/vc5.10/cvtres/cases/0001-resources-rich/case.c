#include <recon-tool.h>
extern int main(int, char **, char **);
extern int cvtres_case_setup(void);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
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
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "resources-rich",
    .description = "Saved original invocation; exact streams, status, complete artifacts and unseeded App order. Migration only.",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_EXIT,
    .setup = cvtres_case_setup, .io = invoke,
};
