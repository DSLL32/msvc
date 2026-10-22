#ifndef TREE_REWRITE_H
#define TREE_REWRITE_H
#include "functions.h"
#include "temporaries.h"
extern unsigned expression_side_effects;
extern int expression_integer_bytes, checking_expression;
typedef struct TreeReference {
    SymbolHeader *symbol;
    unsigned flags;
} TreeReference;
extern Expression *current_expression;
Node *__fastcall rewrite_before_children(Node *node, Node *parent, unsigned flags);
Node *__fastcall rewrite_after_left(Node *node, Node *parent, unsigned flags);
Node *__fastcall rewrite_subtree(Node *node, Node *parent, unsigned flags);
void __fastcall analyze_tree_references(Node *node, TreeReference *reference);
Node *__fastcall rewrite_short_circuit(Node *node);
void __cdecl splice_expressions(const char *order, ...);
Node *__fastcall skip_node_conversions(Node *node);
int __fastcall can_keep_conditional(Node *node);
Node *__fastcall lower_conditional(Node *node, Node *container);
Node *__fastcall lower_comma(Node *node, Node *container);
Node *__fastcall materialize_expression(Node *node);
Node *__fastcall extract_assembly_labels(Node *node);
Node *__fastcall convert_node_type(Node *node, unsigned short type);
Node *__fastcall convert_node_once(Node *node, unsigned short type);
Node *__fastcall new_temporary_reference(unsigned bytes, unsigned flags);
Node *__cdecl build_tree(const char *format, ...);
unsigned __fastcall map_assignment_opcode(unsigned opcode, int reverse);
Node *__fastcall copy_tree(Node *node);
Node *__fastcall assignment_value(Node *node);
Node *__fastcall clone_node(Node *node);
void __fastcall lower_bitfield_reference(Node *node);
void __fastcall release_tree(Node *node);
Node *__fastcall lower_target_address(Node *node);
void __fastcall lower_bitfield_assignment(Node *node);
Node *__fastcall reverse_node_list(Node *node);
void __fastcall insert_return_parameter(Function *function, Node *node);
Node *__fastcall canonicalize_function_reference(Node *node);
Node *__fastcall canonicalize_return_value(Function *function, Node *node);
int __fastcall is_constant_value(Node *node, int value);
Node *__fastcall new_storage_reference(unsigned bytes);
int __fastcall can_repeat_tree(Node *node);
Node *__fastcall prepare_target_tree(Node *node);
Node *__fastcall canonicalize_node(Function *function, Node *node, Node *parent);
Node *__fastcall canonicalize_before_children(Function *function, Node *node, Node *parent);
Node *__fastcall canonicalize_after_children(Node *node, Node *parent);
Node *__fastcall finish_canonical_node(Node *node);
Node *__fastcall make_truth_test(Node *node);
Node *__fastcall negate_condition(Node *node);
Node *__fastcall materialize_boolean(Node *node, Node *parent);
Node *__fastcall simplify_short_circuit(Node *node, Node *parent);
Node *__fastcall lift_left_sequence(Node *node);
Node *__fastcall lift_right_sequence(Node *node);
Node *__fastcall canonicalize_conditional(Node *node);
Node *__fastcall integer_node(unsigned value, unsigned type);
unsigned __fastcall node_value_size(Node *node);
int __fastcall trees_equal(Node *left, Node *right, int flags);
Node *__fastcall lower_intrinsic(Node *node, Node *parent);
int __fastcall assign_parameter_register(unsigned *used, Node *argument, int convention);
unsigned short __fastcall node_type_kind(unsigned type);
unsigned short __fastcall common_node_type(unsigned short left, unsigned short right);
Node *__fastcall discard_sequence_tail(Node *node);
Node *__fastcall lift_typed_sequence(Node *node, unsigned opcode);
int __fastcall is_function_reference(Node *node);
extern int data_pointer_bytes, suppress_type_warnings;
#endif
