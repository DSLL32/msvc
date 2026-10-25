/* Cached original invocation; no original execution during migration. */
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <recon-tool.h>
#include "../padding-check.h"
static int check_outputs(const ReconOutputContext *context)
{
    return check_padding(context, "hello-debug.dll", 71800, 0x110ae, 0x110b0, 1);
}
RECON_EXPORT const ReconOutputPolicy case_output_policy = {
    RECON_OUTPUT_POLICY_VERSION, sizeof(ReconOutputPolicy), check_outputs
};
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static int setup(void) {
    char path[PATH_MAX];
    if (!getcwd(path, sizeof(path)) || strlen(path)+9 >= sizeof(path)) return 0;
    strcat(path, "/drive-z");
    return setenv("RECON_DRIVE_Z", path, 1) == 0;
}
static const uint32_t order[] = {
    0x7000, /* main */
    0x74c0, /* parse_options */
    0x198a0, /* files_initialize */
    0x1a8e0, /* new_page */
    0x19a10, /* new_file_slot */
    0x19cf0, /* open_file */
    0x19b20, /* find_cached_file */
    0x19ac0, /* acquire_file_slot */
    0x1a990, /* move_file_slot */
    0x19db0, /* open_buffered_file */
    0x1a1f0, /* seek_buffered_file */
    0x1a910, /* allocate_file_page */
    0x1a7f0, /* attach_page */
    0x1a540, /* read_file_page */
    0x1a6a0, /* touch_page */
    0x93a0, /* initialize_type_tables */
    0xf060, /* checked_zero */
    0x12f20, /* initialize_buffer */
    0xf030, /* checked_alloc */
    0xf250, /* read_image_debug */
    0x1a2e0, /* seek_file */
    0x1a400, /* read_file */
    0x1a350, /* read_buffered_file */
    0x1a430, /* tell_file */
    0x10dd0, /* read_cv_signature */
    0x11540, /* read_new_directory */
    0x113a0, /* compare_directory */
    0xfb60, /* initialize_modules */
    0xff70, /* read_subsection */
    0x10010, /* find_module */
    0x76f0, /* pack_module */
    0x7fd0, /* read_module */
    0x7e20, /* read_publics */
    0x152a0, /* pack_public_symbols */
    0x15980, /* convert_old_type */
    0x17d80, /* map_primitive */
    0xb310, /* intern_public */
    0xafe0, /* hash_public */
    0x9530, /* allocate_symbol */
    0x7b60, /* read_global_types */
    0x9e50, /* read_new_global_types */
    0xa270, /* reset_local_types */
    0xa4d0, /* import_type_server */
    0x1b968, /* type_server_open */
    0x1aef0, /* type_server_open_file */
    0x1b14c, /* type_server_read_records */
    0x1bca8, /* type_server_signature */
    0x1bcb4, /* type_server_age */
    0xa970, /* load_type_server_records */
    0x1bcc0, /* type_server_first */
    0x1bccc, /* type_server_limit */
    0x1badc, /* type_server_record */
    0xa2b0, /* reset_type_hash */
    0x1a5c0, /* advance_file_page */
    0x1a120, /* flush_file_page */
    0x7d80, /* read_types */
    0x1a850, /* release_page */
    0x164e0, /* convert_new_type */
    0x14cc0, /* index_new_types */
    0x14fa0, /* reserve_extra_type */
    0xbea0, /* has_extra_type */
    0x15000, /* pack_new_types */
    0xbee0, /* write_search_symbol */
    0x155c0, /* convert_object_name */
    0x15800, /* convert_udt_symbol */
    0xb500, /* intern_global */
    0x16520, /* compact_new_type */
    0x17100, /* compact_pointer */
    0x172a0, /* compact_type_list */
    0x9580, /* store_type */
    0xc1a0, /* hash_type */
    0x9860, /* save_type */
    0x12f40, /* append_buffer */
    0x162d0, /* finish_recursive_types */
    0xad40, /* find_named_type */
    0xaf10, /* find_global_type */
    0xae40, /* find_local_type */
    0x9770, /* record_derived_types */
    0x156f0, /* convert_global_data32 */
    0xe700, /* get_type_record */
    0x17b10, /* add_recursive_offset */
    0xac80, /* intern_type */
    0xc640, /* hash_recursive_type */
    0xeac0, /* commit_recursive_type */
    0x9ca0, /* publish_recursive_type */
    0xc880, /* hash_referenced_type */
    0x9c20, /* publish_packed_type */
    0x157a0, /* convert_procedure32 */
    0x15920, /* address_in_module */
    0x154f0, /* convert_bp_relative32 */
    0x15530, /* convert_register_symbol */
    0x15480, /* end_symbol_block */
    0xbd60, /* link_symbol_scopes */
    0xd840, /* record_symbol_offsets */
    0xd790, /* create_symbol_reference */
    0x15450, /* begin_symbol_block */
    0x18240, /* strip_debug */
    0x19550, /* initialize_strip_sections */
    0x18790, /* read_coff_symbol */
    0x18d90, /* classify_coff_symbol */
    0x18800, /* next_coff_symbol */
    0x18410, /* collect_strip_records */
    0x18630, /* find_first_coff_function */
    0x188f0, /* read_coff_auxiliary */
    0x18cb0, /* fill_coff_function */
    0x18940, /* read_coff_file_name */
    0x18990, /* set_coff_function_name */
    0x189f0, /* assign_coff_function_module */
    0x18b00, /* read_coff_function_block */
    0x18ae0, /* set_coff_function_section */
    0x18af0, /* set_coff_function_line */
    0x18ac0, /* set_coff_function_offset */
    0x18b90, /* read_coff_function_lines */
    0x18ad0, /* set_coff_function_end */
    0x18e00, /* publish_coff_function */
    0x18700, /* find_next_coff_function */
    0x19190, /* write_strip_records */
    0x19850, /* free_strip_records */
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
    0x1a450, /* write_buffered_file */
    0x19ba0, /* truncate_file */
    0x19c50, /* truncate_buffered_file */
    0x19ed0, /* close_file */
    0x19f50, /* release_file_slot */
    0x1bf00, /* update_image_checksum */
    0x1be60, /* checksum_image */
    0x1c0d0, /* checksum_words */
    0x1bff0, /* image_nt_header */
};
RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase),
    "hello-debug-dll", "Buffered PE packing with an owned PDB input; exact 71,800-byte packed DLL.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_EXIT, 0,
    setup, invoke, NULL, NULL, 0
};
