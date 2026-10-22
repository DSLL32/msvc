#ifndef FUNCTION_DEFINITIONS_H
#define FUNCTION_DEFINITIONS_H
#include "parser.h"
#include "frontend-records.h"

extern CppSymbol *qualified_definition_class;
extern CType *current_return_type, *structure_return_pointer_type;
extern SegmentSymbol *default_code_segment;
/* Suppress the macro-end browser event while emitting a synthetic scope. */
extern int browser_macro_end_suppressed;
extern int convention_warning_emitted;
extern IrToken *pending_constructor_initializers;
void begin_function_body(void);
void __stdcall begin_function_output(int global_only);
IrToken *__stdcall this_pointer_expression(CppSymbol *parameter);
IrToken *__stdcall new_left_typed_expression(unsigned char operation, IrToken *left, IrToken *right);
void __stdcall emit_expression_tree(IrToken *expression);
void __stdcall emit_parameter_declaration(CppSymbol *parameter);
void __stdcall analyze_inline_parameter(CppSymbol *parameter);
unsigned __stdcall function_parameter_bytes(CppSymbol *parameters);
void __stdcall print_declaration(CppSymbol *symbol, FILE *stream);
typedef struct LocalStaticGuardState {
    CppSymbol *guard;
    unsigned state, count;
} LocalStaticGuardState;
typedef char LocalStaticGuardStateSizeMustBe12[(sizeof(LocalStaticGuardState) == 12) ? 1 : -1];
extern LocalStaticGuardState local_static_guard;
extern int default_inline_candidate;
extern IrToken *function_exit_label;
extern unsigned function_expression_sequence;
CppSymbol *make_missing_function_declaration(void);
void initialize_function_inline_analysis(void);
void discard_function_definition(void);
void __stdcall report_declaration_error(unsigned code, CppSymbol *symbol);
void activate_qualified_definition_scopes(void);
CppSymbol *__stdcall class_tag_for_type(CType *type);
void __stdcall assign_symbol_segment(CppSymbol *symbol, SegmentSymbol *segment);
CppSymbol *__stdcall prepare_function_definition(CppSymbol *symbol);
#endif
