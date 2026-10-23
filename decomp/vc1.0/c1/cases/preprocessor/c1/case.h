#ifndef OUTPUT_CASE_H
#define OUTPUT_CASE_H
#define FACTOR 2
#define APPLY(value) ((value) * FACTOR)
typedef struct Result { int value; } Result;
int transform(int value);
#endif
