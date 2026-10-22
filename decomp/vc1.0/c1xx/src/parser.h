#ifndef PARSER_H
#define PARSER_H
#include "parser-state.h"
#include "parser-grammar.h"
#include "keywords.h"
#include "cpp-context.h"
#include "parameters.h"
#include "ir.h"

/* Expression operators reuse the lexer node's common twenty-byte prefix. */
typedef struct IrExpression {
    unsigned char kind, flags;
    unsigned short attributes;
    CType *type;
    unsigned field08;
    IrToken *left, *right;
} IrExpression;
#define EXPR(node_) ((IrExpression *)(node_))
typedef struct ClassContext {
    CppSymbol *symbol;
    CppSymbol *last_member;
    CType *type;
    unsigned field0c, field10, size;
    unsigned short bit_field_type, alignment;
    unsigned char bit_field_bits, flags;
    unsigned short field1e;
} ClassContext;
typedef struct LabelStack {
    struct LabelStack *next;
    IrToken *label;
} LabelStack;
extern ClassContext class_contexts[16];
extern unsigned char expression_storage[2052];
extern unsigned char *text_work_cursor;
extern int initializer_list_started;
extern int declarator_nesting;
extern DeclarationType declaration_spec_storage[61];
#define declaration_spec_stack (declaration_spec_storage + 1)
extern unsigned enumerator_value, array_bound, new_array_bound;
extern CType *enumerator_context;
extern void *based_symbol;
extern unsigned char member_lookup_operator;
extern IrToken *member_lookup_context;
extern LabelStack *control_labels, *break_labels, *continue_labels;
extern LabelStack *switch_labels, *default_labels;
extern int label_stack_count;
extern IrToken *default_label;
extern unsigned current_qc_context;
extern unsigned browser_symbol_limit;
typedef struct DeclarationCode {
    unsigned token, value;
} DeclarationCode;
extern DeclarationCode declaration_codes[];
extern unsigned declaration_code_count;

unsigned char read_parser_token(void);
unsigned char scan_token(void);
void __stdcall report_inserted_token(int actual, int expected);
ParserValue __stdcall recovery_token_value(int token);
void __stdcall report_parser_error(char *text);
void __stdcall unget_token(unsigned char token);
unsigned char synchronize_tokens(unsigned first, ...);
void __stdcall finish_external_definition(int recovering);
void skip_balanced_brace_body(void);
void __stdcall declare_nctag(char *name);
DeclarationType *__stdcall current_declaration_type(DeclarationType *explicit_type);
void __stdcall check_constructor_storage(StorageSpec storage, char *class_name);
void __stdcall begin_function_definition(DeclarationType *type, CppSymbol *declaration);
void __stdcall apply_constructor_initializers(IrToken *initializers);
void end_function_definition(void);
IrToken *__stdcall append_constructor_initializer(IrToken *list, IrToken *initializer);
IrToken *__stdcall symbol_reference(CppSymbol *symbol);
IrToken *__stdcall new_ir_expression(unsigned char operation, IrToken *left, IrToken *right, CType *type);
IrToken *__stdcall append_expression(IrToken *list, IrToken *expression, int mode);
void pop_linkage(void);
void __stdcall check_class_specifier(DeclarationType *type);
void __stdcall check_empty_declaration(DeclarationType *type);
unsigned __stdcall parse_linkage(DeclarationType *type, IrToken *language);
unsigned char __stdcall read_type_specifier(unsigned char token);
CType *end_class_specifier(void);
void __stdcall flush_scanned_declaration(DeclarationType *type);
CType *end_enum_specifier(void);
void begin_class_body(void);
void __stdcall add_named_base(unsigned char access, int is_virtual, char *name);
void __stdcall add_qualified_base(unsigned char access, int is_virtual, QualifiedDeclarator *name);
unsigned __stdcall next_enumerator_value(unsigned value, char *unused_name);
void __stdcall declare_enumerator(char *name, CType *context, unsigned value);
void __stdcall declare_class_member(DeclarationType *type, CppSymbol *member);
void __stdcall set_member_access(unsigned char access);
IrToken *__stdcall validate_value_expression(IrToken *expression);
void __stdcall validate_pure_specifier(IrToken *expression);
CppSymbol *__stdcall new_abstract_declarator(TypeShape *shape);
CppSymbol *__stdcall declare_constructor(DeclarationType *type, CppSymbol *class_symbol, CppSymbol *parameters);
CppSymbol *__stdcall complete_declaration(DeclarationType *type, CppSymbol *declaration);
int __stdcall reject_function_initializer(CppSymbol *declaration);
CppSymbol *__stdcall begin_object_initializer(CppSymbol *declaration);
void __stdcall mark_initialized_symbol(CppSymbol *symbol);
void __stdcall initialize_object(CppSymbol *symbol, IrToken *value);
void __stdcall finish_object_initializer(CppSymbol *symbol);
void __stdcall begin_braced_initializer(CppSymbol *symbol);
void __stdcall end_braced_initializer(CppSymbol *symbol);
void __stdcall report_missing_declarator(unsigned code, DeclarationType *type);
void __stdcall initialize_element(IrToken *value, int mode);
void begin_nested_initializer(void);
void end_nested_initializer(void);
void __stdcall resolve_member_pointer_class(CppSymbol **result, QualifiedDeclarator *name);
CppSymbol *__stdcall apply_declarator_modifier(CppSymbol *declaration, TypeShape *modifier);
char *current_class_name(void);
char *__stdcall encode_type_name(CType *type);
QualifierResolution *__stdcall lookup_qualified_member(QualifierResolution *scope, char *name, CType *type);
char *make_anonymous_identifier(void);
void __stdcall apply_conversion_declarator(DeclarationType *type, TypeShape *shape);
CppSymbol *make_ellipsis_parameter(void);
void __stdcall begin_parameter_scope(CppSymbol *first);
CppSymbol *__stdcall append_parameter_declaration(CppSymbol *list, CppSymbol *declaration);
int __stdcall can_initialize(CType *type, IrToken *expression);
void __stdcall report_type_conversion(unsigned code, int mode, unsigned char operation, CType *source, CType *destination);
void __stdcall check_parameter_default(IrToken *expression, CType *type);
IrToken *__stdcall convert_initializer(unsigned char operation, CType *type, IrToken *expression);
IrToken *__stdcall prepare_reference_initializer(CType *type, IrToken *expression);
DefaultArgument *__stdcall save_default_argument(IrToken *expression);
void increment_expression_record_depth(void);
void mark_evaluation_boundary(void);
CppSymbol *__stdcall lookup_based_symbol(char *name);
IrToken *validate_based_self(void);
unsigned short __stdcall validate_bit_field_bound(unsigned short flags, char *name);
unsigned short __stdcall validate_extended_bound(unsigned short flags, char *name);
unsigned short __stdcall validate_integer_bit_field_type(unsigned short flags);
void __stdcall add_array_declarator(CppSymbol *declaration, IrToken *bound);

void begin_compound_statement(void);
void end_compound_statement(void);
IrToken *__stdcall prepare_expression(IrToken *expression, int mode);
void __stdcall emit_expression_statement(IrToken *expression);
void __stdcall finish_implicit_statement_scope(IrToken *label, unsigned opened);
void __stdcall emit_label(IrToken *label);
void __stdcall end_if_statement(int mode);
void end_else_statement(void);
void begin_while_condition(void);
void __stdcall begin_qc_control(unsigned char mode);
void end_qc_control(void);
IrToken *__stdcall boolean_expression(IrToken *expression, int mode);
void __stdcall emit_conditional_branch(IrToken *first, IrToken *condition, IrToken *increment);
void begin_while_body(void);
unsigned __stdcall begin_implicit_statement_scope(IrToken *label);
void end_while_statement(void);
void finish_conditional_branch(void);
void begin_for_condition(void);
void begin_for_increment(void);
void begin_for_body(void);
void end_for_statement(void);
void begin_do_statement(void);
void prepare_do_body(void);
void begin_do_condition(void);
void __stdcall emit_condition_to_label(IrToken *condition, IrToken *label, int sense);
void end_do_statement(void);
void begin_switch_condition(void);
void __stdcall begin_switch_body(IrToken *expression);
void write_switch_dispatch(void);
void end_switch_statement(void);
void emit_seh_leave(void);
void record_break_statement(void);
void __stdcall record_qc_jump(unsigned char kind);
void record_continue_statement(void);
void begin_return_expression(void);
void __stdcall emit_return_statement(IrToken *expression, int mode);
CppSymbol *__stdcall find_label(char *name);
CppSymbol *__stdcall new_label(char *name);
void __stdcall register_label(CppSymbol *label);
void __stdcall record_goto(CppSymbol *label, int known);
void emit_inline_assembly(void);
void end_seh_except(void);
void begin_seh_finally(void);
void end_seh_finally(void);
void begin_seh_try(void);
void end_seh_try(void);
void begin_seh_filter(void);
void __stdcall emit_seh_filter(IrToken *expression);
void end_seh_filter(void);
void __stdcall define_named_label(char *name);
void __stdcall add_case_label(IrToken *expression);
IrToken *__stdcall new_branch_label(unsigned attributes);
void record_default_label(void);
void begin_if_condition(void);
void begin_if_body(void);
void begin_else_body(void);

unsigned __stdcall evaluate_integer_constant(IrToken *expression);
IrToken *__stdcall binary_expression(unsigned char operation, IrToken *left, IrToken *right);
IrToken *__stdcall resolve_assignment_operand(CType *type, IrToken *expression);
IrToken *__stdcall report_expression_error(unsigned char operation, IrToken *expression);
IrToken *__stdcall conditional_expression(IrToken *condition, IrToken *if_true, IrToken *if_false);
IrToken *__stdcall segment_offset_expression(IrToken *segment, IrToken *offset);
IrToken *__stdcall cast_expression(CppSymbol *type, IrToken *expression);
IrToken *__stdcall unary_expression(unsigned char operation, IrToken *expression);
IrToken *__stdcall member_pointer_expression(QualifiedDeclarator *qualifier);
void __stdcall clear_reference_cache(IrToken *expression);
IrToken *__stdcall sizeof_expression(IrToken *expression);
IrToken *__stdcall sizeof_type(CType *type);
IrToken *__stdcall build_call_expression(IrToken *expression, IrToken *arguments);
IrToken *__stdcall finish_call_expression(IrToken *expression, unsigned short flags);
IrToken *__stdcall begin_member_lookup(unsigned char operation, IrToken *object, int mode);
IrToken *__stdcall operator_operand_expression(unsigned char operation, IrToken *arguments);
void __stdcall record_string_literal(IrToken *literal);
IrToken *__stdcall builtin_unary_expression(unsigned char operation, IrToken *expression);
IrToken *__stdcall new_unary_ir_expression(unsigned char operation, void *value);
IrToken *__stdcall member_expression(unsigned char operation, IrToken *object, IrToken *member);
IrToken *__stdcall member_destructor_expression(unsigned char operation, IrToken *object, char *name);
IrToken *__stdcall identifier_expression(char *name);
IrToken *__stdcall symbol_expression(CppSymbol *symbol);
CppSymbol *__stdcall recover_undeclared_identifier(char *name);
IrToken *most_derived_expression(void);
typedef struct SegmentSymbol SegmentSymbol;
SegmentSymbol *__stdcall lookup_segment_symbol(char *name, char *class_name, unsigned char kind, unsigned unused_mode);
void __stdcall concatenate_strings(ParserValue *destination, ParserValue *source);
IrToken *__stdcall new_expression(unsigned char operation, TypeShape *modifier, IrToken *placement,
    CppSymbol *type, IrToken *initializer, unsigned bound);
void __stdcall detach_new_array_bound(CppSymbol *type);
int __stdcall has_cv_qualifiers(CppSymbol *symbol);
void __stdcall report_type_error(unsigned code, CType *type);
IrToken *__stdcall delete_expression(unsigned char operation, IrToken *expression, IrToken *bound, int is_array);

typedef char IrExpressionSizeMustBe20[(sizeof(IrExpression) == 20) ? 1 : -1];
typedef char ClassContextSizeMustBe32[(sizeof(ClassContext) == 32) ? 1 : -1];
#endif
