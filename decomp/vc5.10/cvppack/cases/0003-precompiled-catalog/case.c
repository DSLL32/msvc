#include <recon-tool.h>
extern int cvpack_native_setup(int,const char *);
extern int cvpack_main(int,char **,char **);
/* No external Widen response is assumed for a different input. */
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
    0x4ee0, /* _read_module_types */
    0xc100, /* _read_new_module_types */
    0xc720, /* _initialize_module_type_storage */
    0xc7b0, /* @append_module_type@20 */
    0xc760, /* _reset_type_name_cache */
    0x2c90, /* _prepare_precompiled_types */
    0x3080, /* @map_type_index@4 */
    0x30a0, /* @compact_type_index@4 */
    0xccb0, /* @match_forward_type@12 */
    0xcda0, /* _find_local_named_type */
    0xce70, /* _lookup_named_type */
    0xb210, /* @intern_type@4 */
    0xe1a0, /* @hash_type_record@4 */
    0xb490, /* @allocate_global_type@4 */
    0xff80, /* @copy_type_arena@12 */
    0x2ea0, /* _flush_type_stack */
    0x3a90, /* @compact_legacy_pointer@4 */
    0x3c20, /* @compact_type_list@8 */
    0x2e50, /* @associate_forward_type@8 */
    0x5070, /* _read_module_local_symbols */
    0x5250, /* _widen_subsection */
    0x5130, /* _read_module_symbols */
    0x53c0, /* _read_module_source_lines */
    0xaa00, /* _pack_symbols */
    0xd1f0, /* _insert_public_symbol */
    0xcf50, /* _symbol_record_key */
    0xb1d0, /* _allocate_public_block */
    0x5470, /* _rewrite_module_global_types */
    0xc930, /* _use_precompiled_types */
    0xcb70, /* _find_precompiled_module */
    0x54d0, /* _fatal_error */
    0x7610, /* _message_text */
    0x1680, /* _close_file */
    0x23d0, /* _close_mapped_file */
    0x1000, /* _free_bytes */
};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="precompiled-catalog",.description="Shared C++ precompiled schema catalog, derived-type growth and minimum verbose packing",
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .execution=RECON_IO,.terminal=RECON_EXIT,.frontier_rva=0,.setup=setup,.io=invoke
};
