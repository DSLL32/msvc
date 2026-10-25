#ifndef TAIL_RECURSION_H
#define TAIL_RECURSION_H
#include "functions.h"
#include <recon.h>
int __fastcall is_tail_call_position(Expression *expressions, Expression *call);
SymbolHeader *__fastcall tail_argument_address(Node *node);
Expression *__fastcall find_tail_label(Expression *expressions, SymbolHeader *label);
#endif
