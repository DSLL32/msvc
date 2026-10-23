#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include "builtins.h"
#include "hash-table.h"
#include <recon.h>

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
    ArgumentNode *arguments;
    TypeShape *shape;
} ParserValue;

typedef union ParserValueStack {
    ParserValue values[341];
    struct {
        ParserValue reserved_values[340];
        int fastcall_stack_warning;
        unsigned reserved_word;
    } control;
} ParserValueStack;
extern ParserValueStack parser_values;
IrToken *__stdcall make_typed_ir_expression(unsigned char operation, IrToken *left, IrToken *right, CType *type);
IrToken *__stdcall make_based_pointer_base(CType *type);
IrToken *__stdcall make_symbol_ir_leaf(unsigned char kind, FrontendSymbol *symbol);
IrToken *__stdcall make_typed_unary_expression(unsigned char operation, CType *type, IrToken *operand);
int __stdcall emit_intrinsic_expression(IrToken *expression);
void start_function_ir(void);
void __stdcall initialize_expression_stream(int mode);
void __stdcall emit_ir_expression(IrToken *expression);
IrToken *__stdcall make_ir_expression(unsigned char operation, IrToken *left, IrToken *right);
void __stdcall define_register_parameter(FrontendSymbol *parameter);
void __stdcall record_inline_parameter(FrontendSymbol *parameter);
unsigned __stdcall parameter_list_bytes(FrontendSymbol *parameters);
int __stdcall compatible_parameter_types(CType *expected, CType *actual);
extern FrontendSymbol *based_declarator_symbol;
extern FrontendSymbol *function_parameter_declarations;
short begin_declaration_text(void);
void end_declaration_text(void);
void __stdcall print_function_declaration(FrontendSymbol *symbol, FILE *stream);
void __stdcall record_line_information(unsigned kind);

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
#define PARSER_PRODUCTION_COUNT 227
extern ParserProduction parser_productions[PARSER_PRODUCTION_COUNT];
extern ParserRecovery parser_recoveries[];
extern ParserValue parser_value;
extern int parser_lookahead, parser_error_count;
extern short parser_recovery_count;
extern DeclarationSpec *declaration_spec_stack;
extern int declaration_depth, unevaluated_expression_depth;
/* Declaration lookahead may use flags[-1], the high byte of the active
   parameter-context word. The byte view keeps that preceding cell valid. */
typedef union DeclarationControlStorage {
    struct { int parameter_context; unsigned char flags[60]; } fields;
    unsigned char bytes[64];
} DeclarationControlStorage;
extern DeclarationControlStorage declaration_controls;
#define declaration_spec_flags (declaration_controls.bytes + sizeof(int))
#define active_parameter_context declaration_controls.fields.parameter_context
extern unsigned char parameter_nesting;
FrontendSymbol *__stdcall find_identifier_symbol(char *name);
FrontendSymbol *__stdcall make_implicit_declaration(char *name, unsigned char nesting);
void __stdcall browser_symbol_reference(FrontendSymbol *symbol);
extern TypeShape *declaration_shapes[60];
unsigned char __stdcall lookup_identifier_token(FrontendSymbol *explicit_symbol);

/* Semantic routines retain their individual original entry points. */
CType *__stdcall merge_type_specifier(CType *type, unsigned short *flags, unsigned short added);
FrontendSymbol *__stdcall lookup_tag(char *name);
FrontendSymbol *__stdcall lookup_or_declare_tag(char *name, unsigned short kind,
    void (__stdcall *insert)(FrontendSymbol *));
FrontendSymbol *__stdcall lookup_tag_chain(char *name, FrontendSymbol *head);
FrontendSymbol *__stdcall allocate_frontend_symbol(int arena);
SymbolScope *enclosing_declaration_scope(void);
extern int tag_in_current_scope;
FrontendSymbol *__stdcall create_tag(char *name, unsigned short kind,
    void (__stdcall *insert)(FrontendSymbol *), int definition);
FrontendSymbol *__stdcall check_tag_kind(FrontendSymbol *tag, unsigned short kind);
FrontendSymbol *__stdcall begin_tag_definition(char *name, unsigned short kind, unsigned short attributes);
unsigned char __stdcall read_tag_specifier(unsigned short kind, unsigned short *attributes);
CType *__stdcall merge_named_type(FrontendSymbol *symbol, unsigned short *flags, unsigned short *attributes);
void __stdcall merge_storage_class(unsigned char *storage, unsigned class, unsigned modifier, int replace);
void parse_declspec(unsigned char *storage);
IrToken *__stdcall normalize_based_expression(IrToken *expression);
int __stdcall same_based_pointer_base(IrToken *left, IrToken *right);
IrToken *__stdcall make_constant_token(unsigned char token, void *value);
unsigned char __stdcall resolve_identifier_token(unsigned char token);
void __stdcall report_parser_error(char *text);
void __stdcall report_expected_token(int actual, int expected);
RECON_RETURN8_TYPE(ParserValue) __stdcall recovery_token_value(int token);
unsigned char recover_parser_token(unsigned token, ...);
void skip_declaration_tokens(void);
void __stdcall finish_external_declaration(int recovering);
void begin_function_definition(void);
void reset_inline_function_state(void);
void abort_function_definition(void);
CType *__stdcall function_result_type(FrontendSymbol *symbol);
void __stdcall report_symbol_error(unsigned code, FrontendSymbol *symbol);
int __stdcall is_record_type(CType *type);
FrontendSymbol *__stdcall find_scope_declaration(SymbolScope *scope, char *name, int case_fold);
int __stdcall compatible_base_type_flags(unsigned short left, unsigned short right);
int __stdcall is_function_pointer_type(CType *type);
CType *__stdcall default_argument_promotion(CType *type);
CType *__stdcall merge_function_parameter_types(CType *left, CType *right, int find_function, int outermost);
int __stdcall compare_declaration_types(FrontendSymbol *left, FrontendSymbol *right, CType *right_type);
int __stdcall compatible_storage_modifiers(unsigned left, unsigned right);
int __stdcall compatible_storage_classes(FrontendSymbol *left, FrontendSymbol *right);
int __stdcall compatible_declaration_attributes(unsigned short left, unsigned short right, int function);
void __stdcall report_type_conflict(unsigned code, FrontendSymbol *symbol, char *name, CType *actual, CType *expected);
int __stdcall declaration_conflict(FrontendSymbol *existing, FrontendSymbol *symbol);
int __stdcall merge_storage_modifiers(unsigned char *target, unsigned source);
void __stdcall commit_redeclaration(FrontendSymbol *existing, FrontendSymbol *symbol);
FrontendSymbol *__stdcall reconcile_declarations(FrontendSymbol *existing, FrontendSymbol *symbol);
FrontendSymbol *__stdcall merge_scoped_declaration(FrontendSymbol *symbol, SymbolScope *scope);
FrontendSymbol *__stdcall merge_global_declaration(FrontendSymbol *symbol);
FrontendSymbol *__stdcall merge_local_declaration(FrontendSymbol *symbol);
void __stdcall record_local_declaration(FrontendSymbol *symbol);
void __stdcall record_symbol_alias(FrontendSymbol *symbol, FrontendSymbol *alias);
char *__stdcall format_local_static_name(FrontendSymbol *symbol, unsigned level);
char *__stdcall local_static_name(FrontendSymbol *symbol);
extern FrontendSymbol *function_symbol, *current_parameter;
extern int function_parameter_context;
void __stdcall assign_symbol_segment(FrontendSymbol *symbol, FrontendSymbol *segment);
FrontendSymbol *__stdcall initialize_function_definition(FrontendSymbol *symbol);
void browser_begin_function_scope(void);
void __stdcall browser_function_parameters(FrontendSymbol *symbol);
extern FrontendSymbol *pending_global_symbols;
void __stdcall emit_debug_type(CType *type);
void flush_debug_types(void);
void write_function_directory(void);
FrontendSymbol *__stdcall prepare_initializer_symbol(FrontendSymbol *symbol);
void finish_label_symbols(void);
int defer_global_symbol(FrontendSymbol *symbol);
int __stdcall serialize_scope(SymbolScope *scope);
void finish_inline_function(void);
void __stdcall record_function_definition(FrontendSymbol *symbol);
void finish_function_records(void);
void __stdcall end_function_context(int direct_scope_exit);
void clear_symbol_aliases(void);
void finish_function_definition(void);
void __stdcall process_declaration(DeclarationSpec *declaration);
void __stdcall prepare_function_definition(DeclarationSpec *declaration, FrontendSymbol *symbol);
FrontendSymbol *__stdcall declare_old_style_parameter(DeclarationSpec *declaration, FrontendSymbol *declarator);
CType *parse_struct_specifier(void);
CType *parse_enum_specifier(void);
unsigned __stdcall new_enumerator(unsigned value, char *name);
void __stdcall add_enumerator(char *name, CType *context, unsigned value);
void __stdcall add_anonymous_members(CType *type);
int __stdcall check_member_name(char *name);
void __stdcall layout_bitfield_member(FrontendSymbol *member, char *name, unsigned short flags);
unsigned __stdcall aggregate_padding(unsigned bytes);
void __stdcall align_aggregate_member(unsigned bytes);
int __stdcall addition_overflows(unsigned left, unsigned right, unsigned short is_unsigned, unsigned short bytes);
unsigned __stdcall add_member_offset(unsigned base, unsigned displacement);
unsigned __stdcall layout_data_member(FrontendSymbol *member);
void __stdcall append_aggregate_member_range(FrontendSymbol *first, FrontendSymbol *last);
void __stdcall append_aggregate_member(FrontendSymbol *member);
void __stdcall add_struct_member(DeclarationSpec *declaration, FrontendSymbol *member);
FrontendSymbol *__stdcall make_abstract_declarator(TypeShape *prefix);
unsigned short __stdcall effective_type_qualifiers(CType *type);
void __stdcall initialize_thread_symbol(FrontendSymbol *symbol);
void __stdcall process_symbol_storage(FrontendSymbol *symbol);
FrontendSymbol *__stdcall apply_declarator_type(DeclarationSpec *declaration, FrontendSymbol *symbol);
void __stdcall finish_initializer(FrontendSymbol *symbol, IrToken *initializer);
void __stdcall begin_aggregate_initializer(FrontendSymbol *symbol);
void __stdcall finish_aggregate_initializer(FrontendSymbol *symbol);
FrontendSymbol *__stdcall declare_initialized_symbol(FrontendSymbol *symbol);
void __stdcall report_declaration_error(unsigned code, DeclarationSpec *declaration);
void __stdcall emit_initializer(IrToken *initializer);
void begin_nested_initializer(void);
void finish_nested_initializer(void);
FrontendSymbol *__stdcall make_named_declarator(char *name);
FrontendSymbol *__stdcall apply_declarator_prefix(FrontendSymbol *declarator, TypeShape *prefix);
void __stdcall attach_type_shape(FrontendSymbol *symbol, TypeShape *shape);
FrontendSymbol *__stdcall make_function_declaration(FrontendSymbol *symbol, FrontendSymbol *parameters);
FrontendSymbol *__stdcall finish_function_declarator(FrontendSymbol *symbol, FrontendSymbol *parameters);
void __stdcall begin_parameter_list(FrontendSymbol *parameter);
void __stdcall insert_current_scope_symbol(FrontendSymbol *parameter);
void __stdcall link_parameter_after(FrontendSymbol *list, FrontendSymbol *parameter);
FrontendSymbol *__stdcall append_parameter(FrontendSymbol *list, FrontendSymbol *parameter);
FrontendSymbol *__stdcall make_parameter(char *name);
IrToken *__stdcall finish_call_arguments(IrToken *call);
FrontendSymbol *__stdcall declare_symbol(DeclarationSpec *declaration, FrontendSymbol *symbol);
typedef struct ShapeCursor {
    TypeShape *shape;
    unsigned short attributes, flags;
} ShapeCursor;
typedef struct AggregateContext {
    FrontendSymbol *tag;
    FrontendSymbol *last_member;
    CType *type;
    unsigned size;
    unsigned short bitfield_type, alignment;
    unsigned char bitfield_used_bits, flags;
    unsigned short reserved;
} AggregateContext;
extern unsigned enumerator_value;
extern CType *enumerator_context;
extern FrontendSymbol *current_enum_tag;
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
void report_declaration_context_error(unsigned warning, unsigned error, ...);
void __stdcall warn_ignored_storage_model(FrontendSymbol *symbol);
unsigned short __stdcall normalize_symbol_storage(unsigned short attributes, FrontendSymbol *symbol);
void __stdcall finalize_symbol_type(CType *type, FrontendSymbol *symbol);
TypeShape *__stdcall append_type_shape(TypeShape *head, TypeShape *tail);
FrontendSymbol *__stdcall resolve_based_pointer_symbol(char *name);
unsigned short __stdcall based_string_modifier(unsigned short kind, FrontendSymbol *symbol);
unsigned short __stdcall based_symbol_modifier(unsigned short kind, FrontendSymbol *symbol);
unsigned short __stdcall based_type_modifier(unsigned short kind);
FrontendSymbol *finish_parameter_scope(void);
int __stdcall classify_parameter_declarations(FrontendSymbol *parameters);
void __stdcall report_type_syntax_error(unsigned code, CType *type);
FrontendSymbol *__stdcall attach_function_parameters(FrontendSymbol *declarator, FrontendSymbol *parameters);
extern unsigned short function_scope_level;
void enter_compound_scope(void);
void begin_compound_statement(void);
void leave_compound_scope(void);
void browser_finish_scope(void);
void *__stdcall push_work_list(void **head, int arena);
void *__stdcall pop_work_list(void **head);
void emit_scope_cleanup_records(void);
void __stdcall queue_completed_scope(SymbolScope *scope);
void finish_compound_statement(void);
void __stdcall emit_expression_statement(IrToken *expression);
void __stdcall emit_label_statement(FrontendSymbol *label);
void __stdcall begin_inline_region(unsigned flags);
void finish_inline_region(void);
void __stdcall emit_inline_jump(int kind);
void __stdcall begin_for_statement(IrToken *initial, IrToken *condition, IrToken *increment);
void finish_for_statement(void);
void begin_do_statement(void);
typedef struct SwitchCase {
    struct SwitchCase *next;
    unsigned value, label_index;
} SwitchCase;
typedef struct LabelStack {
    struct LabelStack *next;
    union { IrToken *label; SwitchCase *cases; struct QcLoopState *qc_loop; } value;
} LabelStack;
void __stdcall push_label_stack(LabelStack **stack, unsigned flags);
void __stdcall begin_switch_statement(IrToken *condition);
void finish_switch_statement(void);
void emit_leave_statement(void);
extern CType *function_return_type;
extern IrToken *function_exit_label;
void warn_void_function_pointer(void);
int __stdcall check_conversion_attributes(unsigned char operation, unsigned short target, unsigned short source, int first);
void __stdcall conversion_diagnostic(unsigned code, unsigned level, unsigned char operation, CType *target, CType *source);
void __stdcall check_based_pointer_conversion(unsigned char operation, TypeShape *target, TypeShape *source);
int __stdcall compatible_shape_attributes(unsigned short target, unsigned short source);
int __stdcall compare_prototype_parameter_types(CType *target, CType *source);
void __stdcall report_conversion_mismatch(unsigned char operation, CType *target, CType *source);
int __stdcall check_type_conversion(unsigned char operation, CType *target, CType *source);
int __stdcall integer_conversion_overflow(unsigned short bytes, unsigned short target_unsigned, unsigned short source_unsigned, unsigned value);
int __stdcall check_assignment_conversion(unsigned char operation, CType *target, IrToken *value);
IrToken *__stdcall convert_based_pointer(CType *target, IrToken *value);
void __stdcall validate_self_based_source(IrToken *value);
void __stdcall emit_return_statement(IrToken *value, int explicit_return);
FrontendSymbol *__stdcall find_label(char *name);
FrontendSymbol *__stdcall declare_label(char *name);
void __stdcall reference_label(FrontendSymbol *label);
IrToken *__stdcall make_symbol_reference(FrontendSymbol *symbol);
IrToken *__stdcall make_ir_leaf(unsigned char kind, void *value, CType *type);
void parse_asm_statement(void);
void finish_except_block(void);
void begin_finally_block(void);
void finish_finally_block(void);
void begin_try_block(void);
void finish_try_block(void);
void adjust_seh_label_depths(void);
extern int label_stack_count;
extern FrontendSymbol *label_symbols;
void enter_except_filter(void);
void leave_except_filter(void);
void __stdcall begin_except_block(IrToken *filter);
void __stdcall emit_named_label(char *name);
void __stdcall emit_case_statement(IrToken *value);
IrToken *__stdcall make_internal_label(unsigned flags);
void __stdcall emit_conditional_branch(IrToken *condition, IrToken *label, int sense);
void __stdcall emit_code_label(IrToken *label);
unsigned __stdcall integer_constant_value(IrToken *expression);
IrToken *__stdcall make_binary_expression(unsigned char operation, IrToken *left, IrToken *right);
IrToken *__stdcall make_unary_expression(unsigned char operation, IrToken *operand);
IrToken *__stdcall make_cast_expression(FrontendSymbol *declarator, IrToken *operand);
IrToken *__stdcall select_member_expression(unsigned char operation, IrToken *operand, char *name);
IrToken *__stdcall make_based_address(IrToken *base, IrToken *offset);
IrToken *__stdcall make_sizeof_expression(IrToken *operand);
IrToken *__stdcall make_sizeof_type(CType *type);
IrToken *__stdcall make_identifier_expression(char *name);
IrToken *__stdcall make_subscript_expression(IrToken *base, IrToken *index);
IrToken *__stdcall make_segment_expression(IrToken *left, IrToken *right);
FrontendSymbol *__stdcall find_named_entity(char *name);
FrontendSymbol *__stdcall new_named_entity(char *name, char *alias, unsigned index, unsigned char kind);
int __stdcall name_has_text_suffix(char *name, char *suffix);
FrontendSymbol *__stdcall declare_named_entity(char *name, char *alias, unsigned kind, int arena);
void __stdcall concatenate_string_literals(StringToken *left, StringToken *right);
IrToken *__stdcall prepare_named_function_call(char *name);
IrToken *__stdcall prepare_call_expression(IrToken *operand);
ArgumentNode *__stdcall append_argument(ArgumentNode *arguments, IrToken *argument, int flags);
void __stdcall prepare_symbol_debug_type(FrontendSymbol *symbol);
unsigned short __stdcall debug_symbol_type_index(FrontendSymbol *symbol);

extern unsigned constant_one;
extern unsigned char expression_storage[2052];
extern unsigned char *text_work_cursor;

typedef struct QcOperand {
    unsigned char operation, reserved[3];
    union { unsigned integer; FrontendSymbol *symbol; struct QcOperand *pair; } value;
} QcOperand;
typedef struct QcLoopState {
    unsigned char condition, increment, reserved[2];
    QcOperand left, right, target, step;
} QcLoopState;
typedef char QcOperand_must_be_8_bytes[sizeof(QcOperand) == 8 ? 1 : -1];
typedef char QcLoopState_must_be_36_bytes[sizeof(QcLoopState) == 36 ? 1 : -1];
extern LabelStack *qc_loop_labels, *qc_loop_work;
int __stdcall recognize_qc_loop(IrToken *condition, IrToken *increment, LabelStack *entry);
void finish_qc_loop(void);
#endif
