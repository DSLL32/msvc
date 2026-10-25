/* Invocation descriptor; native DLL adapter is in harness/dll-cases.c. */
#include <stdint.h>
#include "dll-cases.h"
#include "case.h"
#include <recon-tool.h>
#include <recon.h>
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
 0x134f4, /* compile_translation_unit */
 0x1354b, /* run_compiler */
 0x138e4, /* initialize_frontend */
 0x13f0e, /* create_input */
};
RECON_EXPORT const ReconCase case_descriptor = {
 RECON_ABI_VERSION, sizeof(ReconCase), "minimal-c",
 "Typed host forwards actual Invoke11/BOOL1 with test.c; only MSC_CMD_FLAGS absence is an environment input.",
 order, RECON_COUNT(order), NULL, 0, RECON_IO, RECON_FRONTIER, 0x13f0e,
 c1_minimal_setup, c1_minimal_invoke, NULL, minimal_tests, 1
};
