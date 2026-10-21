/* Frontend floating-point environment initialization. */
#include <float.h>
#include "initialization.h"

/* Original 0x74b30..0x74b3d. */
void initialize_floating_point(void)
{
    _control87(4, 4);
}
