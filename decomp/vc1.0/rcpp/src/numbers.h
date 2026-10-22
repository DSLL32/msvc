#ifndef RCPP_NUMBERS_H
#define RCPP_NUMBERS_H
#include "directives.h"
/* Compiler token/type layouts observed in constant construction. Unknown
 * ancillary fields retain explicit offsets until their uses are recovered. */
typedef struct RcType {
    unsigned short flags, attributes;
    void *related;
    unsigned field08, field0c;
    unsigned short references, field12;
} RcType;
typedef struct RcConstant {
    unsigned char kind, flags;
    unsigned short field02;
    RcType *type;
    unsigned field08;
    union { long integer; void *pointer; char *text; } value;
    unsigned short length, auxiliary_flags;
} RcConstant;
extern int rc_collecting_macro_text;
extern char *rc_expansion_output;
int __stdcall rc_digit_value(int);
unsigned long __stdcall rc_integer_value(const char *, int);
unsigned char __stdcall rc_integer_nondecimal_kind(unsigned long);
unsigned char __stdcall rc_integer_decimal_kind(unsigned long);
unsigned char __stdcall rc_long_kind(unsigned long);
unsigned char __stdcall rc_unsigned_kind(unsigned long);
RcConstant *__stdcall rc_make_constant(unsigned char, const void *);
RcType *__stdcall rc_intern_type(RcType *);
int __stdcall rc_equal_type(RcType *, RcType *);
#endif
