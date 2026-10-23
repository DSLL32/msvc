#ifndef AUTO_PCH_H
#define AUTO_PCH_H
typedef struct Pair { int value, bias; } Pair;
#define ADJUST(value) ((value) + MAIN_BIAS)
int adjust(int value);
#endif
