#ifndef INLINE_CALLS_H
#define INLINE_CALLS_H
#include "functions.h"
typedef struct InlineSite {
    struct InlineSite *next;
    Node *call, *parent;
    Expression *expression;
    unsigned flags;
} InlineSite;
typedef struct InlineBinding {
    struct InlineBinding *next;
    Node *parameter, *argument, *storage;
    unsigned flags;
} InlineBinding;
extern InlineSite *free_inline_sites;
extern InlineBinding *free_inline_bindings;
extern int inline_expansion_cost;
extern int optimize_global;
int __fastcall inline_function_calls(Function *function, int depth, int budget);
InlineSite *__fastcall collect_inline_sites(Function *function, int *count);
InlineBinding *__fastcall bind_inline_arguments(Function *function, Node *entry, Node *call);
void __fastcall check_inline_argument_aliases(InlineBinding *bindings);
int __fastcall analyze_inline_returns(Expression *entry, InlineBinding *bindings);
void __fastcall merge_inline_scope(ScopeHash *outer, ScopeHash *inner);
Node *__fastcall new_inline_storage(signed char kind, unsigned bytes);
InlineBinding *__fastcall find_inline_binding(InlineBinding *bindings, SymbolHeader *symbol);
Node *__fastcall inline_argument_prefix(InlineBinding *bindings);
void __fastcall materialize_inline_arguments(Function *function, InlineSite *site, InlineBinding *bindings);
Expression *__fastcall substitute_inline_body(Function *function, InlineBinding *bindings,
    Node *result, unsigned source_location, InlineSite *site, int hidden_result);
void __fastcall recycle_inline_site(InlineSite *site);
void __fastcall recycle_inline_binding(InlineBinding *binding);
void __fastcall scan_inline_candidates(InlineSite **sites, Expression *expression,
    Node *node, Node *parent, unsigned flags, int *count);
InlineSite *__fastcall new_inline_site(Node *call, Node *parent, Expression *expression, unsigned flags);
#endif
