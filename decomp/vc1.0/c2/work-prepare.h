#ifndef WORK_PREPARE_H
#define WORK_PREPARE_H
#include "functions.h"
#include "machine.h"
void __fastcall prepare_parameter_storage(WorkItem *work);
void __fastcall lower_assignment(Node *node);
void __fastcall lower_expression_stack(WorkItem *work, Expression *expression, Node *node);
BitSet *__fastcall bitset_include(BitSet *set, unsigned member);
extern BitSet *spilled_parameter_registers;
extern int tree_uses_floating;
Node *__fastcall lower_machine_tree(Node *node);
void __fastcall lower_float_expression(Node *node);
void finish_target_expression(void);
void __fastcall prepare_generated_intrinsic(Node *node, int mode);
#endif
