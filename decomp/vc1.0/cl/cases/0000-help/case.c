/* Cached original-backed invocation; no new original execution.
 * Reference SHA256 4cd72b2230abe2138801d89d34975a0747a0f1c87deebfa70447b70f8c06df69. */
#include <recon-tool.h>
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x86cb, /* main */
    0xaffa, /* install_control_handler */
    0xa587, /* split_arguments */
    0x857b, /* count_arguments */
    0x859a, /* has_nologo */
    0x8e74, /* banner */
    0xa8e9, /* write_message */
    0xa81a, /* format_message */
    0x8e6e, /* program_name */
    0x8d89, /* initialize_driver */
    0x8f4f, /* program_directory */
    0xac6b, /* copy_string */
    0x8bdd, /* join_path */
    0x9244, /* search_path */
    0xa3e8, /* concatenate */
    0xa431, /* allocate */
    0x8335, /* parse_arguments */
    0x81e5, /* scan_option_kind */
    0x7f87, /* parse_option */
    0x7a6d, /* find_rule */
    0x7a14, /* match_rule */
    0x7866, /* read_option */
    0x7668, /* new_item */
    0x76f8, /* append_items */
    0x7c8b, /* expand_options */
    0xa382, /* skip_space */
    0x7b26, /* expand_text */
    0xa3a1, /* append_optional */
    0x7e8d, /* select_options */
    0x7649, /* free_items */
    0x7d99, /* match_item */
    0x7767, /* validate_value */
    0xa7c5, /* unsigned_digits */
    0x761c, /* recycle_item */
    0xac56, /* release */
    0x8215, /* parse_attached_option */
    0xa34d, /* is_decimal */
    0x7ad4, /* expand_choice */
    0x7c03, /* expand_extension */
    0xa54d, /* last_of */
    0x90d0, /* option_callback_190d0 */
    0x7100, /* remove_define */
    0x8ca0, /* finish_options */
    0x7713, /* is_for_pass */
    0x9d9f, /* expand_filename */
    0xa2de, /* filename_stem */
    0x71fc, /* driver_switch_171fc */
    0x72e4, /* driver_switch_172e4 */
    0x7274, /* driver_switch_17274 */
    0xa989, /* load_messages */
    0xb01d, /* console_rows */
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase), .name = "help",
    .description = "Complete public invocation: cached status, streams, full artifacts and original first entries",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_EXIT, .io = invoke
};
