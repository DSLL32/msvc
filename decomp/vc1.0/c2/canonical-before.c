#include "tree-rewrite.h"
#include "tree-input.h"
#include "expression-fields.h"
#include "records.h"
unsigned function_tree_state; /* 0007e81c */
extern int floating_frontend;
/* Original [00050e30,00051130), including the convention dispatch table. */
Node *__fastcall canonicalize_before_children(Function *function, Node *node, Node *parent)
{
    Node *left = node->left.node, *right = node->second.node;
    Node *storage, *prefix, *zero1, *zero2;
    unsigned intrinsic, convention;
    unsigned short type;
    if (node_shapes[node->opcode] & NODE_SIDE_EFFECT) expression_side_effects |= 2;
    switch (node->opcode) {
    case 0x28:
        if (right->opcode == 0x44 || (right->opcode == 4 && right->left.node->opcode == 0x44))
            compiler_warning(4, 0x2c5);
        break;
    case 0x2c:
        node->opcode = 0x34;
        node->attributes.words.flags |= 0x4000;
        break;
    case 0x39:
        return build_tree("PNOL", left, 0x1a, 0x38, node);
    case 0x40:
        intrinsic = left->left.value;
        /* The call-class bit in the valid intrinsic descriptors. */
        if ((intrinsic >= 1 && intrinsic <= 6) ||
            (intrinsic >= 0x0d && intrinsic <= 0x18) ||
            (intrinsic >= 0x1c && intrinsic <= 0x1d) ||
            (intrinsic >= 0x21 && intrinsic <= 0x22) || intrinsic == 0x47 ||
            (intrinsic >= 0x49 && intrinsic <= 0x4a) ||
            (intrinsic >= 0x4e && intrinsic <= 0x4f) ||
            (intrinsic >= 0x58 && intrinsic <= 0x8b) || intrinsic == 0xae ||
            (intrinsic >= 0xb4 && intrinsic <= 0xb5) ||
            (intrinsic >= 0xc3 && intrinsic <= 0xd6) ||
            (intrinsic >= 0xd9 && intrinsic <= 0xda) ||
            (intrinsic >= 0xec && intrinsic <= 0xef) || intrinsic == 0xf3 || intrinsic == 0xf5)
            node->opcode = 0x7a;
        break;
    case 0x41:
        node = canonicalize_return_value(function, node);
        break;
    case 0x46:
        reset_temporary_registers();
        function_tree_state = 0;
        node->right.symbol = left->right.symbol;
        convention = left->right.symbol->field0e & 0x1c;
        switch (convention) {
        case 0: case 4: case 12: case 16: case 20:
            node->second.node = reverse_node_list(node->second.node);
            break;
        case 8:
            break;
        default:
            compiler_internal_error("@(#)canon.c:1.182", 0x141);
            break;
        }
        insert_return_parameter(function, node);
        node = canonicalize_function_reference(node);
        break;
    case 0x68:
        if (right->opcode == 0x86 && right->left.node->attributes.words.type == 0x404) {
            release_tree(right);
            release_node(node);
            return canonicalize_before_children(function, left, parent);
        }
        break;
    case 0x77:
        if (is_constant_value(right, 0)) {
            release_node(node);
            release_node(right);
            return canonicalize_before_children(function, left, parent);
        }
        type = node->attributes.words.type;
        if (can_repeat_tree(left)) prefix = 0;
        else {
            storage = new_storage_reference(type & 255);
            left = build_tree("PNT", storage, 0x30, node);
            prefix = build_tree("PPNT", storage, node->left.node, 0x32, node);
        }
        release_node(node);
        zero1 = intern_leaf(0x33, 0, type, 0);
        zero2 = intern_leaf(0x33, 0, type, 0);
        node = build_tree("PPNDPNPNN", left, zero2, 0x20, left, right, 2, zero1, 0x43, 0x42);
        if (prefix) node = build_tree("PPN", prefix, node, 0x44);
        break;
    }
    return node;
}

/* Original [000512a0,000512d0). Prepend the hidden return pointer when one
   was allocated while the function body was read. */
void __fastcall insert_return_parameter(Function *function, Node *node)
{
    Node *parameter;
    if (function->return_temp) {
        parameter = new_node(0x2d);
        parameter->right.symbol = function->return_temp;
        parameter->second.node = node->second.node;
        node->second.node = parameter;
    }
}

/* Original [000512d0,00051380). Return an aggregate through the caller's
   hidden pointer, preserving the expression's value for the return record. */
Node *__fastcall canonicalize_return_value(Function *function, Node *node)
{
    FunctionSymbol *symbol = (FunctionSymbol *)function->symbol;
    SymbolHeader *result;
    Node *value;
    if (!needs_return_temp(symbol->symbol.field0e & 0x1c,
                           symbol->field24 & 15, symbol->field28)) return node;
    result = function->return_temp;
    if (!result) compiler_internal_error("@(#)canon.c:1.182", 0x6af);
    value = intern_leaf(0x26, 0, result, 0);
    value = build_tree("PNt", value, 0x30, 0x404);
    if (floating_frontend) release_tree(node->left.node);
    else value = build_tree("PPNTDNY", value, node->left.node, 0x32, node, value, 0x44);
    build_tree("PLT", value, node, value);
    return node;
}
