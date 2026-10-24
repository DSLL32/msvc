/* Frozen original-backed public invocation: minimal.
 * Historical reference SHA256 f6df5eeeea56e10c0a644973ed5f2fa1ca4c8c54d3739ba46bd95181d8852b35.
 * Packing preserves captured bytes/status/paths; no new original execution. */
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <recon-tool.h>
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }

static const uint32_t order[] = {
    0x33850, /* main */
    0x3c350, /* message_text */
    0x1d500, /* parse_options */
    0xe4e0,  /* initialize_packing */
    0xe4c0,  /* new_packing_stack */
    0x34ba0, /* allocate_permanent */
    0x34b00, /* new_permanent_page */
    0x223b0, /* new_bitset */
    0xe480,  /* push_packing */
    0x3bf40, /* initialize_checksums */
    0x1d4b0, /* initialize_lead_bytes */
    0x1d450, /* mark_lead_byte_ranges */
    0x3b4b0, /* read_env_arguments */
    0x3b450, /* extend_argument_vector */
    0x3a570, /* checked_malloc */
    0x224c0, /* new_vector */
    0x3a600, /* refill_arena */
    0x1d420, /* next_argument */
    0x3b090, /* match_option */
    0x3b3d0, /* prefix_character */
    0x3b1b0, /* apply_option */
    0x3b330, /* parse_option_number */
    0x3b3f0, /* match_option_suffix */
    0x22780, /* append_vector */
    0x1d770, /* finalize_options */
    0x338d0, /* guarded_compile */
    0x33950, /* compile_translation_unit */
    0x32f30, /* initialize_frontend */
    0x19cf0, /* initialize_work_stack */
    0x19cd0, /* reset_work_stack */
    0xadb0,  /* initialize_preprocessor */
    0xb590,  /* initialize_macro_text */
    0xc330,  /* initialize_keyword_table */
    0xb460,  /* intern_keyword */
    0x3c380, /* initialize_floating_point */
    0xe450,  /* top_packing */
    0xc3f0,  /* disable_keyword */
    0xb330,  /* intern_identifier */
    0x3baa0, /* open_intermediate */
    0x362f0, /* open_expression_intermediate */
    0x178f0, /* make_integer_constant */
    0x178a0, /* new_integer_constant */
    0x295f0, /* write_ir_record */
    0x2fb20, /* initialize_type_table */
    0x23690, /* initialize_global_scope */
    0x2fd60, /* intern_type */
    0x2ff30, /* make_function_type */
    0x1cb30, /* normalize_type_attributes */
    0x305c0, /* new_type_shape */
    0x30820, /* build_parameter_info */
    0x307e0, /* empty_parameter_info */
    0x2fe40, /* intern_type_shape */
    0x30b60, /* make_tagged_type */
    0x30360, /* make_qualified_type */
    0x30530, /* copy_type_description */
    0x1d860, /* initialize_intermediate_output */
    0x1dbd0, /* builtin_enabled */
    0x1db30, /* mark_builtin */
    0x256a0, /* insert_scope_symbol */
    0x25990, /* initialize_translation_state */
    0xbf70,  /* define_command_line_macros */
    0x3be00, /* frontend_time */
    0x3bda0, /* format_timestamp */
    0xc550,  /* initialize_macro_storage */
    0xc8c0,  /* define_macro */
    0xca40,  /* lookup_macro */
    0xc190,  /* define_command_line_macro */
    0x3bce0, /* copy_arena_bytes */
    0x26b30, /* write_global_header */
    0x26e00, /* write_global_record */
    0xb680,  /* open_source_file */
    0x33e20, /* open_input_handle */
    0x33d80, /* normalize_input_name */
    0xbe70,  /* fill_source_buffer */
    0x3bee0, /* pack_option_flags */
    0x99a0,  /* scan_token */
    0x9a40,  /* scan_source_token */
    0x11bb0, /* begin_source_line */
    0xadd0,  /* scan_source_identifier */
    0x8bd0,  /* unget_token */
    0x13192, /* parse_translation_unit */
    0x9600,  /* read_parser_token */
    0x8f90,  /* resolve_identifier_token */
    0xa970,  /* merge_type_specifier */
    0x2fca0, /* finalize_type_description */
    0x9530,  /* lookup_identifier_token */
    0x1cf00, /* make_named_declarator */
    0x1b6a0, /* begin_parameter_list */
    0x26410, /* push_symbol_scope */
    0x26440, /* activate_symbol_scope */
    0x1b1e0, /* finish_function_declarator */
    0x1b160, /* make_function_declaration */
    0x1b230, /* attach_function_parameters */
    0x1b280, /* classify_parameter_declarations */
    0x30560, /* attach_type_shape */
    0x1b730, /* finish_parameter_scope */
    0x26490, /* deactivate_symbol_scope */
    0x26430, /* pop_symbol_scope */
    0x1a530, /* prepare_function_definition */
    0x239f0, /* declare_symbol */
    0x24060, /* finalize_symbol_type */
    0x21520, /* merge_declarator_type */
    0x22090, /* prepare_declarator_shapes */
    0x21c00, /* extract_declarator_prefix */
    0x2fb90, /* merge_type_attributes */
    0x21e70, /* merge_function_shape */
    0x24690, /* merge_global_declaration */
    0x246b0, /* merge_scoped_declaration */
    0x24650, /* find_scope_declaration */
    0x25810, /* lookup_symbol_chain */
    0x246f0, /* reconcile_declarations */
    0x255a0, /* insert_current_scope_symbol */
    0x1a880, /* initialize_function_definition */
    0x1c600, /* make_internal_label */
    0x1c4a0, /* reference_label */
    0x1a830, /* function_result_type */
    0x30000, /* remove_type_prefix */
    0x301c0, /* drop_first_type_shape */
    0x22d50, /* type_size */
    0x22e30, /* base_type_size */
    0x1a9e0, /* begin_function_definition */
    0x1ad90, /* start_function_ir */
    0x29f70, /* initialize_expression_stream */
    0x2a690, /* record_line_information */
    0x3b680, /* write_ir_index */
    0x3b9a0, /* write_ir_filename */
    0x2ad90, /* emit_warning_controls */
    0x2f420, /* make_symbol_reference */
    0x2f320, /* make_ir_expression */
    0x2f360, /* make_typed_ir_expression */
    0x293a0, /* emit_ir_expression */
    0x2a040, /* get_ir_symbol_index */
    0x30040, /* replace_outer_type_shape */
    0x25e70, /* begin_compound_statement */
    0x25e80, /* enter_compound_scope */
    0x11db0, /* scan_number */
    0x12ac0, /* read_numeric_character */
    0x12b00, /* next_source_character */
    0x12160, /* convert_integer_digits */
    0x122e0, /* integer_digit_value */
    0x12270, /* classify_radix_integer */
    0x17780, /* make_constant_token */
    0x22f10, /* emit_return_statement */
    0x2e320, /* check_assignment_conversion */
    0x2e760, /* check_type_conversion */
    0x265f0, /* emit_record_node */
    0x26540, /* node_alignment */
    0x264c0, /* expression_type_alignment */
    0x29ed0, /* reference_ir_symbol */
    0x3a5a0, /* release_arena */
    0x25f30, /* finish_compound_statement */
    0x25f40, /* leave_compound_scope */
    0x24c80, /* emit_scope_cleanup_records */
    0x26050, /* queue_completed_scope */
    0x1b110, /* finish_function_definition */
    0x1afb0, /* end_function_context */
    0x29e60, /* emit_code_label */
    0x26220, /* finish_function_records */
    0x1c690, /* finish_label_symbols */
    0x26110, /* serialize_scope */
    0x2a0b0, /* record_function_definition */
    0x26360, /* clear_symbol_aliases */
    0x130c0, /* finish_external_declaration */
    0xb9b0,  /* close_source_file */
    0x262c0, /* finish_declarations */
    0x2a430, /* write_function_directory */
    0x1e160, /* write_decorated_name */
    0x26500, /* type_alignment */
    0x260a0, /* defer_global_symbol */
    0x26b50, /* emit_symbol_declaration */
    0x3bb60, /* close_intermediate */
    0x36e30, /* cleanup_precompiled_files */
};

RECON_EXPORT const ReconCase case_descriptor = {RECON_ABI_VERSION,
                                                sizeof(ReconCase),
                                                "minimal",
                                                "Public standalone C1 interface; cached original "
                                                "status, streams, full artifacts and first entries",
                                                order,
                                                RECON_COUNT(order),
                                                NULL,
                                                0,
                                                RECON_IO,
                                                RECON_EXIT,
                                                0,
                                                NULL,
                                                invoke,
                                                NULL,
                                                NULL,
                                                0};
