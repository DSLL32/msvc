#include "dos-cv.h"
static long bias = 5L;
int __near visit(struct Node __near *node, int limit)
{
    int sum = 0;
    int index;
    for (index = 0; index < limit; ++index) {
        short term = node->samples[index % 5];
        sum += term + node->colour;
        if (term != 0) {
            long adjusted = bias + node->value.number;
            sum += (int)adjusted;
        }
    }
    return sum;
}
int __far combine(struct Node __far *node, short count)
{
    int result = 0;
    while (count-- != 0) {
        result += node->samples[0];
        total += node->value.number;
        ++node;
    }
    return result;
}
