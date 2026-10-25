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
    0xee20, /* warning */
    0x198a0, /* files_initialize */
    0x1a8e0, /* new_page */
    0x19a10, /* new_file_slot */
    0x19cf0, /* open_file */
    0x19b20, /* find_cached_file */
    0x19ac0, /* acquire_file_slot */
    0x1a990, /* move_file_slot */
    0x1aa30, /* open_mapped_file */
    0x1ad30, /* map_file */
    0x93a0, /* initialize_type_tables */
    0xf060, /* checked_zero */
    0x12f20, /* initialize_buffer */
    0xf030, /* checked_alloc */
    0xf250, /* read_image_debug */
    0x1a2e0, /* seek_file */
    0x1ab50, /* seek_mapped_file */
    0x1a400, /* read_file */
    0x1abb0, /* read_mapped_file */
    0x1a430, /* tell_file */
    0x10dd0, /* read_cv_signature */
    0x11540, /* read_new_directory */
    0x113a0, /* compare_directory */
    0xfb60, /* initialize_modules */
    0xff70, /* read_subsection */
    0x10010, /* find_module */
    0x76f0, /* pack_module */
    0x7fd0, /* read_module */
    0xeea0, /* module_name */
    0x7b60, /* read_global_types */
    0x9e50, /* read_new_global_types */
    0xa270, /* reset_local_types */
    0xa2b0, /* reset_type_hash */
    0x7e20, /* read_publics */
    0x7d80, /* read_types */
    0x7f40, /* read_source_module */
    0x152a0, /* pack_public_symbols */
    0x164e0, /* convert_new_type */
    0xb310, /* intern_public */
    0xafe0, /* hash_public */
    0x9530, /* allocate_symbol */
    0x14cc0, /* index_new_types */
    0x14fa0, /* reserve_extra_type */
    0xbea0, /* has_extra_type */
    0x15000, /* pack_new_types */
    0xbee0, /* write_search_symbol */
    0x155c0, /* convert_object_name */
    0x15660, /* convert_global_data16 */
    0xb500, /* intern_global */
    0x15620, /* convert_local_data16 */
    0x16520, /* compact_new_type */
    0xad40, /* find_named_type */
    0xaf10, /* find_global_type */
    0x172a0, /* compact_type_list */
    0x17100, /* compact_pointer */
    0x17b10, /* add_recursive_offset */
    0x9580, /* store_type */
    0xc1a0, /* hash_type */
    0x9860, /* save_type */
    0x12f40, /* append_buffer */
    0xac80, /* intern_type */
    0xc640, /* hash_recursive_type */
    0xeac0, /* commit_recursive_type */
    0x9ca0, /* publish_recursive_type */
    0xc880, /* hash_referenced_type */
    0x9c20, /* publish_packed_type */
    0x9770, /* record_derived_types */
    0x162d0, /* finish_recursive_types */
    0x15740, /* convert_procedure16 */
    0x15920, /* address_in_module */
    0x154b0, /* convert_bp_relative16 */
    0x15450, /* begin_symbol_block */
    0x15480, /* end_symbol_block */
    0xbd60, /* link_symbol_scopes */
    0xd840, /* record_symbol_offsets */
    0xd790, /* create_symbol_reference */
    0x83c0, /* pack_old_lines */
    0xe780, /* compare_recursive_type */
    0x15980, /* convert_old_type */
    0x17d80, /* map_primitive */
    0x94f0, /* finalize_types */
    0xbf10, /* finalize_derived_types */
    0xd440, /* build_source_file_index */
    0xd390, /* find_source_name */
    0xd350, /* hash_source_name */
    0xd3f0, /* add_source_name */
    0x100a0, /* write_packed_debug */
    0x11320, /* compare_output_directory */
    0x18090, /* initialize_output */
    0x10cb0, /* write_subsection */
    0xb6d0, /* write_global_publics */
    0xdde0, /* pad_symbols */
    0xcb90, /* build_symbol_name_hash */
    0x13050, /* append_fill */
    0xd070, /* build_symbol_address_hash */
    0xd050, /* compare_symbol_address */
    0x130e0, /* first_buffer_block */
    0x130f0, /* next_buffer_block */
    0xba10, /* write_global_symbols */
    0xdac0, /* write_static_symbols */
    0x10ec0, /* write_packed_types */
    0xb240, /* build_type_offsets */
    0x181f0, /* flush_output */
    0x1a670, /* write_file */
    0x1ac00, /* write_mapped_file */
    0x19ba0, /* truncate_file */
    0x19ed0, /* close_file */
    0x1ac60, /* close_mapped_file */
};
RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase),
    "dos-mapped-growth-feedback", "Mapped DOS growth and unknown-option warning with captured IDE feedback streams.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_EXIT, 0,
    NULL, invoke, NULL, NULL, 0
};
