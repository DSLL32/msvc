#ifndef WORK_PREPARE_H
#define WORK_PREPARE_H
#include "functions.h"
#include "machine.h"
#include "bitsets.h"
#include <recon.h>
void __fastcall prepare_parameter_storage(WorkItem *work);
void __fastcall lower_assignment(Node *node);
void __fastcall lower_switch_dispatch(WorkItem *work, Expression *expression, Node *node);
extern BitSet *spilled_parameter_registers;
extern int tree_uses_floating;
Node *__fastcall lower_machine_tree(Node *node);
void __fastcall lower_float_expression(Node *node);
void finish_target_expression(void);
void __fastcall prepare_generated_intrinsic(Node *node, int mode);
extern Expression *switch_dispatch_expression;
extern Node *switch_selector, *switch_dispatch_node;
extern unsigned short switch_selector_type;
int __fastcall coalesce_switch_ranges(Node *node);
void __fastcall emit_switch_dispatch(Node *ranges, int count);
void __fastcall summarize_switch_ranges(Node *ranges, int count, unsigned *cases, unsigned *span, int *tests);
void __fastcall emit_linear_switch(Node *ranges, int count);
Node *__fastcall make_switch_pivot(Node *range);
void __fastcall split_switch_ranges(Node *ranges, unsigned cases, int count, unsigned span, Node **pivot, int *left_count);
void __fastcall emit_switch_table(Node *ranges, int count, int option1, int option2);
extern Node *assignment_node, *assignment_destination, *assignment_match;
extern unsigned assignment_match_left, assignment_has_addition, assignment_operator;
Node *__fastcall operator_identity_node(Node *operation, Node *type_source);
int __fastcall matches_assignment_load(Node *destination, Node *operand);
void __fastcall reduce_assignment_operator(Node *assignment, Node *operand, Node *load, Node *operation);
int __fastcall find_assignment_operand(Node *node, unsigned has_addition);
Node *__fastcall strip_assignment_conversion(Node *node);
#endif
