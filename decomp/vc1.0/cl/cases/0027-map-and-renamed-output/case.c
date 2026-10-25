/* Cached original-backed invocation; no new original execution.
 * Reference SHA256 fb5a86d237aea91de9d43583a938fe4c59bc885fae3b45536034a76c4856e5d6. */
#include <stdio.h>
#include <string.h>
#include <recon-output.h>
#include <recon-tool.h>
#include "../../harness/child-contract.h"
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x9f3c, /* map_filename */
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
    0x9198, /* option_callback_19198 */
    0x70a0, /* driver_switch_170a0 */
    0x713c, /* driver_switch_1713c */
    0x719c, /* driver_switch_1719c */
    0x7010, /* driver_switch_17010 */
    0xa2fc, /* output_stem */
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase), .name = "map-and-renamed-output",
    .description = "Derived map filename and quoted output paths",
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
    "MSC_CMD_FLAGS=-ef R:\\toolchains\\1.0-nt\\bin\\c1.err -il .\\001140 -f test.c -W 1 -Ze -Zp8 -D_M_IX86=400 -G4 -Gs -Ot -Ob0 -Fotest.obj -pc \\:/ -D_MSC_VER=800 -D_WIN32 -I R:\\toolchains\\1.0-nt\\include ",
};
static const ClCapturedFile child_0_before[] = {
    { "output dir/.keep", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 0 },
    { "test.c", "9b8044c13cc843fe16795fd957e3ede9da9c5c12f3104e538396ea98230389b2", 26 },
};
static const ClCapturedFile child_0_after[] = {
    { "001140ex", "0d9286f298f6b7c7ed18130d6ad429da4a053b7bed65abd223c606b689e71070", 866 },
    { "001140gl", "97b9a9b6d3935f0bf52fc9ce60ecec9696e25f15d6776c3dee7e865db7c28e8f", 50 },
    { "001140in", "75a8e1fec5044c324759815b5b040b07908d4432ddbe67100ea03b286b4a843b", 3 },
    { "001140sy", "a7f231245a77dfb06e5ce3790fcb7187f73b3a23270e5e16deaa431cfe524671", 13 },
    { "output dir/.keep", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 0 },
    { "test.c", "9b8044c13cc843fe16795fd957e3ede9da9c5c12f3104e538396ea98230389b2", 26 },
};
static const char *const child_1_argv[] = {
    "R:\\toolchains\\1.0-nt\\bin\\c23232.exe",
};
static const char *const child_1_environment[] = {
    "MSC_CMD_FLAGS=-ef R:\\toolchains\\1.0-nt\\bin\\c23.err -il .\\001140 -f test.c -W 1 -G4 -Gs4096 -dos -Fotest.obj -ML ",
};
static const ClCapturedFile child_1_before[] = {
    { "001140ex", "0d9286f298f6b7c7ed18130d6ad429da4a053b7bed65abd223c606b689e71070", 866 },
    { "001140gl", "97b9a9b6d3935f0bf52fc9ce60ecec9696e25f15d6776c3dee7e865db7c28e8f", 50 },
    { "001140in", "75a8e1fec5044c324759815b5b040b07908d4432ddbe67100ea03b286b4a843b", 3 },
    { "001140sy", "a7f231245a77dfb06e5ce3790fcb7187f73b3a23270e5e16deaa431cfe524671", 13 },
    { "output dir/.keep", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 0 },
    { "test.c", "9b8044c13cc843fe16795fd957e3ede9da9c5c12f3104e538396ea98230389b2", 26 },
};
static const ClCapturedFile child_1_after[] = {
    { "001140ex", "0d9286f298f6b7c7ed18130d6ad429da4a053b7bed65abd223c606b689e71070", 866 },
    { "001140gl", "97b9a9b6d3935f0bf52fc9ce60ecec9696e25f15d6776c3dee7e865db7c28e8f", 50 },
    { "001140in", "75a8e1fec5044c324759815b5b040b07908d4432ddbe67100ea03b286b4a843b", 3 },
    { "001140sy", "a7f231245a77dfb06e5ce3790fcb7187f73b3a23270e5e16deaa431cfe524671", 13 },
    { "output dir/.keep", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 0 },
    { "test.c", "9b8044c13cc843fe16795fd957e3ede9da9c5c12f3104e538396ea98230389b2", 26 },
    { "test.obj", "93cf4e8cd1c715b340cc26236b1ef6d2691bd584cd14770369dee8131fb3f927", 286 },
};
static const char *const child_2_argv[] = {
    "R:\\toolchains\\1.0-nt\\bin\\link.exe",
    "-link",
    "@.\\001140lk",
};
static const ClCapturedFile child_2_before[] = {
    { "001140lk", "8b6b826a7c033a5f5723bdedf1ade4ae495c88bb9afab9f34e922c826840f51a", 83 },
    { "output dir/.keep", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 0 },
    { "test.c", "9b8044c13cc843fe16795fd957e3ede9da9c5c12f3104e538396ea98230389b2", 26 },
    { "test.obj", "93cf4e8cd1c715b340cc26236b1ef6d2691bd584cd14770369dee8131fb3f927", 286 },
};
static const ClCapturedFile child_2_after[] = {
    { "001140lk", "8b6b826a7c033a5f5723bdedf1ade4ae495c88bb9afab9f34e922c826840f51a", 83 },
    { "output dir/.keep", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 0 },
    { "output dir/custom.exe", "abfe0d4f1f8f496a640db0b32e4923cdd17a5f0678384a433688a84ba26a0361", 2097152 },
    { "output dir/custom.map", "1e6809d396cd533d57ccebfb92e313b37531fc655ff6cf5bb1106f28c11315f7", 10568 },
    { "test.c", "9b8044c13cc843fe16795fd957e3ede9da9c5c12f3104e538396ea98230389b2", 26 },
    { "test.obj", "93cf4e8cd1c715b340cc26236b1ef6d2691bd584cd14770369dee8131fb3f927", 286 },
};
static const ClCapturedChild children[] = {
    {
        .file = "R:\\toolchains\\1.0-nt\\bin\\c13232.exe", .logical_cwd = "S:\\vc1.0\\cl\\map-and-renamed-output",
        .argv = child_0_argv, .argc = 1,
        .environment = child_0_environment, .environment_count = 1, .explicit_environment = 1,
        .before = child_0_before, .before_count = 2,
        .after = child_0_after, .after_count = 6,
        .image_asset = "7061440a14554b4c81d4908ddb34cd674265594d6b04932d3f70cecf7a731063", .image_size = 389632,
        .image_path = "R:\\toolchains\\1.0-nt\\bin\\C13232.EXE",
        .stdout_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stdout_size = 0,
        .stderr_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stderr_size = 0,
        .status = 0, .failure_errno = 0, .missing = 0, .signal_number = 0,
    },
    {
        .file = "R:\\toolchains\\1.0-nt\\bin\\c23232.exe", .logical_cwd = "S:\\vc1.0\\cl\\map-and-renamed-output",
        .argv = child_1_argv, .argc = 1,
        .environment = child_1_environment, .environment_count = 1, .explicit_environment = 1,
        .before = child_1_before, .before_count = 6,
        .after = child_1_after, .after_count = 7,
        .image_asset = "0a64eab35cfb4bfb8af452653d21707947ada33952a82a5e712c275ab7712d26", .image_size = 482816,
        .image_path = "R:\\toolchains\\1.0-nt\\bin\\C23232.EXE",
        .stdout_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stdout_size = 0,
        .stderr_asset = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", .stderr_size = 0,
        .status = 0, .failure_errno = 0, .missing = 0, .signal_number = 0,
    },
    {
        .file = "R:\\toolchains\\1.0-nt\\bin\\link.exe", .logical_cwd = "S:\\vc1.0\\cl\\map-and-renamed-output",
        .argv = child_2_argv, .argc = 3,
        .before = child_2_before, .before_count = 4,
        .after = child_2_after, .after_count = 6,
        .image_asset = "3a53166175a2e98179d4da41bb0f3bfafec0ecf78884441736f138c8ab4d3718", .image_size = 385536,
        .image_path = "R:\\toolchains\\1.0-nt\\bin\\LINK.EXE",
        .stdout_asset = "7f236b655bfee4e33cdc1357c0f3955324a34292c2a0789137a4a2d446b21ed2", .stdout_size = 202,
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

/* The historical map normalizer required this fixed label before excluding the
 * timestamp line. Keep that guard in addition to the frozen byte/hash checks. */
static int map_timestamp_label(const ReconOutputContext *context)
{
    char path[4096], prefix[14];
    FILE *file;
    int length = snprintf(path, sizeof(path), "%s/output dir/custom.map", context->workspace_cwd);
    int ok;
    if (length < 0 || length >= sizeof(path)) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    ok = !fseek(file, 11, SEEK_SET) && fread(prefix, 1, sizeof(prefix), file) == sizeof(prefix) &&
        !memcmp(prefix, " Timestamp is ", sizeof(prefix));
    fclose(file);
    return ok;
}
RECON_EXPORT const ReconOutputPolicy case_output_policy = {
    RECON_OUTPUT_POLICY_VERSION, sizeof(ReconOutputPolicy), map_timestamp_label
};
