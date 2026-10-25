/* Typed host adapter; file loading uses host libc, not target shim FILE state. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "c1.h"
#include <recon-tool.h>
#include <recon.h>
static int read_vector(const char *path, char ***result)
{
    FILE *file = fopen(path, "rb");
    unsigned address, count, pointer, length, i;
    char **values;
    if (!file || fread(&address, 4, 1, file) != 1 ||
        fread(&count, 4, 1, file) != 1 || !address || !count || count > 512) return -1;
    values = (char **)malloc((count + 1) * sizeof(char *));
    if (!values) return -1;
    for (i = 0; i < count; ++i) {
        if (fread(&pointer, 4, 1, file) != 1 || fread(&length, 4, 1, file) != 1 ||
            !pointer || !length || length > 32768) return -1;
        values[i] = (char *)malloc(length);
        if (!values[i] || fread(values[i], 1, length, file) != length ||
            values[i][length - 1] || memchr(values[i], 0, length - 1)) return -1;
    }
    values[count] = NULL;
    if (fgetc(file) != EOF || fclose(file)) return -1;
    *result = values;
    return count;
}
static int setup(void)
{
 const char *reference = "cc2f15a9e873f45b33fba0c0d3861cdfb1238d87de021fe230e4cfea7909af63";
 recon_load_system_info_profile("system-info-profile.bin", reference);
 recon_crt_bind_empty_ctype_1252("e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210", reference);
 recon_controlfp(0x10000, 0x30000);
 recon_apply_x87_empty_entry_profile(reference, 0x027f, 0x0020, 0xffff);
 return 1;
}
static int invoke(int argc, char **argv)
{
 ReconModuleSpec spec = { "C1.DLL", "R:\\toolchains\\5.10\\bin\\C1.DLL",
 "7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d", RECON_MODULE_NO_TLS };
 HINSTANCE module = recon_register_module(&spec);
 char **host_arguments, **host_environment;
 int host_argc, result;
 host_argc = read_vector("/root/.ai-tmp/msvc2/vc5.10/c1/minimal-packing-frame-normal-001/original-crt-argv.bin", &host_arguments);
 if (host_argc != 3 || read_vector("/root/.ai-tmp/msvc2/vc5.10/c1/minimal-packing-frame-normal-001/original-crt-environment.bin", &host_environment) != 78) return 126;
 errno = 0; recon_doserrno = 0; recon_SetLastError(14007);
 if (!module || !_DllMainCRTStartup(module, DLL_PROCESS_ATTACH, NULL)) return 126;
 recon_crt_bind_arguments(host_argc, host_arguments, host_environment);
 recon_SetLastError(0);
 result = InvokeCompilerPass(argc, argv, TRUE);
 if (!_DllMainCRTStartup(module, DLL_PROCESS_DETACH, NULL)) return 126;
 return result;
}
static const uint32_t order[] = {
 0x1218e, /* DllMain */
 0x12229, /* InvokeCompilerPass */
 0x122a5, /* initialize_compiler_pass */
 0xe799, /* allocate_compiler_memory */
 0x12365, /* initialize_state_12365 */
 0x12377, /* initialize_state_12377 */
 0x12389, /* initialize_state_12389 */
 0x123a4, /* initialize_state_123a4 */
 0x123a5, /* initialize_state_123a5 */
 0x123a6, /* initialize_state_123a6 */
 0x123a7, /* initialize_state_123a7 */
 0x123ae, /* initialize_output_buffer_123ae */
 0x123f6, /* register_compiler_cleanup */
 0x1241c, /* initialize_state_1241c */
 0x12423, /* initialize_output_buffer_12423 */
 0x1246b, /* initialize_state_1246b */
 0x12472, /* initialize_output_buffer_12472 */
 0x124ba, /* initialize_state_124ba */
 0x124c1, /* initialize_output_buffer_124c1 */
 0x12509, /* initialize_state_12509 */
 0x12510, /* initialize_output_buffer_12510 */
 0x12558, /* initialize_state_12558 */
 0x1255f, /* initialize_output_buffer_1255f */
 0x125a7, /* initialize_state_125a7 */
 0x125ae, /* initialize_output_buffer_125ae */
 0x125f6, /* initialize_state_125f6 */
 0x125fd, /* initialize_output_buffer_125fd */
 0x12645, /* initialize_state_12645 */
 0x12646, /* initialize_state_12646 */
 0x12663, /* initialize_state_12663 */
 0x12664, /* initialize_state_12664 */
 0x1266f, /* initialize_state_1266f */
 0x12676, /* initialize_pdb_context_12676 */
 0x126af, /* initialize_state_126af */
 0x126b6, /* initialize_pdb_context_126b6 */
 0x126ef, /* initialize_state_126ef */
 0x126fc, /* _ZN12CompilerPool10InitializeEj */
 0x12712, /* initialize_state_12712 */
 0x1271f, /* initialize_state_1271f */
 0x1272c, /* initialize_state_1272c */
 0x12739, /* initialize_state_12739 */
 0x12746, /* initialize_state_12746 */
 0x12753, /* initialize_state_12753 */
 0x12760, /* initialize_state_12760 */
 0x1276d, /* initialize_state_1276d */
 0x1277a, /* initialize_state_1277a */
 0x1277b, /* initialize_state_1277b */
 0x1279c, /* initialize_state_1279c */
 0x11e2a, /* _ZN16CompilerPchState10InitializeEv */
 0x127a6, /* initialize_state_127a6 */
 0x127a7, /* c1_main */
 0x12830, /* initialize_work_buffers */
 0x12873, /* reserve_work_buffer */
 0x128b1, /* scan_internal_switches */
 0x12900, /* parse_internal_switch */
 0x12998, /* initialize_state_12998 */
 0x12999, /* initialize_compiler_arenas */
 0x129db, /* initialize_primary_arenas */
 0x12a18, /* _ZN13CompilerArena10InitializeEP23CompilerArenaDescriptorPv */
 0x12a7a, /* arena_reservation_bytes */
 0x12ab2, /* _ZN13CompilerArena5ResetEv */
 0x12ac3, /* initialize_default_pools */
 0x12b30, /* _ZN12CompilerPool8SetArenaEj */
 0xaa9c, /* _ZN12CompilerPool6RefillEj */
 0xaacb, /* _ZN13CompilerArena13AllocatePagesEjPj */
 0x9883, /* _ZN13CompilerArena12CommitExtentEv */
 0x12b49, /* parse_command_line */
 0x12df6, /* initialize_include_aliases */
 0x12e1c, /* initialize_checksums */
 0x12f08, /* initialize_multibyte_scanner */
 0x12f3e, /* initialize_path_fold_table */
 0x12f87, /* read_env_arguments */
 0x11e3c, /* new_option_vector */
 0x12f9c, /* next_argument */
 0x12fbe, /* match_option */
 0x131aa, /* apply_option_action */
 0x132f9, /* parse_option_integer */
 0x1335b, /* resolve_option_path */
 0x1344d, /* normalize_path */
 0x1348d, /* initialize_packing */
 0x134c9, /* allocate_packing_memory */
 0xd458, /* _ZN15CompilerPacking4PushEv */
 0xd49b, /* push_packing */
};
RECON_EXPORT const ReconCase case_descriptor = {
 RECON_ABI_VERSION, sizeof(ReconCase), "minimal-c",
 "Actual direct Invoke11/BOOL1 with test.c, partial at push_packing; HOST CRT3/env78 are separate private hash-bound inputs.",
 order, RECON_COUNT(order), NULL, 0, RECON_IO, RECON_FRONTIER, 0xd49b,
 setup, invoke, NULL, NULL, 0
};
