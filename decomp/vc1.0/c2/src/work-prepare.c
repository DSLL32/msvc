#include "work-prepare.h"
#include "work-optimizer.h"
#include "tree-rewrite.h"
#include "tree-input.h"
#include "expression-fields.h"
#include <recon.h>
extern int floating_frontend;
extern int optimize_global;
int tree_uses_floating; /* 00084f60 */
/* Original [000675c0,000677e0). Select final scalar representations and
   lower the remaining intrinsic/annotation records before instruction selection. */
Node *__fastcall lower_machine_tree(Node *node)
{
    Node *old, *value, *comparison, *intrinsic;
    unsigned opcode, bytes;
    unsigned short type;
    if (node->attributes.words.type & 0x1000) tree_uses_floating = 1;
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return node;
    node->left.node = lower_machine_tree(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        node->second.node = lower_machine_tree(node->second.node);
    opcode = node->opcode;
    if (opcode == 7) {
        type = node->attributes.words.type;
        if (type & 0x1000) {
            old = node;
            value = integer_node(0xae, 0x102);
            node = build_tree("NPPPNt", 0x4c, node->second.node, node->left.node, value, 0x7a, type);
            release_node(old);
            prepare_generated_intrinsic(node, 1);
        }
        return node;
    }
    if (opcode == 0x38 || opcode == 0x42) {
        comparison = node->left.node;
        intrinsic = comparison->left.node;
        if (intrinsic->opcode == 0x40 && intrinsic->left.node->left.value == 0xaa &&
            comparison->opcode >= 0x1f && comparison->opcode <= 0x20 &&
            comparison->second.node->opcode == 0x33 && !comparison->second.node->left.value)
            intrinsic->left.node = intern_leaf(0x33, 0, 0x104, 0xab);
        return node;
    }
    if (opcode == 0x40) {
        if (node->left.node->left.value == 0xf0) {
            release_tree(node);
            return new_node(1);
        }
        return node;
    }
    if (opcode == 0x59) {
        value = node->left.node;
        release_node(node);
        return value;
    }
    if (opcode == 0x7a) {
        if (node->left.node->left.value == 0xf3) node->opcode = 0x40;
        return node;
    }
    if (opcode == 0x55 && node->left.node->opcode == 0x40) {
        bytes = node->left.node->left.node->left.value;
        if (bytes == 0xdf || bytes == 0xb3) return node;
    }
    if (opcode == 0x30 || opcode == 0x32 || opcode == 0x3e || opcode == 0x41 || opcode == 0x55) {
        type = node->attributes.words.type;
        if ((type & 0xff00) == 0x8000 && !floating_frontend) {
            bytes = type & 255;
            if (is_register_return_size(bytes)) node->attributes.words.type = (unsigned short)(bytes + 0x200);
        }
    }
    return node;
}
/* Original [00067350,000673a0). Final tree lowering records floating-point
   operands while walking each expression, then runs the floating pass. */
void __fastcall finalize_work_trees(WorkItem *work)
{
    Expression *expression;
    for (; work; work = work->next) {
        for (expression = work->body.expressions; expression; expression = expression->next) {
            tree_uses_floating = 0;
            expression->tree = lower_machine_tree(expression->tree);
            if (tree_uses_floating) {
                lower_float_expression(expression->tree);
                finish_target_expression();
            }
        }
    }
}
/* Original [00066e30,00066f10). Allocate parameter storage before cleaning
   blocks, then make assignments and expression-stack records explicit. */
void __fastcall prepare_work(WorkItem *work)
{
    WorkItem *block;
    Expression *expression, *next;
    Node *node, *value;
    prepare_parameter_storage(work);
    for (block = work; block; block = block->next)
        cleanup_expressions(&block->body.expressions);
    for (block = work; block; block = block->next) {
        expression = block->body.expressions;
        while (expression) {
            if (optimize_global) expression->tree = tree_transform(expression->tree);
            node = expression->tree;
            next = expression->next;
            if (node->opcode == 0x32) lower_assignment(node);
            else if (node->opcode == 0x3b) {
                node->second.node = node->left.node;
                node->left.node = new_temporary_reference(node->attributes.words.type & 255, 0);
                node->opcode = 0x32;
                value = build_tree("DNTS", node->left.node, 0x30, node);
                /* Label prefix field04 retains its switch-selector tree. */
                node->right.symbol->field04 = (unsigned)value;
                node->right.symbol = 0;
            } else if (node->opcode == 0x3c)
                lower_switch_dispatch(block, expression, node);
            expression = next;
        }
    }
}

Expression *switch_dispatch_expression; /* 0007b968 */
Node *switch_selector; /* 0007b96c */
unsigned short switch_selector_type; /* 0007b970 */
Node *switch_dispatch_node; /* 0007b974 */
/* Original [00066f10,00066fa0). Dispatch records retain the expression
   that computes their selector. Coalesce ranges, generate the tests, then
   recycle the old range list and selector tree. */
void __fastcall lower_switch_dispatch(WorkItem *work, Expression *expression, Node *node)
{
    Node *range, *next;
    int count;
    switch_selector = (Node *)node->left.node->right.symbol->field04;
    switch_selector_type = node->attributes.words.type;
    switch_dispatch_node = node;
    switch_dispatch_expression = expression;
    if (optimize_global) {
        intern_work_value(switch_selector);
        work->flow_sets[WORK_MEMORY_STORES] = bitset_include(
            work->flow_sets[WORK_MEMORY_STORES], (unsigned)(int)node->info.words.identifier);
    }
    count = coalesce_switch_ranges(node);
    emit_switch_dispatch(node->second.node, count);
    for (range = node->second.node; range; range = next) {
        next = range->second.node;
        release_node(range);
    }
    node->opcode = 1;
    release_tree(switch_selector);
}

/* Original [00067bd0,00067c80). Range left/detail hold inclusive low/high
   values, right holds the target label, and second links the sorted list.
   Remove ranges reaching the default label, then join adjacent ranges
   which share a target. The 4c leaf terminates this list. */
int __fastcall coalesce_switch_ranges(Node *node)
{
    Node *range, *next;
    int count = 0;
    range = node->second.node;
    while (range->opcode != 0x4c &&
           range->right.pointer == switch_dispatch_node->right.pointer) {
        next = range->second.node;
        node->second.node = next;
        release_node(range);
        range = next;
    }
    while (range->opcode != 0x4c) {
        next = range->second.node;
        ++count;
        if (next->opcode != 0x4c) {
            if (next->right.pointer == switch_dispatch_node->right.pointer) {
                range->second.node = next->second.node;
                release_node(next);
                next = range->second.node;
            }
            if (next->opcode != 0x4c &&
                next->left.value - range->detail.value == 1 &&
                next->right.pointer == range->right.pointer) {
                range->detail.value = next->detail.value;
                range->second.node = next->second.node;
                release_node(next);
                --count;
                next = range;
            }
        }
        range = next;
    }
    return count;
}

/* Original [00067ed0,00067fc0). Choose linear comparisons for fewer than
   six tests, a jump table when its span costs at most four entries per test,
   and otherwise split the sorted ranges and emit a balanced decision tree. */
void __fastcall emit_switch_dispatch(Node *ranges, int count)
{
    Node *pivot, *label, *branch;
    unsigned cases, span;
    int tests, left_count;
    while (count) {
        summarize_switch_ranges(ranges, count, &cases, &span, &tests);
        if (tests < 6) {
            emit_linear_switch(ranges, count);
            return;
        }
        if ((unsigned)(tests << 2) >= span) {
            emit_switch_table(ranges, count, 1, 0);
            return;
        }
        split_switch_ranges(ranges, cases, count, span, &pivot, &left_count);
        label = make_switch_pivot(pivot);
        emit_switch_dispatch(ranges, left_count);
        insert_tree_expression(switch_dispatch_expression, switch_dispatch_node, label);
        count -= left_count + 1;
        ranges = pivot->second.node;
    }
    branch = new_node(0x3a);
    branch->right.symbol = switch_dispatch_node->right.symbol;
    insert_tree_expression(switch_dispatch_expression, switch_dispatch_node, branch);
}

/* Original [00067960,000679e0). Count represented values and comparison
   costs across count sorted, nonempty ranges; span excludes the final +1.
   The caller guarantees count is nonzero. A singleton costs one test and
   an interval costs two. */
void __fastcall summarize_switch_ranges(Node *ranges, int count,
    unsigned *cases, unsigned *span, int *tests)
{
    Node *last = ranges;
    unsigned low = ranges->left.value;
    *cases = 0;
    *tests = 0;
    *span = low;
    while (count--) {
        *cases += ranges->detail.value - ranges->left.value + 1;
        ++*tests;
        if (ranges->detail.value != ranges->left.value) ++*tests;
        last = ranges;
        ranges = ranges->second.node;
    }
    *span = last->detail.value - low;
}

/* Original [00067a00,00067b10). Test each singleton for equality; intervals
   first reject values below their low bound, then branch on the high bound.
   Finish with an unconditional branch to the default target. */
void __fastcall emit_linear_switch(Node *ranges, int count)
{
    Node *selector = switch_selector;
    Node *constant, *branch;
    unsigned comparison;
    while (count--) {
        if (ranges->detail.value == ranges->left.value) {
            constant = intern_leaf(0x33, 0, (unsigned)switch_selector_type, ranges->left.value);
            comparison = 0x20;
        } else {
            constant = intern_leaf(0x33, 0, (unsigned)switch_selector_type, ranges->left.value);
            branch = build_tree("DPNYN", selector, constant, 0x23, 0x38);
            branch->right.symbol = switch_dispatch_node->right.symbol;
            insert_tree_expression(switch_dispatch_expression, switch_dispatch_node, branch);
            constant = intern_leaf(0x33, 0, (unsigned)switch_selector_type, ranges->detail.value);
            comparison = 0x24;
        }
        branch = build_tree("DPNYN", switch_selector, constant, comparison, 0x38);
        branch->right.symbol = ranges->right.symbol;
        insert_tree_expression(switch_dispatch_expression, switch_dispatch_node, branch);
        selector = switch_selector;
        ranges = ranges->second.node;
    }
    branch = new_node(0x3a);
    branch->right.symbol = switch_dispatch_node->right.symbol;
    insert_tree_expression(switch_dispatch_expression, switch_dispatch_node, branch);
}

Node *assignment_node; /* 0007b978 */
Node *assignment_destination; /* 0007b97c */
Node *assignment_match; /* 0007b980 */
unsigned assignment_match_left; /* 0007b984 */
unsigned assignment_has_addition; /* 0007b988 */
unsigned assignment_operator; /* 0007b98c */
/* Original [00066fa0,00067170), including its 12-entry opcode dispatch
   table. Recognize updates of a destination by one of its own loads and
   replace the repeated load with the operator's typed identity value. */
void __fastcall lower_assignment(Node *node)
{
    Node *destination, *operation, *left, *right, *identity;
    if (node->attributes.words.type & 0x3000) return;
    assignment_node = node;
    destination = node->left.node;
    operation = strip_assignment_conversion(node->second.node);
    switch (operation->opcode) {
    case 2: case 0xb: case 0xc:
        assignment_operator = operation->opcode;
        break;
    case 3:
        assignment_operator = 2;
        break;
    case 0xd:
        if (operation->second.node->attributes.words.type == assignment_node->attributes.words.type &&
            matches_assignment_load(destination, operation->second.node)) {
            reduce_assignment_operator(node, operation->left.node, operation->second.node, operation);
            return;
        }
        /* Fall through: test the left load for these nonassociative operators. */
    case 9: case 10:
        if (operation->left.node->attributes.words.type == assignment_node->attributes.words.type &&
            matches_assignment_load(destination, operation->left.node))
            reduce_assignment_operator(node, operation->second.node, operation->left.node, operation);
        return;
    default:
        return;
    }
    assignment_destination = destination;
    assignment_has_addition = 0;
    if (!find_assignment_operand(operation, 0)) return;
    left = assignment_match->left.node;
    right = assignment_match->second.node;
    if (operation->opcode == 3 && assignment_has_addition) node->opcode = 0xf;
    else {
        node->opcode = map_assignment_opcode(operation->opcode, 1);
        if (operation->opcode == 3)
            node->second.node = build_tree("PNt", node->second.node, 8,
                (unsigned)operation->second.node->attributes.words.type);
    }
    if (assignment_match_left) {
        assignment_match->attributes.words.type = right->attributes.words.type;
        identity = operator_identity_node(assignment_match, assignment_match);
        assignment_match->left.node = identity;
        release_tree(left);
    } else {
        assignment_match->attributes.words.type = left->attributes.words.type;
        identity = operator_identity_node(assignment_match, assignment_match);
        assignment_match->second.node = identity;
        release_tree(right);
    }
    tree_transform(node);
}

/* Original [00067930,00067960). Ignore equal-width scalar conversions
   while recognizing an assignment update; floating encodings stop the walk. */
Node *__fastcall strip_assignment_conversion(Node *node)
{
    Node *operand;
    if (!(node->attributes.words.type & 0xf00)) return node;
    while (node->opcode == 0x34) {
        operand = node->left.node;
        if (!(operand->attributes.words.type & 0xf00) ||
            (unsigned char)node->attributes.words.type != (unsigned char)operand->attributes.words.type)
            break;
        node = operand;
    }
    return node;
}

/* Original [000583c0,000583dc). Match an ordinary load of the store's
   destination address through the compiler's structural equality query. */
int __fastcall matches_assignment_load(Node *destination, Node *operand)
{
    return operand->opcode == 0x30 && trees_equal(destination, operand->left.node, 1);
}

/* Original [00067820,0006792d). Search the associative assignment chain
   for a same-typed load of its destination. Remember which operand matched
   and whether an addition occurred before replacing that load by identity. */
int __fastcall find_assignment_operand(Node *node, unsigned has_addition)
{
    unsigned opcode = node->opcode;
    if (opcode == 2) has_addition = 1;
    if (opcode != assignment_operator && !(assignment_operator == 2 && opcode == 3)) return 0;
    if ((node_shapes[opcode] & NODE_ASSOCIATIVE) &&
        node->second.node->attributes.words.type == assignment_node->attributes.words.type &&
        matches_assignment_load(assignment_destination, node->second.node)) {
        assignment_match_left = 0;
        assignment_match = node;
        assignment_has_addition = has_addition;
        return 1;
    }
    if (node->left.node->attributes.words.type == assignment_node->attributes.words.type &&
        matches_assignment_load(assignment_destination, node->left.node)) {
        assignment_match_left = 1;
        assignment_match = node;
        assignment_has_addition = has_addition;
        return 1;
    }
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        if (find_assignment_operand(node->left.node, has_addition)) return 1;
        if ((node_shapes[node->opcode] & NODE_ASSOCIATIVE) &&
            find_assignment_operand(node->second.node, has_addition)) return 1;
    }
    return 0;
}

/* Original [000677e0,00067812). Reuse the assignment record as a
   compound operator, retaining its destination and the other operand. */
void __fastcall reduce_assignment_operator(Node *assignment, Node *operand,
    Node *load, Node *operation)
{
    release_tree(load);
    assignment->opcode = map_assignment_opcode(operation->opcode, 1);
    assignment->second.node = operand;
    release_node(operation);
}
