/* Native-only host glue: host file reads do not consume target CRT streams.
 * The public DLL pass uses its own Invoke vector, distinct from HOST CRT argv. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <errno.h>
#include "frontend.h"
#include <recon-tool.h>
#include <recon.h>

/* Original debugger vectors preserve ordered bytes; private vectors are explicit
 * hash-bound readonly fixture dependencies, never copied into this repository. */
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
    const char *reference = "b3d9cd5dbedf70b9083e516bca9b28cc3cf7af12cd26974e9fd92b39bde699d8";
    recon_load_system_info_profile("system-info-profile.bin", reference);
    recon_crt_bind_empty_ctype_1252(
        "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210", reference);
    recon_controlfp(_PC_53, _MCW_PC);
    recon_apply_x87_empty_entry_profile(reference, 0x027f, 0x0020, 0xffff);
    return 1;
}
static int invoke(int argc, char **argv)
{
    ReconModuleSpec spec = { "C1XX.DLL", "R:\\toolchains\\5.10\\bin\\C1XX.DLL",
        "e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9", RECON_MODULE_NO_TLS };
    HINSTANCE module = recon_register_module(&spec);
    char **host_arguments, **host_environment;
    int host_argc, result;
    host_argc = read_vector("/root/.ai-tmp/msvc2/vc5.10/c1xx/minimal-option-append-native-1/original-crt-argv.bin", &host_arguments);
    if (host_argc != 3 || read_vector("/root/.ai-tmp/msvc2/vc5.10/c1xx/minimal-option-append-native-1/original-crt-environment.bin", &host_environment) < 0) return 126;
    /* Host file reads precede the captured loader context; they are not target API calls. */
    errno = 0;
    recon_doserrno = 0;
    recon_SetLastError(203);
    if (!module || !_DllMainCRTStartup(module, DLL_PROCESS_ATTACH, NULL)) return 126;
    recon_crt_bind_arguments(host_argc, host_arguments, host_environment);
    result = InvokeCompilerPass(argc, argv, 1);
    if (!_DllMainCRTStartup(module, DLL_PROCESS_DETACH, NULL)) return 126;
    return result;
}
static const uint32_t order[] = {
    0x2e52a, /* DllMain */
    0x384d4, /* InvokeCompilerPass */
    0x38550, /* initialize_compiler_pass */
    0x233ba, /* checked_malloc */
    0x38610, /* initialize_frontend_01 */
    0x38622, /* initialize_frontend_02 */
    0x38634, /* initialize_frontend_03 */
    0x3864f, /* initialize_frontend_04 */
    0x38650, /* initialize_frontend_05 */
    0x38651, /* initialize_frontend_06 */
    0x38652, /* initialize_frontend_07 */
    0x3865f, /* initialize_frontend_08 */
    0x38666, /* initialize_output_buffer_01 */
    0x386ae, /* register_cleanup */
    0x386d4, /* initialize_frontend_09 */
    0x386db, /* initialize_output_buffer_02 */
    0x38723, /* initialize_frontend_10 */
    0x3872a, /* initialize_output_buffer_03 */
    0x38772, /* initialize_frontend_11 */
    0x38779, /* initialize_output_buffer_04 */
    0x387c1, /* initialize_frontend_12 */
    0x387c8, /* initialize_output_buffer_05 */
    0x38810, /* initialize_frontend_13 */
    0x38817, /* initialize_output_buffer_06 */
    0x3885f, /* initialize_frontend_14 */
    0x38866, /* initialize_output_buffer_07 */
    0x388ae, /* initialize_frontend_15 */
    0x388b5, /* initialize_output_buffer_08 */
    0x388fd, /* initialize_frontend_16 */
    0x38904, /* initialize_deferred_work */
    0x3891d, /* initialize_frontend_17 */
    0x3891e, /* initialize_frontend_18 */
    0x3893b, /* initialize_frontend_19 */
    0x3893c, /* initialize_frontend_20 */
    0x3893d, /* initialize_frontend_21 */
    0x38948, /* initialize_frontend_22 */
    0x3894f, /* initialize_pdb_context_01 */
    0x38988, /* initialize_frontend_23 */
    0x3898f, /* initialize_pdb_context_02 */
    0x389c8, /* initialize_frontend_24 */
    0x389d5, /* _ZN12CompilerPool10InitializeEj */
    0x389eb, /* initialize_frontend_25 */
    0x389f8, /* initialize_frontend_26 */
    0x38a05, /* initialize_frontend_27 */
    0x38a12, /* initialize_frontend_28 */
    0x38a1f, /* initialize_frontend_29 */
    0x38a2c, /* initialize_frontend_30 */
    0x38a39, /* initialize_frontend_31 */
    0x38a46, /* initialize_frontend_32 */
    0x38a53, /* initialize_frontend_33 */
    0x38a60, /* initialize_frontend_34 */
    0x38a6b, /* initialize_type_ranks */
    0x38ab6, /* initialize_frontend_35 */
    0x38ab7, /* initialize_frontend_36 */
    0x38ad8, /* initialize_frontend_37 */
    0x3bd41, /* _ZN10PchContext10InitializeEv */
    0x38ae2, /* initialize_frontend_38 */
    0x38aef, /* initialize_frontend_39 */
    0x38af9, /* initialize_generated_comment */
    0x38be9, /* initialize_frontend_40 */
    0x38bea, /* initialize_frontend_41 */
    0x38bfc, /* initialize_frontend_42 */
    0x38bfd, /* c1xx_main */
    0x38c86, /* initialize_work_buffers */
    0x38cc9, /* reserve_work_buffer */
    0x38d07, /* initialize_intermediate_files */
    0x38d56, /* process_early_option */
    0x38df0, /* initialize_frontend_hook */
    0x38df1, /* initialize_compiler_environment */
    0x38e33, /* initialize_default_arena */
    0x38e70, /* _ZN13CompilerArena7ReserveEPK15ArenaDescriptorPh */
    0x38ed2, /* calculate_arena_reservation */
    0x38f0a, /* _ZN13CompilerArena5ResetEv */
    0x38f1b, /* initialize_pool_arenas */
    0x38f94, /* _ZN12CompilerPool9BindArenaEj */
    0x1ee29, /* _ZN12CompilerPool6RefillEj */
    0x1ee58, /* _ZN13CompilerArena13AllocatePagesEjPj */
    0x1d738, /* _ZN13CompilerArena12CommitExtentEv */
    0x38fad, /* process_compiler_options */
    0x2b834, /* push_declaration_context */
    0x39282, /* initialize_include_aliases */
    0x392a8, /* initialize_checksums */
    0x39394, /* initialize_multibyte_categories */
    0x393ca, /* initialize_lowercase_table */
    0x39413, /* read_env_arguments */
    0x682f, /* new_option_vector */
    0x39428, /* next_argument */
    0x3944a, /* match_option */
    0x39632, /* apply_option_action */
    0x39779, /* parse_option_integer */
    0x39757, /* match_option_suffix */
    0x100ea, /* append_option_vector */
    0x397db, /* resolve_option_path */
};
RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase), "minimal-cpp",
    "Actual CL-derived Invoke26/BOOL1 on minimal.cpp; accepted partial compilation ends at the genuine resolve_option_path INT3. HOST CRT3/private environment are separate authenticated inputs.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_FRONTIER, 0x397db, setup, invoke, NULL, NULL, 0
};
