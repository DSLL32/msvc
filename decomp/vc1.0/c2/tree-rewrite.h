#ifndef TREE_REWRITE_H
#define TREE_REWRITE_H
#include "functions.h"
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
Node *__fastcall prepare_target_tree(Node *node);
Node *__fastcall canonicalize_node(Function *function, Node *node, unsigned flags);
#endif
