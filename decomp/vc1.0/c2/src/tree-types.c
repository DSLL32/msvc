#include "tree-rewrite.h"
#include "tree-input.h"
#include "expression-fields.h"
#include "function-input.h"
#include "tree-scratch.h"
#include "records.h"
#include <recon.h>
extern int floating_frontend, code_pointer_bytes;
int data_pointer_bytes = 4; /* 0008317c */
int suppress_type_warnings; /* 00083478 */
TreeScratch tree_scratch; /* 0007e820..0007e844 */
#define expression_type_stack tree_scratch.type_stack_with_limit
unsigned short *expression_type_top = expression_type_stack; /* 00083474 */
int allow_selector_six = 1; /* 0008061c: suppresses one selector-conversion warning */

/* Original [00057fc0,00058000). Ignore narrowing casts and casts which
   preserve a floating representation class while inspecting an operand. */
Node *__fastcall skip_node_conversions(Node *node)
{
    Node *operand;
    unsigned short type;
    while (node->opcode == 0x34) {
        operand = node->left.node;
        type = operand->attributes.words.type;
        if (type & 0x3000) {
            if ((node->attributes.words.type ^ type) & 0xff00) break;
        } else if ((node->attributes.words.type & 255) > (type & 255)) break;
        node = operand;
    }
    return node;
}

/* Original [00052530,000527b0). Convert in explicit representation/width
   stages so each required intermediate conversion remains in the tree. */
Node *__fastcall convert_node_type(Node *node, unsigned short target)
{
    unsigned short source, intermediate;
    unsigned selector;
    int warn;
    for (;;) {
        source = node->attributes.words.type;
        if (source == target) return node;
        if ((source & target) & 0xff00) {
            if (((source | target) & 255) == 5)
                node = convert_node_once(node, (unsigned short)((target & 0xff02) | 2));
            else if (!floating_frontend && source == 0x408 && (target & 255) < 4 &&
                     backend_stage == 1 && !is_constant_value(node, 0)) {
                warn = 1;
                if (node->opcode == 0x68 && node->second.node->opcode == 0x69) {
                    selector = node->second.node->right.symbol->identifier;
                    if (selector == 2 || !allow_selector_six || selector == 6) warn = 0;
                }
                if (warn && !suppress_type_warnings) compiler_warning(2, 0x2f7);
            }
            return convert_node_once(node, target);
        }
        if (source > target) {
            if (source & 0xf00) {
                if ((source & 255) != (target & 255) &&
                    !((source & 255) > (target & 255) &&
                      (source == 0x804 || source == 0x808 || source == 0x404))) {
                    if (source == 0x408) node = convert_node_type(node, 0x404);
                    else if (source == 0xc08) node = discard_sequence_tail(node);
                    else node = convert_node_type(node, (unsigned short)((source & 0xff00) | (target & 255)));
                    continue;
                }
                if ((target & 0x300) && node->opcode == 0x68 && source == 0x404 && (target & 255) <= 4)
                    node = discard_sequence_tail(node);
                node = convert_node_once(node, (unsigned short)((target & 0xff00) | (source & 255)));
            } else if (source & 0x1000) {
                intermediate = source == 0x1008 ? (target & 0xff04) | 4 : 0x1008;
                node = convert_node_once(node, intermediate);
            } else node = convert_node_once(node, (unsigned short)((0x2000 | (source & 0xfe)) >> 1));
        } else {
            if (target & 0xf00)
                intermediate = target == 0x408 ? 0x404 : (source & 0xff00) | (target & 255);
            else if (target & 0x1000) {
                if (target == 0x1008)
                    intermediate = (source & 0xff00) | ((source & 255) <= 4 ? 4 : source & 255);
                else intermediate = 0x1008;
            } else intermediate = (0x2000 | (target & 0xfe)) >> 1;
            node = convert_node_type(node, intermediate);
            return convert_node_once(node, target);
        }
    }
}

/* Original [00052860,00053290), including the 176-entry opcode table.
   Operands are typed before this routine. The separate stack carries types
   across the paired expression records 3b/3c. */
Node *__fastcall finish_canonical_node(Node *node)
{
    unsigned short left_type, right_type, type, left_class, right_class;
    unsigned shape = node_shapes[node->opcode] & NODE_ARITY;
    Node *operand, *value;
    SymbolHeader *symbol;
    int saved_suppression, warn;
    unsigned bytes;
    suppress_type_warnings = backend_stage != 1;
    if (shape == NODE_BINARY) right_type = node->second.node->attributes.words.type;
    if (shape == NODE_BINARY || shape == NODE_UNARY)
        left_type = node->left.node->attributes.words.type;
    switch (node->opcode) {
    case 2: case 3:
        type = common_node_type(left_type, right_type);
        if (!(type & 0xc00)) goto type_operands;
        saved_suppression = suppress_type_warnings;
        suppress_type_warnings = 1;
        left_class = left_type & 0xc00;
        right_class = right_type & 0xc00;
        node->left.node = convert_node_type(node->left.node,
            (unsigned short)(left_class ? type : (left_type & 0xff00) | 4));
        node->second.node = convert_node_type(node->second.node,
            (unsigned short)(right_class ? type : (right_type & 0xff00) | 4));
        suppress_type_warnings = saved_suppression;
        node->attributes.words.type = left_class && right_class ? 0x104 : type;
        type = node->attributes.words.type;
        if (((type & 0xc00) == 0xc00 || type == 0x408) && backend_stage == 1)
            node = lift_typed_sequence(node, 0x68);
        break;
    case 4: case 5: case 6: case 7: case 0xb: case 0xc: case 0xd:
    case 0x43: case 0x7b: case 0x7c: case 0xa6: case 0xae: case 0xaf:
        goto type_operands;
    case 9: case 0xa: case 0x75: case 0xab: case 0xac: case 0xad:
        node->second.node = convert_node_type(node->second.node, 0x101);
    case 8: case 0xe: case 0x59: case 0x84:
        if (((unsigned)left_type & 255) < (unsigned)expression_integer_bytes) {
            left_type = (unsigned short)(expression_integer_bytes | 0x100);
            node->left.node = convert_node_type(node->left.node, left_type);
        }
    case 0x50: case 0x51:
        node->attributes.words.type = left_type;
        break;
    case 0x15: case 0x16:
        node->second.node = convert_node_type(node->second.node, 0x101);
        break;
    case 0x1f: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24:
        operand = 0;
        if (left_type == 0xc08) operand = node->second.node;
        else if (right_type == 0xc08) operand = node->left.node;
        else if (left_type == 0x404 && node->left.node->opcode == 0x68)
            operand = node->second.node;
        else if (right_type == 0x404 && node->second.node->opcode == 0x68)
            operand = node->left.node;
        if (operand) {
            if (operand->opcode == 0x68 &&
                (((operand->attributes.words.type & 0xc00) == 0xc00 ||
                   operand->attributes.words.type == 0x404) &&
                  node->left.node->second.node->opcode != 0x85 &&
                  trees_equal(node->left.node->second.node, node->second.node->second.node, 1) ||
                 operand->second.node->opcode == 0x69 && !operand->second.node->right.pointer)) {
                node->second.node = discard_sequence_tail(node->second.node);
                node->left.node = discard_sequence_tail(node->left.node);
            } else if (((operand->attributes.words.type & 0x300) &&
                         (operand->attributes.words.type & 255) <= 4) || is_constant_value(operand, 0)) {
                if (node->left.node == operand)
                    node->second.node = discard_sequence_tail(node->second.node);
                else node->left.node = discard_sequence_tail(node->left.node);
            }
        }
        if (node->second.node->attributes.words.type == 0x404 && is_constant_value(node->left.node, 0))
            node->left.node = intern_leaf(0x33, 0, 0x404, 0);
        else if (node->left.node->attributes.words.type == 0x404 && is_constant_value(node->second.node, 0))
            node->second.node = intern_leaf(0x33, 0, 0x404, 0);
        left_type = node->left.node->attributes.words.type;
        right_type = node->second.node->attributes.words.type;
        goto type_operands;
    case 0x29:
        node->attributes.words.type = (unsigned short)(data_pointer_bytes | 0x400);
        break;
    case 0x32:
        if (right_type != node->attributes.words.type)
            node->second.node = convert_node_type(node->second.node, node->attributes.words.type);
        break;
    case 0x34:
        type = node->attributes.words.type;
        if ((left_type & 0xc00) && !(type & 0xc00) && (type & 255) > (left_type & 255)) {
            warn = 1;
            operand = node->left.node;
            if (data_pointer_bytes == 8 && operand->opcode == 0x26 &&
                (symbol = operand->right.symbol) && symbol->kind != 0x13 && !(symbol->field0e & 1))
                warn = 0;
            if (warn && !suppress_type_warnings) compiler_warning(2, 0x301);
        }
        value = convert_node_type(node->left.node, node->attributes.words.type);
        if (value->opcode == 0x34) {
            node->left.node = value->left.node;
            release_node(value);
        } else node->left.node = value;
        break;
    case 0x3b:
        if (backend_stage == 1) {
            if (((unsigned)left_type & 255) < (unsigned)expression_integer_bytes) {
                left_type = (unsigned short)(expression_integer_bytes | 0x100);
                node->left.node = convert_node_type(node->left.node, left_type);
            }
            ++expression_type_top;
            node->attributes.words.type = left_type;
            *expression_type_top = left_type;
            if (expression_type_top > expression_type_stack + 16)
                compiler_internal_error("@(#)typer.c:1.114", 0x308);
        }
        break;
    case 0x3c:
        if (backend_stage == 1) {
            node->attributes.words.type = *expression_type_top--;
            if (expression_type_top < expression_type_stack)
                compiler_internal_error("@(#)typer.c:1.114", 0x310);
        }
        break;
    case 0x41:
        goto convert_left;
    case 0x42: case 0x44:
        node->attributes.words.type = right_type;
        break;
    case 0x55:
        type = node->attributes.words.type;
        if (!(type & 0xff00)) {
            if (floating_frontend) type = left_type;
            else if (left_type & 0x300) {
                bytes = left_type & 255;
                if (bytes <= 4) bytes = 4;
                type = (unsigned short)(bytes | (left_type & 0xff00));
            } else if (left_type & 0x1000) {
                bytes = left_type & 255;
                if (bytes <= 8) bytes = 8;
                type = (unsigned short)(bytes | 0x1000);
            } else if (!(left_type & 0xc00) || (left_type & 0x800)) type = left_type;
            else if (left_type == 0xc08) type = 0x404;
            else {
                bytes = is_function_reference(node->left.node) ? code_pointer_bytes : data_pointer_bytes;
                if (((unsigned)left_type & 255) > bytes) bytes = left_type & 255;
                else bytes = is_function_reference(node->left.node) ? code_pointer_bytes : data_pointer_bytes;
                type = (unsigned short)(bytes | 0x400);
            }
            node->attributes.words.type = type;
        } else if (!floating_frontend && backend_stage == 1) {
            if ((type & 0x300) && (type & 255) < (left_type & 255) && node->left.node->opcode != 0x33)
                compiler_warning(1, 0x2f9);
            else if ((type & 0xc00) && (type & 255) < (left_type & 255) &&
                     !is_constant_value(node->left.node, 0) &&
                     (node->left.node->attributes.words.type & 0xc00) != 0xc00)
                compiler_warning(1, 0x2fa);
        }
        goto convert_left;
    case 0x68:
        if (!node->attributes.words.type) {
            operand = node->second.node;
            node->attributes.words.type = 0x404;
            if (!(operand->opcode == 0x69 && !operand->right.pointer) &&
                operand->attributes.words.type != 0x404) node->attributes.words.type = 0xc08;
        }
        node->left.node = convert_node_type(node->left.node, 0x404);
        if (!(node->attributes.words.type & 0xc00))
            node->second.node = convert_node_type(node->second.node, 0x202);
        break;
    case 0x85: case 0x86:
        node->attributes.words.type = 0x202;
        break;
    case 0x8b:
        if (((left_type & 0x300) && (left_type & 255) <= 4) || is_constant_value(node->left.node, 0)) {
            release_tree(node->second.node);
            operand = node->left.node;
            release_node(node);
            node = operand;
        } else node->attributes.words.type = common_node_type(left_type, right_type);
        break;
    case 0x8d:
        node->left.node = convert_node_type(node->left.node, 0x104);
        goto no_value;
    case 0xaa:
        node->attributes.words.type = left_type;
        break;
    case 1: case 0x1a: case 0x1b: case 0x1c: case 0x2d: case 0x37: case 0x38:
    case 0x3a: case 0x3d: case 0x46: case 0x47: case 0x48: case 0x49: case 0x4a:
    case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x53: case 0x54: case 0x56:
    case 0x8e: case 0x8f: case 0x90: case 0x99: case 0x9d:
        goto no_value;
    case 0xf: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14:
    case 0x17: case 0x18: case 0x19: case 0x26: case 0x2a: case 0x2b: case 0x30:
    case 0x33: case 0x35: case 0x36: case 0x3e: case 0x40: case 0x58: case 0x69:
    case 0x78: case 0x79: case 0x7a: case 0x7d: case 0x9a: case 0xa1: case 0xa2:
    case 0xa4: case 0xa7: case 0xb0:
        break;
    default:
        compiler_internal_error("@(#)typer.c:1.114", 0x395);
        break;
    }
    return node;
type_operands:
    type = common_node_type(left_type, right_type);
    node->attributes.words.type = type;
    if (left_type != type) node->left.node = convert_node_type(node->left.node, type);
    if (right_type != node->attributes.words.type)
        node->second.node = convert_node_type(node->second.node, node->attributes.words.type);
    return node;
convert_left:
    node->left.node = convert_node_type(node->left.node, node->attributes.words.type);
    return node;
no_value:
    node->attributes.words.type = 0;
    return node;
}

/* Original [000524f0,00052530). Keep an existing type, otherwise add one
   explicit conversion node and fold integer constants immediately. */
Node *__fastcall convert_node_once(Node *node, unsigned short type)
{
    Node *conversion;
    if (node->attributes.words.type == type) return node;
    conversion = new_node(0x34);
    conversion->left.node = node;
    conversion->attributes.words.type = type;
    if (node->opcode == 0x33) conversion = fold_constant_conversion(conversion);
    return conversion;
}

/* Original [000585f0,00058630). Constants and symbols with the indicated
   storage modes are known to denote a non-null address. */
int __fastcall is_nonnull_address(Node *node)
{
    TemporarySymbol *symbol = (TemporarySymbol *)node->right.symbol;
    unsigned storage;
    if (symbol->symbol.kind == 19) return 1;
    if (symbol->symbol.kind == 1 &&
        (!symbol->symbol.field0d || symbol->symbol.field0d == 0x26)) {
        storage = symbol->attributes & 0x70;
        if (storage == 0x10 || storage == 0x30) return 1;
    }
    return 0;
}

/* Original [000592c0,000592d0). Ignore every explicit conversion while
   locating the underlying expression, regardless of its resulting type. */
Node *__fastcall strip_conversions(Node *node)
{
    while (node->opcode == 0x34) node = node->left.node;
    return node;
}

/* Original [000527e0,00052860). Select the common scalar representation,
   retaining selector/offset pointer precedence, then apply integral promotion
   to the target's minimum arithmetic width. Aggregate encodings stay intact. */
unsigned short __fastcall common_node_type(unsigned short left, unsigned short right)
{
    unsigned short type;
    if (left == 0xc08) type = right <= 0x408 ? 0xc08 : right;
    else if (right == 0xc08) type = left <= 0x408 ? 0xc08 : left;
    else type = right > left ? right : left;
    if ((type & 0x200) && (unsigned char)left != (unsigned char)right)
        type = (unsigned char)left > (unsigned char)right ? left : right;
    if ((type & 0xff00) != 0x8000 && (int)(type & 255) < expression_integer_bytes)
        type = (unsigned short)(expression_integer_bytes | 0x100);
    return type;
}

/* Original [00027310,0002732e). A symbol-address leaf denotes a code
   pointer only when its symbol descriptor is a function (kind4). */
int __fastcall is_function_reference(Node *node)
{
    return node->opcode == 0x26 && node->right.symbol && node->right.symbol->kind == 4;
}

int tree_repeatable; /* 000836f0: result shared by the recursive inspection. */

/* Original [000586d0,000586e5). Reset the result for each independent tree. */
int __fastcall can_repeat_tree(Node *node)
{
    tree_repeatable = 1;
    inspect_tree_repeatability(node);
    return tree_repeatable;
}

/* Original [00058660,000586c9). A repeatable expression has neither a
   side-effecting operator nor a volatile access anywhere in its operands.
   These semantic shape properties correspond to original bits 0200/2000,
   rather than those numeric masks in the reconstructed classification. */
void __fastcall inspect_tree_repeatability(Node *node)
{
    for (;;) {
        if (node_shapes[node->opcode] & NODE_SIDE_EFFECT) break;
        if (node->attributes.words.flags & 0x800) break;
        if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return;
        inspect_tree_repeatability(node->left.node);
        if (!(node_shapes[node->opcode] & NODE_HAS_SECOND)) return;
        node = node->second.node;
    }
    tree_repeatable = 0;
}

static unsigned searched_opcode; /* 000836f4: current recursive operator search. */
int __fastcall find_tree_opcode(Node *node);
/* Original [000586f0,000586fb). The recursive worker shares one search
   opcode while walking all non-leaf operators. */
int __fastcall tree_contains_opcode(Node *node, unsigned opcode)
{
    searched_opcode = opcode;
    return find_tree_opcode(node);
}

/* Original [00058700,00058765). Search left recursively and the right
   spine iteratively. Leaf opcodes are deliberately excluded from this query. */
int __fastcall find_tree_opcode(Node *node)
{
    for (;;) {
        if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return 0;
        if (node->opcode == searched_opcode) return 1;
        if (find_tree_opcode(node->left.node) == 1) return 1;
        if (!(node_shapes[node->opcode] & NODE_HAS_SECOND)) return 0;
        node = node->second.node;
    }
}

/* Original [00058470,000584d8). Argument wrappers inherit their value's
   storage size. Aggregate-value/copy records carry an explicit size tree;
   ordinary records use aggregate info or the scalar type's low byte. */
unsigned __fastcall aligned_value_size(Node *node)
{
    extern int default_segment_alignment;
    unsigned size, intrinsic;
    while (node->opcode == 0x55) node = node->left.node;
    if (node->opcode == 0x40) {
        intrinsic = node->left.node->left.value;
        if (intrinsic == 0xb3 || intrinsic == 0xde) {
            size = node->second.node->second.node->left.node->left.value;
            goto align_size;
        }
        if (intrinsic == 0xdf) {
            size = node->second.node->left.node->left.value;
            goto align_size;
        }
    }
    size = node->attributes.words.type == 0x8000 ? node->info.value :
        node->attributes.words.type & 255;
align_size:
    return (size + default_segment_alignment - 1) & ~(default_segment_alignment - 1);
}

/* Original [00058b20,00058b58). Small C aggregate return values use a
   sized8000 representation; other values use the normal frontend-kind
   encoder. Preserve the short input and AX result widths. */
unsigned short __fastcall value_node_type(short kind, unsigned bytes)
{
    if (kind == 6 && !floating_frontend && is_register_return_size(bytes))
        return (unsigned short)(bytes | 0x8000);
    return (unsigned short)encode_node_type((signed char)kind, bytes);
}
