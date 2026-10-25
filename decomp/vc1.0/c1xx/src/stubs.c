#include "mangling.h"
#include "asm.h"
#include "browser.h"
#include "member-pointers.h"
#include "member-access.h"
#include "segments.h"
#include "builtin-operators.h"
#include "reference-initializers.h"
#include "symbol-output.h"
#include "function-finalization.h"
#include "record-processing.h"
#include "binary-expressions.h"
#include "return-statements.h"
#include "expression-preparation.h"
#include "constructor-initializers.h"
#include "statement-scopes.h"
#include "work-list.h"
#include "loop-output.h"
#include "comments.h"
#include "macro-expansion.h"
#include "cpp-context.h"
#include "expression-records.h"
#include "function-definitions.h"
#include "unary-expressions.h"
#include "control-ir.h"
#include "debug-types.h"
/* Named, unaccepted reconstruction frontiers. */
#include "initialization.h"
#include "inline-vector.h"
#include "class-lookup.h"
#include "virtual-table-output.h"
#include "lookahead.h"
#include "ir.h"
#include "hash-table.h"
#include "types.h"
#include "parameters.h"
#include "macros.h"
#include "global-ir.h"
#include "source-input.h"
#include "operators.h"
#include "parser.h"
#include "lexer.h"
#include "class-members.h"
#include "braced-initializers.h"
#include "class-finalization.h"
#include "class-type-sets.h"
#include "special-members.h"
#include "special-member-lookup.h"
#include "inline-capture.h"
#include "type-sizes.h"
#include "object-initializers.h"
#include "initializer-records.h"
#include "call-expressions.h"
#include "text-format.h"
#include "directives-internal.h"
#include "literals-internal.h"
#include "type-diagnostics-internal.h"
#include "preprocessor-expressions.h"
#include "pragma.h"
#include "diagnostics.h"
#include <recon.h>
void interrupt_compile(int signal_number) { __asm { int 3 } }
void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }
void clear_packing(PackingEntry *entry) { __asm { int 3 } }
void preprocess_translation_unit(void) { __asm { int 3 } }
void begin_automatic_pch(void) { __asm { int 3 } }

void record_pch_string_event(unsigned kind, unsigned value, char *text) { __asm { int 3 } }
int resume_pch_source_input(void) { __asm { int 3 } return 0; }
unsigned __stdcall source_file_timestamp(char *filename) { __asm { int 3 } return 0; }
void __stdcall record_pch_open_include(char *filename) { __asm { int 3 } }
void __stdcall emit_deferred_type_table(void *description, CppSymbol *symbol) { __asm { int 3 } }
void generate_inline_queries(void) { __asm { int 3 } }
void report_error(unsigned code, ...) { __asm { int 3 } }
void report_fatal(unsigned code, ...) { __asm { int 3 } }
void report_syntax_error(unsigned code, ...) { __asm { int 3 } }
void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall report_macro_error(char *directive) { __asm { int 3 } }
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count) { __asm { int 3 } return 0; }
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_symbol_binding(CppSymbol *symbol, CppSymbol **value) { __asm { int 3 } return 0; }
void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
void report_ide_source(void) { __asm { int 3 } }
int release_input_handle(void) { __asm { int 3 } return 0; }
void browser_symbol_rollover(void) { __asm { int 3 } }
void report_declaration_diagnostic(unsigned warning_code, unsigned error_code, ...) { __asm { int 3 } }
void __stdcall warn_ignored_distance(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *get_current_class_symbol(void) { __asm { int 3 } return 0; }
char *__stdcall format_qualified_name(QualifierBinding *binding, int verbose) { __asm { int 3 } return 0; }
void __stdcall initialize_zero_object(CppSymbol *symbol) { __asm { int 3 } }

void __stdcall report_symbol_declaration_error(unsigned code, CppSymbol *symbol) { __asm { int 3 } }
void __stdcall check_declarator_prefix(CppSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }
TypeShape *__stdcall merge_type_shape_prefix(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }
void __stdcall copy_shape_prefix(CppSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }


void __stdcall report_parameter_error(unsigned code, CppSymbol *symbol, unsigned parameter) { __asm { int 3 } }


/* Complete yacc semantic callees, still unaccepted. */

void __stdcall add_qualified_base(unsigned access, int is_virtual, QualifiedDeclarator *name) { __asm { int 3 } }



void __stdcall record_boolean_constant(unsigned value) { __asm { int 3 } }
void __stdcall diagnose_overload_failure(OverloadCandidate *candidate, OverloadSearch *search, IrToken *argument) { __asm { int 3 } }
OverloadCandidate *__stdcall resolve_overload_tie(OverloadSearch *search) { __asm { int 3 } return 0; }
OverloadCandidate *__stdcall report_call_overload_failure(OverloadSearch *search) { __asm { int 3 } return 0; }
IrToken *__stdcall cast_to_reference(CType *type, IrToken *operand) { __asm { int 3 } return 0; }
IrToken *__stdcall adjust_general_member_pointer_object(IrToken *object, IrToken *address, IrToken *adjustment, IrToken *pointer_offset, IrToken *virtual_offset, CType *type) { __asm { int 3 } return 0; }
void __stdcall report_argument_conversion(unsigned code, CppSymbol *function,
    CType *function_type, char *name, unsigned argument, CType *source, CType *target) { __asm { int 3 } }
void __stdcall report_call_argument_error(unsigned code, CppSymbol *function, unsigned index) { __asm { int 3 } }
CppSymbol *__stdcall create_initializer_temporary(unsigned char storage, CType *type) { __asm { int 3 } return 0; }
void __stdcall record_initialized_temporary(CppSymbol *symbol, CType *type) { __asm { int 3 } }
IrToken *__stdcall materialize_call_result(IrToken *call) { __asm { int 3 } return 0; }

void __stdcall record_conditional_cleanup(IrToken *expression) { __asm { int 3 } }



int __stdcall emit_qc_loop_control(IrToken *condition, IrToken *increment, WorkList *analysis) { __asm { int 3 } return 0; }
IrToken *__stdcall strip_address_expression(IrToken *expression) { __asm { int 3 } return 0; }

void __stdcall remove_matching_frontend_record(unsigned char kind) { __asm { int 3 } }

void finish_qc_loop(void) { __asm { int 3 } }
void increment_expression_record_depth(void) { __asm { int 3 } }




IrToken *__stdcall initialize_allocated_scalar(IrToken *object, IrToken *initializer) { __asm { int 3 } return 0; }


unsigned short __stdcall fit_string_initializer(CppSymbol *symbol, IrToken *value) { __asm { int 3 } return 0; }
int __stdcall remaining_string_initializer_bytes(CppSymbol *symbol, IrToken *value) { __asm { int 3 } return 0; }

IrToken *__stdcall member_destructor_expression(unsigned char operation, IrToken *object, char *name) { __asm { int 3 } return 0; }

IrToken *__stdcall new_unary_ir_expression(unsigned char operation, void *value) { __asm { int 3 } return 0; }

IrToken *__stdcall operator_operand_expression(unsigned char operation, IrToken *arguments) { __asm { int 3 } return 0; }


void __stdcall record_qc_jump(unsigned char kind) { __asm { int 3 } }
IrToken *__stdcall report_expression_error(unsigned char operation, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall report_missing_declarator(unsigned code, DeclarationType *type) { __asm { int 3 } }
void __stdcall report_type_conversion(unsigned code, int mode, unsigned char operation, CType *source, CType *destination) { __asm { int 3 } }
void __stdcall report_type_error(unsigned code, CType *type) { __asm { int 3 } }
IrToken *__stdcall segment_offset_expression(IrToken *segment, IrToken *offset) { __asm { int 3 } return 0; }
void skip_balanced_brace_body(void) { __asm { int 3 } }
unsigned char synchronize_tokens(unsigned first, ...) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_bit_field_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_extended_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_integer_bit_field_type(unsigned short flags) { __asm { int 3 } return 0; }
void __stdcall report_inserted_token(int actual, int expected) { __asm { int 3 } }
ParserValue __stdcall recovery_token_value(int token) { ParserValue value; __asm { int 3 } return value; }
void __stdcall report_parser_error(char *text) { __asm { int 3 } }

/* C++ token classification and deferred lookahead. */
unsigned char read_deferred_argument_text(void) { __asm { int 3 } return 0; }
QualifiedDeclarator *__stdcall copy_qualified_declarator(QualifiedDeclarator *source, int arena) { __asm { int 3 } return 0; }







void skip_line_comment(void) { __asm { int 3 } }
void copy_block_comment(void) { __asm { int 3 } }
void report_ide_source_end(void) { __asm { int 3 } }
void chain_initializer_function(void) { __asm { int 3 } }
void __stdcall register_initializer_function(CppSymbol *function) { __asm { int 3 } }
void __stdcall copy_quoted_literal(unsigned char delimiter) { __asm { int 3 } }
void __stdcall macro_expansion_error(unsigned code) { __asm { int 3 } }
long current_source_timestamp(void) { __asm { int 3 } return 0; }

void report_ide_line(void) { __asm { int 3 } }
void finish_pch_capture(void) { __asm { int 3 } }


CType *__stdcall copy_unfinished_type(CType *type) { __asm { int 3 } return 0; }


void __stdcall flush_current_function(int retain) { __asm { int 3 } }


CppSymbol *__stdcall find_tag_declaration(CppSymbol *symbol) { __asm { int 3 } return 0; }






void __stdcall inherit_class_dll_storage(DeclarationType *context, CppSymbol *member) { __asm { int 3 } }









void __stdcall set_deferred_type_name(DeferredToken *token) { __asm { int 3 } }
int __stdcall deferred_scope_allows_expression(int depth) { __asm { int 3 } return 0; }
int recursive_lookahead_parse(void) { __asm { int 3 } return 0; }


int __stdcall has_qualified_this_type(CType *type) { __asm { int 3 } return 0; }














































void __stdcall declare_anonymous_aggregate_object(DeclarationType *type) { __asm { int 3 } }



int __stdcall qualifier_path_is_hidden(QualifierBinding *path, HashTable *hidden) { __asm { int 3 } return 0; }

void discard_function_definition(void) { __asm { int 3 } }
void __stdcall report_declaration_error(unsigned code, CppSymbol *symbol) { __asm { int 3 } }

void __stdcall inherit_function_defaults(CppSymbol *declaration, DefaultArgument *defaults,
    FunctionParameters *previous, FunctionParameters *current, DefaultArgument *following) { __asm { int 3 } }

void __stdcall report_named_type_difference(unsigned code, CType *context, char *name,
    CType *source, CType *destination) { __asm { int 3 } }








CppSymbol *__stdcall recover_undeclared_identifier(char *name) { __asm { int 3 } return 0; }






int __stdcall equal_based_expressions(IrToken *first, IrToken *second) { __asm { int 3 } return 0; }

void __stdcall validate_based_self_source(IrToken *expression) { __asm { int 3 } }

IrToken *__stdcall attach_expression_symbols(IrToken *expression, ExpressionSymbolList *symbols) { __asm { int 3 } return 0; }





int __stdcall report_operator_resolution_failure(unsigned char operation, unsigned char arity,
    unsigned failure, CType *left, CType *right) { __asm { int 3 } return 0; }


void __stdcall type_warning(unsigned level, unsigned code, CType *type) { __asm { int 3 } }
IrToken *__stdcall make_address_temporary(IrToken *expression) { __asm { int 3 } return 0; }
int __stdcall subtraction_overflows(int left, int right, short unsigned_arithmetic, unsigned short bytes) { __asm { int 3 } return 0; }
void __stdcall resolve_untyped_binary_operand(unsigned char operation, IrToken **operand, IrToken *other) { __asm { int 3 } }
void __stdcall report_incompatible_type_shapes(unsigned char operation, CType *target, CType *source) { __asm { int 3 } }
void __stdcall check_based_pointer_conversion(unsigned char operation, TypeShape *target, TypeShape *source) { __asm { int 3 } }
void __stdcall report_invalid_operand(unsigned char operation, int position) { __asm { int 3 } }
IrToken * __stdcall convert_class_condition(IrToken *expression, int mode, unsigned *selection) { __asm { int 3 } return 0; }
IrToken *__stdcall make_assignment_lvalue(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall make_comma_lvalue(IrToken *expression) { __asm { int 3 } return 0; }
IrToken *__stdcall make_conditional_lvalue(IrToken *expression) { __asm { int 3 } return 0; }
int __stdcall find_inline_parameter_slot(unsigned *slots, unsigned count, unsigned symbol_index) { __asm { int 3 } return 0; }




void __stdcall restore_expression_symbols(ExpressionSymbolList *symbols) { __asm { int 3 } }
unsigned __stdcall classify_special_scalar_conversion(CType *target, CType *source, unsigned char flags, OverloadCandidate *context) { __asm { int 3 } return 0; }
int __stdcall has_pointer_or_member_pointer_shape(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall resolve_overloaded_initializer(IrToken *expression, CType *type, int mode) { __asm { int 3 } return 0; }

FrontendRecord *__stdcall skip_declaration_records(FrontendRecord *from) { __asm { int 3 } return 0; }


unsigned __stdcall scan_return_paths(FrontendRecord **cursor, unsigned char stop_kind) { __asm { int 3 } return 0; }




void finish_inline_function(void) { __asm { int 3 } }


void __cdecl prepare_line_record_tag(CppSymbol *tag) { __asm { int 3 } }
unsigned short __cdecl line_record_tag_type(CppSymbol *tag) { __asm { int 3 } return 0; }
unsigned short __cdecl reference_line_record_type(CType *type) { __asm { int 3 } return 0; }



IrToken *make_error_expression(void) { __asm { int 3 } return 0; }



void report_denied_member_access(void) { __asm { int 3 } }

void __stdcall set_inherited_member_access_failure(CppSymbol *member, CppSymbol *base,
    unsigned char previous_access, unsigned char member_access, CppSymbol *previous_base) { __asm { int 3 } }


int __stdcall has_assignment_member_model(CType *type) { __asm { int 3 } return 0; }
void __stdcall report_conversion_rank_failure(unsigned char operation, unsigned rank,
    CType *target, CType *source) { __asm { int 3 } }

int integer_to_extended(const long *value, Extended *result) { __asm { int 3 } return 0; }
int unsigned_to_extended(const unsigned long *value, Extended *result) { __asm { int 3 } return 0; }

IrToken *__stdcall resolve_qualified_assignment_operand(CType *type, QualifiedDeclarator *reference) { __asm { int 3 } return 0; }


char *__stdcall mangle_enum_representation(char *cursor, CType *unused_type) { __asm { int 3 } return 0; }


unsigned __cdecl hash_resolved_declaration(void *key) { __asm { int 3 } return 0; }
unsigned __cdecl hash_word_pair(void *key) { __asm { int 3 } return 0; }

int __cdecl equal_resolved_declarations(void *left, void *right) { __asm { int 3 } return 0; }
int __cdecl equal_word_triplets(void *left, void *right) { __asm { int 3 } return 0; }
int __cdecl equal_word_pairs(void *left, void *right) { __asm { int 3 } return 0; }
int __stdcall vector_contains(Vector *vector, void *item) { __asm { int 3 } return 0; }
int __stdcall can_bind_conversion_reference(CType *reference, CType *value) { __asm { int 3 } return 0; }
IrToken *__stdcall make_once_virtual_base_pointer(IrToken *object, CType *type, unsigned pointer_offset, unsigned table_offset, unsigned member_offset, CType *pointer_type, int check_null) { __asm { int 3 } return 0; }
unsigned char *__stdcall copy_bytes_forward(unsigned char *destination, const unsigned char *source, unsigned bytes) { __asm { int 3 } return 0; }
ExpressionReference *__stdcall clone_expression_references(ExpressionReference *references, int arena) { __asm { int 3 } return 0; }
unsigned char __stdcall compute_virtual_base_access(CppSymbol *base, CppSymbol *derived) { __asm { int 3 } return 0; }
int __stdcall qualified_path_has_virtual_base(QualifiedDeclarator *path) { __asm { int 3 } return 0; }
OverloadCandidate *__stdcall report_operator_overload_failure(OverloadSearch *search) { __asm { int 3 } return 0; }

void capture_directive_text(void) { __asm { int 3 } }
void __stdcall skip_directive_literal(unsigned char delimiter) { __asm { int 3 } }
int __stdcall record_pch_condition(int condition) { __asm { int 3 } return 0; }
int process_pch_directive(void) { __asm { int 3 } return 0; }
void warn_ident_requires_string(void) { __asm { int 3 } }

void __stdcall finish_pascal_literal(ParserValue *literal) { __asm { int 3 } }








unsigned __stdcall initializer_address_size(CppSymbol *symbol) { __asm { int 3 } return 0; }

void write_ir_float(float *value) { __asm { int 3 } }
void write_ir_double(double *value) { __asm { int 3 } }


IrToken *__stdcall retain_reference_temporary(CType *referent, CType *reference, IrToken *expression) { __asm { int 3 } return 0; }
CppSymbol *__stdcall class_tag_for_result_type(CType *type) { __asm { int 3 } return 0; }
int __stdcall temporary_cleanup_is_suppressed(IrToken *object, unsigned index) { __asm { int 3 } return 0; }
int __stdcall equivalent_operand_types(CType *left, CType *right) { __asm { int 3 } return 0; }

IrToken *__stdcall convert_delete_operand_to_pointer(IrToken *expression) { __asm { int 3 } return 0; }


void begin_line_record_debug_info(void) { __asm { int 3 } }




void __stdcall apply_symbol_based_type(CppSymbol *symbol, CppSymbol *base) { __asm { int 3 } }


void __stdcall emit_runtime_bit_field_initializer(IrToken *value) { __asm { int 3 } }
void __stdcall emit_initializer_padding(CppSymbol *symbol) { __asm { int 3 } }
void emit_runtime_zero_initializer(void) { __asm { int 3 } }
unsigned remaining_braced_initializer_bytes(void) { __asm { int 3 } return 0; }
IrToken *__stdcall wrap_default_argument_symbols(IrToken *expression, ExpressionSymbolList *symbols) { __asm { int 3 } return 0; }

void __stdcall queue_debug_type_fixup(CppSymbol *symbol, unsigned char *location, unsigned bytes) { __asm { int 3 } }
unsigned __stdcall encode_debug_signed(unsigned char *destination, int value) { __asm { int 3 } return 0; }

void __stdcall record_typedef_alias(CppSymbol *symbol, CppSymbol *alias) { __asm { int 3 } }

void __stdcall set_default_inheritance_model(int model) { __asm { int 3 } }
CppSymbol * __stdcall enclosing_qualifier_class(QualifierBinding *binding) { __asm { int 3 } return 0; }
void browser_end_macro_expansion(void) { __asm { int 3 } }



char *__stdcall make_local_static_destructor_name(CppSymbol *object, CppSymbol *function) { __asm { int 3 } return 0; }

IrToken *__stdcall convert_compact_to_virtual_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_compact_to_general_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_virtual_to_compact_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_virtual_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_virtual_to_general_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_general_to_compact_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_general_to_virtual_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_general_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_compact_to_virtual_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_compact_to_general_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_virtual_to_compact_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_virtual_data_member_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_virtual_to_general_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_general_to_compact_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_general_to_virtual_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }
IrToken *__stdcall convert_general_data_member_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base) { __asm { int 3 } return 0; }

IrToken *__stdcall adjust_member_pointer_displacement(IrToken *adjustment, int displacement) { __asm { int 3 } return 0; }
void finish_line_record_debug_info(void) { __asm { int 3 } }
void __stdcall fill_browser_generated_symbol(BrowserRecord *record) { __asm { int 3 } }

void suppress_browser_records(void) { __asm { int 3 } }

/* Newly encountered pragma callees; bodies remain unaccepted entry traps. */
void warn_bad_pragma(void) { __asm { int 3 } }
void synchronize_pch_pragma(void) { __asm { int 3 } }
void __stdcall emit_listing_directive(unsigned char mode) { __asm { int 3 } }
void warn_expected_identifier(void) { __asm { int 3 } }
void __stdcall set_default_inheritance_policy(int general) { __asm { int 3 } }
void __stdcall assign_allocated_text(char *name) { __asm { int 3 } }



void browser_compilation_error(void) { __asm { int 3 } }

char *__stdcall member_access_keyword(unsigned access) { __asm { int 3 } return 0; }


void __stdcall emit_runtime_initializer_call(unsigned operation, IrToken *destination, IrToken *source, IrToken *bytes) { __asm { int 3 } }




void __stdcall report_asm_expected_token(int actual, int expected) { __asm { int 3 } }

RECON_RETURN8_TYPE(ParserValue) __stdcall asm_recovery_value(int token) { ParserValue value; __asm { int 3 } value.words.low = value.words.high = 0; RECON_RETURN8(value); }

void __stdcall report_asm_parser_error(char *text) { __asm { int 3 } }



void __stdcall set_asm_directive(short directive, unsigned value) { __asm { int 3 } }







void __stdcall set_asm_address_operator(short operation) { __asm { int 3 } }

char *asm_error_context(void) { __asm { int 3 } return 0; }


AsmExpression *__stdcall evaluate_asm_unary(short operation, AsmExpression *value) { __asm { int 3 } return 0; }





unsigned __stdcall asm_member_offset(char *name) { __asm { int 3 } return 0; }


AsmExpression *__stdcall make_asm_segment_value(AsmExpression *value) { __asm { int 3 } return 0; }











void __stdcall swap_asm_pointer_slots(void **first, void **second) { __asm { int 3 } }

int __stdcall asm_operand_accepts_size(AsmOperand *operand, unsigned char size) { __asm { int 3 } return 0; }
int __stdcall asm_operand_has_type_flags(AsmOperand *operand, unsigned short flags) { __asm { int 3 } return 0; }

void __stdcall report_asm_operand_error(unsigned code, int operand) { __asm { int 3 } }

CppSymbol *__stdcall find_memory_intrinsic(unsigned operation) { __asm { int 3 } return 0; }

int __stdcall uniform_overload_protection(CppSymbol *overloads, QualifierBinding *binding, unsigned char *protection) { __asm { int 3 } return 0; }

char *__stdcall member_protection_keyword(unsigned char protection) { __asm { int 3 } return 0; }

int __stdcall segment_name_has_suffix(char *name, char *suffix) { __asm { int 3 } return 0; }

unsigned __stdcall asm_scale_encoding(unsigned scale) { __asm { int 3 } return 0; }

IrToken *__stdcall build_scalar_reference_temporary(CType *referent, CType *reference, IrToken *expression) { __asm { int 3 } return 0; }
int __stdcall match_pch_string_event(unsigned kind, unsigned value, char *text) { __asm { int 3 } return 0; }
void __stdcall reference_pch_debug_library(char *name) { __asm { int 3 } }
void restore_pch_separate_debug(void) { __asm { int 3 } }
void declare_pch_debug_library(void) { __asm { int 3 } }
unsigned short __stdcall write_pch_events(WorkingStack *stack, FILE *file, int mode) { __asm { int 3 } return 0; }

void __stdcall mark_pch_virtual_base(VirtualBaseEntry *entry) { __asm { int 3 } }
void __stdcall mark_pch_virtual_group(VirtualTableGroup *group) { __asm { int 3 } }
void __stdcall mark_pch_expression_references(ExpressionReference *reference) { __asm { int 3 } }
void __stdcall mark_pch_reference_groups(MemberReferenceGroup *group) { __asm { int 3 } }
struct VirtualThunkKey;
void __stdcall mark_pch_virtual_thunk_key(struct VirtualThunkKey *key) { __asm { int 3 } }
void __stdcall mark_pch_declaration_thunk_key(MemberPointerThunkKey *key) { __asm { int 3 } }
void __stdcall mark_pch_deferred_type_key(void *description) { __asm { int 3 } }

void __stdcall mark_pch_symbol_binding(void *binding) { __asm { int 3 } }
void __stdcall mark_pch_access_key(AccessDeclaration *access) { __asm { int 3 } }
