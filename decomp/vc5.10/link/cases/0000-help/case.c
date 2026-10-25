#include <recon-tool.h>
#include <recon.h>
extern int recon_link_setup(void);
extern int recon_link_invoke(int,char **);
static const uint32_t order[] = {
 0x24e60, /* _initialize_buffer12 */
 0x24e70, /* _allocate_buffer12 */
 0x24ea0, /* _register_buffer12_cleanup */
 0x31d70, /* _initialize_buffer8 */
 0x31d80, /* _allocate_buffer8 */
 0x31db0, /* _register_buffer8_cleanup */
 0x31de0, /* _initialize_buffer4 */
 0x31df0, /* _allocate_buffer4 */
 0x31e20, /* _register_buffer4_cleanup */
 0x44e60, /* _initialize_timing */
 0x44e70, /* _setup_timing */
 0x44ea0, /* _register_timing_cleanup */
 0xde90, /* _main */
 0x15230, /* _initialize_debug@8 */
 0xb560, /* _initialize_file_cache@16 */
 0x152f0, /* _get_debug_flag@4 */
 0x4fe70, /* _zero_allocate@4 */
 0xb660, /* _new_file_record */
 0x4f560, /* _initialize_memory */
 0x48cb0, /* _run_linker@8 */
 0x44b10, /* _initialize_repro */
 0x5caa0, /* _collect_file_arguments@16 */
 0x5cb20, /* _parse_command_line@16 */
 0x5c780, /* _parse_argument@8 */
 0x5c750, /* _is_libpath@4 */
 0x5cca0, /* _append_argument@8 */
 0x5cc00, /* _append_argument_record@12 */
 0x44f20, /* _has_switch@4 */
 0x4fd40, /* _allocate_link_memory@8 */
 0x337d0, /* LinkContext::Initialize */
 0x62220, /* _create_string_table@12 */
 0x30c70, /* _initialize_hash_table@20 */
 0x30c30, /* _hash_allocate@16 */
 0x4f5e0, /* _reserve_memory@8 */
 0x4f640, /* _create_memory_arena@4 */
 0x4fe50, /* _allocate_memory@4 */
 0x45130, /* _configure_link_options@12 */
 0xd0c0, /* _parse_switch@8 */
 0x4fed0, /* _duplicate_argument@4 */
 0x44f80, /* _linker_usage */
 0x615b0, /* _print_logo */
 0x44eb0, /* _close_timing */
 0x31e30, /* _free_buffer4 */
 0x31dc0, /* _free_buffer8 */
 0x24eb0, /* _free_buffer12 */
};
RECON_EXPORT const ReconCase case_descriptor = {
 RECON_ABI_VERSION,sizeof(ReconCase),"help",
 "Completed /? invocation.",
 order,RECON_COUNT(order),NULL,0,RECON_IO,RECON_EXIT,0,
 recon_link_setup,recon_link_invoke,NULL,NULL,0
};
