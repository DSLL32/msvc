#ifndef WORK_PREPARE_H
#define WORK_PREPARE_H
#include "functions.h"
#include "machine.h"
#include "bitsets.h"
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
#endif
