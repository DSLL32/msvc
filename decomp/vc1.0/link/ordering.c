/* Optional user-specified ordering of code contributions. */
#include "pass-one.h"
#include "options.h"
unsigned char order_enabled;
void reorder_code_contributions(Image *);

void apply_order_file(Image *image)
{
    unsigned char allowed=!debug_enabled(24);
    if (order_enabled && allowed) reorder_code_contributions(image);
}

FatalRoutine order_fatal;
WarningRoutine order_warning;
void initialize_order(FatalRoutine fatal,WarningRoutine warning)
{
    order_fatal=fatal;
    order_warning=warning;
    order_enabled=1;
}
