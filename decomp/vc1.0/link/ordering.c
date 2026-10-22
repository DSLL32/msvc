/* Optional user-specified ordering of code contributions. */
#include "pass-one.h"
unsigned char order_enabled;
void reorder_code_contributions(Image *);

void apply_order_file(Image *image)
{
    unsigned char allowed=!debug_enabled(24);
    if (order_enabled && allowed) reorder_code_contributions(image);
}
