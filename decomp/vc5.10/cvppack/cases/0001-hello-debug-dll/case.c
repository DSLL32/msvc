#include <recon-tool.h>
extern int cvpack_native_setup(int,const char *);
extern int cvpack_main(int,char **,char **);
static int setup(void) { return cvpack_native_setup(1,"T:\\run-nOh4cQ\\CVPACK.EXE"); }
static int invoke(int argc,char **argv) { return cvpack_main(argc,argv,(char **)0); }
extern const ReconTest cvpack_tests[13];
static const uint32_t order[] = {
    0x4620, /* cvpack_main */
    0x7680, /* initialize_messages */
    0x48c0, /* parse_options */
    0x10a0, /* files_initialize */
    0x1f10, /* allocate_page */
    0x1010, /* allocate_zero */
    0x1190, /* allocate_file */
    0x4a30, /* select_input_name */
    0x1310, /* open_cached_file */
    0x1380, /* prepare_cached_file */
    0x1470, /* find_cached_file */
    0x1400, /* take_free_file */
    0x20a0, /* move_cached_file */
    0x1070, /* duplicate_string */
    0x2140, /* open_mapped_file */
    0x2480, /* create_mapping */
    0xaff0, /* initialize_type_tables */
    0xfb90, /* allocate_cleared */
    0xff60, /* initialize_arena */
    0xfb60, /* allocate_bytes */
    0x5d00, /* read_image_debug */
    0x1910, /* seek_cached_file */
    0x2270, /* mapped_seek */
    0x1980, /* read_cached_file */
    0x22f0, /* mapped_read */
    0x1a80, /* tell_cached_file */
    0x5690, /* read_codeview_signature */
    0x57b0, /* read_codeview_directory */
    0x5760, /* compare_directory_entries */
    0x5a10, /* classify_debug_directory */
    0x5910, /* load_debug_subsection */
    0x59a0, /* find_module_info */
    0x51e0, /* initialize_module_pack */
    0x4c10, /* pack_module */
    0x4b50, /* initialize_single_module */
    0x5130, /* read_module_symbols */
    0x5250, /* widen_subsection */
    0xaa00, /* pack_symbols */
    0x25d0, /* map_legacy_index */
    0x25f0, /* compact_legacy_index */
    0x25a0, /* map_primitive_type */
    0x2ea0, /* flush_type_stack */
    0xd1f0, /* insert_public_symbol */
    0xcf50, /* symbol_record_key */
    0xb1d0, /* allocate_public_block */
    0x4ee0, /* read_module_types */
    0xc100, /* read_new_module_types */
    0xc720, /* initialize_module_type_storage */
    0xc760, /* reset_type_name_cache */
    0x5070, /* read_module_local_symbols */
    0x53c0, /* read_module_source_lines */
    0x3080, /* map_type_index */
    0x30a0, /* compact_type_index */
    0xa380, /* scan_module_symbols */
    0xa730, /* add_symbol_segment */
    0xdef0, /* has_symbol_segment */
    0xa790, /* rewrite_module_symbols */
    0xdf30, /* write_segment_record */
    0xad00, /* rewrite_object_name_symbol */
    0xaec0, /* rewrite_procedure32_symbol */
    0xaf80, /* module_contains_address */
    0x3c20, /* compact_type_list */
    0xb210, /* intern_type */
    0xe1a0, /* hash_type_record */
    0xb490, /* allocate_global_type */
    0xff80, /* copy_type_arena */
    0xac70, /* rewrite_bp_relative_symbol */
    0xccb0, /* match_forward_type */
    0xce70, /* lookup_named_type */
    0xcda0, /* find_local_named_type */
    0xb3c0, /* record_base_classes */
    0xabc0, /* leave_symbol_scope */
    0x3a90, /* compact_legacy_pointer */
    0xacb0, /* rewrite_type_name_symbol */
    0xd430, /* insert_global_symbol */
    0x4360, /* add_type_reference */
    0xcc10, /* resolve_recursive_type */
    0xe640, /* hash_recursive_type */
    0x8900, /* intern_recursive_type */
    0xc080, /* mark_recursive_type */
    0xe820, /* hash_type_reference */
    0xb890, /* index_recursive_type */
    0x2c20, /* lookup_global_type */
    0x8650, /* compare_recursive_type */
    0xdd60, /* link_symbol_scopes */
    0xf4a0, /* record_symbol_references */
    0xf400, /* record_symbol_reference */
    0x5470, /* rewrite_module_global_types */
    0x10290, /* create_widen */
    0x102e0, /* query_widen */
    0x10300, /* emit_widen */
    0x102b0, /* release_widen */
    0xab90, /* enter_symbol_scope */
    0x5230, /* free_widen_buffers */
    0xb140, /* free_type_tables */
    0xdf60, /* finalize_derived_types */
    0xf100, /* build_source_file_index */
    0xf0d0, /* append_source_name */
    0x69e0, /* flush_packed_file */
    0x63f0, /* compare_packed_directory_entries */
    0x100e0, /* begin_buffered_output */
    0x68a0, /* write_subsection */
    0xd600, /* write_publics */
    0xf960, /* insert_symbol_page_padding */
    0xe920, /* build_symbol_name_hash */
    0x10010, /* fill_type_arena */
    0xede0, /* build_symbol_address_hash */
    0xedc0, /* compare_symbol_address */
    0x100c0, /* first_hash_output_block */
    0x100d0, /* next_hash_output_block */
    0xd9a0, /* write_globals */
    0xf5f0, /* write_static_symbols */
    0x6460, /* write_global_types */
    0xd130, /* prepare_type_offsets */
    0x10240, /* end_buffered_output */
    0x1cc0, /* write_cached_file */
    0x2360, /* mapped_write */
    0x11b0, /* resize_cached_file */
    0x1680, /* close_file */
    0x23d0, /* close_mapped_file */
    0x1000, /* free_bytes */
};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="hello-debug-dll",.description="Embedded CodeView DLL; complete NB11 packing",
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .execution=RECON_IO,.terminal=RECON_EXIT,.frontier_rva=0,
 .setup=setup,.io=invoke,.tests=cvpack_tests,.test_count=13
};
