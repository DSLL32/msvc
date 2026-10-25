#include <recon-tool.h>
extern int cvpack_native_setup(int,const char *);
extern int cvpack_main(int,char **,char **);
/* No Widen oracle is installed for this different DLL: unknown requests deny.
 * Module path is unconsumed host metadata, not an asserted loader-path match. */
static int setup(void) { return cvpack_native_setup(0,"R:\\toolchains\\5.10\\bin\\CVPACK.EXE"); }
static int invoke(int argc,char **argv) { return cvpack_main(argc,argv,(char **)0); }
static const uint32_t order[] = {
    0x4620, /* _main */
    0x7680, /* _initialize_messages */
    0x48c0, /* _parse_options */
    0x10a0, /* _files_initialize */
    0x1f10, /* _allocate_page */
    0x1010, /* _allocate_zero */
    0x1190, /* _allocate_file */
    0x4a30, /* _select_input_name */
    0x1310, /* _open_cached_file */
    0x1380, /* _prepare_cached_file */
    0x1470, /* _find_cached_file */
    0x1400, /* _take_free_file */
    0x20a0, /* _move_cached_file */
    0x1070, /* _duplicate_string */
    0x2140, /* _open_mapped_file */
    0x2480, /* _create_mapping */
    0xaff0, /* _initialize_type_tables */
    0xfb90, /* _allocate_cleared */
    0xff60, /* _initialize_arena */
    0xfb60, /* _allocate_bytes */
    0x5d00, /* _read_image_debug */
    0x1910, /* _seek_cached_file */
    0x2270, /* _mapped_seek */
    0x1980, /* _read_cached_file */
    0x22f0, /* _mapped_read */
    0x1a80, /* _tell_cached_file */
    0x5690, /* _read_codeview_signature */
    0x57b0, /* _read_codeview_directory */
    0x5760, /* _compare_directory_entries */
    0x5a10, /* _classify_debug_directory */
    0x5910, /* _load_debug_subsection */
    0x59a0, /* _find_module_info */
    0x51e0, /* _initialize_module_pack */
    0x4c10, /* _pack_module */
    0x4b50, /* _initialize_single_module */
    0x5130, /* _read_module_symbols */
    0x5250, /* _widen_subsection */
    0xaa00, /* _pack_symbols */
    0x25d0, /* @map_legacy_index@4 */
    0x25f0, /* @compact_legacy_index@4 */
    0x25a0, /* _map_primitive_type */
    0x2ea0, /* _flush_type_stack */
    0xd1f0, /* _insert_public_symbol */
    0xcf50, /* _symbol_record_key */
    0xb1d0, /* _allocate_public_block */
    0x4ee0, /* _read_module_types */
    0xc100, /* _read_new_module_types */
    0xc720, /* _initialize_module_type_storage */
    0xc760, /* _reset_type_name_cache */
    0x5070, /* _read_module_local_symbols */
    0x53c0, /* _read_module_source_lines */
    0x3080, /* @map_type_index@4 */
    0x30a0, /* @compact_type_index@4 */
    0xa380, /* _scan_module_symbols */
    0xa730, /* _add_symbol_segment */
    0xdef0, /* _has_symbol_segment */
    0xa790, /* _rewrite_module_symbols */
    0xdf30, /* _write_segment_record */
    0xad00, /* _rewrite_object_name_symbol */
    0xaec0, /* _rewrite_procedure32_symbol */
    0xaf80, /* _module_contains_address */
    0x3c20, /* @compact_type_list@8 */
    0x3a90, /* @compact_legacy_pointer@4 */
    0xccb0, /* @match_forward_type@12 */
    0xcda0, /* _find_local_named_type */
    0xce70, /* _lookup_named_type */
    0x2c20, /* _lookup_global_type */
    0x4360, /* @add_type_reference@8 */
    0xb210, /* @intern_type@4 */
    0xe1a0, /* @hash_type_record@4 */
    0xb490, /* @allocate_global_type@4 */
    0xff80, /* @copy_type_arena@12 */
    0xcc10, /* _resolve_recursive_type */
    0xe640, /* @hash_recursive_type@4 */
    0xe820, /* @hash_type_reference@8 */
    0x8900, /* @intern_recursive_type@4 */
    0xc080, /* @mark_recursive_type@4 */
    0xb890, /* @index_recursive_type@8 */
    0xb3c0, /* _record_base_classes */
    0xfcb0, /* _numeric_leaf_size */
    0xac70, /* _rewrite_bp_relative_symbol */
    0xabc0, /* _leave_symbol_scope */
    0x8650, /* @compare_recursive_type@16 */
    0xacb0, /* _rewrite_type_name_symbol */
    0xd430, /* @insert_global_symbol@8 */
    0xdd60, /* _link_symbol_scopes */
    0xf4a0, /* _record_symbol_references */
    0xf400, /* _record_symbol_reference */
    0x5470, /* _rewrite_module_global_types */
    0xe520, /* _record_base_relation */
    0x5230, /* _free_widen_buffers */
    0xb140, /* _free_type_tables */
    0xdf60, /* _finalize_derived_types */
    0xfae0, /* _allocate_type_buffer */
    0xf100, /* _build_source_file_index */
    0xf0d0, /* _append_source_name */
    0x69e0, /* _flush_packed_file */
    0x63f0, /* _compare_packed_directory_entries */
    0x100e0, /* _begin_buffered_output */
    0x68a0, /* _write_subsection */
    0xd600, /* _write_publics */
    0xf960, /* _insert_symbol_page_padding */
    0xe920, /* _build_symbol_name_hash */
    0x10010, /* _fill_type_arena */
    0xede0, /* _build_symbol_address_hash */
    0xedc0, /* _compare_symbol_address */
    0x100c0, /* _first_hash_output_block */
    0x100d0, /* _next_hash_output_block */
    0xd9a0, /* _write_globals */
    0x10140, /* _buffered_write */
    0x1cc0, /* _write_cached_file */
    0x2360, /* _mapped_write */
    0xf5f0, /* _write_static_symbols */
    0x6460, /* _write_global_types */
    0xd130, /* _prepare_type_offsets */
    0x10240, /* _end_buffered_output */
    0x11b0, /* _resize_cached_file */
    0x1680, /* _close_file */
    0x23d0, /* _close_mapped_file */
    0x1000, /* _free_bytes */
};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="cpp-scene-growth",
 .description="Three C++ modules with inheritance, recursive telemetry schemas and large CodeView output",
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .execution=RECON_IO,.terminal=RECON_EXIT,.frontier_rva=0,
 .setup=setup,.io=invoke
};
