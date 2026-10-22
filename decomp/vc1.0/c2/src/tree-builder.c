#include "tree-rewrite.h"
#include "expression-fields.h"
#include "tree-input.h"
#include <stdarg.h>
#include <recon.h>

/* Original [0004fe10,000501c0), including its 51-entry character dispatch
   table [000500f0,000501bc). The format is a postfix tree recipe. Helpers
   may call the builder recursively, so publish every stack change before
   calling them. The original stack starts at 0007e718; the next independent
   object, current_expression, starts at 0007e818 (64 pointer slots). Slot
   zero is the initial sentinel position, not the first pushed operand. */
Node *tree_builder_stack[64];
Node **tree_builder_top = tree_builder_stack; /* 00083320 */
Node *__cdecl build_tree(const char *format, ...)
{
    va_list arguments, lookahead;
    Node *node, *source, *tail, *operand, *result;
    unsigned opcode, value;
    unsigned *destination;
    va_start(arguments, format);
    for (; *format; ++format) {
        switch (*format) {
        case 'B':
            (*tree_builder_top)->attributes.words.flags |=
                (unsigned short)va_arg(arguments, unsigned);
            break;
        case 'C':
            ++tree_builder_top;
            source = va_arg(arguments, Node *);
            node = clone_node(source);
            *tree_builder_top = node;
            break;
        case 'D':
            ++tree_builder_top;
            source = va_arg(arguments, Node *);
            node = copy_tree(source);
            *tree_builder_top = node;
            break;
        case 'F':
            node = fold_constant_node(*tree_builder_top);
            *tree_builder_top = node;
            break;
        case 'I':
            (*tree_builder_top)->left.value = va_arg(arguments, unsigned);
            break;
        case 'L':
        case 'N':
        case 'n':
            if (*format == 'L') node = va_arg(arguments, Node *);
            else {
                opcode = va_arg(arguments, unsigned);
                if (node_shapes[opcode] & NODE_INTERNED) node = intern_leaf(opcode);
                else node = new_node(opcode);
            }
            switch (node_shapes[node->opcode] & NODE_ARITY) {
            case NODE_BINARY:
                node->second.node = *tree_builder_top--;
                /* Fall through: binary recipes pop the right operand first. */
            case NODE_UNARY:
                node->left.node = *tree_builder_top--;
                break;
            case NODE_LEAF:
                break;
            case NODE_CHAIN:
                node->left.node = *tree_builder_top--;
                operand = *tree_builder_top--;
                tail = node;
                while (operand->opcode != 0x4c) {
                    tail->second.node = new_node(0x55);
                    tail = tail->second.node;
                    tail->detail.value = 0;
                    tail->left.node = operand;
                    if (format[1] == 'Y') finish_canonical_node(tail);
                    operand = *tree_builder_top--;
                }
                tail->second.node = operand;
                break;
            default:
                compiler_internal_error("@(#)trees.c:1.292", 0x577);
                break;
            }
            *++tree_builder_top = node;
            if (*format == 'n') finish_canonical_node(node);
            break;
        case 'O':
            /* Consume the value, leaving its destination for the next recipe. */
            value = va_arg(arguments, unsigned);
            lookahead = arguments;
            destination = va_arg(lookahead, unsigned *);
            *destination = value;
            break;
        case 'P':
            *++tree_builder_top = va_arg(arguments, Node *);
            break;
        case 'S':
            /* ST shares T's source argument without consuming it. */
            lookahead = arguments;
            source = va_arg(lookahead, Node *);
            node = *tree_builder_top;
            if (format[1] != 'T' || (source->attributes.words.flags & 1))
                node->attributes.words.flags |= 1;
            if (format[1] == 'T') node->right.symbol = source->right.symbol;
            else node->right.symbol = node->left.node->right.symbol;
            node->detail.node = node;
            break;
        case 'T':
        case 'f':
            source = va_arg(arguments, Node *);
            node = *tree_builder_top;
            if (*format == 'f') node->attributes.words.flags = source->attributes.words.flags;
            node->attributes.words.type = source->attributes.words.type;
            if (node->attributes.words.type == 0x8000) node->info.value = source->info.value;
            break;
        case 'Y':
            node = finish_canonical_node(*tree_builder_top);
            *tree_builder_top = node;
            break;
        case 't':
            (*tree_builder_top)->attributes.words.type =
                (unsigned short)va_arg(arguments, unsigned);
            break;
        default:
            compiler_internal_error("@(#)trees.c:1.292", 0x581);
            break;
        }
    }
    result = *tree_builder_top--;
    va_end(arguments);
    return result;
}
