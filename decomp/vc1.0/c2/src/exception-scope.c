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

/* Original [00065ef0,00065f21). Ordinary producers keep depth0..19.
   Overflow first enters the named diagnostic; if it returns, the original
   reloads depth and continues its unchecked store. No clamp is invented. */
void __fastcall push_exception_scope(ExceptionScope *scope)
{
    unsigned depth = exception_scope_stack_depth;
    if ((int)depth >= 20) {
        compiler_internal_error("@(#)except.c:1.25", 0x301);
        depth = exception_scope_stack_depth;
    }
    exception_scope_stack[depth] = scope;
    exception_scope_stack_depth = depth + 1;
}

/* Original [000660d0,000660e8), including the internal alignment LEA.
   Only the active top pointer is read; neither its pointee nor inactive
   slots are consumed. Push/pop establish the valid depth0..20 contract. */
ExceptionScope *__cdecl current_exception_scope(void)
{
    unsigned depth = exception_scope_stack_depth;
    return depth ? exception_scope_stack[depth - 1] : 0;
}

/* Original [00065ec0,00065ef0). The returning-error branch reloads the
   shared cursor and retains the original unchecked decrement/store. */
void __cdecl pop_exception_scope(void)
{
    unsigned depth = exception_scope_stack_depth;
    if ((int)depth <= 0) {
        compiler_internal_error("@(#)except.c:1.25", 0x307);
        depth = exception_scope_stack_depth;
    }
    exception_scope_stack_depth = --depth;
    exception_scope_stack[depth] = 0;
}

/* Original [00065730,00065ae1). Walk only the original expression stream:
   inserted annotation/unwind expressions are deliberately skipped. The
   predecessor starts at the head because that head is a retained marker. */
void __fastcall rewrite_exception_expressions(Function *function)
{
    Expression *expression, *previous, *next;
    Node *node, *tree, *unwind, *temporary;
    ExceptionScope *scope;
    LabelSymbol *label;
    unsigned kind, bytes;
    previous = function->expressions;
    for (expression = function->expressions; expression; expression = next) {
        next = expression->next;
        node = expression->tree;
        switch (node->opcode) {
        case 0x3a:
            insert_exception_unwind(previous, node,
                ((LabelSymbol *)node->right.symbol)->field28);
            scope = current_exception_scope();
            if (scope && scope->state == 0x8e &&
                ((LabelSymbol *)node->right.symbol)->field28 != scope->identifier)
                insert_tree_expression(previous, node, new_finally_unwind(0));
            break;
        case 0x41:
            unwind = insert_exception_unwind(previous, node, -1);
            if (!unwind) break;
            bytes = node->attributes.words.type == 0x8000 ? node->info.value :
                node->attributes.words.type & 0xff;
            temporary = new_temporary_reference(bytes, 0x200);
            /* Original recipes at80d10 (PPNT) and8022c (PNT). */
            tree = build_tree("PPNT", temporary, node->left.node, 0x32, node);
            insert_tree_expression(previous, unwind, tree);
            node->left.node = build_tree("PNT", temporary, 0x30, node);
            if (next->tree->opcode == 0x3a) next = next->next;
            break;
        case 0x8d:
            scope = node->right.pointer;
            scope->state = 0x8d;
            insert_tree_expression(previous, node, new_exception_annotation(0x27));
            tree = new_node(0x3a);
            tree->right.symbol = scope->alternate_label;
            insert_tree_expression(previous, node, tree);
            tree = new_node(0x29);
            tree->right.symbol = scope->filter_label;
            insert_tree_expression(previous, node, tree);
            insert_tree_expression(previous, node, new_exception_annotation(0x28));
            insert_tree_expression(previous, next->tree, new_exception_annotation(0x29));
            tree = new_node(0x3a);
            tree->right.symbol = exception_return_label->right.symbol;
            insert_tree_expression(previous, next->tree, tree);
            tree = new_node(0x29);
            tree->right.symbol = scope->handler_label;
            insert_tree_expression(previous, next->tree, tree);
            expression->next->next->next->source_location = next->source_location;
            insert_tree_expression(previous, next->tree, new_exception_annotation(0x2a));
            insert_tree_expression(previous, next->tree,
                new_finally_unwind(scope->finally_depth));
            break;
        case 0x8e:
            scope = node->right.pointer;
            scope->state = 0x8e;
            insert_tree_expression(previous, node, new_exception_annotation(0x27));
            tree = new_node(0x3a);
            tree->right.symbol = scope->handler_label;
            insert_tree_expression(previous, node, tree);
            tree = new_node(0x29);
            tree->right.symbol = scope->handler_label;
            insert_tree_expression(previous, next->tree, tree);
            insert_tree_expression(previous, next->tree, new_exception_annotation(0x2c));
            break;
        case 0x8f:
            scope = node->right.pointer;
            push_exception_scope(scope);
            scope->state = 0x8f;
            kind = scope->kind;
            if (kind == 1) {
                label = new_internal_label();
                scope->filter_label = &label->symbol;
                label->label_kind |= 4;
                label->symbol.field0e |= 0x1000;
                label->field28 = scope->identifier;
            }
            if (kind == 1 || kind == 2) {
                label = new_internal_label();
                scope->handler_label = &label->symbol;
                label->label_kind |= 4;
                label->symbol.field0e |= 0x1000;
                label->field28 = scope->identifier;
                label = new_internal_label();
                scope->alternate_label = &label->symbol;
                label->field28 = scope->parent_identifier;
                if (kind == 2) {
                    label->symbol.field0e |= 0x1000;
                    label->label_kind |= 4;
                }
            }
            insert_tree_expression(previous, next->tree, new_exception_annotation(0x26));
            expression->next->source_location = next->source_location;
            break;
        case 0x90:
            scope = node->right.pointer;
            if (scope->kind == 2) {
                insert_tree_expression(previous, node, new_exception_annotation(0x2d));
                tree = new_node(0x3a);
                tree->right.symbol = scope->alternate_label;
                insert_tree_expression(previous, next->tree, tree);
            } else {
                insert_tree_expression(previous, node, new_exception_annotation(0x2b));
            }
            tree = new_node(0x29);
            tree->right.symbol = scope->alternate_label;
            insert_tree_expression(previous, next->tree, tree);
            pop_exception_scope();
            break;
        }
        previous = expression;
    }
}

/* Original [00066240,00066261): the private annotation operand is a
   16-bit control code in CX; the current scope is borrowed, not copied. */
Node *__fastcall new_exception_annotation(unsigned short code)
{
    Node *node = new_node(0x4f);
    node->detail.control.code = (short)code;
    node->left.pointer = current_exception_scope();
    return node;
}

/* Original [000662c0,00066375). Crossed finally scopes select the
   finally-aware unwind form. A handler already executing its finally
   starts searching at the enclosing try scope rather than itself. */
Node *__fastcall insert_exception_unwind(Expression *previous, Node *before, long target)
{
    ExceptionScope *scope = current_exception_scope();
    Node *tree;
    if (!scope || scope->identifier == (unsigned)target) return 0;
    if (scope->state == 0x8e) {
        if (scope->parent_identifier == -1) return 0;
        scope = lookup_exception_scope(prepared_exception_scopes, scope->parent_identifier);
    }
    while (scope->identifier != (unsigned)target) {
        if (scope->kind == 2) {
            tree = new_scope_finally_unwind(target);
            insert_tree_expression(previous, before, tree);
            return tree;
        }
        if (scope->parent_identifier == -1) break;
        scope = lookup_exception_scope(prepared_exception_scopes, scope->parent_identifier);
    }
    tree = new_scope_unwind(target);
    insert_tree_expression(previous, before, tree);
    return tree;
}
