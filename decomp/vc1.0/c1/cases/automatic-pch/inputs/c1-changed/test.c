#define FLAVOR 2
#if FLAVOR == 1
#define MAIN_BIAS 7
#else
#define MAIN_BIAS 11
#endif
#include "case.h"
int adjust(int value) { Pair pair = { value, MAIN_BIAS }; return pair.value + pair.bias; }
int main(void) { return adjust(5) != ADJUST(5); }
