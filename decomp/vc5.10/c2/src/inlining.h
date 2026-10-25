#ifndef C2_INLINING_H
#define C2_INLINING_H
#include "code-input.h"
#include "functions.h"
/* Prefixes consumed by the inliner. Their allocators establish any further
 * fields when reached; these declarations do not invent initial defaults. */
typedef struct C2InlineCall {
    struct C2InlineCall *next;
    C2Expression *call;
    unsigned char allowed_depth, pending09[3];
} C2InlineCall;
typedef struct C2InlineBinding {
    struct C2InlineBinding *next;
    C2Expression *argument;
    C2FunctionRegister *parameter;
    C2Expression *value;
} C2InlineBinding;
typedef char InlineCallDepthOffset[offsetof(C2InlineCall,allowed_depth) == 8 ? 1 : -1];
typedef char InlineBindingValueOffset[offsetof(C2InlineBinding,value) == 12 ? 1 : -1];
void __fastcall insert_input_record(C2Record *prior, C2Record *record);
void __fastcall unlink_and_release_input_record(C2Record *record);
C2InlineCall * __fastcall collect_inline_calls(C2Function *function, int *count, int force);
char * __fastcall set_diagnostic_function(C2FunctionSymbol *symbol, int decorated);
int __fastcall mark_inline_arguments(C2Function *function, C2Expression *entry, C2Expression *call);
void __fastcall merge_inline_symbols(C2ScopeHash *caller, C2ScopeHash *callee);
C2InlineBinding * __fastcall make_inline_bindings(C2Function *function, C2Expression *entry, C2Expression *call);
int __fastcall rewrite_inline_returns(C2Expression *entry, C2InlineBinding *bindings, C2Expression **result);
C2Expression * __fastcall splice_inline_arguments(C2Expression *call, C2InlineBinding *bindings, C2Expression *before, C2Expression *last);
void __fastcall rewrite_inline_result(C2Function *function, C2FunctionRegister *result, C2Expression *call);
void __fastcall materialize_inline_arguments(C2Function *function);
int __fastcall can_forward_inline_result(C2Expression *call, C2Expression *value);
C2Expression * __fastcall forward_inline_result(C2Expression *call, C2Expression *value);
void __fastcall insert_inline_result(C2Expression *entry, C2Expression *result);
int __fastcall is_direct_inline_return(C2Expression *record, C2Expression *before, C2Expression *call);
C2InputSymbol * __fastcall new_inline_result_symbol(unsigned short kind, unsigned bytes, short indirect, int local, unsigned char pool, unsigned attributes);
C2FunctionRegister * __fastcall find_inline_result_register(C2Expression *call, C2Expression **before, C2Expression *entry);
C2Expression * __fastcall raise_input_control_level(C2Expression *record, signed char amount);
int __fastcall propagate_inline_constants(C2Expression **first, int count);
void __fastcall fold_inline_constants(C2Expression *entry);
void __fastcall prepare_inline_call(C2Expression *call);
void __fastcall discard_inline_function(C2Function *function);
C2Expression * __fastcall save_inline_argument_context(C2Expression *before, C2Expression *call);
void __fastcall print_inline_function(unsigned serial);
int __fastcall reject_inline_cost(C2Function *function, C2Expression *call, C2Expression *entry);
#include <recon.h>
#endif
