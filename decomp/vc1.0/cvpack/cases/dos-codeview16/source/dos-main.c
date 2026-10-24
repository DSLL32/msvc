#include "dos-cv.h"
struct Node roots[3];
long total = 17L;
static int local_global = 4;
int main(void)
{
    int i;
    int result = 0;
    int (__near *step)(struct Node __near *, int) = visit;
    int (__far *finish)(struct Node __far *, short) = combine;
    for (i = 0; i != 3; ++i) {
        struct Node __near *item = &roots[i];
        item->next = i != 2 ? &roots[i + 1] : 0;
        item->colour = (enum Colour)i;
        item->value.number = (long)i * total;
        item->samples[0] = local_global + i;
        result += (*step)(item, i + 2);
    }
    result += (*finish)(roots, 3);
    return result == 0;
}
