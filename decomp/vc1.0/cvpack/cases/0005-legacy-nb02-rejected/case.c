/* Cached original invocation; no original execution during migration. */
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <recon-tool.h>
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x7000, /* main */
    0x74c0, /* parse_options */
    0x198a0, /* files_initialize */
    0x1a8e0, /* new_page */
    0x19a10, /* new_file_slot */
    0x19cf0, /* open_file */
    0x19b20, /* find_cached_file */
    0x19ac0, /* acquire_file_slot */
    0x1a990, /* move_file_slot */
    0x19db0, /* open_buffered_file */
    0x1a1f0, /* seek_buffered_file */
    0x1a910, /* allocate_file_page */
    0x1a7f0, /* attach_page */
    0x1a540, /* read_file_page */
    0x1a6a0, /* touch_page */
    0x93a0, /* initialize_type_tables */
    0xf060, /* checked_zero */
    0x12f20, /* initialize_buffer */
    0xf030, /* checked_alloc */
    0xf250, /* read_image_debug */
    0x1a2e0, /* seek_file */
    0x1a400, /* read_file */
    0x1a350, /* read_buffered_file */
    0x10dd0, /* read_cv_signature */
    0x1a430, /* tell_file */
    0x11400, /* read_old_directory */
    0x1a5c0, /* advance_file_page */
    0x1a850, /* release_page */
    0x1a120, /* flush_file_page */
    0xed60, /* fatal_error */
};
RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase),
    "legacy-nb02-rejected", "Reject unsupported NB02 information with exit 1 and unchanged 37,096-byte input.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_EXIT, 0,
    NULL, invoke, NULL, NULL, 0
};
