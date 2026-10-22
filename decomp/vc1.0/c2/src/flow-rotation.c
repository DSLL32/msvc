/* Loop-test rotation over the nested global flow regions. */
#include "flow-edges.h"
#include "tree-rewrite.h"
#include "function-input.h"
#include "expression-fields.h"
#include "options.h"
#include <recon.h>

/* Original [00059d30,0005a140). Rotate an eligible natural loop's test
   into its bottom block, retaining an entry test and an explicit exit bridge.
   Process child regions before their containing region. */
void __fastcall rotate_flow_regions(FlowRegion *regions)
{
    FlowRegion *region;
    WorkItem *entry, *header, *last, *bottom, *bridge, *other;
    Expression *test_head, *test, *last_expression, *previous, *copy;
    Expression *source, *inserted;
    Node *bottom_label, *node;
    FlowLink *edge, *next;
    unsigned short type;
    for (region = regions; region; region = region->next) {
        rotate_flow_regions(region->children);
        if (!global_optimization_active || (region->flags & 1)) continue;
        entry = region->entry;
        header = entry->next;
        last = region->last;
        test_head = header->body.expressions->next;
        for (test = test_head; test; test = test->next) {
            if (test->tree->opcode == 0x38) break;
            if (tree_contains_opcode(test->tree, 0x3e) ||
                tree_contains_opcode(test->tree, 0x40)) break;
        }
        if (!test || test->tree->opcode != 0x38) continue;
        edge = header->successors;
        if (edge->work == header->next) edge = edge->next;
        if (!edge || work_in_flow_region(edge->work, region)) continue;
        last_expression = last->body.expressions;
        while (last_expression->next) last_expression = last_expression->next;
        if (last_expression->tree->opcode != 0x3a ||
            last_expression->tree->right.symbol != header->body.expressions->tree->right.symbol)
            continue;
        entry->body.expressions->next = test_head;
        header->body.expressions->next = 0;
        for (edge = header->successors; edge; edge = next) {
            next = edge->next;
            other = edge->work;
            if (other == header->next) continue;
            remove_flow_reference(&header->successors, other);
            remove_flow_reference(&other->predecessors, header);
            connect_flow_blocks(entry, other);
        }
        previous = last->body.expressions;
        copy = last_expression;
        if (last_expression == previous) {
            copy = new_expression();
            last_expression->next = copy;
            copy->tree = last_expression->tree;
            last_expression->tree = new_flow_label_node();
        } else {
            while (previous->next && previous->next != last_expression)
                previous = previous->next;
        }
        if (previous->tree->opcode == 0x29) {
            bottom = last;
            bottom_label = last->body.expressions->tree;
        } else {
            bottom_label = new_flow_label_node();
            ((LabelSymbol *)bottom_label->right.symbol)->references = 0;
            bottom = build_work_lists("SNRnlTL", last, bottom_label);
            previous->next = 0;
            bottom->body.expressions->next = copy;
            bottom->successors = last->successors;
            last->successors = 0;
            connect_flow_blocks(last, bottom);
            for (edge = bottom->successors; edge; edge = edge->next) {
                remove_flow_reference(&edge->work->predecessors, last);
                add_flow_predecessor(edge->work, bottom);
            }
            bottom->flags |= last->flags & 1;
        }
        release_tree(copy->tree);
        copy->tree = copy_tree(test_head->tree);
        for (source = test_head; source != test;) {
            inserted = new_expression();
            copy->next = inserted;
            copy = inserted;
            source = source->next;
            copy->tree = copy_tree(source->tree);
        }
        copy->tree->right.symbol = header->body.expressions->tree->right.symbol;
        copy->tree->left.node = negate_condition(copy->tree->left.node);
        for (edge = header->predecessors; edge; edge = next) {
            next = edge->next;
            other = edge->work;
            if (other == bottom || other == header->previous) continue;
            remove_flow_reference(&header->predecessors, other);
            remove_flow_reference(&other->successors, header);
            connect_flow_blocks(other, bottom);
            redirect_flow_label(other, header->body.expressions->tree->right.symbol,
                                bottom_label->right.symbol);
        }
        replace_flow_region_last(region, bottom);
        node = new_node(0x3a);
        bridge = build_work_lists("SNRnlTL", bottom, node);
        bridge->region = region->parent;
        if (region->parent && region->parent->last == bottom)
            replace_flow_region_last(region->parent, bridge);
        connect_flow_blocks(bottom, bridge);
        bridge->body.expressions->tree->right.symbol = test->tree->right.symbol;
        ++((LabelSymbol *)test->tree->right.symbol)->references;
        for (edge = entry->successors; edge; edge = edge->next) {
            node = edge->work->body.expressions->tree;
            if (node->opcode == 0x29 && node->right.symbol == test->tree->right.symbol) break;
        }
        connect_flow_blocks(bridge, edge->work);
        bridge->flags |= edge->work->flags & 1;
        type = test->tree->left.node->attributes.words.type;
        if ((type & 0x300) && (type & 255) == 4) region->flags |= 2;
    }
}
