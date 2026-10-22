#include "mangling.h"
#include "symbol-output.h"
#include "function-finalization.h"
#include "record-processing.h"
#include "binary-expressions.h"
#include "return-statements.h"
#include "expression-preparation.h"
#include "constructor-initializers.h"
#include "statement-scopes.h"
#include "work-list.h"
#include "cpp-context.h"
#include "expression-records.h"
#include "function-definitions.h"
#include "unary-expressions.h"
#include "control-ir.h"
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
#include "object-initializers.h"
#include "call-expressions.h"
#include <recon.h>
void interrupt_compile(int signal_number) { __asm { int 3 } }
void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }
void clear_packing(PackingEntry *entry) { __asm { int 3 } }
void __stdcall grow_vector(Vector *vector) { __asm { int 3 } }
void compiler_warning(int level, unsigned code, ...) { __asm { int 3 } }
void preprocess_translation_unit(void) { __asm { int 3 } }
void __stdcall record_string_literal(IrToken *token) { __asm { int 3 } }
IrToken *__stdcall emit_version_token(IrToken *token) { __asm { int 3 } return 0; }
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
void report_syntax_error(unsigned code, ...) { __asm { int 3 } }
void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall report_macro_error(char *directive) { __asm { int 3 } }
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count) { __asm { int 3 } return 0; }
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall browser_macro(MacroDefinition *definition) { __asm { int 3 } }
unsigned char __stdcall member_pointer_is_aggregate(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall method_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_symbol_binding(CppSymbol *symbol, CppSymbol **value) { __asm { int 3 } return 0; }
void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
int __stdcall parameter_stack_bytes(CppSymbol *symbol) { __asm { int 3 } return 0; }
CppSymbol *__stdcall symbol_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
void write_ir_string(char *text) { __asm { int 3 } }
long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin) { __asm { int 3 } return 0; }
void emit_line_directive(void) { __asm { int 3 } }
void report_ide_source(void) { __asm { int 3 } }
void browser_include_begin(void) { __asm { int 3 } }
void browser_open_file(void) { __asm { int 3 } }
int release_input_handle(void) { __asm { int 3 } return 0; }
char *__stdcall copy_string(char *text, int arena) { __asm { int 3 } return 0; }
int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes) { __asm { int 3 } return 0; }
void browser_symbol_rollover(void) { __asm { int 3 } }
char *__stdcall format_symbol_name(CppSymbol *symbol) { __asm { int 3 } return 0; }
void report_declaration_diagnostic(unsigned warning_code, unsigned error_code, ...) { __asm { int 3 } }
void __stdcall warn_ignored_distance(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *get_current_class_symbol(void) { __asm { int 3 } return 0; }
unsigned short begin_type_diagnostic(void) { __asm { int 3 } return 0; }
void end_type_diagnostic(void) { __asm { int 3 } }
short __stdcall format_declarator(CppSymbol *symbol, CType *type) { __asm { int 3 } return 0; }
char *__stdcall type_diagnostic_text(short text) { __asm { int 3 } return 0; }
void __stdcall browser_declaration(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall declare_qualified_member(CppSymbol *symbol, QualifiedDeclarator *qualifier, CppSymbol *member) { __asm { int 3 } return 0; }
char *__stdcall format_qualified_name(QualifierBinding *binding, int verbose) { __asm { int 3 } return 0; }
void __stdcall initialize_zero_object(CppSymbol *symbol) { __asm { int 3 } }
int __stdcall has_destructor_member_model(CType *type) { __asm { int 3 } return 0; }
void __stdcall defer_object_definition(CppSymbol *symbol) { __asm { int 3 } }
short __stdcall format_storage_spec(short text, StorageSpec storage) { __asm { int 3 } return 0; }
void __stdcall report_symbol_declaration_error(unsigned code, CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall static_member_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
char *__stdcall make_local_static_name(CppSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall record_function_symbol_alias(CppSymbol *symbol, CppSymbol *alias) { __asm { int 3 } }
void __stdcall check_declarator_prefix(CppSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }
void __stdcall finish_array_declarator(CppSymbol *symbol, TypeShape **result, ShapeCursor current, int is_typedef) { __asm { int 3 } }
void __stdcall check_pointer_attributes(CppSymbol *symbol, ShapeCursor *current, unsigned diagnostic) { __asm { int 3 } }
unsigned char __stdcall member_pointer_class_kind(CppSymbol *class_symbol) { __asm { int 3 } return 0; }
TypeShape *__stdcall merge_type_shape_prefix(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }
void __stdcall copy_shape_prefix(CppSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }
CppSymbol *__stdcall reconcile_typedef(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } return 0; }
int __stdcall find_intrinsic_index(char *name) { __asm { int 3 } return 0; }
int __stdcall intrinsic_enabled(int index) { __asm { int 3 } return 0; }
void __stdcall apply_intrinsic(CppSymbol *symbol, int index) { __asm { int 3 } }
void __stdcall report_parameter_error(unsigned code, CppSymbol *symbol, unsigned parameter) { __asm { int 3 } }


/* Complete yacc semantic callees, still unaccepted. */
void __stdcall add_array_declarator(CppSymbol *declaration, IrToken *bound) { __asm { int 3 } }
void __stdcall add_case_label(IrToken *expression) { __asm { int 3 } }
void __stdcall add_named_base(unsigned char access, int is_virtual, char *name) { __asm { int 3 } }
void __stdcall add_qualified_base(unsigned char access, int is_virtual, QualifiedDeclarator *name) { __asm { int 3 } }

void __stdcall apply_conversion_declarator(DeclarationType *type, TypeShape *shape) { __asm { int 3 } }
CppSymbol *__stdcall apply_declarator_modifier(CppSymbol *declaration, TypeShape *modifier) { __asm { int 3 } return 0; }
void __stdcall begin_braced_initializer(CppSymbol *symbol) { __asm { int 3 } }
void begin_do_condition(void) { __asm { int 3 } }
void begin_do_statement(void) { __asm { int 3 } }
void begin_else_body(void) { __asm { int 3 } }
void begin_for_body(void) { __asm { int 3 } }
void begin_for_condition(void) { __asm { int 3 } }
void begin_for_increment(void) { __asm { int 3 } }
void begin_if_body(void) { __asm { int 3 } }
void begin_if_condition(void) { __asm { int 3 } }
unsigned __stdcall begin_implicit_statement_scope(IrToken *label) { __asm { int 3 } return 0; }
void begin_nested_initializer(void) { __asm { int 3 } }
void __stdcall begin_qc_control(unsigned char mode) { __asm { int 3 } }
void begin_seh_filter(void) { __asm { int 3 } }
void begin_seh_finally(void) { __asm { int 3 } }
void begin_seh_try(void) { __asm { int 3 } }
void __stdcall begin_switch_body(IrToken *expression) { __asm { int 3 } }
void begin_switch_condition(void) { __asm { int 3 } }
void begin_while_body(void) { __asm { int 3 } }
void begin_while_condition(void) { __asm { int 3 } }
IrToken *__stdcall boolean_expression(IrToken *expression, int mode) { __asm { int 3 } return 0; }
void __stdcall diagnose_overload_failure(OverloadCandidate *candidate, OverloadSearch *search, IrToken *argument) { __asm { int 3 } }
OverloadCandidate *__stdcall resolve_overload_tie(OverloadSearch *search) { __asm { int 3 } return 0; }
OverloadCandidate *__stdcall report_call_overload_failure(OverloadSearch *search) { __asm { int 3 } return 0; }
CppSymbol *__stdcall first_function_vector_item(Vector *functions) { __asm { int 3 } return 0; }
void __stdcall register_temporary_destruction(IrToken *object, CType *type) { __asm { int 3 } }
QualifierBinding *__stdcall find_named_class_binding(CppSymbol *tag, char *name, unsigned categories) { __asm { int 3 } return 0; }
IrToken *__stdcall try_elide_copy_construction(CppSymbol *function, IrToken *object, IrToken *arguments) { __asm { int 3 } return 0; }
IrToken *__stdcall cast_to_type(CType *type, IrToken *operand) { __asm { int 3 } return 0; }
int __stdcall resolve_function_style_cast(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall lower_member_pointer_call(IrToken *expression, IrToken **prefix) { __asm { int 3 } return 0; }
unsigned __stdcall validate_call_arguments(CType *type, TypeShape *function, IrToken *arguments, unsigned count) { __asm { int 3 } return 0; }
void __stdcall report_call_argument_error(unsigned code, CppSymbol *function, unsigned index) { __asm { int 3 } }
IrToken *__stdcall append_default_call_argument(IrToken *arguments, void *expression) { __asm { int 3 } return 0; }
CppSymbol *__stdcall create_initializer_temporary(unsigned char storage, CType *type) { __asm { int 3 } return 0; }
void __stdcall record_initialized_temporary(CppSymbol *symbol, CType *type) { __asm { int 3 } }
IrToken *__stdcall materialize_call_result(IrToken *call) { __asm { int 3 } return 0; }
int __stdcall is_this_object_expression(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall cast_expression(CppSymbol *type, IrToken *expression) { __asm { int 3 } return 0; }

void __stdcall check_parameter_default(IrToken *expression, CType *type) { __asm { int 3 } }
void __stdcall clear_reference_cache(IrToken *expression) { __asm { int 3 } }
CppSymbol *__stdcall complete_declaration(DeclarationType *type, CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall concatenate_strings(ParserValue *destination, ParserValue *source) { __asm { int 3 } }
IrToken *__stdcall conditional_expression(IrToken *condition, IrToken *if_true, IrToken *if_false) { __asm { int 3 } return 0; }
IrToken * __stdcall convert_derived_initializer(CType *type, IrToken *expression) { __asm { int 3 } return 0; }
CppSymbol * __stdcall find_copy_constructor(CType *type) { __asm { int 3 } return 0; }
IrToken * __stdcall construct_copy_initializer(CppSymbol *constructor, CType *type, IrToken *expression) { __asm { int 3 } return 0; }
IrToken * __stdcall convert_scalar_initializer(unsigned char operation, CType *type, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall declare_enumerator(char *name, CType *context, unsigned value) { __asm { int 3 } }
void __stdcall declare_nctag(char *name) { __asm { int 3 } }
void __stdcall define_named_label(char *name) { __asm { int 3 } }
IrToken *__stdcall delete_expression(unsigned char operation, IrToken *expression, IrToken *bound, int is_array) { __asm { int 3 } return 0; }
void __stdcall detach_new_array_bound(CppSymbol *type) { __asm { int 3 } }
void __stdcall emit_condition_to_label(IrToken *condition, IrToken *label, int sense) { __asm { int 3 } }
void __stdcall emit_conditional_branch(IrToken *first, IrToken *condition, IrToken *increment) { __asm { int 3 } }
IrToken *__stdcall strip_address_expression(IrToken *expression) { __asm { int 3 } return 0; }
void emit_inline_assembly(void) { __asm { int 3 } }
void emit_pending_function_cleanup(void) { __asm { int 3 } }
IrToken *__stdcall new_right_typed_expression(unsigned char operation, IrToken *left, IrToken *right) { __asm { int 3 } return 0; }
void __stdcall emit_seh_filter(IrToken *expression) { __asm { int 3 } }
void emit_seh_leave(void) { __asm { int 3 } }
char *__stdcall encode_type_name(CType *type) { __asm { int 3 } return 0; }
void __stdcall end_braced_initializer(CppSymbol *symbol) { __asm { int 3 } }
void end_do_statement(void) { __asm { int 3 } }
void end_else_statement(void) { __asm { int 3 } }
CType * end_enum_specifier(void) { __asm { int 3 } return 0; }
void end_for_statement(void) { __asm { int 3 } }
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
void increment_expression_record_depth(void) { __asm { int 3 } }

void __stdcall finish_implicit_statement_scope(IrToken *label, unsigned opened) { __asm { int 3 } }
void __stdcall finish_object_initializer(CppSymbol *symbol) { __asm { int 3 } }
int __stdcall has_cv_qualifiers(CppSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall initialize_element(IrToken *value, int mode) { __asm { int 3 } }
CppSymbol *__stdcall lookup_destructor_member(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_vector_delete_destructor(CType *type) { __asm { int 3 } return 0; }
IrToken *__stdcall initialize_allocated_scalar(IrToken *object, IrToken *initializer) { __asm { int 3 } return 0; }
void __stdcall capture_allocation_pointer(IrToken **saved, IrToken **object) { __asm { int 3 } }
IrToken *__stdcall guard_allocation_result(IrToken *saved, IrToken *object) { __asm { int 3 } return 0; }
IrToken *__stdcall initialize_allocated_array(IrToken *object, int allocated,
    unsigned prefix_bytes, IrToken *element_count, int element_bytes) { __asm { int 3 } return 0; }
void __stdcall finish_local_static_initialization(CppSymbol *symbol) { __asm { int 3 } }
IrToken *__stdcall prepare_constant_initializer(CppSymbol *symbol, IrToken *value) { __asm { int 3 } return 0; }
int __stdcall is_character_array_initializer(CppSymbol *symbol, IrToken *value) { __asm { int 3 } return 0; }
unsigned short __stdcall fit_string_initializer(CppSymbol *symbol, IrToken *value) { __asm { int 3 } return 0; }
int __stdcall check_static_initializer(CppSymbol *symbol, IrToken **value) { __asm { int 3 } return 0; }
void __stdcall begin_initializer_function(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall create_generated_object(unsigned char storage, CType *type) { __asm { int 3 } return 0; }
int __stdcall is_scalar_initializer_symbol(CppSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall emit_initializer_assignment(IrToken *target, IrToken *value) { __asm { int 3 } }
void __stdcall finish_initializer_symbol(CppSymbol *symbol) { __asm { int 3 } }
IrToken *__stdcall normalize_initializer_expression(IrToken *value) { __asm { int 3 } return 0; }
void __stdcall write_literal_initializer(CppSymbol *symbol, IrToken *value, void *context) { __asm { int 3 } }
void __stdcall emit_runtime_symbol_initializer(CppSymbol *symbol, IrToken *value) { __asm { int 3 } }
int __stdcall remaining_string_initializer_bytes(CppSymbol *symbol, IrToken *value) { __asm { int 3 } return 0; }
void __stdcall emit_initializer_constant(IrToken *target) { __asm { int 3 } }
CppSymbol *__stdcall lookup_based_symbol(char *name) { __asm { int 3 } return 0; }

SegmentSymbol *__stdcall lookup_segment_symbol(char *name, char *class_name, unsigned char kind, unsigned unused_mode) { __asm { int 3 } return 0; }
char * make_anonymous_identifier(void) { __asm { int 3 } return 0; }
void mark_evaluation_boundary(void) { __asm { int 3 } }
IrToken *__stdcall member_destructor_expression(unsigned char operation, IrToken *object, char *name) { __asm { int 3 } return 0; }
CppSymbol *__stdcall new_abstract_declarator(TypeShape *shape) { __asm { int 3 } return 0; }
IrToken *__stdcall new_expression(unsigned char operation, TypeShape *modifier, IrToken *placement,
    CppSymbol *type, IrToken *initializer, unsigned bound) { __asm { int 3 } return 0; }
CppSymbol *__stdcall new_label(char *name) { __asm { int 3 } return 0; }
IrToken *__stdcall new_unary_ir_expression(unsigned char operation, void *value) { __asm { int 3 } return 0; }
unsigned __stdcall next_enumerator_value(unsigned value, char *unused_name) { __asm { int 3 } return 0; }
IrToken *__stdcall operator_operand_expression(unsigned char operation, IrToken *arguments) { __asm { int 3 } return 0; }
unsigned __stdcall parse_linkage(DeclarationType *type, IrToken *language) { __asm { int 3 } return 0; }
void pop_linkage(void) { __asm { int 3 } }
void prepare_do_body(void) { __asm { int 3 } }
void record_break_statement(void) { __asm { int 3 } }
void record_continue_statement(void) { __asm { int 3 } }
void record_default_label(void) { __asm { int 3 } }
void __stdcall record_goto(CppSymbol *label, int known) { __asm { int 3 } }
void __stdcall record_qc_jump(unsigned char kind) { __asm { int 3 } }
IrToken *__stdcall member_pointer_expression(QualifiedDeclarator *qualifier) { __asm { int 3 } return 0; }
IrToken *__stdcall report_expression_error(unsigned char operation, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall report_missing_declarator(unsigned code, DeclarationType *type) { __asm { int 3 } }
void __stdcall report_type_conversion(unsigned code, int mode, unsigned char operation, CType *source, CType *destination) { __asm { int 3 } }
void __stdcall report_type_error(unsigned code, CType *type) { __asm { int 3 } }
void __stdcall resolve_member_pointer_class(CppSymbol **result, QualifiedDeclarator *name) { __asm { int 3 } }
DefaultArgument *__stdcall save_default_argument(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall segment_offset_expression(IrToken *segment, IrToken *offset) { __asm { int 3 } return 0; }
IrToken *__stdcall sizeof_expression(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall sizeof_type(CType *type) { __asm { int 3 } return 0; }
void skip_balanced_brace_body(void) { __asm { int 3 } }
unsigned char synchronize_tokens(unsigned first, ...) { __asm { int 3 } return 0; }
IrToken * validate_based_self(void) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_bit_field_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_extended_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_integer_bit_field_type(unsigned short flags) { __asm { int 3 } return 0; }
void __stdcall validate_pure_specifier(IrToken *expression) { __asm { int 3 } }
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







int skip_source_comment(void) { __asm { int 3 } return 0; }
int consume_line_escape(void) { __asm { int 3 } return 0; }
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






void __stdcall set_deferred_type_name(DeferredToken *token) { __asm { int 3 } }
int __stdcall deferred_scope_allows_expression(int depth) { __asm { int 3 } return 0; }
void __stdcall resolve_nested_deferred_types(unsigned char unused_mode) { __asm { int 3 } }
int recursive_lookahead_parse(void) { __asm { int 3 } return 0; }

void __stdcall report_lookahead_error(char *unused_text) { __asm { int 3 } }

int __stdcall has_qualified_this_type(CType *type) { __asm { int 3 } return 0; }
CType *__stdcall transform_function_parameters(CType *type, ParameterTransform transform) { __asm { int 3 } return 0; }
FunctionParameters *__stdcall mark_static_parameters(FunctionParameters *parameters) { __asm { int 3 } return 0; }

int __stdcall compatible_override_return(CType *member, CType *inherited) { __asm { int 3 } return 0; }
void __stdcall set_pure_virtual_member(CppSymbol *member, CppSymbol *owner, StorageSpec storage) { __asm { int 3 } }
void browser_end_tag(void) { __asm { int 3 } }



void emit_deferred_class_terminator(void) { __asm { int 3 } }

TypeSet *union_type_sets(TypeSet *set, TypeSet *source) { __asm { int 3 } return 0; }
void begin_type_set(TypeSetIterator *iterator, TypeSet *set) { __asm { int 3 } }
int next_type_set(TypeSetIterator *iterator) { __asm { int 3 } return 0; }
int type_set_contains(TypeSet *set, CType *type) { __asm { int 3 } return 0; }
InlineVector *add_inline_pointer(InlineVector *vector, void *pointer) { __asm { int 3 } return 0; }
TypeSet *remove_type_set(TypeSet *set, CType *type) { __asm { int 3 } return 0; }
CType *__stdcall canonical_conversion_type(CppSymbol *function) { __asm { int 3 } return 0; }
int equal_inline_vectors(InlineVector *left, InlineVector *right) { __asm { int 3 } return 0; }
void __stdcall inherit_base_virtual_tables(CppSymbol *tag, BaseClassLink *base, int *virtual_base) { __asm { int 3 } }
CppSymbol *__stdcall create_virtual_pointer(CppSymbol *tag, int kind) { __asm { int 3 } return 0; }
VirtualTable *__stdcall new_virtual_table(unsigned kind, CppSymbol *tag, unsigned offset, CppSymbol *pointer) { __asm { int 3 } return 0; }
VirtualTable *__stdcall build_virtual_base_table(CppSymbol *tag, CppSymbol *pointer) { __asm { int 3 } return 0; }

void __stdcall merge_subobject_special_members(CppSymbol *tag, CppSymbol *subobject, SpecialMemberState *states, int model) { __asm { int 3 } }
void __stdcall record_special_member(CppSymbol *tag, SpecialMemberState *states, int kind, int model, unsigned nonprivate, unsigned warning) { __asm { int 3 } }
CppSymbol *__stdcall find_c_linkage_function(CppSymbol *functions) { __asm { int 3 } return 0; }



short __stdcall format_function_parameters(short text, FunctionParameters *parameters,
    unsigned flags) { __asm { int 3 } return 0; }
















void __stdcall complete_direct_base_layout(BaseClassLink *base, CppSymbol *tag) { __asm { int 3 } }

int __stdcall find_virtual_base_group_offsets(VirtualTableGroup *group, CppSymbol *tag) { __asm { int 3 } return 0; }
void __stdcall create_virtual_base_group_offsets(VirtualTableGroup *group, CppSymbol *tag) { __asm { int 3 } }



void __stdcall resolve_table_override(CppSymbol *member, VirtualTable *table,
    CppSymbol *tag, char virtual_path) { __asm { int 3 } }
VirtualTable *__stdcall primary_virtual_table(CppSymbol *tag) { __asm { int 3 } return 0; }
void __stdcall initialize_virtual_entry(VirtualEntry *entry, CppSymbol *member) { __asm { int 3 } }
void __stdcall append_virtual_entry(VirtualTable *table, VirtualEntry *entry) { __asm { int 3 } }
unsigned __stdcall reserve_virtual_table_offset(VirtualTable *table, unsigned size) { __asm { int 3 } return 0; }

void __stdcall collapse_virtual_conflicts(VirtualEntry *entry) { __asm { int 3 } }

int __stdcall equal_vectors(Vector *left, Vector *right) { __asm { int 3 } return 0; }


void *__stdcall clone_class_virtual_table(void *item, int arena) { __asm { int 3 } return 0; }
void *__stdcall clone_class_virtual_group(void *item, int arena) { __asm { int 3 } return 0; }



CppSymbol *__stdcall declare_virtual_table_symbol(int kind, VirtualTable *table, CppSymbol *tag) { __asm { int 3 } return 0; }


void __stdcall make_default_constructor_closure(CppSymbol *tag, CppSymbol *constructor, int model) { __asm { int 3 } }




void __stdcall make_deleting_destructors(CppSymbol *tag, CppSymbol *destructor, int model) { __asm { int 3 } }

void __stdcall write_storage_specification(StorageSpec storage, FILE *file) { __asm { int 3 } }

void reset_pushed_tokens(void) { __asm { int 3 } }

void __stdcall finish_work_stack(WorkingStack *stack) { __asm { int 3 } }

void __stdcall declare_anonymous_aggregate_object(DeclarationType *type) { __asm { int 3 } }

CppSymbol *__stdcall lookup_tag_name(char *name) { __asm { int 3 } return 0; }

void *__stdcall find_hash_value(HashTable *table, void *key) { __asm { int 3 } return 0; }
HashEntry *__stdcall set_hash_value(HashTable *table, void *key, void *value) { __asm { int 3 } return 0; }

int __stdcall qualifier_path_is_hidden(QualifierBinding *path, HashTable *hidden) { __asm { int 3 } return 0; }

void discard_function_definition(void) { __asm { int 3 } }
void __stdcall report_declaration_error(unsigned code, CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall class_tag_for_type(CType *type) { __asm { int 3 } return 0; }
void __stdcall assign_symbol_segment(CppSymbol *symbol, SegmentSymbol *segment) { __asm { int 3 } }

void __stdcall inherit_function_defaults(CppSymbol *declaration, DefaultArgument *defaults,
    FunctionParameters *previous, FunctionParameters *current, DefaultArgument *following) { __asm { int 3 } }

void __stdcall report_named_type_difference(unsigned code, CType *context, char *name,
    CType *source, CType *destination) { __asm { int 3 } }

void __stdcall diagnose_parameter_list_difference(FunctionParameters *existing,
    FunctionParameters *declaration) { __asm { int 3 } }



CppSymbol *make_missing_function_declaration(void) { __asm { int 3 } return 0; }
void initialize_function_inline_analysis(void) { __asm { int 3 } }



CppSymbol *__stdcall recover_undeclared_identifier(char *name) { __asm { int 3 } return 0; }
IrToken *most_derived_expression(void) { __asm { int 3 } return 0; }


IrToken *__stdcall enumerator_expression(CppSymbol *symbol) { __asm { int 3 } return 0; }
CType *__stdcall make_expression_qualified_type(CType *base, unsigned short kind,
    unsigned short attributes, void *parameter) { __asm { int 3 } return 0; }

CType *__stdcall parameter_reference_type(IrToken *reference) { __asm { int 3 } return 0; }
IrToken *__stdcall based_type_expression(CType *type) { __asm { int 3 } return 0; }


IrToken *__stdcall qualify_based_storage(IrToken *expression) { __asm { int 3 } return 0; }

IrToken *__stdcall convert_based_expression(CType *type, IrToken *expression) { __asm { int 3 } return 0; }

IrToken *__stdcall attach_expression_symbols(IrToken *expression, ExpressionSymbolList *symbols) { __asm { int 3 } return 0; }

unsigned __stdcall function_parameter_bytes(CppSymbol *parameters) { __asm { int 3 } return 0; }
void __stdcall print_declaration(CppSymbol *symbol, FILE *stream) { __asm { int 3 } }

void __stdcall analyze_inline_parameter(CppSymbol *parameter) { __asm { int 3 } }

void write_counted_ir(short length, char *text) { __asm { int 3 } }

IrToken *__stdcall resolve_operator_expression(unsigned char operation, IrToken *left,
    IrToken *right, unsigned arity, unsigned *failure) { __asm { int 3 } return 0; }
int __stdcall report_operator_resolution_failure(unsigned char operation, unsigned char arity,
    unsigned failure, CType *left, CType *right) { __asm { int 3 } return 0; }

int __stdcall has_single_function(CppSymbol *symbol) { __asm { int 3 } return 0; }
IrToken *__stdcall promote_arithmetic_expression(IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall type_warning(unsigned level, unsigned code, CType *type) { __asm { int 3 } }
IrToken *__stdcall make_address_temporary(IrToken *expression) { __asm { int 3 } return 0; }
IrToken * __stdcall fold_binary_constant(IrToken *expression, IrToken *left, IrToken *right) { __asm { int 3 } return 0; }
IrToken * __stdcall apply_member_pointer(IrToken *expression) { __asm { int 3 } return 0; }
IrToken * __stdcall convert_pointer_operands(IrToken *expression, IrToken *left, IrToken *right) { __asm { int 3 } return 0; }
int __stdcall reject_member_pointer_arithmetic(IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall resolve_untyped_binary_operand(unsigned char operation, IrToken **operand, IrToken *other) { __asm { int 3 } }
IrToken * __stdcall compare_member_pointer_expression(IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall report_incompatible_type_shapes(unsigned char operation, CType *target, CType *source) { __asm { int 3 } }
int __stdcall check_conversion_qualifiers(unsigned char operation, unsigned short target, unsigned short source, int first_layer) { __asm { int 3 } return 0; }
void __stdcall check_based_pointer_conversion(unsigned char operation, TypeShape *target, TypeShape *source) { __asm { int 3 } }
void __stdcall report_invalid_operand(unsigned char operation, int position) { __asm { int 3 } }
IrToken * __stdcall convert_class_condition(IrToken *expression, int mode, unsigned *selection) { __asm { int 3 } return 0; }
IrToken *__stdcall find_expression_type_base(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall make_assignment_lvalue(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall make_comma_lvalue(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall make_conditional_lvalue(IrToken *expression) { __asm { int 3 } return 0; }
CType *__stdcall change_function_reference_kind(CType *type, int mode) { __asm { int 3 } return 0; }
CType *__stdcall function_designator_type(CType *type) { __asm { int 3 } return 0; }
int subtract_extended(const Extended *left, const Extended *right, Extended *result) { __asm { int 3 } return 0; }
Extended *__stdcall save_extended_constant(Extended *value) { __asm { int 3 } return 0; }
int __stdcall find_inline_parameter_slot(unsigned *slots, unsigned count, unsigned symbol_index) { __asm { int 3 } return 0; }

int __stdcall emit_string_intrinsic(IrToken *expression) { __asm { int 3 } return 0; }


unsigned __stdcall complete_type_size(CType *type) { __asm { int 3 } return 0; }
unsigned __stdcall member_pointer_inheritance_model(CType *type) { __asm { int 3 } return 0; }
void __stdcall begin_destructor_body(CppSymbol *tag) { __asm { int 3 } }
void __stdcall emit_deleting_destructor_body(CppSymbol *tag) { __asm { int 3 } }
void __stdcall emit_vector_deleting_destructor_body(CppSymbol *tag) { __asm { int 3 } }
void __stdcall emit_default_ctor_closure_body(CppSymbol *tag) { __asm { int 3 } }
void __stdcall set_base_initialization_access(int enabled) { __asm { int 3 } }
void __stdcall emit_virtual_base_initializers(CppSymbol *tag, IrToken **initializers, unsigned short attributes) { __asm { int 3 } }
void __stdcall emit_array_copy_initialization(CppSymbol *member, unsigned displacement, IrToken *initializers, unsigned short attributes) { __asm { int 3 } }
void __stdcall emit_class_array_initialization(CppSymbol *member, CppSymbol *tag, IrToken *initializers, unsigned displacement, int copying, unsigned short attributes) { __asm { int 3 } }
IrToken *__stdcall make_subobject_pointer(CppSymbol *tag, unsigned short attributes, unsigned displacement) { __asm { int 3 } return 0; }
IrToken *__stdcall clear_most_derived_argument(IrToken *call) { __asm { int 3 } return 0; }
QualifierBinding * __stdcall make_table_qualifier_path(VirtualTable *table, CppSymbol *tag) { __asm { int 3 } return 0; }
void __stdcall initialize_qualified_member(QualifierBinding *path, IrToken *value) { __asm { int 3 } }
unsigned __stdcall virtual_table_object_offset(VirtualTable *table) { __asm { int 3 } return 0; }
IrToken * __stdcall make_vtordisp_value(CppSymbol *tag, VirtualTableGroup *group) { __asm { int 3 } return 0; }
QualifierBinding * __stdcall make_vtordisp_qualifier_path(CppSymbol *tag, VirtualTableGroup *group) { __asm { int 3 } return 0; }
void __stdcall restore_expression_symbols(ExpressionSymbolList *symbols) { __asm { int 3 } }
int __stdcall has_deferred_expression_symbols(unsigned mode) { __asm { int 3 } return 0; }
IrToken *__stdcall new_temporary_reference(unsigned index, CType *type) { __asm { int 3 } return 0; }
unsigned __stdcall classify_special_scalar_conversion(CType *target, CType *source, unsigned char flags, OverloadCandidate *context) { __asm { int 3 } return 0; }
int __stdcall has_pointer_or_member_pointer_shape(CType *type) { __asm { int 3 } return 0; }
unsigned __stdcall classify_null_pointer_shape(unsigned rank, TypeShape *target, TypeShape *source) { __asm { int 3 } return 0; }
int __stdcall based_pointer_has_indirect_base(TypeShape *shape) { __asm { int 3 } return 0; }
unsigned __stdcall classify_nested_shape_conversion(TypeShape *target, TypeShape *source) { __asm { int 3 } return 0; }
unsigned __stdcall rank_reference_attributes(unsigned rank, CType *target, CType *source) { __asm { int 3 } return 0; }
unsigned __stdcall classify_member_pointer_conversion(CType *source, CType *target, int reverse) { __asm { int 3 } return 0; }
CppSymbol *__stdcall find_source_conversion(CppSymbol *tag, CType *target, unsigned *rank) { __asm { int 3 } return 0; }
int __stdcall type_has_base_relation(CType *derived, CType *base) { __asm { int 3 } return 0; }
CppSymbol *__stdcall resolve_overloaded_initializer(IrToken *expression, CType *type, int mode) { __asm { int 3 } return 0; }

IrToken *__stdcall collect_declaration_cleanup(IrToken *expression, FrontendRecord *from, int mode) { __asm { int 3 } return 0; }
void __stdcall resolve_goto_cleanup(FrontendRecord *from, FrontendRecord *to, int mode) { __asm { int 3 } }
IrToken *__stdcall append_symbol_destructor(IrToken *expression, CppSymbol *symbol) { __asm { int 3 } return 0; }
IrToken *__stdcall append_expression_destructor(IrToken *expression, IrToken *object) { __asm { int 3 } return 0; }
FrontendRecord *__stdcall find_label_record(CppSymbol *label) { __asm { int 3 } return 0; }
FrontendRecord *__stdcall skip_declaration_records(FrontendRecord *from) { __asm { int 3 } return 0; }


unsigned __stdcall scan_return_paths(FrontendRecord **cursor, unsigned char stop_kind) { __asm { int 3 } return 0; }



void __stdcall emit_member_and_base_destructors(CppSymbol *tag) { __asm { int 3 } }
void __stdcall emit_vbase_destructor_epilogue(CppSymbol *tag) { __asm { int 3 } }

void finish_inline_function(void) { __asm { int 3 } }

void __stdcall emit_function_symbols(CppSymbol *symbols) { __asm { int 3 } }
int defer_global_symbol(CppSymbol *symbol) { __asm { int 3 } return 0; }

unsigned short __stdcall debug_tag_type(CppSymbol *tag) { __asm { int 3 } return 0; }
void __stdcall reference_debug_type(CType *type) { __asm { int 3 } }
void flush_debug_types(void) { __asm { int 3 } }
void __stdcall prepare_symbol_debug_type(CppSymbol *symbol) { __asm { int 3 } }

char *__stdcall format_qualified_tag_name(char *cursor, CppSymbol *tag) { __asm { int 3 } return 0; }

char *__stdcall mangle_symbol_identifier(char *cursor, CppSymbol *symbol, unsigned encoding) { __asm { int 3 } return 0; }
char *__stdcall mangle_symbol_scope(char *cursor, CppSymbol *symbol) { __asm { int 3 } return 0; }
char *__stdcall mangle_symbol_encoding(char *cursor, CppSymbol *symbol, CppSymbol *context, unsigned encoding) { __asm { int 3 } return 0; }
char *__stdcall mangle_identifier(char *cursor, char *name, int convert_leading_dot) { __asm { int 3 } return 0; }
void __stdcall cache_function_parameter_bytes(CppSymbol *function) { __asm { int 3 } }
int format_integer(unsigned value, char *cursor, unsigned radix) { __asm { int 3 } return 0; }

IrToken *make_error_expression(void) { __asm { int 3 } return 0; }

IrToken *__stdcall materialize_member_object(IrToken *object) { __asm { int 3 } return 0; }


void report_denied_member_access(void) { __asm { int 3 } }

int __stdcall find_friend_access(CppSymbol *candidate, CppSymbol *owner) { __asm { int 3 } return 0; }
QualifierBinding *__stdcall skip_base_qualifier_path(QualifierBinding *binding) { __asm { int 3 } return 0; }
unsigned char __stdcall virtual_base_access_level(CppSymbol *base, CppSymbol *derived) { __asm { int 3 } return 0; }
int __stdcall has_member_access_declaration(CppSymbol *owner, BaseClassLink *base,
    CppSymbol *member) { __asm { int 3 } return 0; }
unsigned char __stdcall combine_member_access(unsigned char member_access,
    unsigned char base_access) { __asm { int 3 } return 0; }
void __stdcall set_inherited_member_access_failure(CppSymbol *member, CppSymbol *base,
    unsigned char previous_access, unsigned char member_access, CppSymbol *previous_base) { __asm { int 3 } }

IrToken *__stdcall adjust_pointer_by_offset(IrToken *object, unsigned displacement, CType *type) { __asm { int 3 } return 0; }
IrToken *__stdcall adjust_virtual_member_object(IrToken *object, QualifierBinding *binding,
    CType *type, unsigned displacement, int pointer_conversion) { __asm { int 3 } return 0; }

int __stdcall has_assignment_member_model(CType *type) { __asm { int 3 } return 0; }
void __stdcall report_conversion_rank_failure(unsigned char operation, unsigned rank,
    CType *target, CType *source) { __asm { int 3 } }

IrToken *__stdcall new_cast_expression(CType *type, IrToken *expression) { __asm { int 3 } return 0; }
int extended_to_integer(const Extended *value, long *result) { __asm { int 3 } return 0; }
int integer_to_extended(const long *value, Extended *result) { __asm { int 3 } return 0; }
int unsigned_to_extended(const unsigned long *value, Extended *result) { __asm { int 3 } return 0; }
int extended_to_double(const Extended *value, double *result) { __asm { int 3 } return 0; }
int double_to_extended(const double *value, Extended *result) { __asm { int 3 } return 0; }
int extended_to_float(const Extended *value, float *result) { __asm { int 3 } return 0; }
int float_to_extended(const float *value, Extended *result) { __asm { int 3 } return 0; }

IrToken *__stdcall strip_function_designator(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall resolve_qualified_assignment_operand(CType *type, QualifiedDeclarator *reference) { __asm { int 3 } return 0; }

unsigned char __stdcall classify_radix_integer(unsigned value) { __asm { int 3 } return 0; }
unsigned char __stdcall classify_long_integer(unsigned value) { __asm { int 3 } return 0; }
unsigned char __stdcall classify_unsigned_integer(unsigned value) { __asm { int 3 } return 0; }
