/* Frontend floating-point environment initialization. */
#include <float.h>
#include "initialization.h"
#include "floating.h"
#include <recon.h>

/* Original 0x74b30..0x74b3d. */
void initialize_floating_point(void)
{
    _control87(4, 4);
}

Extended extended_zero = { 0, 0, 0, 4 }; /* original 0x9fd50 */

Extended scanned_floating_value; /* original 0x97ba8, scanner conversion result */
