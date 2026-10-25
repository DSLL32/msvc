/* Cached original-backed invocation; no new original execution.
 * Reference SHA256 b292b520b76adc190dee16568936c5456ea83d2c68e67d06694c795b911f3564. */
#include <recon-tool.h>
#include "../../harness/child-contract.h"
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x7368, /* driver_switch_17368 */
    0x8fd6, /* any_match */
};
static const uint32_t seen[] = {
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
    0x7557, /* classify_input */
    0x75dd, /* add_input */
    0x75ae, /* new_input */
    0x8a85, /* prepare_inputs */
    0xb007, /* prepare_execution */
    0x93cd, /* run_driver */
    0x9383, /* run_passes */
    0x9306, /* initialize_pass */
    0x959e, /* compile_pass */
    0xa1d1, /* expanded_length */
    0xa455, /* nonblank_length */
    0xa366, /* is_space */
    0xa3c7, /* append_string */
    0xa4ae, /* quote_if_space */
    0xae65, /* spawn_pass */
    0xa6b8, /* cleanup_files */
    0x9980, /* link_pass */
    0x9ffe, /* write_coff_response */
    0xa936, /* file_message */
    0xa7b3, /* driver_exit */
    0x773c, /* detach_item */
    0x8f2b, /* usage */
    0x920c, /* option_callback_1920c */
    0x91b4, /* force_input */
    0xa757, /* fatal */
    0xaa63, /* diagnostic_text */
    0xa910, /* write_message_list */
    0xab77, /* warning */
    0x7f2a, /* warn_override */
    0x76d4, /* copy_item */
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase), .name = "required-option-present",
    .description = "Comment preservation has its preprocessing prerequisite",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .first_entry_seen = seen, .first_entry_seen_count = RECON_COUNT(seen),
    .execution = RECON_IO, .terminal = RECON_EXIT, .io = invoke
};

/* CL compiler/linker requests: exact captured command, environment and trees.
 * PE/artifact/stream bytes remain outside the target cwd in capture.assets. */
static const char *const child_0_argv[] = {
    "R:\\toolchains\\1.0-nt\\bin\\c13232.exe",
};
static const char *const child_0_environment[] = {
    "MSC_CMD_FLAGS=-ef R:\\toolchains\\1.0-nt\\bin\\c1.err -il .\\001068 -f test.c -W 1 -Ze -Zp8 -D_M_IX86=400 -G4 -Gs -Ot -Ob0 -Fotest.obj -pc \\:/ -D_MSC_VER=800 -D_WIN32 -C -E -I R:\\toolchains\\1.0-nt\\include ",
};
static const ClCapturedFile child_0_before[] = {
    { "test.c", "9b8044c13cc843fe16795fd957e3ede9da9c5c12f3104e538396ea98230389b2", 26 },
};
static const ClCapturedFile child_0_after[] = {
    { "test.c", "9b8044c13cc843fe16795fd957e3ede9da9c5c12f3104e538396ea98230389b2", 26 },
};
static const ClCapturedChild children[] = {
    {
        .file = "R:\\toolchains\\1.0-nt\\bin\\c13232.exe", .logical_cwd = "S:\\vc1.0\\cl\\required-option-present",
        .argv = child_0_argv, .argc = 1,
        .environment = child_0_environment, .environment_count = 1, .explicit_environment = 1,
        .before = child_0_before, .before_count = 1,
        .after = child_0_after, .after_count = 1,
        .image_asset = "7061440a14554b4c81d4908ddb34cd674265594d6b04932d3f70cecf7a731063", .image_size = 389632,
        .image_path = "R:\\toolchains\\1.0-nt\\bin\\C13232.EXE",
        .stdout_asset = "bb64c6594f6d22d7881a576899ae98ffd10470b1512598348ba39a5140f569b6", .stdout_size = 45,
        .stderr_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stderr_size = 0,
        .status = 0, .failure_errno = 0, .missing = 0, .signal_number = 0,
    },
};
static int reply(const ReconProcessRequest *request, ReconProcessResponse *response)
{
    return cl_captured_child(request, response, children, RECON_COUNT(children));
}
RECON_EXPORT const ReconProcessPolicy case_process_policy = {
    RECON_PROCESS_POLICY_VERSION, sizeof(ReconProcessPolicy), RECON_COUNT(children), reply
};
