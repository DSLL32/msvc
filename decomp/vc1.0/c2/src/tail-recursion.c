/* Tail-call rewriting before global work blocks are built. */
#include "tree-rewrite.h"
#include "function-input.h"
#include "expression-fields.h"
#include "instruction-selection.h"
#include <recon.h>
SymbolHeader *__fastcall tail_argument_address(Node *node);
int __fastcall is_tail_call_position(Expression *expressions, Expression *call);

Expression *__fastcall find_tail_label(Expression *expressions, SymbolHeader *label);

/* Original587a0..58805, fastcall ECX tree/RET0. Ignore addresses only
   consumed by a load, call or assignment destination; reject the first symbol
   whose address escapes in an argument value. The binary child is followed
   iteratively, preserving the original left-first recursive call graph. */
SymbolHeader *__fastcall tail_argument_address(Node *node)
{
    SymbolHeader *symbol;
    for (;;) {
        switch (node->opcode) {
        case 0x26:
            return node->right.symbol;
        case 0x30: case 0x3c: case 0x3e:
            return 0;
        case 0x32:
            node = node->second.node;
            continue;
        default:
            if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF)
                return 0;
            symbol = tail_argument_address(node->left.node);
            if (symbol) return symbol;
            if (!(node_shapes[node->opcode] & NODE_HAS_SECOND)) return 0;
            node = node->second.node;
            break;
        }
    }
}

/* Original58810..58888, fastcall ECX list/EDX call, RET0. A direct return
   is already final. Otherwise only no-ops, labels, source markers and jumps
   may intervene before the function-end marker. The diagnostic never returns
   on original valid execution; retain its original missing-result error tail. */
int __fastcall is_tail_call_position(Expression *expressions, Expression *call)
{
    Expression *next;
    if (call->tree->opcode == 0x41) return 1;
    for (next = call->next; next; next = next->next) {
        if (next == call) return 0;
        switch (next->tree->opcode) {
        case 1: case 0x29: case 0x4f:
            break;
        case 0x3a:
            next = find_tail_label(expressions, next->tree->right.symbol);
            break;
        case 0x47:
            return 1;
        default:
            return 0;
        }
    }
    compiler_internal_error("@(#)misc.c:1.100", 0x22e);
}

/* Original [00058890,00058b20). Preserve all argument values in temporary
   storage, assign incoming parameters, and replace a self call with a branch
   to a shared label following the function's first block marker. */
void __fastcall optimize_function_flow(Function *function)
{
    Expression *expression, *next, *marker;
    Node *entry = function->root, *call, *arguments, *argument, *parameter;
    Node *temp, *store, *reference, *branch;
    TemporarySymbol *symbol;
    LabelSymbol *label = 0;
    unsigned bytes;
    for (expression = function->expressions; expression; expression = next) {
        next = expression->next;
        call = expression->tree;
        if (call->opcode == 0x41) call = call->left.node;
        if (call->opcode != 0x3e || call->left.node->opcode != 0x26 ||
            call->left.node->right.symbol != function->symbol ||
            !is_tail_call_position(function->expressions, expression)) continue;
        arguments = call->second.node;
        argument = arguments;
        parameter = entry->second.node;
        while (argument->opcode != 0x4c && parameter->opcode != 0x4c &&
            !tail_argument_address(argument->left.node)) {
            argument = argument->second.node;
            parameter = parameter->second.node;
        }
        if (argument->opcode != 0x4c || parameter->opcode != 0x4c) continue;
        if (!label) label = new_internal_label();
        entry->second.node = reverse_node_list(entry->second.node);
        parameter = entry->second.node;
        for (argument = arguments; argument->opcode != 0x4c;
             argument = argument->second.node, parameter = parameter->second.node) {
            symbol = (TemporarySymbol *)parameter->right.symbol;
            bytes = symbol->symbol.value;
            temp = new_temporary_reference(bytes, 0x200);
            argument->attributes.words.type = value_node_type((short)(symbol->attributes & 15), bytes);
            store = build_tree("PPNtYS", temp, argument->left.node, 0x32,
                argument->attributes.words.type, temp);
            argument->left.node = build_tree("PNtS", temp, 0x30, argument->attributes.words.type);
            insert_tree_expression(function->expressions, expression->tree, store);
        }
        parameter = entry->second.node;
        for (argument = arguments; argument->opcode != 0x4c;
             argument = argument->second.node, parameter = parameter->second.node) {
            reference = intern_leaf(0x26, 0, parameter->right.symbol, 0, 0x404);
            store = build_tree("PPNtYS", reference, argument->left.node, 0x32,
                argument->attributes.words.type, reference);
            argument->left.node = new_node(1);
            insert_tree_expression(function->expressions, expression->tree, store);
        }
        release_tree(expression->tree);
        branch = new_node(0x3a);
        expression->tree = branch;
        branch->right.symbol = &label->symbol;
        entry->second.node = reverse_node_list(entry->second.node);
    }
    if (label) {
        for (marker = function->expressions;
             marker->tree->opcode != 0x4f || marker->tree->detail.control.code != 0xf;
             marker = marker->next) ;
        branch = new_node(0x29);
        branch->right.symbol = &label->symbol;
        insert_tree_expression(function->expressions, marker->next->tree, branch);
    }
}
