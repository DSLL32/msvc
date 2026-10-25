/* Cached original-backed invocation; no new original execution.
 * Reference SHA256 187772f9a8b5c0962a6941f7909bf5a9cb135b6f3f2252b00db7942f1a071134. */
#include <recon-tool.h>
#include "../../harness/child-contract.h"
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x9198, /* option_callback_19198 */
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
    0x7368, /* driver_switch_17368 */
    0x8fd6, /* any_match */
    0x842b, /* read_response_file */
    0x9014, /* option_callback_19014 */
    0x9228, /* option_callback_19228 */
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase), .name = "force-object",
    .description = "Link a supplied object with a nonstandard suffix",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .first_entry_seen = seen, .first_entry_seen_count = RECON_COUNT(seen),
    .execution = RECON_IO, .terminal = RECON_EXIT, .io = invoke
};

/* CL compiler/linker requests: exact captured command, environment and trees.
 * PE/artifact/stream bytes remain outside the target cwd in capture.assets. */
static const char *const child_0_argv[] = {
    "R:\\toolchains\\1.0-nt\\bin\\link.exe",
    "-link",
    "@.\\000828lk",
};
static const ClCapturedFile child_0_before[] = {
    { "000828lk", "cfe907e4d8436f6a5d64c4b468e3a4341706f51258b845bb2a00d8bf98e2aa22", 43 },
    { "object.blob", "6c454d6cb96e2a4e1f87aa17311541ca7090b63554efda3044c53c854592599c", 286 },
};
static const ClCapturedFile child_0_after[] = {
    { "000828lk", "cfe907e4d8436f6a5d64c4b468e3a4341706f51258b845bb2a00d8bf98e2aa22", 43 },
    { "object.blob", "6c454d6cb96e2a4e1f87aa17311541ca7090b63554efda3044c53c854592599c", 286 },
    { "object.exe", "56ebfbbc4b15719906f4f8a444f12e6a3e75b2023fcac91c7164dcd38a978da4", 2097152 },
};
static const ClCapturedChild children[] = {
    {
        .file = "R:\\toolchains\\1.0-nt\\bin\\link.exe", .logical_cwd = "S:\\vc1.0\\cl\\force-object",
        .argv = child_0_argv, .argc = 3,
        .before = child_0_before, .before_count = 2,
        .after = child_0_after, .after_count = 3,
        .image_asset = "3a53166175a2e98179d4da41bb0f3bfafec0ecf78884441736f138c8ab4d3718", .image_size = 385536,
        .image_path = "R:\\toolchains\\1.0-nt\\bin\\LINK.EXE",
        .stdout_asset = "25c968bfe86e1d56f9f0dd8cff188b4048bfe4deb1f5e65c2c52d4611201f01f", .stdout_size = 161,
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
