#ifndef EXPRESSION_FIELDS_H
#define EXPRESSION_FIELDS_H
#include "tree-input.h"
#include "extended.h"
/* Semantic fields in the expression-stream recipes. */
typedef enum ExpressionField {
    FIELD_END, FIELD_IGNORE_BYTE, FIELD_LONG, FIELD_CONTROL_BYTE,
    FIELD_CONTROL_WORD, FIELD_DUPLICATE_LONG, FIELD_TYPE, FIELD_SYMBOL_PAIR,
    FIELD_SYMBOL, FIELD_INVALID, FIELD_NUMBER, FIELD_LINE, FIELD_VARUINT
} ExpressionField;
Node *__fastcall new_node(unsigned opcode);
unsigned __fastcall encode_node_type(signed char kind, unsigned bytes);
void __fastcall read_node_type(Node *node);
void __fastcall read_control_fields(Node *node);
Node *__cdecl intern_leaf(unsigned opcode, ...);
Node *__fastcall intern_typed_label(Function *function, Node *node);
extern int expression_line;
#endif
