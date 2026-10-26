#include "tree-rewrite.h"
#include "expression-fields.h"
#include "function-input.h"
#include "records.h"
#include <recon.h>
extern int floating_frontend;

/* Original [000503b0,00050e30), including both opcode/convention tables.
   Rewrite children already visited by canonicalize_node. Aggregate values
   retain their separate byte count until their representation is selected. */
Node *__fastcall canonicalize_after_children(Node *node, Node *parent)
{
    Node *left = node->left.node, *right = node->second.node;
    Node *value, *argument, *temporary, *constant, *callee;
    Node **link;
    SymbolHeader *symbol;
    unsigned short type, saved_type;
    unsigned used_registers, bytes;
    int convention;
    if ((node->attributes.words.type & 0xff00) == 0x8000) {
        if (!floating_frontend && is_register_return_size(node->info.value))
            node->attributes.words.type = (unsigned short)(node->info.value | 0x8000);
        if (parent && parent->opcode != 0x27 && node->opcode != 0x55 &&
            node->opcode != 0x57 && !(parent->opcode == 0x44 && parent->left.node == node)) {
            parent->attributes.words.type = node->attributes.words.type;
            parent->info.value = node->info.value;
        }
    }
    switch (node->opcode) {
    case 0x1a:
        release_node(node);
        value = make_truth_test(left);
        value = negate_condition(value);
        return materialize_boolean(value, parent);
    case 0x1b: case 0x1c:
        if (left->opcode == 0x32) compiler_warning(4, 0x2c2);
        if (right->opcode == 0x32) compiler_warning(4, 0x2c2);
        value = simplify_short_circuit(node, parent);
        if (value) return value;
        node->left.node = make_truth_test(left);
        node->second.node = make_truth_test(right);
        return materialize_boolean(node, parent);
    case 0x1f: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24:
        return materialize_boolean(node, parent);
    case 0x26:
        symbol = node->right.symbol;
        if (symbol->kind != 0x13 && ((TemporarySymbol *)symbol)->attributes & 0x1000)
            return build_tree("PNt", node, 0x30, node->attributes.words.type);
        if (symbol->kind == 1 && (symbol->field0e & 0x1000)) {
            constant = integer_node(0xf3, 0x102);
            value = build_tree("PPNt", intern_leaf(0x4c), constant, 0x7a, 0x104);
            node = build_tree("PPNt", value, node, 2, 0x404);
            lower_intrinsic(value, node);
        }
        break;
    case 0x27:
        if ((left->attributes.words.type & 0xff00) == 0x8000 && !floating_frontend &&
            is_register_return_size(left->info.value)) {
            temporary = new_storage_reference(left->attributes.words.type & 255);
            build_tree("PPNTDNYPL", temporary, left, 0x32, left,
                       temporary, 0x44, right, node);
        }
        /* The aggregate operation is represented by the ordinary binary op
           after its storage has been made explicit. */
    case 0x28:
        node->opcode = 2;
    case 2:
        if (node->second.node->opcode == 0x68 &&
            node->second.node->attributes.words.type != 0x404) {
            node = lift_right_sequence(node);
            node->left.node = finish_canonical_node(node->left.node);
            return node;
        }
    case 3:
        if (node->left.node->opcode == 0x68 &&
            !(node->second.node->attributes.words.type & 0x400)) {
            node = lift_left_sequence(node);
            node->left.node = finish_canonical_node(node->left.node);
        }
        break;
    case 0x30:
        if ((node->attributes.words.type & 0xff00) == 0x8000 &&
            (floating_frontend || !is_register_return_size(node->info.value))) {
            release_node(node);
            return left;
        }
        break;
    case 0x32:
        if (node->attributes.words.type != 0x8000) break;
        if (!floating_frontend && is_register_return_size(node->info.value)) break;
        if (node->left.node->opcode == 0x68) node->left.node->attributes.words.type = 0x408;
        if (node->second.node->opcode == 0x68) node->second.node->attributes.words.type = 0x408;
        callee = integer_node(0xac, 0x104);
        constant = integer_node(node->info.value, 0x104);
        build_tree("PPPPPOLY", intern_leaf(0x4c), left, right, constant, callee, 0x40, node);
        node->attributes.words.type = 0x400;
        node = lower_intrinsic(node, parent);
        break;
    case 0x38:
        if (left->opcode == 0x32) compiler_warning(4, 0x2c2);
        node->left.node = make_truth_test(left);
        break;
    case 0x3e:
        convention = node->detail.control.code;
        switch (convention) {
        case 0: case 4: case 12: case 16: case 20:
            break;
        case 8:
            node->second.node = reverse_node_list(node->second.node);
            break;
        default:
            compiler_internal_error("@(#)canon.c:1.182", 0x27b);
            break;
        }
        node = canonicalize_function_reference(node);
        if (node->detail.control.code == 4 || node->detail.control.code == 20) {
            used_registers = 0;
            node->second.node = reverse_node_list(node->second.node);
            for (argument = node->second.node; argument->opcode == 0x55;
                 argument = argument->second.node)
                assign_parameter_register(&used_registers, argument, node->detail.control.code);
            node->second.node = reverse_node_list(node->second.node);
        }
        type = node->attributes.words.type;
        bytes = type == 0x8000 ? node->info.value : type & 255;
        if (!needs_return_temp(node->detail.control.code, node_type_kind(type), bytes)) break;
        bytes = aligned_value_size(node);
        argument = build_tree("PNt", new_storage_reference(bytes), 0x55, 0x404);
        link = &node->second.node;
        while ((*link)->opcode != 0x4c) link = &(*link)->second.node;
        argument->second.node = *link;
        *link = argument;
        type = node->attributes.words.type;
        if (type == 0x8000 && !floating_frontend && is_register_return_size(node->info.value))
            compiler_internal_error("@(#)canon.c:1.182", 0x2ab);
        value = node;
        if (type & 0x1000) node = build_tree("PNt", node, 0x30, type);
        value->attributes.words.type = 0x404;
        break;
    case 0x40: case 0x7a:
        node = lower_intrinsic(node, parent);
        break;
    case 0x42:
        if (left->opcode == 0x32) compiler_warning(4, 0x2c2);
        node->left.node = make_truth_test(left);
        right = node->second.node;
        if (right->opcode == 0x68 && right->attributes.words.type != 0x404) {
            node->second.node = right->left.node;
            value = finish_canonical_node(node);
            right->left.node = canonicalize_conditional(value);
            node = right;
        } else node = canonicalize_conditional(node);
        break;
    case 0x43:
        if (node->left.node->opcode != 0x68 || node->second.node->opcode != 0x68 ||
            node->second.node->attributes.words.type == 0x404 ||
            left->second.node->opcode == 0x85 || !trees_equal(left->second.node, right->second.node, 0))
            break;
        node = build_tree("PPLPL", left->left.node, right->left.node, node, right->second.node, right);
        release_tree(left->second.node);
        release_node(left);
        node->left.node = finish_canonical_node(node->left.node);
        break;
    case 0x44:
        if ((node->attributes.words.type & 0xff00) == 0x8000 &&
            (floating_frontend || !is_register_return_size(node->info.value))) {
            temporary = new_storage_reference(node->info.value);
            value = build_tree("PPN", temporary, right, 0x32);
            value->attributes.words.type = 0x8000;
            value->info.value = node->info.value;
            node->second.node = canonicalize_after_children(value, node);
        } else if (node->second.node->opcode == 0x68 &&
                   node->second.node->attributes.words.type != 0x404) {
            node = lift_right_sequence(node);
            node->left.node = finish_canonical_node(node->left.node);
        }
        break;
    case 0x55:
        type = node->attributes.words.type;
        if ((type & 0xff00) == 0x8000 &&
            (floating_frontend || !is_register_return_size(node->info.value)) &&
            !(left->opcode == 0x40 &&
              (left->left.node->left.value == 0xdf || left->left.node->left.value == 0xde))) {
            callee = integer_node(0xb3, 0x104);
            constant = integer_node(node->info.value, 0x104);
            build_tree("PPPPNYL", intern_leaf(0x4c), constant, left, callee, 0x40, node);
            node->left.node->attributes.words.type = 0x8000;
            node->attributes.words.type = node->left.node->attributes.words.type;
            node->left.node = lower_intrinsic(node->left.node, node);
        } else if (!type && left->opcode == 0x68) {
            saved_type = left->attributes.words.type;
            value = copy_tree(left->second.node);
            node->left.node = build_tree("PPNt", left, value, 0x8b, saved_type);
            node->attributes.words.type = node->left.node->attributes.words.type;
        }
        break;
    case 0x8a:
        node->opcode = 0x68;
        node->attributes.words.type = 0x408;
        /* Continue with the same simplifications as an ordinary sequence. */
    case 0x68:
        if (right->opcode == 0x85 && left->opcode == 0x30) {
            value = left->left.node;
            if (value->attributes.words.type == 0x404) {
                release_node(right);
                symbol = lookup_global_symbol(2);
                node->second.node = intern_leaf(0x69, 0, symbol);
                break;
            }
            if (value->opcode == 2) left = value;
            value = left->left.node;
            if (value->opcode == 0x68 && value->second.node->opcode == 0x69) {
                value = value->second.node;
                node->second.node = intern_leaf(0x69, value->attributes.words.flags, value->right.symbol);
                release_node(right);
                break;
            }
        }
        left = node->left.node;
        if (left->opcode == 0x68) {
            release_tree(left->second.node);
            node->left.node = left->left.node;
            release_node(left);
        }
        break;
    case 0x8b:
        if (left->opcode == 0x68 && left->second.node->opcode == 0x69 &&
            !left->second.node->right.pointer) {
            release_tree(right);
            release_node(node);
            node = left;
        }
        break;
    }
    return node;
}

/* Original51380..5139f. Lift the left offset/selector pair around its
   binary operation, reusing both owners. The caller types the new offset;
   this recipe deliberately has no Y/type-finishing step of its own. */
Node *__fastcall lift_left_sequence(Node *node)
{
    Node *left = node->left.node;
    return build_tree("PPLPL", left->left.node, node->second.node, node,
                      left->second.node, left);
}

/* Original513a0..513bf. Preserve the right pair's selector while moving
   its offset into the existing binary owner. Type finishing belongs to the
   caller, just as for the separate left-pair operation. */
Node *__fastcall lift_right_sequence(Node *node)
{
    Node *right = node->second.node;
    return build_tree("PPLPL", node->left.node, right->left.node, node,
                      right->second.node, right);
}
