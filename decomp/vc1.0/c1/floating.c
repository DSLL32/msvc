/* Frontend floating-point environment initialization. */
#include <float.h>
#include "initialization.h"

/* Original 0x4c380..0x4c38d. */
void initialize_floating_point(void)
{
    _control87(4, 4);
}
