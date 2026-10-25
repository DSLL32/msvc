/* Cached original-backed invocation; no new original execution.
 * Reference SHA256 db18afc1e0bf530ddac0ec78c151d41891e34d618e1e0e6b3e7323cf0d421d8d. */
#include <recon-tool.h>
#include "../../harness/child-contract.h"
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x773c, /* detach_item */
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
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase), .name = "hello",
    .description = "Complete public invocation: cached status, streams, full artifacts and original first entries",
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
    "MSC_CMD_FLAGS=-ef R:\\toolchains\\1.0-nt\\bin\\c1.err -il .\\000736 -f hello.c -W 1 -Ze -Zp8 -Fohello.obj -pc \\:/ -D_MSC_VER=800 -D_WIN32 -Gs -Gf -Og -Oi -Ot -Oy -Ob1 -D_M_IX86=500 -G4 -I R:\\toolchains\\1.0-nt\\include ",
};
static const ClCapturedFile child_0_before[] = {
    { "hello.c", "bf205b2bd0d9d585f98708a659449ea5d2b662ebc6c43f547b21ae6afb66c0f2", 1629 },
};
static const ClCapturedFile child_0_after[] = {
    { "000736ex", "6d8ecf3592f4eb421bf5c0f242bfcd8cc4522778e4701798f79c6796eaa4510a", 2066 },
    { "000736gl", "b17279eda0148e7c58a880dddf1ee709ec7d9e9d18f3c40da46120efe621399c", 629 },
    { "000736in", "eef2db4fc6cc63f5dedde697db9972dfa205e5c3db3e1a4a445f0312cb05ee50", 46 },
    { "000736sy", "cfdd85b09a04376067df9d6849e0c682c977b43fbd791a554e3d00375a669467", 422 },
    { "hello.c", "bf205b2bd0d9d585f98708a659449ea5d2b662ebc6c43f547b21ae6afb66c0f2", 1629 },
};
static const char *const child_1_argv[] = {
    "R:\\toolchains\\1.0-nt\\bin\\c23232.exe",
};
static const char *const child_1_environment[] = {
    "MSC_CMD_FLAGS=-ef R:\\toolchains\\1.0-nt\\bin\\c23.err -il .\\000736 -f hello.c -W 1 -dos -Fohello.obj -ML -Gs4096 -Gy -G5 ",
};
static const ClCapturedFile child_1_before[] = {
    { "000736ex", "6d8ecf3592f4eb421bf5c0f242bfcd8cc4522778e4701798f79c6796eaa4510a", 2066 },
    { "000736gl", "b17279eda0148e7c58a880dddf1ee709ec7d9e9d18f3c40da46120efe621399c", 629 },
    { "000736in", "eef2db4fc6cc63f5dedde697db9972dfa205e5c3db3e1a4a445f0312cb05ee50", 46 },
    { "000736sy", "cfdd85b09a04376067df9d6849e0c682c977b43fbd791a554e3d00375a669467", 422 },
    { "hello.c", "bf205b2bd0d9d585f98708a659449ea5d2b662ebc6c43f547b21ae6afb66c0f2", 1629 },
};
static const ClCapturedFile child_1_after[] = {
    { "000736ex", "6d8ecf3592f4eb421bf5c0f242bfcd8cc4522778e4701798f79c6796eaa4510a", 2066 },
    { "000736gl", "b17279eda0148e7c58a880dddf1ee709ec7d9e9d18f3c40da46120efe621399c", 629 },
    { "000736in", "eef2db4fc6cc63f5dedde697db9972dfa205e5c3db3e1a4a445f0312cb05ee50", 46 },
    { "000736sy", "cfdd85b09a04376067df9d6849e0c682c977b43fbd791a554e3d00375a669467", 422 },
    { "hello.c", "bf205b2bd0d9d585f98708a659449ea5d2b662ebc6c43f547b21ae6afb66c0f2", 1629 },
    { "hello.obj", "77e7b0cf1b7fe1abe5fdcce19c597cde07c561de6a9edbdb92fa206ec01f8764", 2289 },
};
static const char *const child_2_argv[] = {
    "R:\\toolchains\\1.0-nt\\bin\\link.exe",
    "-link",
    "@.\\000736lk",
};
static const ClCapturedFile child_2_before[] = {
    { "000736lk", "e7369d9ec162e872bfef08728ea6cdcdfc2f44d45182bb5c6de995e34fe81e26", 40 },
    { "hello.c", "bf205b2bd0d9d585f98708a659449ea5d2b662ebc6c43f547b21ae6afb66c0f2", 1629 },
    { "hello.obj", "77e7b0cf1b7fe1abe5fdcce19c597cde07c561de6a9edbdb92fa206ec01f8764", 2289 },
};
static const ClCapturedFile child_2_after[] = {
    { "000736lk", "e7369d9ec162e872bfef08728ea6cdcdfc2f44d45182bb5c6de995e34fe81e26", 40 },
    { "hello.c", "bf205b2bd0d9d585f98708a659449ea5d2b662ebc6c43f547b21ae6afb66c0f2", 1629 },
    { "hello.exe", "ef247ac7e034cb09ad21eb716920a15eb45904936716f9cc499e8f9ea96b2310", 2097152 },
    { "hello.obj", "77e7b0cf1b7fe1abe5fdcce19c597cde07c561de6a9edbdb92fa206ec01f8764", 2289 },
};
static const ClCapturedChild children[] = {
    {
        .file = "R:\\toolchains\\1.0-nt\\bin\\c13232.exe", .logical_cwd = "S:\\vc1.0\\cl\\hello",
        .argv = child_0_argv, .argc = 1,
        .environment = child_0_environment, .environment_count = 1, .explicit_environment = 1,
        .before = child_0_before, .before_count = 1,
        .after = child_0_after, .after_count = 5,
        .image_asset = "7061440a14554b4c81d4908ddb34cd674265594d6b04932d3f70cecf7a731063", .image_size = 389632,
        .image_path = "R:\\toolchains\\1.0-nt\\bin\\C13232.EXE",
        .stdout_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stdout_size = 0,
        .stderr_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stderr_size = 0,
        .status = 0, .failure_errno = 0, .missing = 0, .signal_number = 0,
    },
    {
        .file = "R:\\toolchains\\1.0-nt\\bin\\c23232.exe", .logical_cwd = "S:\\vc1.0\\cl\\hello",
        .argv = child_1_argv, .argc = 1,
        .environment = child_1_environment, .environment_count = 1, .explicit_environment = 1,
        .before = child_1_before, .before_count = 5,
        .after = child_1_after, .after_count = 6,
        .image_asset = "0a64eab35cfb4bfb8af452653d21707947ada33952a82a5e712c275ab7712d26", .image_size = 482816,
        .image_path = "R:\\toolchains\\1.0-nt\\bin\\C23232.EXE",
        .stdout_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stdout_size = 0,
        .stderr_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stderr_size = 0,
        .status = 0, .failure_errno = 0, .missing = 0, .signal_number = 0,
    },
    {
        .file = "R:\\toolchains\\1.0-nt\\bin\\link.exe", .logical_cwd = "S:\\vc1.0\\cl\\hello",
        .argv = child_2_argv, .argc = 3,
        .before = child_2_before, .before_count = 3,
        .after = child_2_after, .after_count = 4,
        .image_asset = "3a53166175a2e98179d4da41bb0f3bfafec0ecf78884441736f138c8ab4d3718", .image_size = 385536,
        .image_path = "R:\\toolchains\\1.0-nt\\bin\\LINK.EXE",
        .stdout_asset = "da68d164bab079192163e8a443b53dc0fe18bd0793ef5ff68501f49eb37d3751", .stdout_size = 158,
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
