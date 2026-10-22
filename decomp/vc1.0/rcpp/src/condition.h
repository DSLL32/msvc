#ifndef RCPP_CONDITION_H
#define RCPP_CONDITION_H
#include "numbers.h"
#include <recon.h>
extern int rc_expression_depth;
extern long rc_expression_value;
extern unsigned char rc_expression_token;
long rc_conditional_expression(void);
long rc_logical_or_expression(void);
long rc_logical_and_expression(void);
long rc_bitwise_or_expression(void);
long rc_bitwise_xor_expression(void);
long rc_bitwise_and_expression(void);
long rc_equality_expression(void);
long rc_relational_expression(void);
long rc_shift_expression(void);
long rc_additive_expression(void);
long rc_multiplicative_expression(void);
long rc_unary_expression(void);
long rc_primary_expression(void);
int __stdcall rc_match_expression_token(unsigned char);
long __stdcall rc_identifier_value(char *);
int rc_skip_macro_whitespace(void);
int __stdcall rc_collect_macro_identifier(unsigned char);
void __stdcall rc_macro_fatal(unsigned);
#endif
