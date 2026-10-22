/* Canonical symbol references, constants and small signed values. */
#include "expression-fields.h"
#include <stdarg.h>
Node *symbol_leaves[32], *number_leaves[32];
typedef struct SmallLeafBucket {
    Node *head;
    unsigned field04;
} SmallLeafBucket;
SmallLeafBucket small_leaves[16];
unsigned next_leaf_identifier = 0xffffffffU;
int code_pointer_bytes = 4; /* 00083160 */

/* Original [0004f790,0004fa60). This is a cdecl varargs interface:
   4c consumes only the opcode; 26 consumes flags/symbol/offset and, when the
   symbol is null, a fifth type argument. 33 and a2 consume four arguments. */
Node *__cdecl intern_leaf(unsigned opcode, ...)
{
    va_list arguments;
    unsigned short flags, type;
    unsigned value;
    SymbolHeader *symbol;
    Node **bucket, *node;
    Extended *number;
    if (opcode == 0x4c) return sentinel_node;
    va_start(arguments, opcode);
    flags = (unsigned short)va_arg(arguments, unsigned);
    switch (opcode) {
    case 0x26:
        symbol = va_arg(arguments, SymbolHeader *);
        value = va_arg(arguments, unsigned);
        bucket = &symbol_leaves[((unsigned)symbol + value) & 31];
        for (node = *bucket; node; node = node->second.node) {
            if (node->right.symbol == symbol && node->left.value == value &&
                node->attributes.words.flags == flags) {
                va_end(arguments);
                return node;
            }
        }
        node = new_node(1);
        node->opcode = 0x26;
        node->info.words.identifier = (short)next_leaf_identifier--;
        node->left.value = value;
        node->right.symbol = symbol;
        if (symbol) node->attributes.words.type = symbol->kind == 4 ? code_pointer_bytes | 0x400 : 0x404;
        else node->attributes.words.type = (unsigned short)va_arg(arguments, unsigned);
        node->attributes.words.flags = flags;
        node->second.node = *bucket;
        *bucket = node;
        break;
    case 0x33:
        type = (unsigned short)va_arg(arguments, unsigned);
        value = va_arg(arguments, unsigned);
        if (!(type & 0x3000)) {
            bucket = &number_leaves[(type + value) & 31];
            for (node = *bucket; node; node = node->second.node) {
                if (node->left.value == value && node->attributes.words.type == type &&
                    node->attributes.words.flags == flags) {
                    va_end(arguments);
                    return node;
                }
            }
            node = new_node(1);
            node->opcode = 0x33;
            node->left.value = value;
            node->attributes.words.type = type;
            node->attributes.words.flags = flags;
            node->info.words.identifier = (short)next_leaf_identifier--;
            node->second.node = *bucket;
            *bucket = node;
        } else {
            bucket = &number_leaves[type & 31];
            for (node = *bucket; node; node = node->second.node) {
                if (node->attributes.words.type == type && node->attributes.words.flags == flags &&
                    compare_extended(1, node->left.pointer, (Extended *)value)) {
                    va_end(arguments);
                    return node;
                }
            }
            node = new_node(1);
            node->opcode = 0x33;
            number = arena_alloc(0, sizeof(Extended));
            node->left.pointer = number;
            *number = *(Extended *)value;
            node->attributes.words.type = type;
            node->attributes.words.flags = flags;
            node->info.words.identifier = (short)next_leaf_identifier--;
            node->second.node = *bucket;
            *bucket = node;
        }
        break;
    case 0xa2:
        type = (unsigned short)va_arg(arguments, unsigned);
        value = va_arg(arguments, unsigned);
        bucket = &small_leaves[value & 15].head;
        for (node = *bucket; node; node = node->second.node) {
            if ((int)node->left.word == (int)value && node->attributes.words.type == type) {
                va_end(arguments);
                return node;
            }
        }
        node = new_node(1);
        node->opcode = 0xa2;
        node->left.word = (short)value;
        node->attributes.words.type = type;
        node->attributes.words.flags = flags;
        node->info.words.identifier = (short)next_leaf_identifier--;
        node->second.node = *bucket;
        *bucket = node;
        break;
    default:
        compiler_internal_error("@(#)trees.c:1.292", 0x300);
        node = 0; /* The diagnostic terminates compilation. */
        break;
    }
    va_end(arguments);
    return node;
}
