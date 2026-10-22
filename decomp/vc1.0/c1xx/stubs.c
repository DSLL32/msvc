/* Named, unaccepted reconstruction frontiers. */
#include "initialization.h"
#include "inline-vector.h"
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
void __stdcall release_arena(int pool) { __asm { int 3 } }
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
CType *__stdcall make_class_pointer_type(CppSymbol *class_symbol, unsigned short attributes) { __asm { int 3 } return 0; }
unsigned char __stdcall member_pointer_is_aggregate(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall method_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_symbol_binding(CppSymbol *symbol, CppSymbol **value) { __asm { int 3 } return 0; }
int __stdcall type_size(CType *type) { __asm { int 3 } return 0; }
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
int __stdcall is_current_class(CppSymbol *symbol) { __asm { int 3 } return 0; }
char *__stdcall format_symbol_name(CppSymbol *symbol) { __asm { int 3 } return 0; }
void report_declaration_diagnostic(unsigned warning_code, unsigned error_code, ...) { __asm { int 3 } }
unsigned short __stdcall type_qualifiers(CType *type) { __asm { int 3 } return 0; }
CType *__stdcall strip_based_shape(CType *type) { __asm { int 3 } return 0; }
unsigned short __stdcall classify_incomplete_type(CType *type) { __asm { int 3 } return 0; }
void __stdcall warn_ignored_distance(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *get_current_class_symbol(void) { __asm { int 3 } return 0; }
CType *__stdcall replace_first_type_shape(CType *type, unsigned short kind, unsigned short attributes, void *parameter) { __asm { int 3 } return 0; }
DeclarationType *__stdcall global_declaration_type(DeclarationType *context) { __asm { int 3 } return 0; }
int __stdcall is_unqualified_class_type(CType *type) { __asm { int 3 } return 0; }
void retire_scope_directory_entry(void) { __asm { int 3 } }
void pop_symbol_scope(void) { __asm { int 3 } }
int __stdcall is_static_qualified_symbol(CppSymbol *symbol) { __asm { int 3 } return 0; }
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
int __stdcall merge_storage_spec(StorageSpec *destination, StorageSpec source) { __asm { int 3 } return 0; }
CType *__stdcall function_return_type(CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall report_symbol_declaration_error(unsigned code, CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall resolve_declaration_alias(CppSymbol *symbol, CppSymbol *declaration) { __asm { int 3 } return 0; }
int __stdcall merge_declared_symbol(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } return 0; }
CppSymbol *__stdcall static_member_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_outer_declaration(CppSymbol *declaration) { __asm { int 3 } return 0; }
CppSymbol *__stdcall find_outer_name(char *name) { __asm { int 3 } return 0; }
void __stdcall record_local_declaration(CppSymbol *declaration) { __asm { int 3 } }
void __stdcall check_declarator_prefix(CppSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }
void __stdcall finish_array_declarator(CppSymbol *symbol, TypeShape **result, ShapeCursor current, int is_typedef) { __asm { int 3 } }
void __stdcall check_pointer_attributes(CppSymbol *symbol, ShapeCursor *current, unsigned diagnostic) { __asm { int 3 } }
int __stdcall merge_member_pointer_shape(CppSymbol *symbol, ShapeCursor inner, ShapeCursor *current, ShapeCursor outer) { __asm { int 3 } return 0; }
unsigned char __stdcall member_pointer_class_kind(CppSymbol *class_symbol) { __asm { int 3 } return 0; }
TypeShape *__stdcall merge_type_shape_prefix(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }
void __stdcall copy_shape_prefix(CppSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }
int __stdcall has_class_initialization(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall bind_function_overload(CppSymbol *overloads, CppSymbol *declaration) { __asm { int 3 } return 0; }
CppSymbol *__stdcall reconcile_typedef(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall merge_declaration_storage(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } }
void __stdcall merge_function_defaults(FunctionParameters *current, FunctionParameters *previous, CppSymbol *declaration, int inherit) { __asm { int 3 } }
void __stdcall initialize_scope_buckets(SymbolScope *scope, CppSymbol *symbol) { __asm { int 3 } }
void __stdcall insert_class_member(CppSymbol *symbol) { __asm { int 3 } }
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
void __stdcall adjust_type_specifier(DeclarationType *type) { __asm { int 3 } }
IrToken *__stdcall append_constructor_initializer(IrToken *list, IrToken *initializer) { __asm { int 3 } return 0; }
IrToken *__stdcall append_expression(IrToken *list, IrToken *expression, int mode) { __asm { int 3 } return 0; }
CppSymbol *__stdcall append_parameter_declaration(CppSymbol *list, CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall apply_constructor_initializers(IrToken *initializers) { __asm { int 3 } }
void __stdcall apply_conversion_declarator(DeclarationType *type, TypeShape *shape) { __asm { int 3 } }
CppSymbol *__stdcall apply_declarator_modifier(CppSymbol *declaration, TypeShape *modifier) { __asm { int 3 } return 0; }
void __stdcall begin_braced_initializer(CppSymbol *symbol) { __asm { int 3 } }
void begin_class_body(void) { __asm { int 3 } }
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
void __stdcall begin_parameter_scope(CppSymbol *first) { __asm { int 3 } }
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
void __stdcall check_class_specifier(DeclarationType *type) { __asm { int 3 } }
void __stdcall check_empty_declaration(DeclarationType *type) { __asm { int 3 } }
void __stdcall check_parameter_default(IrToken *expression, CType *type) { __asm { int 3 } }
void __stdcall clear_reference_cache(IrToken *expression) { __asm { int 3 } }
CppSymbol *__stdcall complete_declaration(DeclarationType *type, CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall concatenate_strings(ParserValue *destination, ParserValue *source) { __asm { int 3 } }
IrToken *__stdcall conditional_expression(IrToken *condition, IrToken *if_true, IrToken *if_false) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_default_argument(CType *type, IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_initializer(int mode, CType *type, IrToken *expression) { __asm { int 3 } return 0; }
char * current_class_name(void) { __asm { int 3 } return 0; }
DeclarationType *__stdcall current_declaration_type(unsigned char *storage) { __asm { int 3 } return 0; }
void __stdcall declare_class_member(DeclarationType *type, CppSymbol *member) { __asm { int 3 } }
CppSymbol *__stdcall declare_constructor(DeclarationType *type, CppSymbol *class_symbol, CppSymbol *parameters) { __asm { int 3 } return 0; }
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
CppSymbol * empty_parameter_declaration(void) { __asm { int 3 } return 0; }
char *__stdcall encode_type_name(CType *type) { __asm { int 3 } return 0; }
void __stdcall end_braced_initializer(CppSymbol *symbol) { __asm { int 3 } }
CType * end_class_specifier(void) { __asm { int 3 } return 0; }
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
void __stdcall finalize_parameter_declaration(CppSymbol *declaration) { __asm { int 3 } }
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
QualifierResolution *__stdcall lookup_conversion_qualification(QualifierResolution *scope, char *name, CType *type) { __asm { int 3 } return 0; }
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
void __stdcall read_type_specifier(unsigned char token) { __asm { int 3 } }
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
unsigned char scan_token(void) { __asm { int 3 } return 0; }
IrToken *__stdcall segment_offset_expression(IrToken *segment, IrToken *offset) { __asm { int 3 } return 0; }
void __stdcall set_member_access(unsigned char access) { __asm { int 3 } }
IrToken *__stdcall sizeof_expression(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall sizeof_type(CType *type) { __asm { int 3 } return 0; }
void skip_balanced_brace_body(void) { __asm { int 3 } }
IrToken *__stdcall symbol_reference(CppSymbol *symbol) { __asm { int 3 } return 0; }
unsigned char synchronize_tokens(unsigned first, ...) { __asm { int 3 } return 0; }
IrToken *__stdcall unary_expression(unsigned char operation, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall unget_token(unsigned char token) { __asm { int 3 } }
IrToken * validate_based_self(void) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_bit_field_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_extended_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_integer_bit_field_type(unsigned short flags) { __asm { int 3 } return 0; }
void __stdcall validate_pure_specifier(IrToken *expression) { __asm { int 3 } }
IrToken *__stdcall validate_value_expression(IrToken *expression) { __asm { int 3 } return 0; }
void write_switch_dispatch(void) { __asm { int 3 } }
unsigned char read_parser_token(void) { __asm { int 3 } return 0; }
void __stdcall report_inserted_token(int actual, int expected) { __asm { int 3 } }
ParserValue __stdcall recovery_token_value(int token) { ParserValue value; __asm { int 3 } return value; }
void __stdcall report_parser_error(char *text) { __asm { int 3 } }
