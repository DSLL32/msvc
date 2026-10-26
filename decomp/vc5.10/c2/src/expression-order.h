#ifndef C2_EXPRESSION_ORDER_H
#define C2_EXPRESSION_ORDER_H
#include "code-records.h"
#include <recon.h>

/* 408d58 and the merge call at 409322 use ECX/EDX, a full EAX
 * comparison result, no incoming stack arguments and RET0. */
typedef int (__fastcall *C2ExpressionCompare)(C2Expression *left, C2Expression *right);
unsigned __fastcall expression_order_key(C2Expression *expression);
unsigned __fastcall expression_value_hash(C2Expression *expression);
C2Expression *__fastcall sort_expression_inputs(C2Expression *head, C2ExpressionCompare compare);
C2Expression *__fastcall merge_expression_inputs(C2Expression *left, C2Expression *right, C2ExpressionCompare compare);
int __fastcall compare_expression_keys(C2Expression *left, C2Expression *right);
#endif
