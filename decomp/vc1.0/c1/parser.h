#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include "builtins.h"

/* Declaration specifiers occupy one yacc semantic value. The three storage
   bytes are shared with the symbol table's declaration representation. */
typedef struct DeclarationSpec {
    unsigned char storage[3], reserved;
    CType *type;
} DeclarationSpec;
typedef union ParserValue {
    TokenValue token;
    DeclarationSpec declaration;
    struct { FrontendSymbol *symbol; CType *type; } type_name;
    struct { unsigned low, high; } words;
    unsigned char bytes[8];
    unsigned short modifier;
    char *name;
    FrontendSymbol *symbol;
    IrToken *node;
    TypeShape *shape;
} ParserValue;

/* The grammar keeps explicit logical transitions here. These structures do
   not reproduce the original yacc table compression or image layout. */
typedef struct ParserTransition {
    unsigned short symbol;
    short destination;
} ParserTransition;
typedef struct ParserState {
    short default_rule;
    unsigned char needs_token;
    unsigned short first_shift, shift_count;
    unsigned short first_exception, exception_count;
} ParserState;
typedef struct ParserGoto {
    short default_state;
    unsigned short first_transition, transition_count;
} ParserGoto;
typedef struct ParserProduction {
    unsigned char nonterminal, length;
} ParserProduction;
typedef struct ParserRecovery {
    short state, token, next_state;
} ParserRecovery;
#define PARSER_ACCEPT (-1)
#define PARSER_ERROR_TOKEN 256
extern ParserTransition parser_shifts[];
extern ParserTransition parser_exceptions[];
extern ParserState parser_states[376];
extern ParserGoto parser_gotos[92];
extern ParserTransition parser_goto_transitions[];
extern ParserProduction parser_productions[225];
extern ParserRecovery parser_recoveries[];
extern ParserValue parser_value;
extern int parser_lookahead, parser_error_count;
extern short parser_recovery_count;
extern DeclarationSpec *declaration_spec_stack;
extern int declaration_depth, unevaluated_expression_depth;
extern unsigned char declaration_spec_flags[60];
extern unsigned char parameter_nesting;
FrontendSymbol *__stdcall find_identifier_symbol(char *name);
void __stdcall browser_symbol_reference(FrontendSymbol *symbol);
extern TypeShape *declaration_shapes[60];
unsigned char __stdcall lookup_identifier_token(FrontendSymbol *explicit_symbol);

/* Semantic routines retain their individual original entry points. */
unsigned char read_parser_token(void);
CType *__stdcall merge_type_specifier(CType *type, unsigned short *flags, unsigned short added);
unsigned char __stdcall read_tag_specifier(unsigned short kind, unsigned short *attributes);
CType *__stdcall merge_named_type(FrontendSymbol *symbol, unsigned short *flags, unsigned short *attributes);
void __stdcall merge_storage_class(unsigned char *storage, unsigned class, unsigned modifier, int replace);
void parse_declspec(unsigned char *storage);
FrontendSymbol *__stdcall finish_declarator(FrontendSymbol *declarator);
IrToken *__stdcall make_constant_token(unsigned char token, void *value);
unsigned char __stdcall resolve_identifier_token(unsigned char token);
void __stdcall report_parser_error(char *text);
void __stdcall report_expected_token(unsigned char actual, int expected);
ParserValue __stdcall recovery_token_value(int token);
unsigned char recover_parser_token(unsigned char token, ...);
void skip_declaration_tokens(void);
void __stdcall finish_external_declaration(int recovering);
void begin_function_definition(void);
void abort_function_definition(void);
CType *__stdcall function_result_type(FrontendSymbol *symbol);
void __stdcall report_symbol_error(unsigned code, FrontendSymbol *symbol);
int __stdcall is_record_type(CType *type);
FrontendSymbol *__stdcall find_scope_declaration(SymbolScope *scope, char *name, int case_fold);
int __stdcall declaration_conflict(FrontendSymbol *existing, FrontendSymbol *symbol);
void __stdcall commit_redeclaration(FrontendSymbol *existing, FrontendSymbol *symbol);
FrontendSymbol *__stdcall reconcile_declarations(FrontendSymbol *existing, FrontendSymbol *symbol);
FrontendSymbol *__stdcall merge_scoped_declaration(FrontendSymbol *symbol, SymbolScope *scope);
FrontendSymbol *__stdcall merge_global_declaration(FrontendSymbol *symbol);
FrontendSymbol *__stdcall merge_local_declaration(FrontendSymbol *symbol);
void __stdcall record_local_declaration(FrontendSymbol *symbol);
extern FrontendSymbol *function_symbol;
extern int function_parameter_context;
void __stdcall replace_function_parameters(FrontendSymbol *symbol, ParameterInfo *parameters);
FrontendSymbol *__stdcall initialize_function_definition(FrontendSymbol *symbol);
void browser_begin_function_scope(void);
void __stdcall browser_function_parameters(FrontendSymbol *symbol);
void finish_function_definition(void);
void __stdcall release_arena(int arena);
void __stdcall process_declaration(DeclarationSpec *declaration);
void __stdcall prepare_function_definition(DeclarationSpec *declaration, FrontendSymbol *symbol);
void __stdcall set_declaration_name(DeclarationSpec *declaration, char *name);
CType *parse_struct_specifier(void);
CType *parse_enum_specifier(void);
unsigned __stdcall new_enumerator(char *name, unsigned value);
void __stdcall add_enumerator(char *name, void *context, unsigned value);
void __stdcall add_struct_member(DeclarationSpec *declaration, FrontendSymbol *member);
FrontendSymbol *__stdcall new_declarator(char *name);
FrontendSymbol *__stdcall apply_declarator_type(DeclarationSpec *declaration, FrontendSymbol *symbol);
void __stdcall finish_initializer(FrontendSymbol *symbol, IrToken *initializer);
void __stdcall begin_aggregate_initializer(IrToken *initializer);
void __stdcall finish_aggregate_initializer(FrontendSymbol *symbol);
FrontendSymbol *__stdcall declare_initialized_symbol(FrontendSymbol *symbol);
void __stdcall report_declaration_error(unsigned code, DeclarationSpec *declaration);
void __stdcall emit_initializer(IrToken *initializer);
void begin_nested_initializer(void);
void finish_nested_initializer(void);
FrontendSymbol *__stdcall make_named_declarator(char *name);
FrontendSymbol *__stdcall combine_declarators(FrontendSymbol *declarator, FrontendSymbol *qualifier);
void __stdcall attach_type_shape(FrontendSymbol *symbol, TypeShape *shape);
FrontendSymbol *__stdcall make_function_declaration(FrontendSymbol *symbol, FrontendSymbol *parameters);
FrontendSymbol *__stdcall finish_function_declarator(FrontendSymbol *symbol, FrontendSymbol *parameters);
void __stdcall begin_parameter_list(FrontendSymbol *parameter);
void __stdcall insert_current_scope_symbol(FrontendSymbol *parameter);
FrontendSymbol *__stdcall append_parameter(FrontendSymbol *list, FrontendSymbol *parameter);
FrontendSymbol *__stdcall make_parameter(char *name);
void __stdcall finish_type_description(CType *type);
FrontendSymbol *__stdcall declare_symbol(DeclarationSpec *declaration, FrontendSymbol *symbol);
typedef struct ShapeCursor {
    TypeShape *shape;
    unsigned short attributes, flags;
} ShapeCursor;
typedef struct AggregateContext {
    FrontendSymbol *tag;
    unsigned field04, field08, field0c, field10, field14;
} AggregateContext;
extern int aggregate_depth;
extern AggregateContext aggregate_contexts[16];
int __stdcall prepare_declarator_shapes(FrontendSymbol *symbol, CType *base,
    TypeShape **work, ShapeCursor *base_shape, TypeShape **prefix, TypeShape **head, int is_typedef);
TypeShape *__stdcall extract_declarator_prefix(TypeShape *shape, TypeShape **prefix, FrontendSymbol *symbol, int is_typedef);
TypeShape *__stdcall extract_base_prefix(TypeShape *shape, TypeShape **prefix, FrontendSymbol *symbol, int is_typedef, unsigned short *attributes);
void __stdcall copy_shape_prefix(FrontendSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef);
void __stdcall check_declarator_prefix(FrontendSymbol *symbol, TypeShape *prefix);
TypeShape *__stdcall merge_function_shape(FrontendSymbol *symbol, ShapeCursor base,
    ShapeCursor *current, ShapeCursor next, int is_typedef);
void __stdcall finish_array_declarator(FrontendSymbol *symbol, TypeShape **result,
    ShapeCursor current, int is_typedef);
void __stdcall merge_declarator_type(FrontendSymbol *symbol, CType *type, int is_typedef);
void report_declaration_context_error(unsigned code, unsigned detail, char *name);
unsigned short __stdcall normalize_symbol_storage(unsigned short attributes, FrontendSymbol *symbol);
void __stdcall finalize_symbol_type(CType *type, FrontendSymbol *symbol);
TypeShape *__stdcall append_type_shape(TypeShape *head, TypeShape *tail);
FrontendSymbol *__stdcall resolve_type_symbol(FrontendSymbol *symbol);
unsigned short __stdcall based_string_modifier(unsigned short kind, FrontendSymbol *symbol);
unsigned short __stdcall based_symbol_modifier(unsigned short kind, FrontendSymbol *symbol);
unsigned short __stdcall based_type_modifier(unsigned short kind);
FrontendSymbol *finish_parameter_scope(void);
int __stdcall classify_parameter_declarations(FrontendSymbol *parameters);
void __stdcall report_type_syntax_error(unsigned code, CType *type);
FrontendSymbol *__stdcall attach_function_parameters(FrontendSymbol *declarator, FrontendSymbol *parameters);
void begin_compound_statement(void);
void finish_compound_statement(void);
void __stdcall emit_expression_statement(IrToken *expression);
void __stdcall emit_label_statement(FrontendSymbol *label);
void __stdcall begin_inline_region(unsigned flags);
void finish_inline_region(void);
void __stdcall emit_inline_jump(int kind);
void __stdcall begin_for_statement(IrToken *initial, IrToken *condition, IrToken *increment);
void finish_for_statement(void);
void begin_do_statement(void);
void __stdcall begin_switch_statement(IrToken *condition);
void finish_switch_statement(void);
void emit_default_statement(void);
void __stdcall emit_return_statement(IrToken *value, int implicit);
FrontendSymbol *__stdcall find_label(char *name);
FrontendSymbol *__stdcall declare_label(char *name);
void __stdcall reference_label(FrontendSymbol *label);
IrToken *__stdcall make_symbol_reference(FrontendSymbol *symbol);
void parse_asm_statement(void);
void emit_leave_statement(void);
void begin_try_statement(void);
void finish_try_statement(void);
void begin_except_statement(void);
void finish_except_statement(void);
void begin_finally_statement(void);
void finish_finally_statement(void);
void __stdcall set_except_filter(IrToken *filter);
void __stdcall emit_named_label(char *name);
void __stdcall emit_case_statement(IrToken *value);
FrontendSymbol *__stdcall make_internal_label(unsigned flags);
void __stdcall emit_conditional_branch(IrToken *condition, FrontendSymbol *label, int sense);
void __stdcall emit_code_label(FrontendSymbol *label);
IrToken *__stdcall make_full_expression(IrToken *expression);
IrToken *__stdcall make_binary_expression(unsigned char operation, IrToken *left, IrToken *right);
IrToken *__stdcall make_unary_expression(unsigned char operation, IrToken *operand);
IrToken *__stdcall make_cast_expression(CType *type, IrToken *operand);
IrToken *__stdcall make_function_call(unsigned char operation, IrToken *function, void *arguments);
IrToken *__stdcall make_member_expression(IrToken *operand, char *member);
IrToken *__stdcall make_sizeof_expression(IrToken *operand);
IrToken *__stdcall make_sizeof_type(CType *type);
IrToken *__stdcall make_identifier_expression(char *name);
IrToken *__stdcall make_subscript_expression(IrToken *base, IrToken *index);
IrToken *__stdcall make_segment_expression(IrToken *left, IrToken *right);
FrontendSymbol *__stdcall declare_named_entity(char *name, char *alias, unsigned kind, int arena);
void __stdcall concatenate_string_literals(void *left, void *right);
IrToken *__stdcall begin_sizeof_expression(IrToken *operand);
IrToken *__stdcall begin_alignof_expression(IrToken *operand);
void *__stdcall append_argument(void *arguments, IrToken *argument, int flags);
#endif
