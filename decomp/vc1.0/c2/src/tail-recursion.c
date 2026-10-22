/* Tail-call rewriting before global work blocks are built. */
#include "tree-rewrite.h"
#include "function-input.h"
#include "expression-fields.h"
#include "instruction-selection.h"
SymbolHeader *__fastcall tail_argument_address(Node *node);
int __fastcall is_tail_call_position(Expression *expressions, Expression *call);

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
