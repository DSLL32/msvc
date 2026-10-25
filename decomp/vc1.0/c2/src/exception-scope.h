#ifndef EXCEPTION_SCOPE_H
#define EXCEPTION_SCOPE_H
#include "flow-edges.h"
#include <recon.h>
/* Arena4 holds sixteen hash heads and eight-byte borrowed-symbol links. */
typedef struct ExceptionLocalEntry {
    SymbolHeader *symbol;
    struct ExceptionLocalEntry *next;
} ExceptionLocalEntry;
typedef struct ExceptionLocalSet {
    ExceptionLocalEntry *buckets[16];
} ExceptionLocalSet;
typedef char ExceptionLocalEntry_size_is_8[sizeof(ExceptionLocalEntry)==8?1:-1];
typedef char ExceptionLocalSet_size_is_64[sizeof(ExceptionLocalSet)==64?1:-1];
/* Per-function structured-exception preparation state. These four owners
   are independently addressed in the original, not an indexed image view. */
extern ExceptionScope *prepared_exception_scopes; /* 0007b998 */
extern Node *exception_return_label; /* 0007b99c */
extern unsigned exception_finally_depth; /* 0007b9a0 */
extern ExceptionScope *exception_scope_stack[20]; /* 0007e850, active slots only */
extern unsigned exception_scope_stack_depth; /* 00084e84 */
extern unsigned next_exception_scope_identifier; /* 0007b9a4 */
void __cdecl reset_exception_scope_stack(void);
ExceptionScope *__cdecl current_exception_scope(void);
ExceptionScope *__cdecl allocate_exception_scope(void);
ExceptionScope *__cdecl enclosing_try_scope(void);
void __fastcall push_exception_scope(ExceptionScope *scope);
void __cdecl pop_exception_scope(void);
void __fastcall collect_exception_scopes(Function *function);
void __fastcall rewrite_exception_expressions(Function *function);
Node *__fastcall new_exception_annotation(unsigned short code);
Node *__fastcall new_finally_unwind(unsigned depth);
Node *__fastcall insert_exception_unwind(Expression *previous, Node *before, long target);
Node *__fastcall new_scope_finally_unwind(long target);
Node *__fastcall new_scope_unwind(long target);
void __fastcall collect_exception_references(Node *node, unsigned filter_depth);
void __fastcall mark_exception_references(Node *node, unsigned filter_depth);
void __fastcall remember_exception_local(SymbolHeader *symbol);
SymbolHeader *__fastcall find_exception_local(ExceptionLocalSet *set, unsigned identifier);
void __fastcall insert_exception_local(ExceptionLocalSet *set, SymbolHeader *symbol);
#endif
