#ifndef WORK_EXPRESSIONS_H
#define WORK_EXPRESSIONS_H
#include "work-optimizer.h"
#include <recon.h>
extern WorkItem *current_expression_work;
extern Expression *work_expression_anchor;
Node *__fastcall rewrite_numbered_tree(Node *node);
Node *__fastcall materialize_numbered_tree(Node *node);
#endif
