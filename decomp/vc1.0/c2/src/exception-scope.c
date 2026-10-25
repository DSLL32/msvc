/* Structured-exception scope discovery and expression preparation. */
#include "exception-scope.h"
#include "backend.h"
#include "expression-fields.h"
#include "tree-rewrite.h"
#include "instruction-selection.h"
#include <recon.h>
ExceptionScope *prepared_exception_scopes;
Node *exception_return_label;
unsigned exception_finally_depth, next_exception_scope_identifier;
ExceptionScope *exception_scope_stack[20];
unsigned exception_scope_stack_depth; /* 00084e84, separate active-scope stack cursor */

/* Original [000656e0,00065725). The established provisional symbol name
   predates recovery of this SEH path. The neighboring routines are separate
   functions, not part of the former 1616-byte analysis envelope. */
void __fastcall prepare_inline_function(Function *function)
{
    next_exception_scope_identifier = 0;
    exception_finally_depth = 0;
    prepared_exception_scopes = 0;
    exception_return_label = 0;
    reset_exception_scope_stack();
    collect_exception_scopes(function);
    reset_exception_scope_stack();
    rewrite_exception_expressions(function);
}

/* Original [00065af0,00065afb). Old depth and all incoming registers are
   unused. The two preparation callers consume no EAX return value. */
void __cdecl reset_exception_scope_stack(void)
{
    exception_scope_stack_depth = 0;
}

/* Original [00065b00,00065d26). Discover protected scopes, attach their
   labels, and move specially marked handler blocks to their jump sites.
   The marked-block protocol keeps a predecessor before each target and a
   closing jump in the moved block; no synthetic cursor initialization or
   repair is supplied for malformed lists. */
void __fastcall collect_exception_scopes(Function *function)
{
    Expression *expression, *search_start = 0, *return_marker = 0;
    Expression *cursor, *found, *previous, *last, *saved;
    Node *node, *tree;
    SymbolHeader *symbol;
    LabelSymbol *new_label;
    ExceptionScope *scope;
    for (expression = function->expressions; expression; expression = expression->next) {
        node = expression->tree;
        switch (node->opcode) {
        case 0x29:
            scope = current_exception_scope();
            ((LabelSymbol *)expression->tree->right.symbol)->field28 =
                scope ? scope->identifier : ~0U;
            break;
        case 0x3a:
            symbol = node->right.symbol;
            if (symbol->field0d != 0x47) break;
            if (!search_start) {
                for (cursor = expression; cursor; cursor = cursor->next) {
                    tree = cursor->tree;
                    if (tree->opcode == 0x29 && tree->right.symbol->field0d == 0x47) break;
                    search_start = cursor;
                }
            }
            for (found = search_start; found; found = found->next) {
                tree = found->tree;
                if (tree->opcode == 0x29 && tree->right.symbol == symbol) break;
                previous = found;
            }
            if (!found) break;
            saved = expression->next;
            expression->next = found;
            for (; found->tree->opcode != 0x3a; found = found->next)
                last = found;
            last->next = saved;
            previous->next = found->next;
            if (saved->tree->opcode != 0x3a)
                compiler_internal_error("@(#)except.c:1.25", 0x13c);
            if (found->tree->right.symbol != saved->tree->right.symbol)
                compiler_internal_error("@(#)except.c:1.25", 0x13d);
            release_node(found->tree);
            release_expression(found);
            break;
        case 0x47:
            return_marker = expression;
            break;
        case 0x8d:
            scope = current_exception_scope();
            node->right.pointer = scope;
            scope->kind = 1;
            break;
        case 0x8e:
            scope = current_exception_scope();
            node->right.pointer = scope;
            scope->kind = 2;
            ++exception_finally_depth;
            break;
        case 0x8f:
            node->right.pointer = allocate_exception_scope();
            break;
        case 0x90:
            node->right.pointer = current_exception_scope();
            pop_exception_scope();
            if (((ExceptionScope *)node->right.pointer)->kind == 2)
                --exception_finally_depth;
            break;
        }
    }
    exception_return_label = new_node(0x29);
    new_label = new_internal_label();
    exception_return_label->right.symbol = &new_label->symbol;
    insert_tree_expression(function->expressions, return_marker->tree, exception_return_label);
}

/* Original [000661c0,00066232). All twelve words belong to the newly
   allocated scope. Keep the clear here rather than adding a CRT call edge. */
ExceptionScope *__cdecl allocate_exception_scope(void)
{
    ExceptionScope *scope, *parent, **link;
    unsigned char *clear;
    unsigned bytes;
    scope = arena_alloc(1, sizeof *scope);
    clear = (unsigned char *)scope;
    for (bytes = sizeof *scope; bytes; --bytes) *clear++ = 0;
    scope->kind = 0;
    scope->identifier = next_exception_scope_identifier++;
    scope->finally_depth = exception_finally_depth;
    parent = enclosing_try_scope();
    scope->parent_identifier = parent ? (long)parent->identifier : -1;
    push_exception_scope(scope);
    for (link = &prepared_exception_scopes; *link; link = &(*link)->next)
        ;
    *link = scope;
    return scope;
}

/* Original [00066380,000663ad). Push/pop establish depth0..20; inactive
   slots are not input to the search. No tighter guard is introduced here. */
ExceptionScope *__cdecl enclosing_try_scope(void)
{
    unsigned depth = exception_scope_stack_depth;
    ExceptionScope *scope;
    while (depth) {
        scope = exception_scope_stack[--depth];
        if (scope->kind == 0) return scope;
    }
    return 0;
}
