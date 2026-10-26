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

void report_error(unsigned code, ...) { __asm { int 3 } }
void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
int release_input_handle(void) { __asm { int 3 } return 0; }
void browser_symbol_rollover(void) { __asm { int 3 } }
void __stdcall warn_ignored_distance(CppSymbol *symbol) { __asm { int 3 } }





/* Complete yacc semantic callees, still unaccepted. */





















void __stdcall report_missing_declarator(unsigned code, DeclarationType *type) { __asm { int 3 } }

/* C++ token classification and deferred lookahead. */





























int recursive_lookahead_parse(void) { __asm { int 3 } return 0; }













































































IrToken *__stdcall make_address_temporary(IrToken *expression) { __asm { int 3 } return 0; }
void __stdcall resolve_untyped_binary_operand(unsigned char operation, IrToken **operand, IrToken *other) { __asm { int 3 } }
void __stdcall report_incompatible_type_shapes(unsigned char operation, CType *target, CType *source) { __asm { int 3 } }





























unsigned char __stdcall compute_virtual_base_access(CppSymbol *base, CppSymbol *derived) { __asm { int 3 } return 0; }

























void browser_end_macro_expansion(void) { __asm { int 3 } }







/* Newly encountered pragma callees; bodies remain unaccepted entry traps. */
void synchronize_pch_pragma(void) { __asm { int 3 } }










void __stdcall report_asm_expected_token(int actual, int expected) { __asm { int 3 } }

RECON_RETURN8_TYPE(ParserValue) __stdcall asm_recovery_value(int token) { ParserValue value; __asm { int 3 } value.words.low = value.words.high = 0; RECON_RETURN8(value); }









































struct VirtualThunkKey;
