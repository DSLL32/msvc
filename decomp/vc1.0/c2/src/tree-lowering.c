#include "tree-rewrite.h"
#include "tree-input.h"
#include "expression-fields.h"
#include "records.h"
#include "algebra.h"
#include "flow-edges.h"
#include <recon.h>
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

/* Original [00052180,000521bc). Move the left side into a preceding
   expression, then preserve the enclosing value form around the right side. */
Node *__fastcall lower_comma(Node *node, Node *container)
{
    Node *value;
    splice_expressions("I", copy_tree(node->left.node));
    value = copy_value_into_container(node->second.node, container);
    if (!container) container = node;
    release_tree(container);
    return value;
}

/* Original [00052460,000524d3). Copy a value into the assignment or
   return-like expression which encloses it, preserving the original shell. */
Node *__fastcall copy_value_into_container(Node *node, Node *container)
{
    if (!container) return copy_tree(node);
    if (node_shapes[container->opcode] & NODE_WRITES_LEFT)
        return build_tree("DDLY", container->left.node, node, clone_node(container));
    if (container->opcode == 0x41)
        return build_tree("DLY", node, clone_node(container));
    compiler_internal_error("@(#)canon.c:1.182", 0x59c);
}

/* Original [000514d0,00051509). Boolean and comparison operators already
   produce truth values; every other scalar is compared with a typed zero. */
Node *__fastcall make_truth_test(Node *node)
{
    if ((node->opcode >= 0x1a && node->opcode <= 0x1c) ||
        (node->opcode >= 0x1f && node->opcode <= 0x24)) return node;
    return build_tree("PPNY", node, typed_integer_node(node, 0), 0x20);
}

/* Original [00051630,000516ae). Remove double negation, apply De Morgan
   to short-circuit pairs, or complement an ordinary comparison opcode. */
Node *__fastcall negate_condition(Node *node)
{
    Node *left = node->left.node, *right = node->second.node;
    unsigned opcode = node->opcode;
    switch (opcode) {
    case 0x1a:
        release_node(node);
        return left;
    case 0x1b: case 0x1c:
        right = negate_condition(right);
        left = negate_condition(left);
        return build_tree("PPOL", left, right, opcode == 0x1b ? 0x1c : 0x1b, node);
    default:
        node->opcode = map_comparison_opcode(opcode, 1);
        return node;
    }
}

/* Original [00051510,00051562). Keep predicates in a Boolean control
   context; materialize all other uses as a typed conditional yielding1/0. */
Node *__fastcall materialize_boolean(Node *node, Node *parent)
{
    Node *zero, *one;
    if (parent && ((parent->opcode >= 0x1a && parent->opcode <= 0x1c) ||
                   parent->opcode == 0x38 || parent->opcode == 0x42)) return node;
    zero = integer_node(0, 0x104);
    one = integer_node(1, 0x104);
    return build_tree("PYPPNYNY", node, one, zero, 0x43, 0x42);
}

/* Original [00052390,000523e3). Produce the value of an assignment
   without evaluating its store again. Bitfield constants are width-masked. */
Node *__fastcall assignment_value(Node *node)
{
    Node *value = constant_assignment_value(node);
    unsigned width;
    if (value->opcode == 0x33) {
        width = node->left.node->second.node->second.node->left.value;
        return typed_integer_node(node, value->left.value &
            (~0U >> ((32 - width) & 31)));
    }
    value = build_tree("DNT", node->left.node, 0x30, node);
    if (node->attributes.words.flags & 0x800) value->attributes.words.flags |= 0x800;
    return value;
}

/* Original [000523f0,00052455). Look through bitfield assignments and
   explicit conversions for a constant which can be used as the stored value. */
Node *__fastcall constant_assignment_value(Node *node)
{
    Node *value;
    unsigned width;
    if (node->opcode == 0x32 && node->left.node->opcode == 0x37) {
        value = constant_assignment_value(node->second.node);
        if (value->opcode == 0x33) {
            width = node->left.node->second.node->second.node->left.value;
            return typed_integer_node(node, value->left.value &
                (~0U >> ((32 - width) & 31)));
        }
    } else if (node->opcode == 0x34) {
        value = constant_assignment_value(node->left.node);
        if (value->opcode == 0x33) return fold_constant_node(value);
    }
    return node;
}

/* Original [000513c0,000514cf). Recognize comparison/conditional trees
   which select their compared operands (max/min), or a signed absolute value. */
Node *__fastcall canonicalize_conditional(Node *node)
{
    Node *condition = node->left.node, *choices = node->second.node;
    unsigned opcode = condition->opcode;
    if (opcode < 0x21 || opcode > 0x24) return node;
    if (opcode <= 0x22) {
        build_tree("PPL", choices->second.node, choices->left.node, choices);
        condition->opcode = map_comparison_opcode(condition->opcode, 1);
    }
    if (trees_equal(condition->left.node, choices->left.node, 1) &&
        trees_equal(condition->second.node, choices->second.node, 1))
        condition->opcode = 0xae;
    else if (trees_equal(condition->left.node, choices->second.node, 1) &&
             trees_equal(condition->second.node, choices->left.node, 1))
        condition->opcode = 0xaf;
    else if (is_constant_value(condition->second.node, 0) &&
             choices->second.node->opcode == 8 &&
             trees_equal(condition->left.node, choices->left.node, 1) &&
             trees_equal(condition->left.node, choices->second.node->left.node, 1)) {
        release_node(condition->second.node);
        condition->opcode = 0x84;
    } else return node;
    release_tree(choices);
    release_node(node);
    return condition;
}

/* Original [000521f0,00052233). Only comparisons with two leaf choices
   fit the retained conditional form. Original shape bit0010 is set exactly
   for the six relation operators1f..24 (all182 entries checked). */
int __fastcall can_keep_conditional(Node *node)
{
    Node *choices = node->second.node;
    if ((node_shapes[choices->left.node->opcode] & NODE_ARITY) != NODE_LEAF) return 0;
    if ((node_shapes[choices->second.node->opcode] & NODE_ARITY) != NODE_LEAF) return 0;
    return node->left.node->opcode >= 0x1f && node->left.node->opcode <= 0x24;
}

/* Original [00052240,000522e2). Expand a conditional into five ordered
   expressions, with a false-branch label and a shared completion label. */
Node *__fastcall lower_conditional(Node *node, Node *container)
{
    Node *choices = node->second.node;
    Node *condition, *false_label, *done_label, *jump, *when_true, *when_false;
    condition = build_tree("DN", node->left.node, 0x38);
    false_label = new_flow_label_node();
    condition->right.symbol = false_label->right.symbol;
    jump = new_node(0x3a);
    done_label = new_flow_label_node();
    jump->right.symbol = done_label->right.symbol;
    when_true = copy_value_into_container(choices->left.node, container);
    when_false = copy_value_into_container(choices->second.node, container);
    splice_expressions("IIIII", condition, when_true, jump, false_label, when_false);
    if (!container) container = node;
    release_tree(container);
    return done_label;
}

/* Original836a0..836f0, owned by the bidirectional operator mapper. */
static const struct AssignmentOpcodePair {
    unsigned operation, assignment;
} assignment_opcode_pairs[] = {
    {2,15},   /* addition */
    {3,16},   /* subtraction */
    {4,17},   /* multiplication */
    {5,18},   /* division */
    {6,19},   /* remainder */
    {9,21},   /* left shift */
    {10,22},  /* right shift */
    {11,23},  /* bitwise AND */
    {12,25},  /* bitwise OR */
    {13,24}   /* bitwise XOR */
};
/* Original [000583e0,00058425). A nonzero direction maps an operation to
   compound assignment; zero recovers the operation from its assignment. */
unsigned __fastcall map_assignment_opcode(unsigned opcode, int reverse)
{
    unsigned i;
    for (i = 0; i < sizeof(assignment_opcode_pairs) / sizeof(assignment_opcode_pairs[0]); ++i) {
        if (opcode == (reverse ? assignment_opcode_pairs[i].operation :
                                assignment_opcode_pairs[i].assignment))
            return reverse ? assignment_opcode_pairs[i].assignment :
                             assignment_opcode_pairs[i].operation;
    }
    compiler_internal_error("@(#)misc.c:1.100", 0x109);
    return 1;
}

/* Original [000590b0,00059184). Lower a bitfield read to its containing
   storage conversion followed by shifts, with unsigned fields masked and
   signed fields sign-extended by the final arithmetic right shift. The
   builder recipe reuses the released field/range nodes in the original
   order; save constant values before returning those nodes to the pool. */
void __fastcall lower_bitfield_reference(Node *node)
{
    Node *field = node->left.node;
    Node *range = field->second.node;
    Node *value;
    unsigned bits = (node->attributes.words.type & 255) * 8;
    unsigned width, offset, shift;
    if ((node->attributes.words.type & 0xff00) == 0x200) {
        value = range->second.node;
        width = value->left.value;
        shift = ~0U >> ((32 - width) & 31);
        release_node(value);
        value = typed_integer_node(node, shift);
        build_tree("POLfPOLYPOLY", field->left.node, 0x30, range, node,
            range->left.node, 0xa, field, value, 0xb, node);
    } else {
        value = range->second.node;
        width = value->left.value;
        release_node(value);
        value = range->left.node;
        offset = value->left.value;
        release_node(value);
        shift = bits - width;
        value = typed_integer_node(node, shift);
        build_tree("POLfPOLYPOLY", field->left.node, 0x30, range, node,
            typed_integer_node(node, shift - offset), 9, field, value, 0xa, node);
    }
}

/* Original [00059190,000592be). Replace a field update by a masked
   storage merge. The flagged form retains the two compound updates and
   their sequencing node; the ordinary form combines one read and write.
   Preserve builder allocation order and the original field/range reuse. */
void __fastcall lower_bitfield_assignment(Node *node)
{
    Node *field = node->left.node;
    Node *range = field->second.node;
    Node *width = range->second.node;
    Node *clear, *value, *mask_node, *converted;
    unsigned mask = ~0U >> ((32 - width->left.value) & 31);
    unsigned offset;
    release_node(width);
    offset = range->left.node->left.value & 31;
    mask_node = typed_integer_node(node, ~(mask << offset));
    if (node->attributes.words.flags & 0x800) {
        clear = build_tree("PPOLY", field->left.node, mask_node, 0x17, field);
        mask_node = typed_integer_node(node, mask);
        converted = convert_node_type(node->second.node, node->attributes.words.type);
        value = build_tree("DPPNYPOLYNY", field->left.node, converted,
            mask_node, 0xb, range->left.node, 9, range, 0x19);
        build_tree("PfPfOLY", clear, node, value, node, 0x44, node);
    } else {
        clear = build_tree("DNfPNY", field->left.node, 0x30, node, mask_node, 0xb);
        mask_node = typed_integer_node(node, mask);
        converted = convert_node_type(node->second.node, node->attributes.words.type);
        value = build_tree("PPNYPOLY", converted, mask_node, 0xb,
            range->left.node, 9, range);
        build_tree("PPPOLYL", field->left.node, clear, value, 0xc, field, node);
    }
}

/* Original521c0..521eb, ECX expression/RET0. Preserve the expression's
   representation on its assignment while giving the result a private stack
   temporary. The builder consumes the original expression as the right arm. */
Node *__fastcall materialize_expression(Node *node)
{
    Node *temporary = new_temporary_reference(node->attributes.words.type & 255, 0x200);
    return build_tree("PPNT", temporary, node, 0x32, node);
}
