#include "cases.h"
#include <recon-tool.h>
#include <recon.h>

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
    0x398cd, /* normalize_path */
    0x3990d, /* initialize_packing */
    0x39949, /* allocate_packing_storage */
    0x2b85a, /* CompilerPacking::AcquireFrame */
    0x2b89d, /* push_packing */
    0x39974, /* compile_translation_unit */
    0x399cb, /* run_compiler */
    0x39d42, /* initialize_frontend */
    0x3a434, /* create_input */
    0x3a483, /* normalize_frontend_options */
    0x3a4a7, /* begin_browser_output */
    0x7ca9, /* acquire_initial_backend */
    0x3a4d3, /* initialize_preprocessor */
    0x3a54b, /* initialize_keyword_table */
    0x3a57c, /* initialize_language_keywords */
    0x3aa9a, /* intern_identifier */
    0x3abf1, /* unreserve_keyword */
    0x3ac0b, /* set_keyword_category3 */
    0x3ac37, /* set_keyword_category1 */
    0x3ac63, /* set_keyword_category5 */
    0x3ac8f, /* set_keyword_category4 */
    0x3acbb, /* initialize_line_tables */
    0x3acbc, /* initialize_frontend_tables */
    0x14954, /* new_pointer_array */
    0x149ba, /* pointer_array_size */
    0x3acef, /* open_expression_output */
    0x3bd53, /* OutputBuffer::Open */
    0x3fc2, /* make_integer_constant */
    0x3fdc, /* allocate_integer_constant */
    0x3296, /* write_ir_record */
    0x11b1, /* write_current_source_line */
    0x11c3, /* OutputBuffer::Ensure */
    0x2e323, /* OutputBuffer::Grow */
    0x3982, /* OutputBuffer::Flush */
    0x3ae14, /* initialize_type_table */
    0x1000, /* intern_type */
    0x3aea7, /* initialize_global_scope */
    0x3aebc, /* initialize_builtin_symbols */
    0x6608, /* find_or_create_identifier */
    0x3aee6, /* CompilerTranslationUnit::Initialize */
    0x6897, /* recycle_symbol */
    0x690d, /* CompilerArena::Allocate */
    0x7cb7, /* CompilerSymbol::Initialize */
    0x3b05d, /* initialize_symbol_groups */
    0x1d287, /* create_hash_table */
    0x3840e, /* make_function_type */
    0x3849a, /* CompilerDeclarator::Initialize */
    0x692c, /* make_abstract_declarator */
    0x7d17, /* CompilerDeclarator::AddFunction */
    0x7e1a, /* CompilerParameterDescription::Normalize */
    0x7e51, /* create_type_shape */
    0x7e8d, /* CompilerDeclarator::AppendShape */
    0x23667, /* CompilerDeclarator::ApplyQualifiers */
    0x7f68, /* CompilerDeclarator::Resolve */
    0x81cc, /* CompilerDeclarator::NormalizeBase */
    0x8334, /* find_declarator_type */
    0x83d3, /* link_parameter_symbols */
    0x8408, /* CompilerTypeShape::SetParameters */
    0x842e, /* create_function_parameters */
    0x1db84, /* empty_parameter_types */
    0x84f9, /* intern_function_type */
    0x85c5, /* CompilerDeclarator::ValidateShapes */
    0x8730, /* CompilerDeclarator::DefaultCallingConvention */
    0x87b2, /* CompilerDeclarator::ValidateContext */
    0x87c3, /* CompilerTypeShape::InternChain */
    0x10cc, /* intern_type_shape */
    0x87ed, /* CompilerDeclarator::BuildType */
    0x3b077, /* make_tagged_type */
    0x1d81d, /* normalize_shape_attributes */
    0x11f9, /* create_declarator_shape */
    0x3b10c, /* initialize_intermediate_output */
    0x3b10d, /* initialize_translation_state */
    0x23688, /* allocate_symbol_storage */
    0x3b253, /* define_command_line_macros */
};
extern int c1xx_test_exception_counters(void);
extern int c1xx_test_input_alias(void);
extern int c1xx_test_browser_aliases(void);
extern int c1xx_test_output_growth(void);
extern int c1xx_test_scope_table(void);
extern int c1xx_test_recycled_symbol(void);
static const ReconTest tests[] = {
    { "frontend-state-restore", 1, c1xx_test_exception_counters },
    { "input-reset-alias", 1, c1xx_test_input_alias },
    { "browser-method-aliases", 1, c1xx_test_browser_aliases },
    { "output-buffer-growth", 1, c1xx_test_output_growth },
    { "scope-virtual-table", 1, c1xx_test_scope_table },
    { "recycled-symbol-record", 1, c1xx_test_recycled_symbol }
};
RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase), "minimal-cpp",
    "Actual CL-derived Invoke26/BOOL1 on minimal.cpp; accepted partial compilation ends at the genuine define_command_line_macros INT3. Typed loader host forwards the actual 26 arguments; MSC_CMD_FLAGS is absent, with no claim about unconsumed Windows environment names.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_FRONTIER, 0x3b253, c1xx_case_minimal_setup, c1xx_case_minimal, NULL, tests, RECON_COUNT(tests)
};
