#include "work-prepare.h"
#include "work-optimizer.h"
#include "tree-rewrite.h"
#include "tree-input.h"
#include "expression-fields.h"
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
                node->right.expression->tree = value;
                node->right.expression = 0;
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
    switch_selector = node->left.node->right.expression->tree;
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
