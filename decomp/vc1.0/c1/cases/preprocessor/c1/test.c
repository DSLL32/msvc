#include "case.h"
#define JOIN_I(first,second) first ## second
#define JOIN(first,second) JOIN_I(first,second)
#define STRING_I(value) #value
#define STRING(value) STRING_I(value)
#if FACTOR == 2 && defined(OUTPUT_CASE_H)
#line 40 "logical-test.c"
int JOIN(trans,form)(int value) { return APPLY(value); }
const char *factor_text = STRING(FACTOR);
#endif
int main(void) { Result result = { 3 }; return transform(result.value) != 6; }
