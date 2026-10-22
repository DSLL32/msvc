#include "tree-rewrite.h"
#include "expression-fields.h"
#include <recon.h>
/* Original [00051130,000512a0). Separate the hidden object argument from
   member-function designators, evaluating the object expression once. */
Node *__fastcall canonicalize_function_reference(Node *node)
{
    Node *wrapper = node->left.node, *hidden, *object, *value, *temporary;
    Node *parameter, **link;
    if (wrapper->opcode != 0x99 && wrapper->opcode != 0x9a) return node;
    if (wrapper->opcode == 0x99) {
        hidden = wrapper->second.node;
        node->left.node = wrapper->left.node;
        release_node(wrapper);
    } else {
        hidden = wrapper->second.node->left.node;
        object = hidden->left.node;
        if (can_repeat_tree(object)) {
            hidden->left.node = copy_tree(object);
            value = build_tree("PNT", object, 0x30, wrapper->second.node);
            value->attributes.words.flags |= 0x40;
        } else {
            temporary = new_temporary_reference(object->attributes.words.type & 255, 0x100);
            temporary = build_tree("PNT", temporary, 0x30, object);
            hidden->left.node = temporary;
            value = build_tree("DPNTDNTNY", temporary->left.node, object, 0x32, object,
                               temporary, 0x30, wrapper->second.node, 0x44);
            value->second.node->attributes.words.flags |= 0x40;
        }
        value = build_tree("PPNYNT", value, wrapper->left.node, 2, 0x30, wrapper);
        node->left.node = value;
        value->attributes.words.flags |= 0x40;
        hidden = copy_tree(hidden);
        release_tree(wrapper);
    }
    if (node->opcode == 0x3e) {
        hidden->opcode = 0x55;
        parameter = finish_canonical_node(hidden);
        link = &node->second.node;
        while ((*link)->opcode != 0x4c) link = &(*link)->second.node;
        parameter->second.node = *link;
        *link = parameter;
    } else {
        hidden->right.symbol = hidden->left.node->right.symbol;
        hidden->opcode = 0x2d;
        release_node(hidden->left.node);
        hidden->second.node = node->second.node;
        node->second.node = hidden;
    }
    return node;
}
