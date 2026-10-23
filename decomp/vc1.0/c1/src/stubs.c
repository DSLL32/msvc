/* Unreconstructed function entries. Each body deliberately traps. */
#include "keywords.h"
#include "pragma.h"
#include "pch.h"
#include "pp-expression.h"
#include "ir.h"
#include "types.h"
#include "builtins.h"
#include "macros.h"
#include "global-ir.h"
#include "source-input.h"
#include "lexer.h"
#include "parser.h"
#include "debug-info.h"
#include "floating.h"
#include "diagnostics.h"
#include "literal-names.h"
#include "initializer-records.h"
#include "initializers.h"
#include "control-ir.h"
#include "expressions.h"

/* Original 0x43ea0. */
void compiler_signal(int signal_number) { __asm { int 3 } }

void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }

void clear_packing(PackingEntry *entry) { __asm { int 3 } }

void __stdcall grow_vector(Vector *vector) { __asm { int 3 } }


/* Original 0x4bee0. */

/* Original 0x43ec0. */

/* Original 0x276c0. */

/* Original 0x3c080. */

/* Original 0x213f0. */

/* Original 0x45880. */
void begin_automatic_pch(void) { __asm { int 3 } }

/* Original 0x47450. */

/* Original 0x1fda0. */

/* Original 0x20490. */

/* Original 0x199a0. */

/* Original 0x18bd0. */

/* Original 0x23192. */


/* Original 0x362c0. */

/* Original 0x474b0. */

/* Original 0x36b30. */

/* Original 0x2e414. */
void generate_inline_queries(void) { __asm { int 3 } }

/* Original 0x4bb60. */

/* Original 0x27210. */

/* Original 0x4aa40. */
void report_error(unsigned code, ...) { __asm { int 3 } }

/* Original 0x4a9d0. */
void report_syntax_error(unsigned code, ...) { __asm { int 3 } }

/* Original 0x46e30. */

/* Original 0x271a0. */


/* Original 0x2e370. */
void initialize_inline_queries(void) { __asm { int 3 } }

/* Original 0x4aa90. */
void report_fatal(unsigned code, ...) { __asm { int 3 } }

/* Original 0x1bf70. */





void browser_index_overflow(void) { __asm { int 3 } }




void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }

void __stdcall report_macro_error(char *directive) { __asm { int 3 } }


void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
FrontendSymbol *__stdcall symbol_alias(FrontendSymbol *symbol) { __asm { int 3 } return 0; }

void report_ide_source(void) { __asm { int 3 } }

void release_input_handle(void) { __asm { int 3 } }

























void report_ide_line(void) { __asm { int 3 } }
void finish_pch_capture(void) { __asm { int 3 } }


/* Original 0x18f90. */

/* Original 0x19600. */

/* Original 0x21b00. */
void __stdcall concatenate_string_literals(void *left, void *right) { __asm { int 3 } }

/* Original 0x22b70. */
void __stdcall report_expected_token(unsigned char actual, int expected) { __asm { int 3 } }

/* Original 0x22cc6. */
ParserValue __stdcall recovery_token_value(int token) { ParserValue value; __asm { int 3 } return value; }

/* Original 0x22d42. */
void __stdcall report_parser_error(char *text) { __asm { int 3 } }

/* Original 0x22e8d. */


/* Original 0x22f72. */
unsigned char recover_parser_token(unsigned char token, ...) { __asm { int 3 } return 0; }

/* Original 0x230c0. */

/* Original 0x25e30. */
FrontendSymbol *__stdcall resolve_type_symbol(FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x25ed0. */
unsigned short __stdcall based_string_modifier(unsigned short kind, FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x25f90. */
unsigned short __stdcall based_symbol_modifier(unsigned short kind, FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x26030. */
unsigned short __stdcall based_type_modifier(unsigned short kind) { __asm { int 3 } return 0; }

/* Original 0x262e0. */
IrToken *__stdcall normalize_based_expression(IrToken *expression) { __asm { int 3 } return 0; }

/* Original 0x27780. */

/* Original 0x28a70. */

/* Original 0x28ae0. */

/* Original 0x29340. */

/* Original 0x29370. */

/* Original 0x29700. */

/* Original 0x29970. */

/* Original 0x2a530. */

/* Original 0x2a9e0. */

/* Original 0x2b110. */

/* Original 0x2b1e0. */

/* Original 0x2b230. */

/* Original 0x2b530. */
void __stdcall set_declaration_name(DeclarationSpec *declaration, char *name) { __asm { int 3 } }

/* Original 0x2b610. */
FrontendSymbol *__stdcall make_parameter(char *name) { __asm { int 3 } return 0; }

/* Original 0x2b6a0. */

/* Original 0x2b730. */

/* Original 0x2b780. */

/* Original 0x2b7c0. */

/* Original 0x2b960. */

/* Original 0x2bc30. */

/* Original 0x2c220. */

/* Original 0x2c3a0. */
void __stdcall report_declaration_error(unsigned code, DeclarationSpec *declaration) { __asm { int 3 } }

/* Original 0x2c4a0. */

/* Original 0x2c500. */


/* Original 0x2c560. */


/* Original 0x2c5b0. */
void __stdcall emit_named_label(char *name) { __asm { int 3 } }

/* Original 0x2c600. */

/* Original 0x2c7b0. */

/* Original 0x2c880. */

/* Original 0x2c8f0. */

/* Original 0x2c980. */

/* Original 0x2cf00. */

/* Original 0x2cf90. */

/* Original 0x2d010. */

/* Original 0x2d150. */

/* Original 0x2e419. */
void __stdcall begin_inline_region(unsigned flags) { __asm { int 3 } }

/* Original 0x2e603. */
void finish_inline_region(void) { __asm { int 3 } }

/* Original 0x2ed95. */
void __stdcall emit_inline_jump(int kind) { __asm { int 3 } }

/* Original 0x2f310. */

/* Original 0x2f420. */

/* Original 0x2fdb0. */

/* Original 0x2fe60. */

/* Original 0x300c0. */

/* Original 0x301a0. */

/* Original 0x30e40. */

/* Original 0x30ec0. */

/* Original 0x31200. */

/* Original 0x32f10. */


/* Original 0x330f0. */

/* Original 0x332d0. */

/* Original 0x33360. */

/* Original 0x33500. */


/* Original 0x33530. */


/* Original 0x33580. */


/* Original 0x335a0. */


/* Original 0x335f0. */


/* Original 0x33630. */


/* Original 0x33650. */


/* Original 0x33670. */


/* Original 0x33680. */


/* Original 0x33800. */

/* Original 0x339f0. */

/* Original 0x34060. */

/* Original 0x355a0. */

/* Original 0x35aa0. */
FrontendSymbol *__stdcall declare_named_entity(char *name, char *alias, unsigned kind, int arena) { __asm { int 3 } return 0; }

/* Original 0x35e70. */

/* Original 0x35f30. */

/* Original 0x39370. */

/* Original 0x39d10. */

/* Original 0x39e60. */

/* Original 0x3c750. */

/* Original 0x3d580. */

/* Original 0x3f420. */

/* Original 0x3f820. */
IrToken *__stdcall make_member_expression(IrToken *operand, char *member) { __asm { int 3 } return 0; }

/* Original 0x3fb50. */
TypeShape *__stdcall append_type_shape(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }

/* Original 0x40560. */

/* Original 0x41490. */

/* Original 0x4a5a0. */

/* Original 0x4e40c. */

















void __stdcall report_type_syntax_error(unsigned code, CType *type) { __asm { int 3 } }


void abort_function_definition(void) { __asm { int 3 } }
void __stdcall assign_symbol_segment(FrontendSymbol *symbol, FrontendSymbol *segment) { __asm { int 3 } }

void __stdcall report_symbol_error(unsigned code, FrontendSymbol *symbol) { __asm { int 3 } }

void report_declaration_context_error(unsigned warning, unsigned error, ...) { __asm { int 3 } }

void __stdcall check_declarator_prefix(FrontendSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }


void __stdcall copy_shape_prefix(FrontendSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }





void reset_inline_function_state(void) { __asm { int 3 } }



int __stdcall compatible_parameter_types(CType *expected, CType *actual) { __asm { int 3 } return 0; }


void __stdcall define_register_parameter(FrontendSymbol *parameter) { __asm { int 3 } }
void __stdcall record_inline_parameter(FrontendSymbol *parameter) { __asm { int 3 } }


IrToken *__stdcall make_based_pointer_base(CType *type) { __asm { int 3 } return 0; }



unsigned char __stdcall classify_unsigned_integer(unsigned value) { __asm { int 3 } return 0; }


IrToken *__stdcall convert_based_pointer(CType *target, IrToken *value) { __asm { int 3 } return 0; }


void warn_void_function_pointer(void) { __asm { int 3 } }
void __stdcall conversion_diagnostic(unsigned code, unsigned level, unsigned char operation, CType *target, CType *source) { __asm { int 3 } }
void __stdcall check_based_pointer_conversion(unsigned char operation, TypeShape *target, TypeShape *source) { __asm { int 3 } }
int __stdcall compare_prototype_parameter_types(CType *target, CType *source) { __asm { int 3 } return 0; }
void __stdcall report_conversion_mismatch(unsigned char operation, CType *target, CType *source) { __asm { int 3 } }




void *__stdcall pop_work_list(void **head) { __asm { int 3 } return 0; }



void finish_inline_function(void) { __asm { int 3 } }


void report_ide_file_end(void) { __asm { int 3 } }



void __stdcall cache_parameter_bytes(FrontendSymbol *symbol) { __asm { int 3 } }




void capture_directive_text(void) { __asm { int 3 } }

int pch_find_conditional(void) { __asm { int 3 } return 0; }



int __stdcall pch_resolve_condition(int condition) { __asm { int 3 } return 0; }








void warn_ignored_token(void) { __asm { int 3 } }




void __stdcall pch_record_include(char *name_end) { __asm { int 3 } }

void pch_record_string(unsigned kind, unsigned value, char *text) { __asm { int 3 } }
int __stdcall pch_macro_defined(char *name) { __asm { int 3 } return 0; }
int pch_replay_mismatch(void) { __asm { int 3 } return 0; }




















void __stdcall macro_scan_error(unsigned code) { __asm { int 3 } }

long source_file_timestamp(void) { __asm { int 3 } return 0; }












void browser_macro_end(void) { __asm { int 3 } }

/* Original 0x1e550. */

/* Original 0x1c4b0. */

/* Original 0x1e650. */
int __stdcall read_pragma_argument(unsigned char kind) { __asm { int 3 } return 0; }

/* Original 0x1e5b0. */
int __stdcall read_parenthesized_pragma_argument(unsigned char kind) { __asm { int 3 } return 0; }

/* Original 0x1e570. */

/* Original 0x1e5d0. */

/* Original 0x2dc60. */
void __stdcall enable_intrinsic_function(char *name) { __asm { int 3 } }

/* Original 0x2dce0. */

/* Original 0x35d60. */
void __stdcall assign_allocated_text(char *name) { __asm { int 3 } }

/* Original 0x2d3a0. */
void warn_expected_identifier(void) { __asm { int 3 } }

/* Original 0x1e9e0. */

/* Original 0x20110. */
void __stdcall emit_listing_directive(unsigned char mode) { __asm { int 3 } }

/* Original 0x1c290. */
void __stdcall pragma_message(char *text) { __asm { int 3 } }

/* Original 0x4bf10. */

/* Original 0x20090. */

/* Original 0x1e530. */
void warn_bad_pragma(void) { __asm { int 3 } }

/* Original 0x1ffc0. */
void synchronize_pch_pragma(void) { __asm { int 3 } }

/* Original 0x28a10. */

/* Original 0x28650. */

/* Original 0x28590. */

/* Original 0x28860. */

/* Original 0x289c0. */

/* Original 0x28c30. */

/* Original 0x328c0. */

/* Original 0x355c0. */
SymbolScope *enclosing_declaration_scope(void) { __asm { int 3 } return 0; }

/* Original 0x32990. */

/* Original 0x32a50. */

/* Original 0x32a80. */

/* Original 0x32af0. */

/* Original 0x328b0. */
unsigned hash_pointer_low12(void *key) { __asm { int 3 } return 0; }

/* Original 0x363e0. */
unsigned hash_symbol_index(void *key) { __asm { int 3 } return 0; }

/* Original 0x32ae0. */

/* Original 0x363c0. */
int hash_symbol_indices_equal(void *left, void *right) { __asm { int 3 } return 0; }

/* Original 0x32ad0. */
unsigned __stdcall hash_table_count(HashTable *table) { __asm { int 3 } return 0; }

/* Original 0x40c10. */

/* Original 0x33750. */
void __stdcall initialize_thread_symbol(FrontendSymbol *symbol) { __asm { int 3 } }

/* Original 0x33d00. */

/* Original 0x33020. */

/* Original 0x285c0. */

/* Original 0x33ce0. */
void __stdcall warn_ignored_storage_model(FrontendSymbol *symbol) { __asm { int 3 } }

/* Original 0x2c950. */

/* Original 0x1cee0. */

/* Original 0x34e00. */

/* Original 0x353b0. */

/* Original 0x34e50. */

/* Original 0x33f60. */

/* Original 0x3c1f0. */
void __stdcall report_type_conflict(unsigned code, FrontendSymbol *symbol, char *name, CType *actual, CType *expected) { __asm { int 3 } }

/* Original 0x34fb0. */

int __stdcall is_function_pointer_type(CType *type) { __asm { int 3 } return 0; }

int __stdcall process_pch_string_event(unsigned kind, unsigned value, char *text) { __asm { int 3 } return 0; }






void __stdcall push_pragma_packing(int alignment, char *label) { __asm { int 3 } }
void __stdcall pop_pragma_packing(int alignment, char *label) { __asm { int 3 } }

unsigned short __stdcall decode_escape(unsigned char character, int wide) { __asm { int 3 } return 0; }
void __stdcall convert_pascal_string(StringToken *string) { __asm { int 3 } }

/* Original 0x3c140. */
/* Original 0x3c620. */
/* Original 0x3c6d0. */
/* Original 0x27930. */
/* Original 0x3d400. */
/* Original 0x3ddf0. */
/* Original 0x3e2e0. */
/* Original 0x3f4f0. */
void __stdcall report_function_operand(unsigned char operation, int argument) { __asm { int 3 } }
/* Original 0x3f5c0. */
/* Original 0x3f670. */
/* Original 0x419a0. */
/* Original 0x41ff0. */

/* Original 0x3f530. */

/* Original 0x4c80f. */
int compare_extended(unsigned operation, const Extended *left, const Extended *right) { __asm { int 3 } return 0; }
/* Original 0x4c71e. */
int unsigned_to_extended(const unsigned *value, Extended *number) { __asm { int 3 } return 0; }
/* Original 0x4c6fc. */
int integer_to_extended(const int *value, Extended *number) { __asm { int 3 } return 0; }
/* Original 0x28190. */
IrToken * __stdcall fold_floating_binary(IrToken *expression, Extended *left, Extended *right) { __asm { int 3 } return 0; }
/* Original 0x4c260. */
/* Original 0x4c080. */

/* Original 0x32dd0. */

/* Original 0x36370. */
void __stdcall record_symbol_alias(FrontendSymbol *symbol, FrontendSymbol *alias) { __asm { int 3 } }
/* Original 0x363f0. */
char *__stdcall local_static_name(FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x2d0e0. */
FrontendSymbol * __stdcall recover_undeclared_identifier(char *name) { __asm { int 3 } return 0; }
/* Original 0x2cdf0. */
FrontendSymbol * __stdcall make_implicit_declaration(char *name, unsigned char nesting) { __asm { int 3 } return 0; }
/* Original 0x2d180. */

/* Original 0x2d2a0. */

/* Original 0x2d250. */
/* Original 0x402e0. */

/* Original 0x33080. */

/* Original 0x41c90. */

/* Original 0x2b5d0. */
FrontendSymbol *__stdcall declare_implicit_function(char *name) { __asm { int 3 } return 0; }

/* Original 0x402b0. */
CType *__stdcall function_designator_type(CType *type) { __asm { int 3 } return 0; }
/* Original 0x2bb90. */
/* Original 0x2b8f0. */

/* Original 0x2bc80. */

/* Original 0x2c760. */

/* Original 0x40220. */
CType * __stdcall change_function_reference_kind(CType *type, int kind) { __asm { int 3 } return 0; }
/* Original 0x40160. */
/* Original 0x2c450. */
/* Original 0x3d500. */
IrToken * __stdcall find_expression_type_base(IrToken *expression) { __asm { int 3 } return 0; }
/* Original 0x2ee64. */
int __stdcall find_inline_parameter_slot(unsigned *slots, unsigned count, unsigned symbol_index) { __asm { int 3 } return 0; }
/* Original 0x3c1b0. */
void __stdcall type_warning(unsigned level, unsigned code, CType *type) { __asm { int 3 } }
/* Original 0x4c538. */
/* Original 0x284f0. */




void __stdcall assign_unpooled_literal_segment(FrontendSymbol *symbol, FrontendSymbol *segment) { __asm { int 3 } }



unsigned __stdcall initializer_address_size(FrontendSymbol *symbol) { __asm { int 3 } }
int extended_to_integer(const Extended *value, int *result) { __asm { int 3 } }
void write_ir_float(float *value) { __asm { int 3 } }
void write_ir_double(double *value) { __asm { int 3 } }

void __stdcall emit_runtime_initializer(IrToken *value) { __asm { int 3 } }



void __stdcall emit_member_padding(FrontendSymbol *member) { __asm { int 3 } }

void __stdcall emit_inline_memory_initializer(unsigned operation, IrToken *target, IrToken *value, IrToken *bytes) { __asm { int 3 } }

/* Original 0x31230. */

/* Original 0x31180. */
/* Original 0x311a0. */

/* Original 0x30f50. */
/* Original 0x31030. */

/* Original 0x30f90. */
FrontendSymbol *__stdcall next_shared_bitfield(unsigned offset, FrontendSymbol *member) { __asm { int 3 } }
/* Original 0x30fd0. */
FrontendSymbol *__stdcall skip_initializer_anonymous_members(FrontendSymbol *member) { __asm { int 3 } }

/* Original 0x2f7b0. */

/* Original 0x358a0. */
/* Original 0x2dd70. */
FrontendSymbol *__stdcall get_intrinsic_fallback_symbol(unsigned operation) { __asm { int 3 } }

/* Original 0x3af90. */

/* Original 0x29930. */
/* Original 0x40c70. */

/* Original 0x336f0. */
CType *__stdcall clear_type_attributes(CType *type, unsigned short attributes) { __asm { int 3 } }
/* Original 0x414e0. */

/* Original 0x413d0. */
/* Original 0x41910. */

/* Original 0x41180. */

/* Original 0x41eb0. */

/* Original 0x41c70. */

/* Original 0x40e60. */

/* Original 0x40cd0. */
/* Original 0x40e00. */
/* Original 0x40d10. */

/* Original 0x4c696. */
/* Original 0x4c6b8. */

/* Original 0x2efb0. */
int __stdcall recognize_qc_loop(IrToken *condition, IrToken *increment, LabelStack *entry) { __asm { int 3 } }

/* Original 0x2f250. */
void finish_qc_loop(void) { __asm { int 3 } }

/* Original 0x48fe0. */
/* Original 0x48d90. */

/* Original 0x17b70. */
/* Original 0x49110. */
/* Original 0x49080. */

/* Original ECX methods: 0x18030, 0x17390, 0x17300, 0x18120. */
void __fastcall destroy_pdb_buffer_list(PdbBufferList *self) { __asm { int 3 } }

/* Original 0x18050, ECX this plus one stack argument. */

/* Program database reference operands, original 0x17fc0/0x17fd0. */
/* Original 0x4a470/0x4a1a0. */

/* Original 0x4bd50. */

/* Original 0x4c390. */

/* Original CRT variant 0x57f40. */

/* Original 0x4c8f5, private x87 inputs. */

/* Original 0x4c8f1, ESI destination and ST(0) value. */

/* Original 0x28530, twelve-byte value plus arena selector on stack. */

/* Original initializer-buffer methods 0x29eb0/0x2a000/0x29e00/0x29cf0. */

/* Original 0x2fe90. */

/* Original 0x31340. */

/* Original 0x4a940 and 0x4a770. */

/* Message-catalog helpers, original 0x4a890/0x4a8f0/0x4a840. */

/* Original CRT variant 0x528d0. */

/* Original 0x4af40. */

int __stdcall is_pointer_based_shape(TypeShape *shape) { __asm { int 3 } }

unsigned short __stdcall primitive_pointer_debug_index(CType *type) { __asm { int 3 } }
unsigned short __stdcall primitive_debug_index(CType *type) { __asm { int 3 } }
unsigned __stdcall queue_debug_record(unsigned char kind) { __asm { int 3 } }
void __stdcall emit_function_debug_type(CType *type) { __asm { int 3 } }



unsigned short pdb_enum_type_index(FrontendSymbol *tag) { __asm { int 3 } }

void __stdcall write_legacy_pointer_type(CType *type, unsigned short nested) { __asm { int 3 } }
void __stdcall write_legacy_array_type(CType *type, unsigned short nested) { __asm { int 3 } }
void __stdcall write_legacy_function_type(CType *type, unsigned short result, unsigned short arguments) { __asm { int 3 } }
void __stdcall write_legacy_qualified_type(CType *type) { __asm { int 3 } }
void __stdcall write_legacy_enum_type(FrontendSymbol *tag) { __asm { int 3 } }
void __stdcall write_legacy_record_type(FrontendSymbol *tag, unsigned short fields, unsigned short auxiliary) { __asm { int 3 } }
void __stdcall write_legacy_field_list(FrontendSymbol *tag, unsigned count, int mode) { __asm { int 3 } }
void __stdcall write_legacy_argument_list(ParameterInfo *parameters) { __asm { int 3 } }
void __stdcall write_legacy_argument_slice(CType **parameters, unsigned first, unsigned limit) { __asm { int 3 } }
void reset_debug_type_records(void) { __asm { int 3 } }









void __stdcall materialize_based_symbol(FrontendSymbol *symbol) { __asm { int 3 } }
unsigned short write_codeview_symbol(unsigned char *destination, FrontendSymbol *symbol) { __asm { int 3 } }


unsigned short pdb_complete_enum_type_index(FrontendSymbol *tag) { __asm { int 3 } }




unsigned short write_codeview_signed_leaf(unsigned char *destination, int number) { __asm { int 3 } }


void declare_pch_debug_library(void) { __asm { int 3 }  }

void finish_pch_debug_symbols(void) { __asm { int 3 }  }

void begin_browser_pch_record(void) { __asm { int 3 }  }





unsigned __stdcall write_pch_event_stack(WorkingStack *stack, FILE *file, unsigned *checksum) { __asm { int 3 } return 0; }




#include "pch-relocate.h"
#include "asm.h"
#include <recon.h>




































void __stdcall restore_pch_browser_name(char *filename) { __asm { int 3 } }

void __stdcall restore_pch_debug_state(char *object_name) { __asm { int 3 } }

void __stdcall reference_pch_debug_library(char *name) { __asm { int 3 } }

void restore_pch_separate_debug(void) { __asm { int 3 } }


/* Original 0x2c710. */



/* Original 0x4dd5f. */

/* Original 0x4ca52. */


/* Original 0x4ddb0. */


/* Original0x44e40. */
void write_pch_record_header(unsigned kind, unsigned value, unsigned bytes) { __asm { int 3 } }
