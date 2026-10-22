/* Named, unaccepted reconstruction frontiers. */
#include "initialization.h"
#include "inline-vector.h"
#include "class-lookup.h"
#include "lookahead.h"
#include "ir.h"
#include "hash-table.h"
#include "types.h"
#include "parameters.h"
#include "macros.h"
#include "runtime.h"
#include "global-ir.h"
#include "source-input.h"
#include "operators.h"
#include "parser.h"
#include "lexer.h"
#include "class-members.h"
#include "class-finalization.h"
#include "class-type-sets.h"
#include "special-members.h"
#include "inline-capture.h"
#include "type-sizes.h"
void interrupt_compile(int signal_number) { __asm { int 3 } }
void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }
void clear_packing(PackingEntry *entry) { __asm { int 3 } }
void __stdcall grow_vector(Vector *vector) { __asm { int 3 } }
void compiler_warning(int level, unsigned code, ...) { __asm { int 3 } }
void preprocess_translation_unit(void) { __asm { int 3 } }
void __stdcall record_string_literal(IrToken *token) { __asm { int 3 } }
void __stdcall emit_version_token(IrToken *token) { __asm { int 3 } }
int compare_filenames(char *first, char *second) { __asm { int 3 } return 0; }
void begin_automatic_pch(void) { __asm { int 3 } }
void begin_debug_info(void) { __asm { int 3 } }
void load_precompiled_header(void) { __asm { int 3 } }
int __stdcall open_forced_include(char *filename, int flags) { __asm { int 3 } return 0; }
void finish_declarations(void) { __asm { int 3 } }
void finish_types(void) { __asm { int 3 } }
void finish_inline_functions(void) { __asm { int 3 } }
void finish_expression_output(void) { __asm { int 3 } }
void finish_debug_info(void) { __asm { int 3 } }
void generate_inline_queries(void) { __asm { int 3 } }
void close_intermediate(FILE *file) { __asm { int 3 } }
void finish_browser_output(void) { __asm { int 3 } }
void report_error(unsigned code, ...) { __asm { int 3 } }
void cleanup_precompiled_files(void) { __asm { int 3 } }
void begin_browser_output(void) { __asm { int 3 } }
void initialize_inline_queries(void) { __asm { int 3 } }
void report_fatal(unsigned code, ...) { __asm { int 3 } }
InlineVector *grow_inline_vector(InlineVector *vector) { __asm { int 3 } return 0; }
void __stdcall prepare_pch_output(char *filename) { __asm { int 3 } }
CType *builtin_integer_type(void) { __asm { int 3 } return 0; }
void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode) { __asm { int 3 } }
void __stdcall reference_ir_symbol(CppSymbol *symbol) { __asm { int 3 } }
unsigned get_ir_symbol_index(CppSymbol *symbol) { __asm { int 3 } return 0; }
long ftell(FILE *file) { __asm { int 3 } return 0; }
void report_syntax_error(unsigned code, ...) { __asm { int 3 } }
void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall report_macro_error(char *directive) { __asm { int 3 } }
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count) { __asm { int 3 } return 0; }
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall browser_macro(MacroDefinition *definition) { __asm { int 3 } }
CType *__stdcall drop_first_type_shape(CType *type) { __asm { int 3 } return 0; }
unsigned char __stdcall member_pointer_is_aggregate(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall method_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_symbol_binding(CppSymbol *symbol, CppSymbol **value) { __asm { int 3 } return 0; }
void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
int __stdcall parameter_stack_bytes(CppSymbol *symbol) { __asm { int 3 } return 0; }
unsigned short __stdcall type_alignment(CType *type) { __asm { int 3 } return 0; }
void __stdcall emit_symbol_declaration(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall symbol_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
char *__stdcall write_decorated_name(FILE *stream, CppSymbol *symbol, void *context, int flags) { __asm { int 3 } return 0; }
void write_ir_filename(char *text) { __asm { int 3 } }
void write_ir_string(char *text) { __asm { int 3 } }
void write_ir_index(unsigned *index) { __asm { int 3 } }
long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin) { __asm { int 3 } return 0; }
void emit_line_directive(void) { __asm { int 3 } }
void report_ide_source(void) { __asm { int 3 } }
void __stdcall record_line_information(unsigned flags) { __asm { int 3 } }
void browser_include_begin(void) { __asm { int 3 } }
void browser_open_file(void) { __asm { int 3 } }
int release_input_handle(void) { __asm { int 3 } return 0; }
char *__stdcall copy_string(char *text, int arena) { __asm { int 3 } return 0; }
int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes) { __asm { int 3 } return 0; }
void browser_symbol_rollover(void) { __asm { int 3 } }
void __stdcall resolve_deferred_declaration(void *declaration) { __asm { int 3 } }
char *__stdcall format_symbol_name(CppSymbol *symbol) { __asm { int 3 } return 0; }
void report_declaration_diagnostic(unsigned warning_code, unsigned error_code, ...) { __asm { int 3 } }
CType *__stdcall strip_based_shape(CType *type) { __asm { int 3 } return 0; }
unsigned short __stdcall classify_incomplete_type(CType *type) { __asm { int 3 } return 0; }
void __stdcall warn_ignored_distance(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *get_current_class_symbol(void) { __asm { int 3 } return 0; }
void __stdcall unwind_qualified_scopes(int stop_at_class) { __asm { int 3 } }
unsigned short begin_type_diagnostic(void) { __asm { int 3 } return 0; }
void end_type_diagnostic(void) { __asm { int 3 } }
short __stdcall format_declarator(CppSymbol *symbol, CType *type) { __asm { int 3 } return 0; }
char *__stdcall type_diagnostic_text(short text) { __asm { int 3 } return 0; }
void __stdcall browser_declaration(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall declare_qualified_member(CppSymbol *symbol, QualifiedDeclarator *qualifier, CppSymbol *member) { __asm { int 3 } return 0; }
char *__stdcall format_qualified_name(QualifierBinding *binding, int verbose) { __asm { int 3 } return 0; }
CppSymbol *__stdcall symbol_construction_class(CppSymbol *symbol) { __asm { int 3 } return 0; }
int __stdcall symbol_has_array_shape(CppSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall initialize_zero_object(CppSymbol *symbol) { __asm { int 3 } }
void __stdcall finalize_object_storage(CppSymbol *symbol) { __asm { int 3 } }
short __stdcall format_storage_spec(short text, StorageSpec storage) { __asm { int 3 } return 0; }
CType *__stdcall function_return_type(CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall report_symbol_declaration_error(unsigned code, CppSymbol *symbol) { __asm { int 3 } }
int __stdcall merge_declared_symbol(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } return 0; }
CppSymbol *__stdcall static_member_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_outer_declaration(CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall record_local_declaration(CppSymbol *declaration) { __asm { int 3 } }
void __stdcall check_declarator_prefix(CppSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }
void __stdcall finish_array_declarator(CppSymbol *symbol, TypeShape **result, ShapeCursor current, int is_typedef) { __asm { int 3 } }
void __stdcall check_pointer_attributes(CppSymbol *symbol, ShapeCursor *current, unsigned diagnostic) { __asm { int 3 } }
unsigned char __stdcall member_pointer_class_kind(CppSymbol *class_symbol) { __asm { int 3 } return 0; }
TypeShape *__stdcall merge_type_shape_prefix(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }
void __stdcall copy_shape_prefix(CppSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }
int __stdcall has_class_initialization(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall reconcile_typedef(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall merge_declaration_storage(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } }
void __stdcall merge_function_defaults(FunctionParameters *current, FunctionParameters *previous, CppSymbol *declaration, int inherit) { __asm { int 3 } }
int __stdcall find_intrinsic_index(char *name) { __asm { int 3 } return 0; }
int __stdcall intrinsic_enabled(int index) { __asm { int 3 } return 0; }
void __stdcall apply_intrinsic(CppSymbol *symbol, int index) { __asm { int 3 } }
void __stdcall report_parameter_error(unsigned code, CppSymbol *symbol, unsigned parameter) { __asm { int 3 } }
TypeShape *__stdcall skip_reference_shape(TypeShape *shape) { __asm { int 3 } return 0; }

/* Complete yacc semantic callees, still unaccepted. */
void __stdcall add_array_declarator(CppSymbol *declaration, IrToken *bound) { __asm { int 3 } }
void __stdcall add_case_label(IrToken *expression) { __asm { int 3 } }
void __stdcall add_named_base(unsigned char access, int is_virtual, char *name) { __asm { int 3 } }
void __stdcall add_qualified_base(unsigned char access, int is_virtual, QualifiedDeclarator *name) { __asm { int 3 } }
void __stdcall flush_scanned_declaration(DeclarationType *type) { __asm { int 3 } }
IrToken *__stdcall append_constructor_initializer(IrToken *list, IrToken *initializer) { __asm { int 3 } return 0; }
IrToken *__stdcall append_expression(IrToken *list, IrToken *expression, int mode) { __asm { int 3 } return 0; }
void __stdcall apply_constructor_initializers(IrToken *initializers) { __asm { int 3 } }
void __stdcall apply_conversion_declarator(DeclarationType *type, TypeShape *shape) { __asm { int 3 } }
CppSymbol *__stdcall apply_declarator_modifier(CppSymbol *declaration, TypeShape *modifier) { __asm { int 3 } return 0; }
void __stdcall begin_braced_initializer(CppSymbol *symbol) { __asm { int 3 } }
void begin_compound_statement(void) { __asm { int 3 } }
void begin_do_condition(void) { __asm { int 3 } }
void begin_do_statement(void) { __asm { int 3 } }
void begin_else_body(void) { __asm { int 3 } }
void begin_for_body(void) { __asm { int 3 } }
void begin_for_condition(void) { __asm { int 3 } }
void begin_for_increment(void) { __asm { int 3 } }
void __stdcall begin_function_definition(DeclarationType *type, CppSymbol *declaration) { __asm { int 3 } }
void begin_if_body(void) { __asm { int 3 } }
void begin_if_condition(void) { __asm { int 3 } }
unsigned __stdcall begin_implicit_statement_scope(IrToken *label) { __asm { int 3 } return 0; }
CppSymbol *__stdcall begin_member_lookup(unsigned char operation, IrToken *object, int mode) { __asm { int 3 } return 0; }
void begin_nested_initializer(void) { __asm { int 3 } }
CppSymbol *__stdcall begin_object_initializer(CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall begin_qc_control(unsigned char mode) { __asm { int 3 } }
void begin_return_expression(void) { __asm { int 3 } }
void begin_seh_filter(void) { __asm { int 3 } }
void begin_seh_finally(void) { __asm { int 3 } }
void begin_seh_try(void) { __asm { int 3 } }
void __stdcall begin_switch_body(IrToken *expression) { __asm { int 3 } }
void begin_switch_condition(void) { __asm { int 3 } }
void begin_while_body(void) { __asm { int 3 } }
void begin_while_condition(void) { __asm { int 3 } }
IrToken *__stdcall binary_expression(unsigned char operation, IrToken *left, IrToken *right) { __asm { int 3 } return 0; }
IrToken *__stdcall boolean_expression(IrToken *expression, int mode) { __asm { int 3 } return 0; }
IrToken *__stdcall build_call_expression(IrToken *expression, IrToken *arguments) { __asm { int 3 } return 0; }
int __stdcall can_initialize(CType *type, IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall cast_expression(CppSymbol *type, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall check_empty_declaration(DeclarationType *type) { __asm { int 3 } }
void __stdcall check_parameter_default(IrToken *expression, CType *type) { __asm { int 3 } }
void __stdcall clear_reference_cache(IrToken *expression) { __asm { int 3 } }
CppSymbol *__stdcall complete_declaration(DeclarationType *type, CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall concatenate_strings(ParserValue *destination, ParserValue *source) { __asm { int 3 } }
IrToken *__stdcall conditional_expression(IrToken *condition, IrToken *if_true, IrToken *if_false) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_default_argument(CType *type, IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_initializer(int mode, CType *type, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall declare_enumerator(char *name, CType *context, unsigned value) { __asm { int 3 } }
void __stdcall declare_nctag(char *name) { __asm { int 3 } }
void __stdcall define_named_label(char *name) { __asm { int 3 } }
IrToken *__stdcall delete_expression(unsigned char operation, IrToken *expression, IrToken *bound, int is_array) { __asm { int 3 } return 0; }
void __stdcall detach_new_array_bound(CppSymbol *type) { __asm { int 3 } }
void __stdcall emit_condition_to_label(IrToken *condition, IrToken *label, int sense) { __asm { int 3 } }
void __stdcall emit_conditional_branch(IrToken *first, IrToken *condition, IrToken *increment) { __asm { int 3 } }
void __stdcall emit_expression_statement(IrToken *expression) { __asm { int 3 } }
void emit_inline_assembly(void) { __asm { int 3 } }
void __stdcall emit_label(IrToken *label) { __asm { int 3 } }
void __stdcall emit_return_statement(IrToken *expression, int mode) { __asm { int 3 } }
void __stdcall emit_seh_filter(IrToken *expression) { __asm { int 3 } }
void emit_seh_leave(void) { __asm { int 3 } }
char *__stdcall encode_type_name(CType *type) { __asm { int 3 } return 0; }
void __stdcall end_braced_initializer(CppSymbol *symbol) { __asm { int 3 } }
void end_compound_statement(void) { __asm { int 3 } }
void end_do_statement(void) { __asm { int 3 } }
void end_else_statement(void) { __asm { int 3 } }
CType * end_enum_specifier(void) { __asm { int 3 } return 0; }
void end_for_statement(void) { __asm { int 3 } }
void end_function_definition(void) { __asm { int 3 } }
void __stdcall end_if_statement(int mode) { __asm { int 3 } }
void end_nested_initializer(void) { __asm { int 3 } }
void end_qc_control(void) { __asm { int 3 } }
void end_seh_except(void) { __asm { int 3 } }
void end_seh_filter(void) { __asm { int 3 } }
void end_seh_finally(void) { __asm { int 3 } }
void end_seh_try(void) { __asm { int 3 } }
void end_switch_statement(void) { __asm { int 3 } }
void end_while_statement(void) { __asm { int 3 } }
unsigned __stdcall evaluate_integer_constant(IrToken *expression) { __asm { int 3 } return 0; }
CppSymbol *__stdcall find_label(char *name) { __asm { int 3 } return 0; }
IrToken *__stdcall finish_call_expression(IrToken *expression, unsigned short flags) { __asm { int 3 } return 0; }
void finish_conditional_branch(void) { __asm { int 3 } }
void finish_default_argument_text(void) { __asm { int 3 } }
void __stdcall finish_external_definition(int recovering) { __asm { int 3 } }
void __stdcall finish_implicit_statement_scope(IrToken *label, unsigned opened) { __asm { int 3 } }
void __stdcall finish_object_initializer(CppSymbol *symbol) { __asm { int 3 } }
int __stdcall has_cv_qualifiers(CppSymbol *symbol) { __asm { int 3 } return 0; }
IrToken *__stdcall identifier_expression(char *name) { __asm { int 3 } return 0; }
void __stdcall initialize_element(IrToken *value, int mode) { __asm { int 3 } }
void __stdcall initialize_object(CppSymbol *symbol, IrToken *value) { __asm { int 3 } }
CppSymbol *__stdcall lookup_based_symbol(char *name) { __asm { int 3 } return 0; }
QualifierResolution *__stdcall lookup_qualified_member(QualifierResolution *scope, char *name, CType *type) { __asm { int 3 } return 0; }
SegmentSymbol *__stdcall lookup_segment_symbol(char *name, char *class_name, unsigned char kind, unsigned unused_mode) { __asm { int 3 } return 0; }
char * make_anonymous_identifier(void) { __asm { int 3 } return 0; }
void mark_evaluation_boundary(void) { __asm { int 3 } }
void __stdcall mark_initialized_symbol(CppSymbol *symbol) { __asm { int 3 } }
IrToken *__stdcall member_destructor_expression(unsigned char operation, CppSymbol *scope, char *name) { __asm { int 3 } return 0; }
IrToken *__stdcall member_name_expression(unsigned char operation, CppSymbol *scope, char *name) { __asm { int 3 } return 0; }
CppSymbol *__stdcall new_abstract_declarator(TypeShape *shape) { __asm { int 3 } return 0; }
IrToken *__stdcall new_branch_label(unsigned attributes) { __asm { int 3 } return 0; }
IrToken *__stdcall new_expression(unsigned char operation, TypeShape *modifier, IrToken *placement,
    CppSymbol *type, IrToken *initializer, unsigned bound) { __asm { int 3 } return 0; }
IrToken *__stdcall new_ir_expression(unsigned char operation, IrToken *left, IrToken *right, CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall new_label(char *name) { __asm { int 3 } return 0; }
IrToken *__stdcall new_unary_ir_expression(unsigned char operation, void *value) { __asm { int 3 } return 0; }
unsigned __stdcall next_enumerator_value(unsigned value, char *unused_name) { __asm { int 3 } return 0; }
IrToken *__stdcall operator_operand_expression(unsigned char operation, IrToken *arguments) { __asm { int 3 } return 0; }
unsigned __stdcall parse_linkage(DeclarationType *type, IrToken *language) { __asm { int 3 } return 0; }
void pop_linkage(void) { __asm { int 3 } }
IrToken *__stdcall postfix_expression(unsigned char operation, IrToken *expression) { __asm { int 3 } return 0; }
void prepare_do_body(void) { __asm { int 3 } }
IrToken *__stdcall prepare_expression(IrToken *expression, int mode) { __asm { int 3 } return 0; }
IrToken *__stdcall qualified_identifier_expression(QualifiedDeclarator *name) { __asm { int 3 } return 0; }
void record_break_statement(void) { __asm { int 3 } }
void record_continue_statement(void) { __asm { int 3 } }
void record_default_label(void) { __asm { int 3 } }
void __stdcall record_goto(CppSymbol *label, int known) { __asm { int 3 } }
void __stdcall record_qc_jump(unsigned char kind) { __asm { int 3 } }
IrToken *__stdcall recover_address_expression(IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall register_label(CppSymbol *label) { __asm { int 3 } }
int __stdcall reject_function_initializer(CppSymbol *unused_declaration) { __asm { int 3 } return 0; }
void __stdcall report_expression_error(unsigned char operation, IrToken *expression) { __asm { int 3 } }
void __stdcall report_missing_declarator(unsigned code, DeclarationType *type) { __asm { int 3 } }
void __stdcall report_type_conversion(unsigned code, int mode, unsigned char operation, CType *source, CType *destination) { __asm { int 3 } }
void __stdcall report_type_error(unsigned code, CType *type) { __asm { int 3 } }
IrToken *__stdcall resolve_assignment_operand(CType *type, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall resolve_member_pointer_class(CppSymbol **result, QualifiedDeclarator *name) { __asm { int 3 } }
DefaultArgument *__stdcall save_default_argument(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall segment_offset_expression(IrToken *segment, IrToken *offset) { __asm { int 3 } return 0; }
IrToken *__stdcall sizeof_expression(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall sizeof_type(CType *type) { __asm { int 3 } return 0; }
void skip_balanced_brace_body(void) { __asm { int 3 } }
IrToken *__stdcall symbol_reference(CppSymbol *symbol) { __asm { int 3 } return 0; }
unsigned char synchronize_tokens(unsigned first, ...) { __asm { int 3 } return 0; }
IrToken *__stdcall unary_expression(unsigned char operation, IrToken *expression) { __asm { int 3 } return 0; }
IrToken * validate_based_self(void) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_bit_field_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_extended_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_integer_bit_field_type(unsigned short flags) { __asm { int 3 } return 0; }
void __stdcall validate_pure_specifier(IrToken *expression) { __asm { int 3 } }
IrToken *__stdcall validate_value_expression(IrToken *expression) { __asm { int 3 } return 0; }
void write_switch_dispatch(void) { __asm { int 3 } }
void __stdcall report_inserted_token(int actual, int expected) { __asm { int 3 } }
ParserValue __stdcall recovery_token_value(int token) { ParserValue value; __asm { int 3 } return value; }
void __stdcall report_parser_error(char *text) { __asm { int 3 } }

/* C++ token classification and deferred lookahead. */
int deferred_tokens_may_be_declaration(void) { __asm { int 3 } return 0; }
unsigned char read_deferred_argument_text(void) { __asm { int 3 } return 0; }
void __stdcall restore_qualified_lookup(QualifierBinding *binding) { __asm { int 3 } }
QualifiedDeclarator *__stdcall copy_qualified_declarator(QualifiedDeclarator *source, int arena) { __asm { int 3 } return 0; }
void __stdcall browser_type_reference(CppSymbol *symbol) { __asm { int 3 } }
unsigned char read_operator_function_id(void) { __asm { int 3 } return 0; }
CppSymbol *__stdcall find_qualifier_name(char *name) { __asm { int 3 } return 0; }
void __stdcall begin_qualification_allocation(int arena) { __asm { int 3 } }
void end_qualification_allocation(void) { __asm { int 3 } }
QualifierResolution *__stdcall append_qualifier_reference(QualifierResolution *resolution,
    QualifierBinding *parent, CppSymbol *target) { __asm { int 3 } return 0; }
void __stdcall enter_qualified_class(CppSymbol *symbol, char *name, int temporary) { __asm { int 3 } }
QualifiedDeclarator *__stdcall new_qualified_declarator(QualifierResolution *resolution,
    char *name, int arena) { __asm { int 3 } return 0; }


int skip_source_comment(void) { __asm { int 3 } return 0; }
int consume_line_escape(void) { __asm { int 3 } return 0; }
unsigned char next_source_character(void) { __asm { int 3 } return 0; }
unsigned recover_trigraph(void) { __asm { int 3 } return 0; }
unsigned recover_special_character(void) { __asm { int 3 } return 0; }
unsigned char close_source_file(void) { __asm { int 3 } return 0; }
void finish_pch_output(void) { __asm { int 3 } }
void finish_translation_symbols(void) { __asm { int 3 } }
int emit_pending_runtime_functions(void) { __asm { int 3 } return 0; }
void skip_source_line(void) { __asm { int 3 } }
int __stdcall write_work_character(int character, WorkingStack *stack) { __asm { int 3 } return 0; }
void __stdcall write_work_bytes(const void *source, unsigned bytes, unsigned count, WorkingStack *stack) { __asm { int 3 } }
unsigned char __stdcall scan_character_literal(int wide) { __asm { int 3 } return 0; }
void __stdcall scan_string_literal(int wide, unsigned char delimiter) { __asm { int 3 } }
void __stdcall copy_quoted_literal(unsigned char delimiter) { __asm { int 3 } }
unsigned __stdcall read_number_text(char *destination) { __asm { int 3 } return 0; }
unsigned char __stdcall scan_number(unsigned char first_character) { __asm { int 3 } return 0; }
unsigned char __stdcall scan_float_tail(char *destination) { __asm { int 3 } return 0; }
unsigned char __stdcall scan_assembler_number(unsigned char first_character) { __asm { int 3 } return 0; }
int scan_macro_identifier(void) { __asm { int 3 } return 0; }
int __stdcall expand_macro(MacroDefinition *macro) { __asm { int 3 } return 0; }
int __stdcall match_source_character(unsigned char character) { __asm { int 3 } return 0; }

void report_ide_line(void) { __asm { int 3 } }
void preprocess_directive(void) { __asm { int 3 } }
void finish_pch_capture(void) { __asm { int 3 } }

void scan_identifier_continuation(char *begin, unsigned hash) { __asm { int 3 } }

void __stdcall add_storage_specifier(StorageSpec *storage, unsigned char kind,
                                    unsigned char modifier, unsigned unused) { __asm { int 3 } }
void read_declspec_attributes(StorageSpec *storage) { __asm { int 3 } }
void __stdcall convert_deferred_type_token(DeferredToken *token, CType *type) { __asm { int 3 } }
CType *__stdcall copy_unfinished_type(CType *type) { __asm { int 3 } return 0; }

CppSymbol *__stdcall reference_tag_symbol(char *name, unsigned short kind,
                                         void (__stdcall *insert)(CppSymbol *)) { __asm { int 3 } return 0; }
char *__stdcall type_kind_name(unsigned short kind) { __asm { int 3 } return 0; }

void __stdcall flush_current_function(int retain) { __asm { int 3 } }
CppSymbol *__stdcall find_tag_in_chain(char *name, CppSymbol *first) { __asm { int 3 } return 0; }
void __stdcall defer_nested_tag(CppSymbol *owner, CppSymbol *tag) { __asm { int 3 } }
void browser_begin_tag(void) { __asm { int 3 } }
int __stdcall browser_name_reference(int reference, char *name) { __asm { int 3 } return 0; }
void __stdcall browser_enter_tag(CppSymbol *tag) { __asm { int 3 } }


CppSymbol *__stdcall find_tag_declaration(CppSymbol *symbol) { __asm { int 3 } return 0; }


int __stdcall query_inheritance_model(int *model) { __asm { int 3 } return 0; }
int __stdcall infer_inheritance_model(CppSymbol *tag) { __asm { int 3 } return 0; }

unsigned __stdcall place_base_subobject(CppSymbol *base, CppSymbol **previous_base, CppSymbol *derived) { __asm { int 3 } return 0; }

void __stdcall record_friend(CppSymbol *owner, DeclarationType *type, CppSymbol *function) { __asm { int 3 } }
void __stdcall browser_friend_reference(CppSymbol *symbol) { __asm { int 3 } }
void __stdcall validate_anonymous_aggregate(DeclarationType *type) { __asm { int 3 } }
void __stdcall declare_anonymous_aggregate(CType *type) { __asm { int 3 } }
void __stdcall select_destructor_type(CppSymbol *member, DeclarationType *context,
    CType **base, StorageSpec *storage) { __asm { int 3 } }
void __stdcall declare_friend_member(DeclarationType *context, CppSymbol *member,
    QualifiedDeclarator *qualified, CppSymbol *owner, StorageSpec storage, int pure) { __asm { int 3 } }
void __stdcall apply_member_access_declaration(CppSymbol *owner, QualifiedDeclarator *qualified) { __asm { int 3 } }
TypeShape *__stdcall last_type_shape(TypeShape *shape) { __asm { int 3 } return 0; }
void __stdcall inherit_class_dll_storage(DeclarationType *context, CppSymbol *member) { __asm { int 3 } }
void __stdcall layout_bit_field(CppSymbol *member, char *name, unsigned short flags, int new_member) { __asm { int 3 } }
int __stdcall validate_union_member(CppSymbol *unused_owner, CppSymbol *member) { __asm { int 3 } return 0; }
void __stdcall create_static_member_alias(CppSymbol *member) { __asm { int 3 } }

void begin_pending_class_names(void) { __asm { int 3 } }
char *next_pending_class_name(void) { __asm { int 3 } return 0; }
void __cdecl format_work_text(WorkingStack *stack, char *format, ...) { __asm { int 3 } }
void __stdcall emit_generated_default_constructor(CppSymbol *symbol, int context, unsigned short kind) { __asm { int 3 } }
void __stdcall emit_generated_copy_constructor(CppSymbol *symbol, int context, unsigned short kind) { __asm { int 3 } }
void __stdcall emit_generated_assignment(CppSymbol *symbol, int context, unsigned short kind) { __asm { int 3 } }
char *__stdcall pointer_kind_spelling(unsigned short kind) { __asm { int 3 } return 0; }
short reserve_type_diagnostic(void) { __asm { int 3 } return 0; }
void __stdcall format_diagnostic_symbol(short text, CppSymbol *symbol) { __asm { int 3 } }
void __stdcall print_inline_declaration(CppSymbol *symbol, unsigned *flags) { __asm { int 3 } }

unsigned __stdcall inner_type_size(unsigned short flags, CppSymbol *tag, TypeShape *shape) { __asm { int 3 } return 0; }
unsigned __stdcall data_member_pointer_size(TypeShape *shape) { __asm { int 3 } return 0; }
unsigned __stdcall member_function_pointer_size(TypeShape *shape) { __asm { int 3 } return 0; }
unsigned __stdcall pointer_kind_size(unsigned short attributes) { __asm { int 3 } return 0; }



QualifierResolution *__stdcall search_class_name_paths(QualifierResolution *path,
    ClassNameSearch *search) { __asm { int 3 } return 0; }
void __stdcall prune_hidden_class_paths(QualifierResolution *paths, HashTable *hidden) { __asm { int 3 } }



void __stdcall set_deferred_type_name(DeferredToken *token) { __asm { int 3 } }
int __stdcall deferred_scope_allows_expression(int depth) { __asm { int 3 } return 0; }
void __stdcall resolve_nested_deferred_types(unsigned char unused_mode) { __asm { int 3 } }
int recursive_lookahead_parse(void) { __asm { int 3 } return 0; }

void __stdcall report_lookahead_error(char *unused_text) { __asm { int 3 } }

int __stdcall has_qualified_this_type(CType *type) { __asm { int 3 } return 0; }
CType *__stdcall transform_function_parameters(CType *type, ParameterTransform transform) { __asm { int 3 } return 0; }
FunctionParameters *__stdcall mark_static_parameters(FunctionParameters *parameters) { __asm { int 3 } return 0; }
void __stdcall classify_copy_assignment(CppSymbol *member) { __asm { int 3 } }
int __stdcall compatible_override_return(CType *member, CType *inherited) { __asm { int 3 } return 0; }
void __stdcall set_pure_virtual_member(CppSymbol *member, CppSymbol *owner, StorageSpec storage) { __asm { int 3 } }
void browser_end_tag(void) { __asm { int 3 } }
void __stdcall finalize_class_virtual_tables(CppSymbol *tag) { __asm { int 3 } }
void __stdcall declare_default_constructor_closures(CppSymbol *tag) { __asm { int 3 } }
void __stdcall declare_deleting_destructors(CppSymbol *tag) { __asm { int 3 } }
void emit_deferred_class_terminator(void) { __asm { int 3 } }
void __stdcall build_class_member_index(CppSymbol *tag) { __asm { int 3 } }
TypeSet *union_type_sets(TypeSet *set, TypeSet *source) { __asm { int 3 } return 0; }
void begin_type_set(TypeSetIterator *iterator, TypeSet *set) { __asm { int 3 } }
int next_type_set(TypeSetIterator *iterator) { __asm { int 3 } return 0; }
int type_set_contains(TypeSet *set, CType *type) { __asm { int 3 } return 0; }
TypeSet *add_type_set(TypeSet *set, CType *type) { __asm { int 3 } return 0; }
TypeSet *remove_type_set(TypeSet *set, CType *type) { __asm { int 3 } return 0; }
CType *__stdcall canonical_conversion_type(CppSymbol *function) { __asm { int 3 } return 0; }
int equal_inline_vectors(InlineVector *left, InlineVector *right) { __asm { int 3 } return 0; }
void __stdcall inherit_base_virtual_tables(CppSymbol *tag, BaseClassLink *base, int *virtual_base) { __asm { int 3 } }
CppSymbol *__stdcall create_virtual_pointer(CppSymbol *tag, int kind) { __asm { int 3 } return 0; }
VirtualTable *__stdcall new_virtual_table(unsigned kind, CppSymbol *tag, unsigned offset, CppSymbol *pointer) { __asm { int 3 } return 0; }
VirtualTable *__stdcall build_virtual_base_table(CppSymbol *tag, CppSymbol *pointer) { __asm { int 3 } return 0; }
int __stdcall find_special_member_model(CppSymbol *tag, unsigned char kind, int first) { __asm { int 3 } return 0; }
void __stdcall merge_subobject_special_members(CppSymbol *tag, CppSymbol *subobject, SpecialMemberState *states, int model) { __asm { int 3 } }
void __stdcall record_special_member(CppSymbol *tag, SpecialMemberState *states, int kind, int model, unsigned nonprivate, unsigned warning) { __asm { int 3 } }
void __stdcall defer_generated_member(CppSymbol *member) { __asm { int 3 } }
CppSymbol *__stdcall find_c_linkage_function(CppSymbol *functions) { __asm { int 3 } return 0; }
void __stdcall prepend_function_overload(CppSymbol *overloads, CppSymbol *declaration) { __asm { int 3 } }

int __stdcall compare_declaration_parameters(DeclarationParameterInfo *left,
    DeclarationParameterInfo *right) { __asm { int 3 } return 0; }
unsigned __stdcall reconcile_declaration_type(CppSymbol *existing,
    CppSymbol *declaration, CType *type) { __asm { int 3 } return 0; }
short __stdcall format_function_parameters(short text, FunctionParameters *parameters,
    unsigned flags) { __asm { int 3 } return 0; }
