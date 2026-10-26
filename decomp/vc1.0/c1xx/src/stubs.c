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
#include "debug-info.h"
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
#include "symbol-bindings.h"
#include <recon.h>
void interrupt_compile(int signal_number) { __asm { int 3 } }
void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }
void clear_packing(PackingEntry *entry) { __asm { int 3 } }

void __stdcall emit_deferred_type_table(void *description, CppSymbol *symbol) { __asm { int 3 } }
void report_error(unsigned code, ...) { __asm { int 3 } }
void report_fatal(unsigned code, ...) { __asm { int 3 } }
void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall report_macro_error(char *directive) { __asm { int 3 } }
void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
void report_ide_source(void) { __asm { int 3 } }
int release_input_handle(void) { __asm { int 3 } return 0; }
void browser_symbol_rollover(void) { __asm { int 3 } }
void __stdcall warn_ignored_distance(CppSymbol *symbol) { __asm { int 3 } }
void __stdcall initialize_zero_object(CppSymbol *symbol) { __asm { int 3 } }

void __stdcall check_declarator_prefix(CppSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }
TypeShape *__stdcall merge_type_shape_prefix(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }
void __stdcall copy_shape_prefix(CppSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }


void __stdcall report_parameter_error(unsigned code, CppSymbol *symbol, unsigned parameter) { __asm { int 3 } }


/* Complete yacc semantic callees, still unaccepted. */

void __stdcall add_qualified_base(unsigned access, int is_virtual, QualifiedDeclarator *name) { __asm { int 3 } }



void __stdcall record_boolean_constant(unsigned value) { __asm { int 3 } }
IrToken *__stdcall adjust_general_member_pointer_object(IrToken *object, IrToken *address, IrToken *adjustment, IrToken *pointer_offset, IrToken *virtual_offset, CType *type) { __asm { int 3 } return 0; }
void __stdcall report_call_argument_error(unsigned code, CppSymbol *function, unsigned index) { __asm { int 3 } }




IrToken *__stdcall strip_address_expression(IrToken *expression) { __asm { int 3 } return 0; }

void __stdcall remove_matching_frontend_record(unsigned char kind) { __asm { int 3 } }









IrToken *__stdcall new_unary_ir_expression(unsigned char operation, void *value) { __asm { int 3 } return 0; }

IrToken *__stdcall operator_operand_expression(unsigned char operation, IrToken *arguments) { __asm { int 3 } return 0; }


IrToken *__stdcall report_expression_error(unsigned char operation, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall report_missing_declarator(unsigned code, DeclarationType *type) { __asm { int 3 } }
IrToken *__stdcall segment_offset_expression(IrToken *segment, IrToken *offset) { __asm { int 3 } return 0; }
void skip_balanced_brace_body(void) { __asm { int 3 } }
unsigned short __stdcall validate_bit_field_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_extended_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_integer_bit_field_type(unsigned short flags) { __asm { int 3 } return 0; }

/* C++ token classification and deferred lookahead. */







void report_ide_source_end(void) { __asm { int 3 } }
void chain_initializer_function(void) { __asm { int 3 } }
void __stdcall register_initializer_function(CppSymbol *function) { __asm { int 3 } }
void __stdcall macro_expansion_error(unsigned code) { __asm { int 3 } }
long current_source_timestamp(void) { __asm { int 3 } return 0; }

void report_ide_line(void) { __asm { int 3 } }


CType *__stdcall copy_unfinished_type(CType *type) { __asm { int 3 } return 0; }


void __stdcall flush_current_function(int retain) { __asm { int 3 } }


CppSymbol *__stdcall find_tag_declaration(CppSymbol *symbol) { __asm { int 3 } return 0; }















int __stdcall deferred_scope_allows_expression(int depth) { __asm { int 3 } return 0; }
int recursive_lookahead_parse(void) { __asm { int 3 } return 0; }



















































int __stdcall qualifier_path_is_hidden(QualifierBinding *path, HashTable *hidden) { __asm { int 3 } return 0; }

void discard_function_definition(void) { __asm { int 3 } }
void __stdcall report_declaration_error(unsigned code, CppSymbol *symbol) { __asm { int 3 } }

void __stdcall inherit_function_defaults(CppSymbol *declaration, DefaultArgument *defaults,
    FunctionParameters *previous, FunctionParameters *current, DefaultArgument *following) { __asm { int 3 } }















int __stdcall equal_based_expressions(IrToken *first, IrToken *second) { __asm { int 3 } return 0; }

void __stdcall validate_based_self_source(IrToken *expression) { __asm { int 3 } }

IrToken *__stdcall attach_expression_symbols(IrToken *expression, ExpressionSymbolList *symbols) { __asm { int 3 } return 0; }







void __stdcall type_warning(unsigned level, unsigned code, CType *type) { __asm { int 3 } }
IrToken *__stdcall make_address_temporary(IrToken *expression) { __asm { int 3 } return 0; }
int __stdcall subtraction_overflows(int left, int right, short unsigned_arithmetic, unsigned short bytes) { __asm { int 3 } return 0; }
void __stdcall resolve_untyped_binary_operand(unsigned char operation, IrToken **operand, IrToken *other) { __asm { int 3 } }
void __stdcall report_incompatible_type_shapes(unsigned char operation, CType *target, CType *source) { __asm { int 3 } }
void __stdcall report_invalid_operand(unsigned char operation, int position) { __asm { int 3 } }
IrToken * __stdcall convert_class_condition(IrToken *expression, int mode, unsigned *selection) { __asm { int 3 } return 0; }




void __stdcall restore_expression_symbols(ExpressionSymbolList *symbols) { __asm { int 3 } }
unsigned __stdcall classify_special_scalar_conversion(CType *target, CType *source, unsigned char flags, OverloadCandidate *context) { __asm { int 3 } return 0; }
int __stdcall has_pointer_or_member_pointer_shape(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall resolve_overloaded_initializer(IrToken *expression, CType *type, int mode) { __asm { int 3 } return 0; }












IrToken *make_error_expression(void) { __asm { int 3 } return 0; }




void __stdcall set_inherited_member_access_failure(CppSymbol *member, CppSymbol *base,
    unsigned char previous_access, unsigned char member_access, CppSymbol *previous_base) { __asm { int 3 } }


int __stdcall has_assignment_member_model(CType *type) { __asm { int 3 } return 0; }

int integer_to_extended(const long *value, Extended *result) { __asm { int 3 } return 0; }
int unsigned_to_extended(const unsigned long *value, Extended *result) { __asm { int 3 } return 0; }





CppSymbol *__stdcall last_overload_declaration(OverloadSet *overloads) { __asm { int 3 } return 0; }
unsigned __cdecl hash_word_pair(void *key) { __asm { int 3 } return 0; }

int __cdecl equal_resolved_declarations(void *left, void *right) { __asm { int 3 } return 0; }
int __cdecl equal_word_triplets(void *left, void *right) { __asm { int 3 } return 0; }
int __cdecl equal_word_pairs(void *left, void *right) { __asm { int 3 } return 0; }
int __stdcall vector_contains(Vector *vector, void *item) { __asm { int 3 } return 0; }
int __stdcall can_bind_conversion_reference(CType *reference, CType *value) { __asm { int 3 } return 0; }
IrToken *__stdcall make_once_virtual_base_pointer(IrToken *object, CType *type, unsigned pointer_offset, unsigned table_offset, unsigned member_offset, CType *pointer_type, int check_null) { __asm { int 3 } return 0; }
ExpressionReference *__stdcall clone_expression_references(ExpressionReference *references, int arena) { __asm { int 3 } return 0; }
unsigned char __stdcall compute_virtual_base_access(CppSymbol *base, CppSymbol *derived) { __asm { int 3 } return 0; }
int __stdcall qualified_path_has_virtual_base(QualifiedDeclarator *path) { __asm { int 3 } return 0; }
OverloadCandidate *__stdcall report_operator_overload_failure(OverloadSearch *search) { __asm { int 3 } return 0; }

void capture_directive_text(void) { __asm { int 3 } }
int __stdcall record_pch_condition(int condition) { __asm { int 3 } return 0; }
int process_pch_directive(void) { __asm { int 3 } return 0; }

void __stdcall finish_pascal_literal(ParserValue *literal) { __asm { int 3 } }








unsigned __stdcall initializer_address_size(CppSymbol *symbol) { __asm { int 3 } return 0; }

void write_ir_float(float *value) { __asm { int 3 } }
void write_ir_double(double *value) { __asm { int 3 } }


int __stdcall temporary_cleanup_is_suppressed(IrToken *object, unsigned index) { __asm { int 3 } return 0; }
int __stdcall equivalent_operand_types(CType *left, CType *right) { __asm { int 3 } return 0; }







void __stdcall apply_symbol_based_type(CppSymbol *symbol, CppSymbol *base) { __asm { int 3 } }


IrToken *__stdcall wrap_default_argument_symbols(IrToken *expression, ExpressionSymbolList *symbols) { __asm { int 3 } return 0; }

void __stdcall queue_debug_type_fixup(CppSymbol *symbol, unsigned char *location, unsigned bytes) { __asm { int 3 } }
unsigned __stdcall encode_debug_signed(unsigned char *destination, int value) { __asm { int 3 } return 0; }


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


/* Newly encountered pragma callees; bodies remain unaccepted entry traps. */
void synchronize_pch_pragma(void) { __asm { int 3 } }
void __stdcall emit_listing_directive(unsigned char mode) { __asm { int 3 } }
void warn_expected_identifier(void) { __asm { int 3 } }
void __stdcall set_default_inheritance_policy(int general) { __asm { int 3 } }
void __stdcall assign_allocated_text(char *name) { __asm { int 3 } }




char *__stdcall member_access_keyword(unsigned access) { __asm { int 3 } return 0; }






void __stdcall report_asm_expected_token(int actual, int expected) { __asm { int 3 } }

RECON_RETURN8_TYPE(ParserValue) __stdcall asm_recovery_value(int token) { ParserValue value; __asm { int 3 } value.words.low = value.words.high = 0; RECON_RETURN8(value); }











void __stdcall set_asm_address_operator(short operation) { __asm { int 3 } }








CppSymbol *__stdcall find_asm_scope_member(char *name) { __asm { int 3 } return 0; }















void __stdcall report_asm_operand_error(unsigned code, int operand) { __asm { int 3 } }

CppSymbol *__stdcall find_memory_intrinsic(unsigned operation) { __asm { int 3 } return 0; }

int __stdcall uniform_overload_protection(CppSymbol *overloads, QualifierBinding *binding, unsigned char *protection) { __asm { int 3 } return 0; }


int __stdcall segment_name_has_suffix(char *name, char *suffix) { __asm { int 3 } return 0; }


void __stdcall reference_pch_debug_library(char *name) { __asm { int 3 } }

struct VirtualThunkKey;
void __stdcall mark_pch_virtual_thunk_key(struct VirtualThunkKey *key) { __asm { int 3 } }
void __stdcall mark_pch_declaration_thunk_key(MemberPointerThunkKey *key) { __asm { int 3 } }
void __stdcall mark_pch_deferred_type_key(void *description) { __asm { int 3 } }

void __stdcall mark_pch_symbol_binding(void *binding) { __asm { int 3 } }
void __stdcall mark_pch_access_key(AccessDeclaration *access) { __asm { int 3 } }
