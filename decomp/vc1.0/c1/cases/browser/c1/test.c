#include "case.h"
int transform(int value) { return APPLY(value); }
int main(void) { Result result = { 3 }; return transform(result.value) != 6; }
