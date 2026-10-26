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

void report_error(unsigned code, ...) { __asm { int 3 } }
void report_fatal(unsigned code, ...) { __asm { int 3 } }
void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall report_macro_error(char *directive) { __asm { int 3 } }
void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
void report_ide_source(void) { __asm { int 3 } }
int release_input_handle(void) { __asm { int 3 } return 0; }
void browser_symbol_rollover(void) { __asm { int 3 } }
void __stdcall warn_ignored_distance(CppSymbol *symbol) { __asm { int 3 } }

void __stdcall check_declarator_prefix(CppSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }
TypeShape *__stdcall merge_type_shape_prefix(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }
void __stdcall copy_shape_prefix(CppSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }




/* Complete yacc semantic callees, still unaccepted. */

void __stdcall add_qualified_base(unsigned access, int is_virtual, QualifiedDeclarator *name) { __asm { int 3 } }



void __stdcall record_boolean_constant(unsigned value) { __asm { int 3 } }





void __stdcall remove_matching_frontend_record(unsigned char kind) { __asm { int 3 } }









IrToken *__stdcall new_unary_ir_expression(unsigned char operation, void *value) { __asm { int 3 } return 0; }

IrToken *__stdcall operator_operand_expression(unsigned char operation, IrToken *arguments) { __asm { int 3 } return 0; }


IrToken *__stdcall report_expression_error(unsigned char operation, IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall report_missing_declarator(unsigned code, DeclarationType *type) { __asm { int 3 } }
IrToken *__stdcall segment_offset_expression(IrToken *segment, IrToken *offset) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_bit_field_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_extended_bound(unsigned short flags, char *name) { __asm { int 3 } return 0; }
unsigned short __stdcall validate_integer_bit_field_type(unsigned short flags) { __asm { int 3 } return 0; }

/* C++ token classification and deferred lookahead. */







void report_ide_source_end(void) { __asm { int 3 } }
void __stdcall macro_expansion_error(unsigned code) { __asm { int 3 } }
long current_source_timestamp(void) { __asm { int 3 } return 0; }

void report_ide_line(void) { __asm { int 3 } }






CppSymbol *__stdcall find_tag_declaration(CppSymbol *symbol) { __asm { int 3 } return 0; }















int recursive_lookahead_parse(void) { __asm { int 3 } return 0; }




















































void discard_function_definition(void) { __asm { int 3 } }
















int __stdcall equal_based_expressions(IrToken *first, IrToken *second) { __asm { int 3 } return 0; }

void __stdcall validate_based_self_source(IrToken *expression) { __asm { int 3 } }








IrToken *__stdcall make_address_temporary(IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall resolve_untyped_binary_operand(unsigned char operation, IrToken **operand, IrToken *other) { __asm { int 3 } }
void __stdcall report_incompatible_type_shapes(unsigned char operation, CType *target, CType *source) { __asm { int 3 } }
void __stdcall report_invalid_operand(unsigned char operation, int position) { __asm { int 3 } }




unsigned __stdcall classify_special_scalar_conversion(CType *target, CType *source, unsigned char flags, OverloadCandidate *context) { __asm { int 3 } return 0; }
int __stdcall has_pointer_or_member_pointer_shape(CType *type) { __asm { int 3 } return 0; }












IrToken *make_error_expression(void) { __asm { int 3 } return 0; }






int __stdcall has_assignment_member_model(CType *type) { __asm { int 3 } return 0; }






CppSymbol *__stdcall last_overload_declaration(OverloadSet *overloads) { __asm { int 3 } return 0; }

int __cdecl equal_resolved_declarations(void *left, void *right) { __asm { int 3 } return 0; }
int __stdcall vector_contains(Vector *vector, void *item) { __asm { int 3 } return 0; }
IrToken *__stdcall make_once_virtual_base_pointer(IrToken *object, CType *type, unsigned pointer_offset, unsigned table_offset, unsigned member_offset, CType *pointer_type, int check_null) { __asm { int 3 } return 0; }
unsigned char __stdcall compute_virtual_base_access(CppSymbol *base, CppSymbol *derived) { __asm { int 3 } return 0; }
int __stdcall qualified_path_has_virtual_base(QualifiedDeclarator *path) { __asm { int 3 } return 0; }


void __stdcall finish_pascal_literal(ParserValue *literal) { __asm { int 3 } }








unsigned __stdcall initializer_address_size(CppSymbol *symbol) { __asm { int 3 } return 0; }

void write_ir_float(float *value) { __asm { int 3 } }
void write_ir_double(double *value) { __asm { int 3 } }


int __stdcall temporary_cleanup_is_suppressed(IrToken *object, unsigned index) { __asm { int 3 } return 0; }










void __stdcall queue_debug_type_fixup(CppSymbol *symbol, unsigned char *location, unsigned bytes) { __asm { int 3 } }


CppSymbol * __stdcall enclosing_qualifier_class(QualifierBinding *binding) { __asm { int 3 } return 0; }
void browser_end_macro_expansion(void) { __asm { int 3 } }



char *__stdcall make_local_static_destructor_name(CppSymbol *object, CppSymbol *function) { __asm { int 3 } return 0; }




/* Newly encountered pragma callees; bodies remain unaccepted entry traps. */
void synchronize_pch_pragma(void) { __asm { int 3 } }
void warn_expected_identifier(void) { __asm { int 3 } }




char *__stdcall member_access_keyword(unsigned access) { __asm { int 3 } return 0; }






void __stdcall report_asm_expected_token(int actual, int expected) { __asm { int 3 } }

RECON_RETURN8_TYPE(ParserValue) __stdcall asm_recovery_value(int token) { ParserValue value; __asm { int 3 } value.words.low = value.words.high = 0; RECON_RETURN8(value); }











void __stdcall set_asm_address_operator(short operation) { __asm { int 3 } }























void __stdcall report_asm_operand_error(unsigned code, int operand) { __asm { int 3 } }

CppSymbol *__stdcall find_memory_intrinsic(unsigned operation) { __asm { int 3 } return 0; }





void __stdcall reference_pch_debug_library(char *name) { __asm { int 3 } }

struct VirtualThunkKey;
void __stdcall mark_pch_virtual_thunk_key(struct VirtualThunkKey *key) { __asm { int 3 } }
void __stdcall mark_pch_declaration_thunk_key(MemberPointerThunkKey *key) { __asm { int 3 } }
void __stdcall mark_pch_deferred_type_key(void *description) { __asm { int 3 } }

void __stdcall mark_pch_symbol_binding(void *binding) { __asm { int 3 } }
