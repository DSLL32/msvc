/* Unreconstructed function entries. Each body deliberately traps. */
#include "keywords.h"
#include "ir.h"
#include "types.h"
#include "builtins.h"
#include "macros.h"
#include "global-ir.h"
#include "source-input.h"
#include "lexer.h"
#include "parser.h"
#include "control-ir.h"

/* Original 0x43ea0. */
void compiler_signal(int signal_number) { __asm { int 3 } }

void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }

void clear_packing(PackingEntry *entry) { __asm { int 3 } }

void __stdcall grow_vector(Vector *vector) { __asm { int 3 } }

void compiler_warning(unsigned category, unsigned code, ...) { __asm { int 3 } }

/* Original 0x4bee0. */

/* Original 0x43ec0. */
void preprocess_translation_unit(void) { __asm { int 3 } }

/* Original 0x276c0. */
void __stdcall record_version_token(IrToken *token) { __asm { int 3 } }

/* Original 0x3c080. */
IrToken *__stdcall emit_version_token(IrToken *token) { __asm { int 3 } return 0; }

/* Original 0x213f0. */
int compare_filenames(char *first, char *second) { __asm { int 3 } return 0; }

/* Original 0x45880. */
void begin_automatic_pch(void) { __asm { int 3 } }

/* Original 0x47450. */
void begin_debug_info(void) { __asm { int 3 } }

/* Original 0x1fda0. */
void load_precompiled_header(void) { __asm { int 3 } }

/* Original 0x20490. */
int __stdcall open_forced_include(char *filename, int flags) { __asm { int 3 } return 0; }

/* Original 0x199a0. */

/* Original 0x18bd0. */

/* Original 0x23192. */


/* Original 0x362c0. */
void finish_declarations(void) { __asm { int 3 } }

/* Original 0x474b0. */
void finish_debug_info(void) { __asm { int 3 } }

/* Original 0x36b30. */

/* Original 0x2e414. */
void generate_inline_queries(void) { __asm { int 3 } }

/* Original 0x4bb60. */
void close_intermediate(FILE *file) { __asm { int 3 } }

/* Original 0x27210. */
void finish_browser_output(void) { __asm { int 3 } }

/* Original 0x4aa40. */
void report_error(unsigned code, ...) { __asm { int 3 } }

/* Original 0x4a9d0. */
void report_syntax_error(unsigned code, ...) { __asm { int 3 } }

/* Original 0x46e30. */
void cleanup_precompiled_files(void) { __asm { int 3 } }

/* Original 0x271a0. */
void begin_browser_output(void) { __asm { int 3 } }

/* Original 0x2e370. */
void initialize_inline_queries(void) { __asm { int 3 } }

/* Original 0x4aa90. */
void report_fatal(unsigned code, ...) { __asm { int 3 } }

/* Original 0x1bf70. */

void __stdcall prepare_pch_output(char *filename) { __asm { int 3 } }

void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode) { __asm { int 3 } }
void __stdcall reference_ir_symbol(FrontendSymbol *symbol) { __asm { int 3 } }


ParameterInfo *void_parameter_info(void) { __asm { int 3 } return 0; }
unsigned __stdcall hash_parameters(ParameterInfo *parameters) { __asm { int 3 } return 0; }

void browser_index_overflow(void) { __asm { int 3 } }
void __stdcall browser_symbol_definition(FrontendSymbol *symbol) { __asm { int 3 } }

void __stdcall create_scope_buckets(SymbolScope *scope, FrontendSymbol *symbol) { __asm { int 3 } }



void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }

void __stdcall report_macro_error(char *directive) { __asm { int 3 } }
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count) { __asm { int 3 } return 0; }
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall browser_macro(MacroDefinition *definition) { __asm { int 3 } }


void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
int __stdcall parameter_stack_bytes(FrontendSymbol *symbol) { __asm { int 3 } return 0; }
unsigned short __stdcall type_alignment(CType *type) { __asm { int 3 } return 0; }
void __stdcall emit_symbol_declaration(FrontendSymbol *symbol) { __asm { int 3 } }
FrontendSymbol *__stdcall symbol_alias(FrontendSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall write_decorated_name(FILE *stream, FrontendSymbol *symbol) { __asm { int 3 } }
void write_ir_string(char *text) { __asm { int 3 } }

long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin) { __asm { int 3 } return -1; }
void emit_line_directive(void) { __asm { int 3 } }
void report_ide_source(void) { __asm { int 3 } }
void browser_include_begin(void) { __asm { int 3 } }
void browser_open_file(void) { __asm { int 3 } }

void release_input_handle(void) { __asm { int 3 } }

char *__stdcall copy_string(char *text, int arena) { __asm { int 3 } return 0; }

int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes) { __asm { int 3 } return 0; }


int skip_source_comment(void) { __asm { int 3 } return 0; }

unsigned recover_special_character(void) { __asm { int 3 } return 0; }

unsigned recover_trigraph(void) { __asm { int 3 } return 0; }

int consume_line_escape(void) { __asm { int 3 } return 0; }



void skip_source_line(void) { __asm { int 3 } }

void __stdcall copy_quoted_literal(int delimiter) { __asm { int 3 } }

unsigned char __stdcall scan_character_literal(int wide) { __asm { int 3 } return 0; }

void __stdcall scan_string_literal(int wide, int delimiter) { __asm { int 3 } }


int scan_macro_identifier(void) { __asm { int 3 } return 0; }

int __stdcall expand_macro(MacroDefinition *definition) { __asm { int 3 } return 0; }

unsigned char close_source_file(void) { __asm { int 3 } return 0; }

void finish_pch_output(void) { __asm { int 3 } }

int __stdcall match_source_character(int character) { __asm { int 3 } return 0; }

unsigned __stdcall read_number_text(char *destination) { __asm { int 3 } return 0; }


unsigned char __stdcall scan_float_tail(char *destination) { __asm { int 3 } return 0; }

unsigned char __stdcall scan_assembler_number(int first_character) { __asm { int 3 } return 0; }

void report_ide_line(void) { __asm { int 3 } }
void finish_pch_capture(void) { __asm { int 3 } }
void preprocess_directive(void) { __asm { int 3 } }

void scan_continued_identifier(char *start, unsigned hash) { __asm { int 3 } }

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
void skip_declaration_tokens(void) { __asm { int 3 } }

/* Original 0x22f72. */
unsigned char recover_parser_token(unsigned char token, ...) { __asm { int 3 } return 0; }

/* Original 0x230c0. */
void __stdcall finish_external_declaration(int recovering) { __asm { int 3 } }

/* Original 0x25e30. */
FrontendSymbol *__stdcall resolve_type_symbol(FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x25ed0. */
unsigned short __stdcall based_string_modifier(unsigned short kind, FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x25f90. */
unsigned short __stdcall based_symbol_modifier(unsigned short kind, FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x26030. */
unsigned short __stdcall based_type_modifier(unsigned short kind) { __asm { int 3 } return 0; }

/* Original 0x262e0. */
FrontendSymbol *__stdcall finish_declarator(FrontendSymbol *declarator) { __asm { int 3 } return 0; }

/* Original 0x27780. */

/* Original 0x28a70. */
CType *parse_enum_specifier(void) { __asm { int 3 } return 0; }

/* Original 0x28ae0. */
CType *parse_struct_specifier(void) { __asm { int 3 } return 0; }

/* Original 0x29340. */
unsigned __stdcall new_enumerator(char *name, unsigned value) { __asm { int 3 } return 0; }

/* Original 0x29370. */
void __stdcall add_enumerator(char *name, void *context, unsigned value) { __asm { int 3 } }

/* Original 0x29700. */
void __stdcall add_struct_member(DeclarationSpec *declaration, FrontendSymbol *member) { __asm { int 3 } }

/* Original 0x29970. */
IrToken *__stdcall make_function_call(unsigned char operation, IrToken *function, void *arguments) { __asm { int 3 } return 0; }

/* Original 0x2a530. */

/* Original 0x2a9e0. */

/* Original 0x2b110. */
void finish_function_definition(void) { __asm { int 3 } }

/* Original 0x2b1e0. */

/* Original 0x2b230. */

/* Original 0x2b530. */
void __stdcall set_declaration_name(DeclarationSpec *declaration, char *name) { __asm { int 3 } }

/* Original 0x2b610. */
FrontendSymbol *__stdcall make_parameter(char *name) { __asm { int 3 } return 0; }

/* Original 0x2b6a0. */

/* Original 0x2b730. */

/* Original 0x2b780. */
FrontendSymbol *__stdcall append_parameter(FrontendSymbol *list, FrontendSymbol *parameter) { __asm { int 3 } return 0; }

/* Original 0x2b7c0. */
IrToken *__stdcall begin_sizeof_expression(IrToken *operand) { __asm { int 3 } return 0; }

/* Original 0x2b960. */
IrToken *__stdcall begin_alignof_expression(IrToken *operand) { __asm { int 3 } return 0; }

/* Original 0x2bc30. */
void *__stdcall append_argument(void *arguments, IrToken *argument, int flags) { __asm { int 3 } }

/* Original 0x2c220. */
void __stdcall finish_type_description(CType *type) { __asm { int 3 } }

/* Original 0x2c3a0. */
void __stdcall report_declaration_error(unsigned code, DeclarationSpec *declaration) { __asm { int 3 } }

/* Original 0x2c4a0. */

/* Original 0x2c500. */
FrontendSymbol *__stdcall find_label(char *name) { __asm { int 3 } return 0; }

/* Original 0x2c560. */
FrontendSymbol *__stdcall declare_label(char *name) { __asm { int 3 } return 0; }

/* Original 0x2c5b0. */
void __stdcall emit_named_label(char *name) { __asm { int 3 } }

/* Original 0x2c600. */

/* Original 0x2c7b0. */
IrToken *__stdcall make_sizeof_expression(IrToken *operand) { __asm { int 3 } return 0; }

/* Original 0x2c880. */
IrToken *__stdcall make_sizeof_type(CType *type) { __asm { int 3 } return 0; }

/* Original 0x2c8f0. */
IrToken *__stdcall make_full_expression(IrToken *expression) { __asm { int 3 } return 0; }

/* Original 0x2c980. */
void __stdcall process_declaration(DeclarationSpec *declaration) { __asm { int 3 } }

/* Original 0x2cf00. */

/* Original 0x2cf90. */
FrontendSymbol *__stdcall combine_declarators(FrontendSymbol *declarator, FrontendSymbol *qualifier) { __asm { int 3 } return 0; }

/* Original 0x2d010. */
FrontendSymbol *__stdcall new_declarator(char *name) { __asm { int 3 } return 0; }

/* Original 0x2d150. */
IrToken *__stdcall make_identifier_expression(char *name) { __asm { int 3 } return 0; }

/* Original 0x2e419. */
void __stdcall begin_inline_region(unsigned flags) { __asm { int 3 } }

/* Original 0x2e603. */
void finish_inline_region(void) { __asm { int 3 } }

/* Original 0x2ed95. */
void __stdcall emit_inline_jump(int kind) { __asm { int 3 } }

/* Original 0x2f310. */
void __stdcall begin_for_statement(IrToken *initial, IrToken *condition, IrToken *increment) { __asm { int 3 } }

/* Original 0x2f420. */
void finish_for_statement(void) { __asm { int 3 } }

/* Original 0x2fdb0. */
FrontendSymbol *__stdcall declare_initialized_symbol(FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x2fe60. */
void __stdcall finish_initializer(FrontendSymbol *symbol, IrToken *initializer) { __asm { int 3 } }

/* Original 0x300c0. */
void __stdcall begin_aggregate_initializer(IrToken *initializer) { __asm { int 3 } }

/* Original 0x301a0. */
void __stdcall emit_initializer(IrToken *initializer) { __asm { int 3 } }

/* Original 0x30e40. */
void begin_nested_initializer(void) { __asm { int 3 } }

/* Original 0x30ec0. */
void finish_nested_initializer(void) { __asm { int 3 } }

/* Original 0x31200. */
void __stdcall finish_aggregate_initializer(FrontendSymbol *symbol) { __asm { int 3 } }

/* Original 0x32f10. */
void __stdcall emit_return_statement(IrToken *value, int implicit) { __asm { int 3 } }

/* Original 0x330b0. */
void begin_do_statement(void) { __asm { int 3 } }

/* Original 0x330f0. */
void __stdcall begin_switch_statement(IrToken *condition) { __asm { int 3 } }

/* Original 0x332d0. */
void finish_switch_statement(void) { __asm { int 3 } }

/* Original 0x33360. */
void __stdcall emit_case_statement(IrToken *value) { __asm { int 3 } }

/* Original 0x33500. */
void emit_default_statement(void) { __asm { int 3 } }

/* Original 0x33530. */
void __stdcall set_except_filter(IrToken *filter) { __asm { int 3 } }

/* Original 0x33580. */
void begin_try_statement(void) { __asm { int 3 } }

/* Original 0x335a0. */
void begin_except_statement(void) { __asm { int 3 } }

/* Original 0x335f0. */
void finish_except_statement(void) { __asm { int 3 } }

/* Original 0x33630. */
void emit_leave_statement(void) { __asm { int 3 } }

/* Original 0x33650. */
void finish_try_statement(void) { __asm { int 3 } }

/* Original 0x33670. */
void begin_finally_statement(void) { __asm { int 3 } }

/* Original 0x33680. */
void finish_finally_statement(void) { __asm { int 3 } }

/* Original 0x33800. */
FrontendSymbol *__stdcall apply_declarator_type(DeclarationSpec *declaration, FrontendSymbol *symbol) { __asm { int 3 } return 0; }

/* Original 0x339f0. */

/* Original 0x34060. */

/* Original 0x355a0. */

/* Original 0x35aa0. */
FrontendSymbol *__stdcall declare_named_entity(char *name, char *alias, unsigned kind, int arena) { __asm { int 3 } return 0; }

/* Original 0x35e70. */

/* Original 0x35f30. */
void finish_compound_statement(void) { __asm { int 3 } }

/* Original 0x39370. */
void __stdcall emit_expression_statement(IrToken *expression) { __asm { int 3 } }

/* Original 0x39d10. */
void __stdcall emit_conditional_branch(IrToken *condition, IrToken *label, int sense) { __asm { int 3 } }

/* Original 0x39e60. */
void __stdcall emit_code_label(IrToken *label) { __asm { int 3 } }

/* Original 0x3c750. */
IrToken *__stdcall make_binary_expression(unsigned char operation, IrToken *left, IrToken *right) { __asm { int 3 } return 0; }

/* Original 0x3d580. */
IrToken *__stdcall make_unary_expression(unsigned char operation, IrToken *operand) { __asm { int 3 } return 0; }

/* Original 0x3f420. */

/* Original 0x3f820. */
IrToken *__stdcall make_member_expression(IrToken *operand, char *member) { __asm { int 3 } return 0; }

/* Original 0x3fb50. */
TypeShape *__stdcall append_type_shape(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }

/* Original 0x40560. */

/* Original 0x41490. */
IrToken *__stdcall make_cast_expression(CType *type, IrToken *operand) { __asm { int 3 } return 0; }

/* Original 0x4a5a0. */
void __stdcall release_arena(int arena) { __asm { int 3 } }

/* Original 0x4e40c. */
void parse_asm_statement(void) { __asm { int 3 } }



unsigned char __stdcall read_tag_specifier(unsigned short kind, unsigned short *attributes) { __asm { int 3 } return 0; }

CType *__stdcall merge_named_type(FrontendSymbol *symbol, unsigned short *flags, unsigned short *attributes) { __asm { int 3 } return 0; }

void __stdcall merge_storage_class(unsigned char *storage, unsigned class, unsigned modifier, int replace) { __asm { int 3 } }

void parse_declspec(unsigned char *storage) { __asm { int 3 } }

TypeDescription *__stdcall copy_mutable_type(CType *type) { __asm { int 3 } return 0; }




char *__stdcall type_specifier_name(unsigned short flags) { __asm { int 3 } return 0; }

FrontendSymbol *__stdcall find_identifier_symbol(char *name) { __asm { int 3 } return 0; }
void __stdcall browser_symbol_reference(FrontendSymbol *symbol) { __asm { int 3 } }




void __stdcall report_type_syntax_error(unsigned code, CType *type) { __asm { int 3 } }


void abort_function_definition(void) { __asm { int 3 } }
void __stdcall replace_function_parameters(FrontendSymbol *symbol, ParameterInfo *parameters) { __asm { int 3 } }
void browser_begin_function_scope(void) { __asm { int 3 } }
void __stdcall browser_function_parameters(FrontendSymbol *symbol) { __asm { int 3 } }

void __stdcall report_symbol_error(unsigned code, FrontendSymbol *symbol) { __asm { int 3 } }
int __stdcall is_record_type(CType *type) { __asm { int 3 } return 0; }
FrontendSymbol *__stdcall merge_local_declaration(FrontendSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall record_local_declaration(FrontendSymbol *symbol) { __asm { int 3 } }

void report_declaration_context_error(unsigned code, unsigned detail, char *name) { __asm { int 3 } }
unsigned short __stdcall normalize_symbol_storage(unsigned short attributes, FrontendSymbol *symbol) { __asm { int 3 } return 0; }

void __stdcall check_declarator_prefix(FrontendSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }
void __stdcall finish_array_declarator(FrontendSymbol *symbol, TypeShape **result, ShapeCursor current, int is_typedef) { __asm { int 3 } }

TypeShape *__stdcall extract_base_prefix(TypeShape *shape, TypeShape **prefix, FrontendSymbol *symbol, int is_typedef, unsigned short *attributes) { __asm { int 3 } return 0; }

void __stdcall copy_shape_prefix(FrontendSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }




int __stdcall declaration_conflict(FrontendSymbol *existing, FrontendSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall commit_redeclaration(FrontendSymbol *existing, FrontendSymbol *symbol) { __asm { int 3 } }

void reset_inline_function_state(void) { __asm { int 3 } }


int __stdcall array_element_size(unsigned short flags, void *information, TypeShape *shape) { __asm { int 3 } return 0; }
int __stdcall pointer_type_size(unsigned short attributes) { __asm { int 3 } return 0; }

unsigned __stdcall parameter_list_bytes(FrontendSymbol *parameters) { __asm { int 3 } return 0; }
int __stdcall compatible_parameter_types(CType *expected, CType *actual) { __asm { int 3 } return 0; }
void begin_syntax_trace(void) { __asm { int 3 } }
void end_syntax_trace(void) { __asm { int 3 } }
void __stdcall print_function_declaration(FrontendSymbol *symbol, void *context) { __asm { int 3 } }

void __stdcall define_register_parameter(FrontendSymbol *parameter) { __asm { int 3 } }
void __stdcall record_inline_parameter(FrontendSymbol *parameter) { __asm { int 3 } }

void write_counted_ir(short length, char *text) { __asm { int 3 } }

IrToken *__stdcall make_based_pointer_base(CType *type) { __asm { int 3 } return 0; }

IrToken *__stdcall make_typed_unary_expression(unsigned char operation, CType *type, IrToken *operand) { __asm { int 3 } return 0; }
int __stdcall emit_intrinsic_expression(IrToken *expression) { __asm { int 3 } return 0; }


unsigned char __stdcall classify_decimal_integer(unsigned value) { __asm { int 3 } return 0; }
unsigned char __stdcall classify_long_integer(unsigned value) { __asm { int 3 } return 0; }
unsigned char __stdcall classify_unsigned_integer(unsigned value) { __asm { int 3 } return 0; }

