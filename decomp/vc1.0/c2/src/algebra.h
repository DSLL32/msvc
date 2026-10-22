#ifndef ALGEBRA_H
#define ALGEBRA_H
#include "tree-rewrite.h"
#include <recon.h>
int __fastcall operator_identity(Node *node);
int __fastcall integer_type_limit(Node *node, int limit);
Node *__fastcall strip_integer_cast(Node *node);
Node *__fastcall typed_integer_node(Node *model, unsigned value);
int __fastcall rewrite_shift_as_multiply(Node *node);
Node *__fastcall fold_absorbing_constant(Node *node, int value);
int __fastcall reduce_constant_bits(Node *node, Node *parent);
int __fastcall is_nonnegative_tree(Node *node);
int __fastcall can_combine_constants(Node *node, unsigned first, unsigned second);
int __fastcall integer_log2(unsigned value);
int __fastcall extract_negative(Node **node);
int __fastcall negate_tree_in_place(Node **node);
int __fastcall constant_as_integer(Node *node, int *value);
Node *__fastcall lift_left_unary(Node *node);
Node *__fastcall remove_left_wrapper(Node *node);
Node *__fastcall take_left_operand(Node *node);
Node *__fastcall lift_unary_operand(Node *node, unsigned opcode);
Node *__fastcall remove_right_wrapper(Node *node);
unsigned __fastcall normalize_integer_width(Node *node, unsigned value);
Node *__fastcall simplify_masked_tree(Node *node, unsigned mask);
unsigned __fastcall shift_discard_mask(Node *node);
int __fastcall factor_common_operator(Node *node, unsigned opcode, int implicit);
int __fastcall factor_compatible_operator(Node *node, unsigned opcode, int implicit);
int __fastcall distribute_constant(Node *node, unsigned opcode);
int __fastcall conversion_loses_value(unsigned short source, unsigned short target);
int __fastcall lower_far_pointer_cast(Node *node);
unsigned __fastcall map_comparison_opcode(unsigned opcode, int negate);
#endif
