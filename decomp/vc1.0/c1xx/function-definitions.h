#ifndef FUNCTION_DEFINITIONS_H
#define FUNCTION_DEFINITIONS_H
#include "parser.h"

extern CppSymbol *qualified_definition_class;
extern CType *current_return_type, *structure_return_pointer_type;
extern SegmentSymbol *default_code_segment;
/* Suppress the macro-end browser event while emitting a synthetic scope. */
extern int browser_macro_end_suppressed;
void discard_function_definition(void);
void __stdcall report_declaration_error(unsigned code, CppSymbol *symbol);
void activate_qualified_definition_scopes(void);
CppSymbol *__stdcall class_tag_for_type(CType *type);
void __stdcall assign_symbol_segment(CppSymbol *symbol, SegmentSymbol *segment);
CppSymbol *__stdcall prepare_function_definition(CppSymbol *symbol);
#endif
