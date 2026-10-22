#include "tree-rewrite.h"
#include "tree-input.h"
#include "expression-fields.h"
#include "records.h"
extern int optimize_global;
/* Original [00051e20,000520d0). Expand statement-level forms before their
   operands are visited, and record uses of variable values. */
Node *__fastcall rewrite_before_children(Node *node, Node *parent, unsigned flags)
{
    Node *left = node->left.node, *right = node->second.node;
    Node *candidate, *inserted;
    SymbolHeader *symbol;
    switch (node->opcode) {
    case 0x26:
        if (!(flags & 4)) {
            symbol = node->right.symbol;
            if (symbol && symbol->kind == 1) {
                symbol->field0e |= 0x2000;
                ((FunctionSymbol *)symbol)->field24 |= 0x2000;
            }
        }
        break;
    case 0x38:
        if (left->opcode == 0x1b || left->opcode == 0x1c)
            return rewrite_short_circuit(node);
        break;
    case 0x3b:
        inserted = new_node(0x3a);
        inserted->right.symbol = node->right.symbol;
        splice_expressions("A", inserted);
        break;
    case 0x3c:
        inserted = new_node(0x29);
        inserted->right.symbol = left->right.symbol;
        splice_expressions("I", inserted);
        break;
    case 0x40:
        if (left->opcode == 0x33 && left->left.value == 0xdb)
            current_function->field28 |= 0x40;
        break;
    case 0x41:
        candidate = skip_node_conversions(left);
        if (candidate->opcode == 0x42 && !can_keep_conditional(candidate))
            return lower_conditional(candidate, node);
        if (candidate->opcode == 0x44) return lower_comma(candidate, node);
        break;
    case 0x42:
        if (!parent) return lower_conditional(node, 0);
        if (!can_keep_conditional(node)) return materialize_expression(node);
        break;
    case 0x44:
        if (parent && (node_shapes[right->opcode] & NODE_ARITY) != NODE_LEAF)
            return materialize_expression(node);
        return lower_comma(node, 0);
    case 0x4f:
        if (optimize_global && node->detail.control.code == 0x16) {
            inserted = extract_assembly_labels(node);
            if (inserted) splice_expressions("I", inserted);
        }
        break;
    case 0x68:
        if (right->opcode == 0x69 && !right->right.symbol) {
            release_node(right);
            release_node(node);
            return left;
        }
        if (right->attributes.words.type & 0xc00) {
            node->opcode = 2;
            node->left.node = convert_node_type(left, 0x204);
        }
        break;
    case 0x8b:
        node->opcode = 3;
        node->second.node = convert_node_type(right, 0x204);
        break;
    }
    return node;
}

/* Original [00051a40,00051e20). Preserve the sequencing of compound and
   post-increment assignments by splicing expression records when needed. */
Node *__fastcall rewrite_after_left(Node *node, Node *parent, unsigned flags)
{
    Node *left, *right, *temporary, *value, *first, *second, *candidate;
    unsigned operation, mapped;
again:
    left = node->left.node;
    right = node->second.node;
    switch (node->opcode) {
    case 0x35: case 0x36:
        operation = node->opcode == 0x35 ? 0x0f : 0x10;
        node->opcode = operation;
        if (!parent) goto ordinary_assignment;
        if (flags & 2) {
            temporary = new_temporary_reference(node->attributes.words.type & 255, 0x100);
            if (!(node->attributes.words.flags & 0x800)) {
                first = build_tree("DDNTNT", temporary, left, 0x30, node, 0x32, node);
                splice_expressions("II", first, node);
            } else {
                mapped = map_assignment_opcode(operation, 0);
                first = build_tree("PPNTPNYOLYB", left, temporary, 0x30, node,
                                   right, mapped, 0x32, node, 0x800);
                second = build_tree("DDNfNT", temporary, left, 0x30, node, 0x32, node);
                splice_expressions("II", second, first);
            }
        } else if (!(node->attributes.words.flags & 0x800)) {
            temporary = copy_tree(left);
            splice_expressions("A", node);
        } else {
            temporary = new_temporary_reference(node->attributes.words.type & 255, 0x100);
            mapped = map_assignment_opcode(operation, 0);
            first = build_tree("PPNTPNYOLY", left, temporary, 0x30, node, right, mapped, 0x32, node);
            second = build_tree("DDNfNT", temporary, left, 0x30, node, 0x32, node);
            splice_expressions("IA", second, first);
        }
        node = build_tree("PNT", temporary, 0x30, node);
        goto again;
    case 0x3e: case 0x40:
        if ((flags & 1) || ((flags & 8) && node->opcode == 0x40 && left->left.value == 0xd8)) {
            node = materialize_expression(node);
            goto again;
        }
        break;
    case 0x0f: case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x32:
        if (!parent) goto ordinary_assignment;
        if (!(node->attributes.words.flags & 0x800)) {
            value = assignment_value(node);
            splice_expressions("I", node);
            node = value;
            goto again;
        }
        temporary = new_temporary_reference(node->attributes.words.type & 255, 0x100);
        if (left->opcode == 0x37) {
            value = clone_node(left);
            temporary = build_tree("PDL", temporary, left->second.node, value);
        }
        if (node->opcode == 0x32)
            first = build_tree("DPNT", temporary, right, 0x32, node);
        else {
            mapped = map_assignment_opcode(node->opcode, 0);
            value = assignment_value(node);
            first = build_tree("DPPNYNTY", temporary, value, right, mapped, 0x32, node);
        }
        splice_expressions("I", first);
        node->opcode = 0x32;
        first = build_tree("PDNTL", left, temporary, 0x30, node, node);
        splice_expressions("I", first);
        node = build_tree("PNT", temporary, 0x30, node);
        goto again;
    case 0x30:
        if (left->opcode == 0x37) lower_bitfield_reference(node);
        break;
    case 0x86:
        release_node(node);
        if (left->attributes.words.type == 0x404) {
            release_tree(left);
            node = intern_leaf(0x69, 0, lookup_global_symbol(2), 0x202);
        } else node = lower_target_address(left);
        break;
    }
    return node;

ordinary_assignment:
    candidate = skip_node_conversions(right);
    if (candidate->opcode == 0x42 && !can_keep_conditional(candidate))
        return lower_conditional(candidate, node);
    if (candidate->opcode == 0x44) {
        node = lower_comma(candidate, node);
        goto again;
    }
    if (node->opcode != 0x32) {
        mapped = map_assignment_opcode(node->opcode, 0);
        build_tree("DPNfPNYOLY", left, left, 0x30, node, right, mapped, 0x32, node);
    }
    if (left->opcode == 0x37) {
        lower_bitfield_assignment(node);
        if (node->opcode == 0x44) return rewrite_subtree(node, parent, flags);
    }
    return node;
}
